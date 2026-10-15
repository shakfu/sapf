// Host-driven audio backend (MaxAudioBackend), as used by Pd/VST3/CLAP embeddings.
#include <gtest/gtest.h>
#include "test_common.hpp"
#include "sapf/MaxAudioBackend.hpp"
#include "VM.hpp"
#include <algorithm>
#include <cmath>

namespace {

// Signal generator that fails on its first pull, i.e. inside the audio callback.
class ThrowGen : public Gen
{
public:
    explicit ThrowGen(Thread& th) : Gen(th, itemTypeZ, false) {}
    const char* TypeName() const override { return "ThrowGen"; }
    void pull(Thread&) override { throw errFailed; }
};

const int kFrames = 64;

class HostAudioTest : public SapfTestBase {
protected:
    void SetUp() override { backend_ = &InstallMaxMSPBackend(); }
    void TearDown() override { backend_->stopAll(); }

    float renderPeak()
    {
        // 20 blocks span several periods of the test sines, so the peak reaches their amplitude.
        float buf[kFrames];
        float* out[1] = { buf };
        float peak = 0.f;
        for (int b = 0; b < 20; ++b) {
            backend_->render(out, 1, kFrames);
            for (float x : buf) peak = std::max(peak, std::fabs(x));
        }
        return peak;
    }

    MaxAudioBackend* backend_ = nullptr;
    Thread th_;
};

} // namespace

TEST_F(HostAudioTest, RendersPlayedSignal) {
    P<Fun> fun;
    ASSERT_TRUE(th_.compile("440 0 sinosc .5 * play", fun, true));
    fun->run(th_);
    renderPeak();
    EXPECT_GT(renderPeak(), 0.4f);
}

TEST_F(HostAudioTest, ExceptionInRenderDropsPlayerWithoutThrowing) {
    V v(new List(new ThrowGen(th_)));
    backend_->play(th_, v);
    EXPECT_NO_THROW(EXPECT_EQ(renderPeak(), 0.f));

    // The failed player is gone; a new one plays normally.
    P<Fun> fun;
    ASSERT_TRUE(th_.compile("440 0 sinosc .5 * play", fun, true));
    fun->run(th_);
    renderPeak();
    EXPECT_GT(renderPeak(), 0.4f);
}

TEST_F(HostAudioTest, StopOnNextPlayReplacesPlayers) {
    P<Fun> fun;
    ASSERT_TRUE(th_.compile("440 0 sinosc .5 * play", fun, true));
    fun->run(th_);
    backend_->setStopOnNextPlay(true);
    ASSERT_TRUE(th_.compile("440 0 sinosc .25 * play", fun, true));
    fun->run(th_);
    renderPeak();
    const float peak = renderPeak();
    EXPECT_GT(peak, 0.2f);
    EXPECT_LT(peak, 0.3f); // only the second sine; additive would reach .75

    // The flag is consumed by one play: the next play adds (a different frequency, so the peaks align).
    ASSERT_TRUE(th_.compile("1000 0 sinosc .25 * play", fun, true));
    fun->run(th_);
    EXPECT_GT(renderPeak(), 0.35f);
}
