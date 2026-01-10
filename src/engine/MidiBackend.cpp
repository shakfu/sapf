#include "sapf/MidiBackend.hpp"

#include <stdexcept>

#include "sapf/backends/CoreMidiBackend.hpp"
#include "sapf/backends/NullMidiBackend.hpp"
#include "sapf/backends/RtMidiBackend.hpp"

namespace {

std::unique_ptr<MidiBackend> gMidiBackend;

} // namespace

MidiBackend& GetMidiBackend()
{
	if (!gMidiBackend) {
		throw std::runtime_error("MIDI backend not configured");
	}
	return *gMidiBackend;
}

void SetMidiBackend(std::unique_ptr<MidiBackend> backend)
{
	gMidiBackend = std::move(backend);
}

bool HasMidiBackend()
{
	return static_cast<bool>(gMidiBackend);
}

void EnsureDefaultMidiBackend()
{
	if (HasMidiBackend()) {
		return;
	}

#if defined(__APPLE__)
	if (SupportsCoreMidiBackend()) {
		SetMidiBackend(CreateCoreMidiBackend());
		return;
	}
	if (auto backend = CreateRtMidiBackend()) {
		SetMidiBackend(std::move(backend));
		return;
	}
#elif defined(__linux__)
	if (auto backend = CreateRtMidiBackend()) {
		SetMidiBackend(std::move(backend));
		return;
	}
#elif defined(_WIN32)
	if (auto backend = CreateRtMidiBackend()) {
		SetMidiBackend(std::move(backend));
		return;
	}
	SetMidiBackend(CreateNullMidiBackend("Windows MIDI backend will use RtMidi once configured."));
	return;
#else
	if (auto backend = CreateRtMidiBackend()) {
		SetMidiBackend(std::move(backend));
		return;
	}
#endif

	SetMidiBackend(CreateNullMidiBackend("MIDI backend not available on this platform."));
}
