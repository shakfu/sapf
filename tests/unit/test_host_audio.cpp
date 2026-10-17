// Host-driven audio backend (MaxAudioBackend), as used by Pd/VST3/CLAP embeddings.
#include <gtest/gtest.h>
#include "test_common.hpp"
#include "sapf/MaxAudioBackend.hpp"
#include "VM.hpp"
#include <algorithm>
#include <cmath>
#include <vector>

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

namespace {

// Input channel c holds (c + 1) * (frame + 1), counting frames across calls: the input FIFO is
// process-wide, so a stream's first frames can be input from before the call.
struct Rendered {
    std::vector<float> in0, out0;
};

Rendered renderWithInput(Thread& th, MaxAudioBackend& backend, const char* code, int hostFrames, int total)
{
    static int64_t frame = 0;
    P<Fun> fun;
    EXPECT_TRUE(th.compile(code, fun, true));
    fun->run(th);
    std::vector<float> in0(hostFrames), in1(hostFrames), out(hostFrames);
    const float* ins[2] = { in0.data(), in1.data() };
    float* outs[1] = { out.data() };
    Rendered r;
    for (int t = 0; t < total; t += hostFrames) {
        for (int i = 0; i < hostFrames; ++i, ++frame) {
            in0[i] = float(frame + 1);
            in1[i] = 2.f * float(frame + 1);
        }
        MaxMSPSetInputBuffers(ins, 2, hostFrames);
        backend.render(outs, 1, hostFrames);
        r.in0.insert(r.in0.end(), in0.begin(), in0.end());
        r.out0.insert(r.out0.end(), out.begin(), out.end());
    }
    return r;
}

// The delay d at which out[t] == scale * in0[t - d] for every t >= d, or -1.
int constantDelay(const Rendered& r, float scale, int maxDelay)
{
    for (int d = 0; d <= maxDelay; ++d) {
        size_t t = d;
        while (t < r.out0.size() && r.out0[t] == scale * r.in0[t - d]) ++t;
        if (t == r.out0.size()) return d;
    }
    return -1;
}

} // namespace

TEST_F(HostAudioTest, AdcPassesInputAtAConstantDelay) {
    RegisterMaxAudioPrimitives();
    for (int hostFrames : { 1, 63, 64, 512, 1000 }) {
        SCOPED_TRACE(hostFrames);
        auto r = renderWithInput(th_, *backend_, "0 adc play", hostFrames, 20000);
        EXPECT_GE(constantDelay(r, 1.f, hostFrames + th_.rate.blockSize), 0);
        backend_->stopAll();
    }
}

TEST_F(HostAudioTest, AdcReadsTheChannelThroughAGraph) {
    RegisterMaxAudioPrimitives();
    const int maxDelay = 100 + th_.rate.blockSize;
    auto r = renderWithInput(th_, *backend_, "1 adc 3 * .5 * play", 100, 20000);
    EXPECT_GE(constantDelay(r, 3.f, maxDelay), 0);  // channel 1 is twice channel 0
    backend_->stopAll();
    r = renderWithInput(th_, *backend_, "2 adcn 1 at play", 100, 20000);
    EXPECT_GE(constantDelay(r, 2.f, maxDelay), 0);
}

TEST_F(HostAudioTest, AdcChannelOutOfRangeFails) {
    RegisterMaxAudioPrimitives();
    P<Fun> fun;
    ASSERT_TRUE(th_.compile("16 adc", fun, true));
    EXPECT_THROW(fun->run(th_), int);
}

TEST_F(HostAudioTest, InterruptStopsEvaluationsNotPlayers) {
    P<Fun> fun;
    ASSERT_TRUE(th_.compile("440 0 sinosc .5 * play", fun, true));
    fun->run(th_);
    gInterrupt = true;
    const float peak = renderPeak();
    EXPECT_THROW(fun->run(th_), int);  // this thread is not rendering: interrupted
    gInterrupt = false;
    EXPECT_GT(peak, 0.4f);
}

namespace {

// 4096 frames of `code` from a thread created at the current vm.ar block size.
std::vector<float> renderAtBlockSize(MaxAudioBackend& backend, const char* code)
{
    Thread th;
    P<Fun> fun;
    EXPECT_TRUE(th.compile(code, fun, true));
    fun->run(th);
    std::vector<float> out(4096);
    for (int t = 0; t < 4096; t += kFrames) {
        float* outs[1] = { out.data() + t };
        backend.render(outs, 1, kFrames);
    }
    backend.stopAll();
    return out;
}

} // namespace

TEST_F(HostAudioTest, BlockSizeDoesNotChangeTheSignal) {
    const char* code = "1 setseed 300 0 sinosc 3 0 lfsaw 2 * 1k * 1.5k + .3 rlpf2 .5 * play";
    auto at512 = renderAtBlockSize(*backend_, code);
    vm.setBlockSize(128);
    EXPECT_EQ(vm.ar.blockSize, 128);
    EXPECT_EQ(vm.kr.blockSize, 1);
    auto at128 = renderAtBlockSize(*backend_, code);
    vm.setBlockSize(100);  // not a multiple of the control block: ignored
    EXPECT_EQ(vm.ar.blockSize, 128);
    vm.setBlockSize(kDefaultZBlockSize);
    EXPECT_GT(*std::max_element(at512.begin(), at512.end()), 0.1f);
    EXPECT_EQ(at512, at128);
}

TEST_F(HostAudioTest, AdcLeadCoversARenderAhead) {
    // A render thread keeps `ahead` host blocks rendered before their input arrives.
    RegisterMaxAudioPrimitives();
    const int hostFrames = 128, ahead = 4;
    MaxMSPSetInputLead(ahead * hostFrames);
    P<Fun> fun;
    ASSERT_TRUE(th_.compile("0 adc play", fun, true));
    fun->run(th_);
    static int64_t frame = 1 << 20;  // apart from the other tests' input
    std::vector<float> in(hostFrames), out(hostFrames);
    const float* ins[1] = { in.data() };
    float* outs[1] = { out.data() };
    Rendered r;
    auto write = [&] {
        for (int i = 0; i < hostFrames; ++i) in[i] = float(++frame);
        MaxMSPSetInputBuffers(ins, 1, hostFrames);
        r.in0.insert(r.in0.end(), in.begin(), in.end());
    };
    auto render = [&] {
        backend_->render(outs, 1, hostFrames);
        r.out0.insert(r.out0.end(), out.begin(), out.end());
    };
    write();
    for (int b = 0; b < ahead; ++b) render();  // the FIFO fills
    for (int b = 0; b < 200; ++b) {
        write();
        render();
    }
    MaxMSPSetInputLead(0);
    // output frame t holds input frame t - delay: the input list is offset by the prefill
    r.in0.insert(r.in0.begin(), size_t(ahead * hostFrames), 0.f);
    int delay = constantDelay(r, 1.f, 2 * hostFrames + ahead * hostFrames + th_.rate.blockSize);
    EXPECT_GE(delay, 0);
}

namespace {

// 2048 frames of what `code` plays, evaluated on `th`
std::vector<float> renderCode(Thread& th, MaxAudioBackend& backend, const char* code)
{
    P<Fun> fun;
    EXPECT_TRUE(th.compile(code, fun, true));
    fun->run(th);
    std::vector<float> out(2048);
    for (int t = 0; t < 2048; t += kFrames) {
        float* outs[1] = { out.data() + t };
        backend.render(outs, 1, kFrames);
    }
    backend.stopAll();
    return out;
}

double popNumber(Thread& th, const char* code)
{
    P<Fun> fun;
    EXPECT_TRUE(th.compile(code, fun, true));
    fun->run(th);
    return th.pop().asFloat();
}

} // namespace

TEST_F(HostAudioTest, SetseedFixesWhatPlayersDraw) {
    // noise draws from the player's thread; ola's voices run on it too
    for (const char* code : { "1 setseed .1 white play",
                              "1 setseed \\[0 1 rand 300 * 200 + 0 sinosc .1 * 600 N] .01 .02 randz 1 1 ola play" }) {
        SCOPED_TRACE(code);
        auto a = renderCode(th_, *backend_, code);
        auto b = renderCode(th_, *backend_, code);
        EXPECT_GT(*std::max_element(a.begin(), a.end()), 0.f);
        EXPECT_EQ(a, b);
    }
    // without setseed each evaluation draws anew
    auto a = renderCode(th_, *backend_, ".1 white play");
    auto b = renderCode(th_, *backend_, ".1 white play");
    EXPECT_NE(a, b);
}

TEST_F(HostAudioTest, StartingAPlayerLeavesTheParentsNumbers) {
    // the noise UGen seeds itself from this thread when made, played or not; play itself must not draw
    const double alone = popNumber(th_, "1 setseed .1 white pop 0 1 rand");
    const double afterPlay = popNumber(th_, "1 setseed .1 white play 0 1 rand");
    backend_->stopAll();
    EXPECT_EQ(alone, afterPlay);
}
