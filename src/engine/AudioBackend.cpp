#include "sapf/AudioBackend.hpp"

#include <stdexcept>

#include "sapf/backends/AlsaAudioBackend.hpp"
#include "sapf/backends/CoreAudioBackend.hpp"
#include "sapf/backends/NullAudioBackend.hpp"
#include "sapf/backends/RtAudioBackend.hpp"

namespace {

std::unique_ptr<AudioBackend> gAudioBackend;

} // namespace

void SetAudioBackend(std::unique_ptr<AudioBackend> backend)
{
	gAudioBackend = std::move(backend);
}

AudioBackend& GetAudioBackend()
{
	if (!gAudioBackend) {
		throw std::runtime_error("Audio backend not configured");
	}
	return *gAudioBackend;
}

bool HasAudioBackend()
{
	return static_cast<bool>(gAudioBackend);
}

void EnsureDefaultAudioBackend()
{
	if (HasAudioBackend()) {
		return;
	}

#if defined(__APPLE__)
	if (SupportsCoreAudioBackend()) {
		SetAudioBackend(CreateCoreAudioBackend());
		return;
	}
	if (auto backend = CreateRtAudioBackend()) {
		SetAudioBackend(std::move(backend));
		return;
	}
#elif defined(__linux__)
	if (auto backend = CreateRtAudioBackend()) {
		SetAudioBackend(std::move(backend));
		return;
	}
	if (SupportsAlsaAudioBackend()) {
		auto backend = CreateAlsaAudioBackend();
		if (backend) {
			SetAudioBackend(std::move(backend));
			return;
		}
	}
#elif defined(_WIN32)
	if (auto backend = CreateRtAudioBackend()) {
		SetAudioBackend(std::move(backend));
		return;
	}
	SetAudioBackend(CreateNullAudioBackend("Windows audio backend will use RtAudio once configured."));
	return;
#else
	if (auto backend = CreateRtAudioBackend()) {
		SetAudioBackend(std::move(backend));
		return;
	}
#endif

	SetAudioBackend(CreateNullAudioBackend("Audio backend not available on this platform."));
}
