// Unit generator edge cases.
#include <gtest/gtest.h>
#include "test_common.hpp"
#include "VM.hpp"
#include <cmath>

class UGenTest : public SapfTestBase {
protected:
    Thread th;

    // Evaluates code that leaves a signal, returns its first n samples.
    std::vector<Z> samples(const char* code, int n) {
        P<Fun> fun;
        if (!th.compile(code, fun, true)) throw errSyntax;
        fun->apply(th);
        V v = th.pop();
        P<List> list = ((List*)v.o())->pack(th, n);
        Array* a = list->mArray();
        return std::vector<Z>(a->z(), a->z() + a->size());
    }
};

// dsf1's normalization (a-1)/(a^(N+1)-1) and denominator were both 0/0 at coef 1, giving NaN.
TEST_F(UGenTest, Dsf1FiniteAtCoefOne) {
    for (Z x : samples("400 5 3 1 16 dsf1 512 N", 512)) {
        ASSERT_TRUE(std::isfinite(x));
    }
}
