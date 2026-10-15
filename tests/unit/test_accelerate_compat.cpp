// The Accelerate calls sapf makes, as AccelerateCompat.hpp emulates them off Apple. On Apple the same
// tests check Accelerate itself, so they pin the conventions the compat layer must follow.
#include <gtest/gtest.h>
#include "sapf/AccelerateCompat.hpp"
#include <cmath>
#include <complex>
#include <vector>

namespace {

const int kLog2n = 4;
const int kN = 1 << kLog2n;

std::vector<double> signal()
{
    std::vector<double> x(kN);
    for (int i = 0; i < kN; ++i) x[i] = std::sin(1.3 * i) + 0.25 * std::cos(0.4 * i * i) + 0.1 * i;
    return x;
}

std::complex<double> dft(const std::vector<double>& x, int k)
{
    std::complex<double> sum = 0.;
    for (int i = 0; i < kN; ++i) sum += x[i] * std::polar(1., -2. * M_PI * k * i / kN);
    return sum;
}

} // namespace

TEST(AccelerateCompat, InterleavedStridesCountDoubles) {
    std::vector<double> interleaved = { 1, 2, 3, 4, 5, 6 };  // three complex values
    std::vector<double> re(3), im(3);
    DSPDoubleSplitComplex split = { re.data(), im.data() };
    vDSP_ctozD((DSPDoubleComplex*)interleaved.data(), 2, &split, 1, 3);
    EXPECT_EQ(re, (std::vector<double>{ 1, 3, 5 }));
    EXPECT_EQ(im, (std::vector<double>{ 2, 4, 6 }));

    std::vector<double> back(6);
    vDSP_ztocD(&split, 1, (DSPDoubleComplex*)back.data(), 2, 3);
    EXPECT_EQ(back, interleaved);
}

TEST(AccelerateCompat, RealFftIsPackedAndScaledAsAccelerates) {
    std::vector<double> x = signal();
    std::vector<double> re(kN / 2), im(kN / 2), outRe(kN / 2), outIm(kN / 2);
    DSPDoubleSplitComplex in = { re.data(), im.data() }, out = { outRe.data(), outIm.data() };
    vDSP_ctozD((DSPDoubleComplex*)x.data(), 2, &in, 1, kN / 2);
    FFTSetupD setup = vDSP_create_fftsetupD(kLog2n, kFFTRadix2);

    vDSP_fft_zropD(setup, &in, 1, &out, 1, kLog2n, kFFTDirection_Forward);
    EXPECT_NEAR(outRe[0], 2. * dft(x, 0).real(), 1e-9);
    EXPECT_NEAR(outIm[0], 2. * dft(x, kN / 2).real(), 1e-9);
    for (int k = 1; k < kN / 2; ++k) {
        EXPECT_NEAR(outRe[k], 2. * dft(x, k).real(), 1e-9) << k;
        EXPECT_NEAR(outIm[k], 2. * dft(x, k).imag(), 1e-9) << k;
    }

    // forward then inverse scales by 2N
    vDSP_fft_zripD(setup, &out, 1, kLog2n, kFFTDirection_Inverse);
    for (int k = 0; k < kN / 2; ++k) {
        EXPECT_NEAR(outRe[k], 2. * kN * x[2 * k], 1e-9) << k;
        EXPECT_NEAR(outIm[k], 2. * kN * x[2 * k + 1], 1e-9) << k;
    }
    vDSP_destroy_fftsetupD(setup);
}
