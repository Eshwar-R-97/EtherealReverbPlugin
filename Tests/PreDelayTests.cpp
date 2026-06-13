#include <catch2/catch_test_macros.hpp>
#include <catch2/catch_approx.hpp>
#include "DSPEngine.h"

// ── Helpers ───────────────────────────────────────────────────────────────────

static DSPParams makeParams (float preDelayMs, float mix = 1.0f)
{
    DSPParams p;
    p.preDelay = preDelayMs;
    p.mix      = mix;
    return p;
}

// Fill a stereo buffer: impulse at sample 0 on both channels, silence elsewhere
static juce::AudioBuffer<float> makeStereoImpulse (int numSamples)
{
    juce::AudioBuffer<float> buf (2, numSamples);
    buf.clear();
    buf.setSample (0, 0, 1.0f);
    buf.setSample (1, 0, 1.0f);
    return buf;
}

// ── Tests ─────────────────────────────────────────────────────────────────────

TEST_CASE ("Pre-delay - 0ms is a true pass-through", "[PreDelay]")
{
    DSPEngine engine;
    engine.prepare (44100.0, 512);

    auto buffer = makeStereoImpulse (32);
    engine.process (buffer, makeParams (0.0f, 1.0f));

    // With 0ms pre-delay the impulse passes immediately. Bloom starts at ~0 and
    // attenuates the wet portion by ~0.06%, so the output is very close to 1.0.
    REQUIRE (buffer.getSample (0, 0) == Catch::Approx (1.0f).margin (1e-2f));
    REQUIRE (buffer.getSample (1, 0) == Catch::Approx (1.0f).margin (1e-2f));
}

TEST_CASE ("Pre-delay - output is silent between the impulse and the delayed onset", "[PreDelay]")
{
    // The pre-delay holds the impulse and releases it at delaySamples.
    // That released impulse then enters the (empty) FDN, so the FDN output at
    // delaySamples is still 0 — the wet echo only appears after a full FDN roundtrip.
    // However samples 1 … delaySamples-1 must be exactly silent because:
    //   dry=0 there, and the FDN has received no input yet (pre-delay buffer was empty).
    const double sampleRate   = 44100.0;
    const float  delayMs      = 10.0f;
    const int    delaySamples = (int) (delayMs * 0.001f * (float) sampleRate); // 441

    DSPEngine engine;
    engine.prepare (sampleRate, 512);

    const int totalSamples = delaySamples + 64;
    juce::AudioBuffer<float> buffer (2, totalSamples);
    buffer.clear();
    buffer.setSample (0, 0, 1.0f);
    buffer.setSample (1, 0, 1.0f);

    engine.process (buffer, makeParams (delayMs, 1.0f));

    // Samples 1 to delaySamples-1: dry=0, FDN has no input yet → must be silent
    for (int i = 1; i < delaySamples; ++i)
        REQUIRE (buffer.getSample (0, i) == Catch::Approx (0.0f).margin (1e-6f));
}

TEST_CASE ("Pre-delay - dry signal is not delayed by the pre-delay buffer", "[PreDelay]")
{
    // The dry signal bypasses the pre-delay path entirely (it's captured before the delay).
    // output = dry + (wet - dry) * mix * bloom.
    // With DC dry=1 and an empty FDN (wet≈0 at t=0), output ≈ 1 - tiny_bloom_factor.
    // This confirms the dry signal arrives immediately regardless of preDelay setting.
    const double sampleRate   = 44100.0;
    const float  delayMs      = 50.0f; // large pre-delay

    DSPEngine engine;
    engine.prepare (sampleRate, 512);

    juce::AudioBuffer<float> buffer (2, 32);
    for (int ch = 0; ch < 2; ++ch)
        for (int i = 0; i < 32; ++i)
            buffer.setSample (ch, i, 1.0f);

    engine.process (buffer, makeParams (delayMs, 1.0f));

    // Dry is always present — output at sample 0 must be close to 1.0
    REQUIRE (buffer.getSample (0, 0) == Catch::Approx (1.0f).margin (1e-2f));
    REQUIRE (buffer.getSample (1, 0) == Catch::Approx (1.0f).margin (1e-2f));
}

TEST_CASE ("Pre-delay - left and right channels are independent", "[PreDelay]")
{
    DSPEngine engine;
    engine.prepare (44100.0, 512);

    juce::AudioBuffer<float> buffer (2, 32);
    buffer.clear();
    buffer.setSample (0, 0, 1.0f);  // impulse on L only, R stays silent

    engine.process (buffer, makeParams (0.0f, 1.0f));

    REQUIRE (buffer.getSample (0, 0) == Catch::Approx (1.0f).margin (1e-2f)); // L has signal
    REQUIRE (buffer.getSample (1, 0) == Catch::Approx (0.0f).margin (1e-6f)); // R is silent
}

TEST_CASE ("Pre-delay - mix=0 outputs dry signal regardless of delay", "[PreDelay]")
{
    DSPEngine engine;
    engine.prepare (44100.0, 512);

    auto buffer = makeStereoImpulse (32);
    engine.process (buffer, makeParams (50.0f, 0.0f)); // 50ms delay but mix=0

    // Stage 6 blend: dryL + (wetL - dryL) * 0 = dryL
    REQUIRE (buffer.getSample (0, 0) == Catch::Approx (1.0f));
    REQUIRE (buffer.getSample (1, 0) == Catch::Approx (1.0f));
}

TEST_CASE ("Pre-delay - reset clears buffered audio", "[PreDelay]")
{
    const double sampleRate   = 44100.0;
    const float  delayMs      = 10.0f;
    const int    delaySamples = (int) (delayMs * 0.001f * (float) sampleRate);

    DSPEngine engine;
    engine.prepare (sampleRate, 512);

    // Feed an impulse so it sits in the pre-delay buffer
    {
        juce::AudioBuffer<float> buffer (2, delaySamples / 2);
        buffer.clear();
        buffer.setSample (0, 0, 1.0f);
        engine.process (buffer, makeParams (delayMs, 1.0f));
    }

    // Reset should flush all buffered audio
    engine.reset();

    // After reset, the delay buffer is zeroed — output should be silent
    {
        juce::AudioBuffer<float> buffer (2, delaySamples + 16);
        buffer.clear();
        engine.process (buffer, makeParams (delayMs, 1.0f));

        for (int i = 0; i < buffer.getNumSamples(); ++i)
            REQUIRE (buffer.getSample (0, i) == Catch::Approx (0.0f));
    }
}
