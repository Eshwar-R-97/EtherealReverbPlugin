#include <catch2/catch_test_macros.hpp>
#include <catch2/catch_approx.hpp>
#include "DSPEngine.h"
#include <cmath>

// ─────────────────────────────────────────────────────────────────────────────
//  GranularShimmer
// ─────────────────────────────────────────────────────────────────────────────

TEST_CASE ("GranularShimmer - reset zeroes the internal buffer", "[Shimmer]")
{
    GranularShimmer g;

    // Fill the buffer with non-zero data
    for (int i = 0; i < GranularShimmer::kBufSize; ++i)
        g.process (1.0f, 1.0f);

    g.reset();

    // After reset the buffer is all zeros, so a single silent process call
    // should produce ~0 (both read heads point into the zeroed buffer)
    const float out = g.process (0.0f, 1.0f);
    REQUIRE (out == Catch::Approx (0.0f).margin (1e-6f));
}

TEST_CASE ("GranularShimmer - Hann window pair always sums to 1.0", "[Shimmer]")
{
    // Two grains are at phase0 and phase1 = phase0 + kBufSize/2.
    // w(ph) = 0.5 - 0.5*cos(2π*ph/kBufSize)
    // w0 + w1 = (0.5 - 0.5cos θ) + (0.5 + 0.5cos θ) = 1.0 exactly.
    // Test this over a representative sweep of phases.
    const float kN     = (float) GranularShimmer::kBufSize;
    const float scale  = juce::MathConstants<float>::twoPi / kN;

    for (float ph0 = 0.0f; ph0 < kN; ph0 += 500.0f)
    {
        float ph1 = ph0 + kN * 0.5f;
        if (ph1 >= kN) ph1 -= kN;

        const float w0 = 0.5f - 0.5f * std::cos (scale * ph0);
        const float w1 = 0.5f - 0.5f * std::cos (scale * ph1);
        REQUIRE (w0 + w1 == Catch::Approx (1.0f).margin (1e-5f));
    }
}

TEST_CASE ("GranularShimmer - DC input at ratio 1.0 produces DC output after warmup", "[Shimmer]")
{
    GranularShimmer g;
    g.reset();

    // Fill the entire circular buffer with 1.0f at unity pitch ratio
    float out = 0.0f;
    for (int i = 0; i < GranularShimmer::kBufSize; ++i)
        out = g.process (1.0f, 1.0f);

    // With a full buffer of DC, both grains read 1.0f, and their Hann weights sum to 1.0,
    // so the output must be 1.0f.
    REQUIRE (out == Catch::Approx (1.0f).margin (1e-3f));
}

TEST_CASE ("GranularShimmer - two independent instances produce identical output", "[Shimmer]")
{
    GranularShimmer a, b;
    a.reset();
    b.reset();

    for (int i = 0; i < 200; ++i)
    {
        const float in   = std::sin ((float) i * 0.3f);
        const float outA = a.process (in, 1.5f);
        const float outB = b.process (in, 1.5f);
        REQUIRE (outA == Catch::Approx (outB).margin (1e-6f));
    }
}
