// Minimal CLAP host: loads the sapf plugin and checks UDP evaluation, audio output,
// state round trip, and the one-instance rule. Usage: test_clap_host <plugin binary>
// Expects SAPF_CLAP_UDP_PORT to be set, which turns UDP on at init.

#include <clap/clap.h>

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <string>
#include <vector>

#ifdef _WIN32
#include <winsock2.h>
#include <windows.h>
typedef int socklen_t;
#else
#include <arpa/inet.h>
#include <dlfcn.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <unistd.h>
#endif

namespace {

int gFailures = 0;

void check(bool ok, const char* what)
{
	printf("%s: %s\n", ok ? "PASS" : "FAIL", what);
	if (!ok) ++gFailures;
}

const clap_plugin_entry_t* loadEntry(const char* path)
{
#ifdef _WIN32
	HMODULE lib = LoadLibraryA(path);
	return lib ? (const clap_plugin_entry_t*)GetProcAddress(lib, "clap_entry") : nullptr;
#else
	void* lib = dlopen(path, RTLD_NOW | RTLD_LOCAL);
	if (!lib) fprintf(stderr, "dlopen: %s\n", dlerror());
	return lib ? (const clap_plugin_entry_t*)dlsym(lib, "clap_entry") : nullptr;
#endif
}

const clap_host_t kHost = {
	CLAP_VERSION_INIT,
	nullptr,
	"test_clap_host",
	"sapf",
	"",
	"1.0",
	[](const clap_host_t*, const char*) -> const void* { return nullptr; },
	[](const clap_host_t*) {},
	[](const clap_host_t*) {},
	[](const clap_host_t*) {},
};

// Sends one datagram and returns the reply ("" on timeout).
std::string udpEval(int port, const std::string& code)
{
	auto fd = socket(AF_INET, SOCK_DGRAM, 0);
#ifdef _WIN32
	DWORD timeout = 15000;
#else
	timeval timeout{15, 0};
#endif
	setsockopt(fd, SOL_SOCKET, SO_RCVTIMEO, (const char*)&timeout, sizeof(timeout));
	sockaddr_in addr{};
	addr.sin_family = AF_INET;
	addr.sin_port = htons((uint16_t)port);
	addr.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
	sendto(fd, code.data(), (int)code.size(), 0, (sockaddr*)&addr, sizeof(addr));
	char buf[65536];
	int n = (int)recv(fd, buf, sizeof(buf), 0);
#ifdef _WIN32
	closesocket(fd);
#else
	close(fd);
#endif
	return n > 0 ? std::string(buf, n) : std::string();
}

float renderPeak(const clap_plugin_t* plugin, int blocks)
{
	const uint32_t frames = 256;
	std::vector<float> left(frames), right(frames);
	float* channels[2] = { left.data(), right.data() };
	clap_audio_buffer_t out{};
	out.data32 = channels;
	out.channel_count = 2;
	clap_process_t process{};
	process.frames_count = frames;
	process.audio_outputs = &out;
	process.audio_outputs_count = 1;

	float peak = 0.f;
	for (int b = 0; b < blocks; ++b) {
		plugin->process(plugin, &process);
		for (uint32_t i = 0; i < frames; ++i) {
			peak = std::fmax(peak, std::fmax(std::fabs(left[i]), std::fabs(right[i])));
		}
	}
	return peak;
}

struct Buffer {
	std::string data;
	size_t pos = 0;
};

} // namespace

int main(int argc, char** argv)
{
	if (argc < 2) {
		fprintf(stderr, "usage: test_clap_host <plugin>\n");
		return 2;
	}
	const char* portEnv = getenv("SAPF_CLAP_UDP_PORT");
	if (!portEnv) {
		fprintf(stderr, "SAPF_CLAP_UDP_PORT must be set\n");
		return 2;
	}
	int port = atoi(portEnv);
#ifdef _WIN32
	WSADATA wsa;
	WSAStartup(MAKEWORD(2, 2), &wsa);
#endif

	const clap_plugin_entry_t* entry = loadEntry(argv[1]);
	check(entry != nullptr, "clap_entry exported");
	if (!entry) return 1;
	check(entry->init(argv[1]), "entry init");
	auto* factory = (const clap_plugin_factory_t*)entry->get_factory(CLAP_PLUGIN_FACTORY_ID);
	check(factory && factory->get_plugin_count(factory) == 1, "factory has one plugin");
	const char* id = factory->get_plugin_descriptor(factory, 0)->id;

	const clap_plugin_t* plugin = factory->create_plugin(factory, &kHost, id);
	check(plugin != nullptr, "create plugin");
	check(factory->create_plugin(factory, &kHost, id) == nullptr, "second instance refused");
	check(plugin->init(plugin), "plugin init");
	check(plugin->activate(plugin, 48000., 32, 1024), "activate");
	check(plugin->start_processing(plugin), "start processing");

	check(udpEval(port, "1 2 +").find('3') != std::string::npos, "udp eval replies with stack");
	check(udpEval(port, "nosuchword").find("error") != std::string::npos, "udp eval replies with error");
	check(udpEval(port, "1 2X").find("[1 1]") != std::string::npos, "embedded prelude defines 2X");
	check(udpEval(port, "clear") == "ok\n", "udp clear leaves an empty stack");
	check(udpEval(port, "440 0 sinosc .1 * play") == "ok\n", "udp play");
	float peak = renderPeak(plugin, 100);
	printf("peak %.4f\n", peak);
	check(peak > 0.09f && peak < 0.11f, "plays sine at 0.1");
	udpEval(port, "stop");
	check(renderPeak(plugin, 4) == 0.f, "stop silences output");

	auto* state = (const clap_plugin_state_t*)plugin->get_extension(plugin, CLAP_EXT_STATE);
	Buffer saved;
	clap_ostream_t os{ &saved, [](const clap_ostream_t* s, const void* p, uint64_t n) -> int64_t {
		((Buffer*)s->ctx)->data.append((const char*)p, n);
		return (int64_t)n;
	} };
	check(state && state->save(plugin, &os) && saved.data.size() >= 2, "state save");
	clap_istream_t is{ &saved, [](const clap_istream_t* s, void* p, uint64_t n) -> int64_t {
		Buffer* b = (Buffer*)s->ctx;
		size_t k = std::min<size_t>(n, b->data.size() - b->pos);
		memcpy(p, b->data.data() + b->pos, k);
		b->pos += k;
		return (int64_t)k;
	} };
	check(state->load(plugin, &is), "state load");
	check((saved.data[1] & 2) == 0, "wrap is on by default");
	Buffer noWrap;
	noWrap.data = std::string("\x01\x02", 2) + "1 2 +";
	clap_istream_t is2{ &noWrap, is.read };
	Buffer resaved;
	clap_ostream_t os2{ &resaved, os.write };
	check(state->load(plugin, &is2) && state->save(plugin, &os2) && resaved.data == noWrap.data,
	      "state round-trips wrap off and editor text");

	// Replace (flag 4): each evaluation's first play stops the previous sounds.
	Buffer replaceOn;
	replaceOn.data = std::string("\x01\x05", 2); // UDP + replace
	clap_istream_t is3{ &replaceOn, is.read };
	check(state->load(plugin, &is3), "load state with replace on");
	udpEval(port, "440 0 sinosc .1 * play");
	udpEval(port, "1000 0 sinosc .1 * play");
	float replaced = renderPeak(plugin, 40);
	printf("peak with replace %.4f\n", replaced);
	check(replaced > 0.09f && replaced < 0.11f, "replace keeps only the last sound");
	Buffer resaved2;
	clap_ostream_t os3{ &resaved2, os.write };
	check(state->save(plugin, &os3) && (resaved2.data[1] & 4), "state saves replace");
	udpEval(port, "stop");

#ifdef __APPLE__
	auto* gui = (const clap_plugin_gui_t*)plugin->get_extension(plugin, CLAP_EXT_GUI);
	check(gui && gui->is_api_supported(plugin, CLAP_WINDOW_API_COCOA, false), "cocoa gui supported");
	check(gui && gui->create(plugin, CLAP_WINDOW_API_COCOA, false), "gui create");
	if (gui) gui->destroy(plugin);
#endif

	plugin->stop_processing(plugin);
	plugin->deactivate(plugin);
	plugin->destroy(plugin);

	const clap_plugin_t* again = factory->create_plugin(factory, &kHost, id);
	check(again != nullptr, "new instance after destroy");
	if (again) again->destroy(again);

	entry->deinit();
	printf("%d failure(s)\n", gFailures);
	return gFailures ? 1 : 0;
}
