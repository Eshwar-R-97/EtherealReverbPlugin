#include <catch2/catch_test_macros.hpp>
#include <catch2/catch_approx.hpp>
#include "DSPEngine.h"
#include <cmath>

// ─────────────────────────────────────────────────────────────────────────────
//  SSBShimmer
// ─────────────────────────────────────────────────────────────────────────────

TEST_CASE ("SSBShimmer - zero input produces zero output", "[SSB]")
{
    SSBShimmer s;
    s.reset();

    const float phaseInc = juce::MathConstants<float>::twoPi * 20.0f / 44100.0f;

    for (int i = 0; i < 1000; ++i)
        REQUIRE (s.process (0.0f, phaseInc) == Catch::Approx (0.0f).margin (1e-7f));
}

TEST_CASE ("SSBShimmer - output is bounded for unit-amplitude input", "[SSB]")
{
    SSBShimmer s;
    s.reset();

    const float phaseInc = juce::MathConstants<float>::twoPi * 15.0f / 44100.0f;

    for (int i = 0; i < 44100; ++i)
    {
        const float in  = std::sin ((float) i * juce::MathConstants<float>::twoPi * 440.0f / 44100.0f);
        const float out = s.process (in, phaseInc);
        // IIR allpass is BIBO stable; amplitude should stay well within ±2
        REQUIRE (std::abs (out) < 2.0f);
    }
}

TEST_CASE ("SSBShimmer - two instances starting from reset are identical", "[SSB]")
{
    SSBShimmer a, b;
    a.reset();
    b.reset();

    const float phaseInc = juce::MathConstants<float>::twoPi * 20.0f / 44100.0f;

    for (int i = 0; i < 500; ++i)
    {
        const float in   = std::sin ((float) i * 0.07f);
        const float outA = a.process (in, phaseInc);
        const float outB = b.process (in, phaseInc);
        REQUIRE (outA == Catch::Approx (outB).margin (1e-6f));
    }
}

TEST_CASE ("SSBShimmer - DC input at phase 0 outputs the allpass-filtered DC value", "[SSB]")
{
    SSBShimmer s;
    s.reset();

    // With phaseInc = 0: phase stays at 0, cos(0) = 1, sin(0) = 0
    // output = sigA * 1 - sigB * 0 = sigA (branch-A allpass of input)
    // For DC input, after settling the allpass output also approaches DC.
    float lastOut = 0.0f;
    for (int i = 0; i < 5000; ++i)
        lastOut = s.process (1.0f, 0.0f);

    // After many samples with phaseInc=0, the allpass state has settled.
    // Branch A allpass of DC: y = a*(x - y_prev) + x_prev → steady state = DC (since a*(1-y) + y = 1).
    // Both allpass sections pass DC at unity when settled.
    REQUIRE (lastOut == Catch::Approx (1.0f).margin (0.01f));
}

TEST_CASE ("SSBShimmer - non-zero phaseInc produces oscillating output for DC input", "[SSB]")
{
    SSBShimmer s;
    s.reset();

    const float phaseInc = juce::MathConstants<float>::twoPi * 100.0f / 44100.0f; // 100Hz shift

    // With DC input the SSB output should oscillate (quadrature modulation).
    // Collect 441 samples (10ms) and check that not all samples are identical.
    float first = s.process (1.0f, phaseInc);
    bool hasVariation = false;
    for (int i = 1; i < 441; ++i)
    {
        const float out = s.process (1.0f, phaseInc);
        if (std::abs (out - first) > 0.01f)
        {
            hasVariation = true;
            break;
        }
    }
    REQUIRE (hasVariation);
}
