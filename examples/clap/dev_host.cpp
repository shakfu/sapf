// Minimal CLAP host for trying the sapf plugin: opens its editor in a window and plays it
// through the default audio output. Usage: sapf_dev_host [plugin binary]
// The default plugin path is the one built alongside this host. Close the window to quit.

#include <clap/clap.h>
#include <pugl/pugl.h>
#include <pugl/stub.h>
#include <RtAudio.h>

#include <atomic>
#include <chrono>
#include <cstdio>
#include <cstring>
#include <vector>

#ifdef _WIN32
#include <windows.h>
#else
#include <dlfcn.h>
#endif

namespace {

struct Timer {
	clap_id id;
	std::chrono::milliseconds period;
	std::chrono::steady_clock::time_point due;
};

const clap_plugin_t* gPlugin = nullptr;
std::vector<Timer> gTimers;
std::atomic<bool> gCallbackRequested{false};

const clap_host_timer_support_t kTimerSupport = {
	[](const clap_host_t*, uint32_t ms, clap_id* id) {
		static clap_id next = 0;
		*id = next++;
		gTimers.push_back({*id, std::chrono::milliseconds(ms), std::chrono::steady_clock::now()});
		return true;
	},
	[](const clap_host_t*, clap_id id) {
		for (auto it = gTimers.begin(); it != gTimers.end(); ++it) {
			if (it->id == id) {
				gTimers.erase(it);
				return true;
			}
		}
		return false;
	},
};

const clap_host_t kHost = {
	CLAP_VERSION_INIT,
	nullptr,
	"sapf dev host",
	"sapf",
	"https://github.com/shakfu/sapf",
	"1.0",
	[](const clap_host_t*, const char* id) -> const void* {
		return strcmp(id, CLAP_EXT_TIMER_SUPPORT) ? nullptr : &kTimerSupport;
	},
	[](const clap_host_t*) {},                             // request_restart
	[](const clap_host_t*) {},                             // request_process: always processing
	[](const clap_host_t*) { gCallbackRequested = true; }, // request_callback
};

const clap_plugin_entry_t* LoadEntry(const char* path)
{
#ifdef _WIN32
	HMODULE lib = LoadLibraryA(path);
	return lib ? (const clap_plugin_entry_t*)GetProcAddress(lib, "clap_entry") : nullptr;
#else
	void* lib = dlopen(path, RTLD_NOW | RTLD_LOCAL);
	if (!lib) fprintf(stderr, "%s\n", dlerror());
	return lib ? (const clap_plugin_entry_t*)dlsym(lib, "clap_entry") : nullptr;
#endif
}

const char* WindowApi()
{
#if defined(__APPLE__)
	return CLAP_WINDOW_API_COCOA;
#elif defined(_WIN32)
	return CLAP_WINDOW_API_WIN32;
#else
	return CLAP_WINDOW_API_X11;
#endif
}

// CLAP gui sizes are points on macOS and pixels elsewhere; pugl sizes are pixels.
double HostUnitsToPixels(PuglView* view)
{
#ifdef __APPLE__
	return puglGetScaleFactor(view);
#else
	(void)view;
	return 1.0;
#endif
}

int Render(void* output, void*, unsigned int frames, double, RtAudioStreamStatus, void*)
{
	static bool started = false;
	if (!started) started = gPlugin->start_processing(gPlugin); // CLAP: on the audio thread
	float* out = static_cast<float*>(output);
	float* channels[2] = { out, out + frames }; // non-interleaved
	clap_audio_buffer_t buffer{};
	buffer.data32 = channels;
	buffer.channel_count = 2;
	clap_process_t process{};
	process.frames_count = frames;
	process.audio_outputs = &buffer;
	process.audio_outputs_count = 1;
	gPlugin->process(gPlugin, &process);
	return 0;
}

struct Window {
	const clap_plugin_gui_t* gui = nullptr;
	bool open = true;
};

PuglStatus OnEvent(PuglView* view, const PuglEvent* event)
{
	auto* win = static_cast<Window*>(puglGetHandle(view));
	if (event->type == PUGL_CLOSE) {
		win->open = false;
	} else if (event->type == PUGL_CONFIGURE && win->gui) {
		const double scale = HostUnitsToPixels(view);
		uint32_t w = (uint32_t)(event->configure.width / scale), h = (uint32_t)(event->configure.height / scale);
		if (win->gui->adjust_size(gPlugin, &w, &h)) win->gui->set_size(gPlugin, w, h);
	}
	return PUGL_SUCCESS;
}

} // namespace

int main(int argc, char** argv)
{
	const char* path = argc > 1 ? argv[1] : SAPF_CLAP_PLUGIN_PATH;
	const clap_plugin_entry_t* entry = LoadEntry(path);
	if (!entry || !entry->init(path)) {
		fprintf(stderr, "cannot load CLAP plugin %s\n", path);
		return 1;
	}
	auto* factory = (const clap_plugin_factory_t*)entry->get_factory(CLAP_PLUGIN_FACTORY_ID);
	gPlugin = factory->create_plugin(factory, &kHost, factory->get_plugin_descriptor(factory, 0)->id);
	if (!gPlugin || !gPlugin->init(gPlugin)) {
		fprintf(stderr, "cannot create plugin\n");
		return 1;
	}

	RtAudio audio;
	const unsigned device = audio.getDefaultOutputDevice();
	unsigned sampleRate = 48000, frames = 256;
	if (device) {
		RtAudio::DeviceInfo info = audio.getDeviceInfo(device);
		if (info.preferredSampleRate) sampleRate = info.preferredSampleRate;
	}
	gPlugin->activate(gPlugin, sampleRate, 1, 8192);
	RtAudio::StreamParameters params;
	params.deviceId = device;
	params.nChannels = 2;
	RtAudio::StreamOptions options;
	options.flags = RTAUDIO_NONINTERLEAVED;
	const bool playing = device
		&& audio.openStream(&params, nullptr, RTAUDIO_FLOAT32, sampleRate, &frames, &Render, nullptr, &options) == RTAUDIO_NO_ERROR
		&& audio.startStream() == RTAUDIO_NO_ERROR;
	if (!playing) fprintf(stderr, "no audio output; the editor still runs\n");
	else printf("audio: %u Hz, %u frames\n", sampleRate, frames);
	fflush(stdout);

	auto* gui = (const clap_plugin_gui_t*)gPlugin->get_extension(gPlugin, CLAP_EXT_GUI);
	uint32_t w = 720, h = 480;
	if (gui) gui->get_size(gPlugin, &w, &h);

	Window win; // win.gui stays null until the editor exists, so early CONFIGUREs are not forwarded

	PuglWorld* world = puglNewWorld(PUGL_PROGRAM, 0);
	puglSetWorldString(world, PUGL_CLASS_NAME, "SapfDevHost");
	PuglView* view = puglNewView(world);
	puglSetHandle(view, &win);
	puglSetBackend(view, puglStubBackend());
	puglSetEventFunc(view, OnEvent);
	puglSetViewString(view, PUGL_WINDOW_TITLE, "sapf");
	puglSetViewHint(view, PUGL_RESIZABLE, 1);
	puglSetSizeHint(view, PUGL_DEFAULT_SIZE, w, h);
	if (puglRealize(view) != PUGL_SUCCESS) {
		fprintf(stderr, "cannot open window\n");
		return 1;
	}
	const double scale = HostUnitsToPixels(view);
	puglSetSizeHint(view, PUGL_CURRENT_SIZE, (unsigned)(w * scale), (unsigned)(h * scale));
	puglShow(view, PUGL_SHOW_RAISE);

	clap_window_t parent{WindowApi(), {}};
#if defined(__APPLE__)
	parent.cocoa = (void*)puglGetNativeView(view);
#elif defined(_WIN32)
	parent.win32 = (void*)puglGetNativeView(view);
#else
	parent.x11 = (clap_xwnd)puglGetNativeView(view);
#endif
	if (gui && gui->create(gPlugin, WindowApi(), false) && gui->set_parent(gPlugin, &parent)) {
		gui->show(gPlugin);
		win.gui = gui;
	} else {
		fprintf(stderr, "plugin editor failed to open\n");
	}

	while (win.open) {
		puglUpdate(world, 0.005);
		const auto now = std::chrono::steady_clock::now();
		auto* timers = (const clap_plugin_timer_support_t*)gPlugin->get_extension(gPlugin, CLAP_EXT_TIMER_SUPPORT);
		for (size_t i = 0; i < gTimers.size(); ++i) { // on_timer may unregister timers
			if (now < gTimers[i].due) continue;
			gTimers[i].due = now + gTimers[i].period;
			if (timers) timers->on_timer(gPlugin, gTimers[i].id);
		}
		if (gCallbackRequested.exchange(false)) gPlugin->on_main_thread(gPlugin);
	}

	if (win.gui) win.gui->destroy(gPlugin);
	puglFreeView(view);
	puglFreeWorld(world);
	if (playing) {
		audio.stopStream();
		audio.closeStream();
	}
	gPlugin->stop_processing(gPlugin); // after the audio thread has stopped
	gPlugin->deactivate(gPlugin);
	gPlugin->destroy(gPlugin);
	entry->deinit();
	return 0;
}
