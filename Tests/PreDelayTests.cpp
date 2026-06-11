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

    // With 0ms pre-delay, the impulse must appear at sample 0
    REQUIRE (buffer.getSample (0, 0) == Catch::Approx (1.0f));
    REQUIRE (buffer.getSample (1, 0) == Catch::Approx (1.0f));
}

TEST_CASE ("Pre-delay - impulse arrives at exactly delaySamples", "[PreDelay]")
{
    const double sampleRate  = 44100.0;
    const float  delayMs     = 10.0f;
    const int    delaySamples = (int) (delayMs * 0.001f * (float) sampleRate); // 441

    DSPEngine engine;
    engine.prepare (sampleRate, 512);

    // Buffer must be large enough for the impulse to arrive within one process call
    const int totalSamples = delaySamples + 64;
    juce::AudioBuffer<float> buffer (2, totalSamples);
    buffer.clear();
    buffer.setSample (0, 0, 1.0f);
    buffer.setSample (1, 0, 1.0f);

    engine.process (buffer, makeParams (delayMs, 1.0f));

    // One sample before the delay — still silent
    REQUIRE (buffer.getSample (0, delaySamples - 1) == Catch::Approx (0.0f));
    // Exactly at delaySamples — impulse arrives
    REQUIRE (buffer.getSample (0, delaySamples) == Catch::Approx (1.0f));
    REQUIRE (buffer.getSample (1, delaySamples) == Catch::Approx (1.0f));
}

TEST_CASE ("Pre-delay - first N output samples are silent on a fresh engine", "[PreDelay]")
{
    const double sampleRate   = 44100.0;
    const float  delayMs      = 5.0f;
    const int    delaySamples = (int) (delayMs * 0.001f * (float) sampleRate); // 220

    DSPEngine engine;
    engine.prepare (sampleRate, 512);

    // Feed a DC signal
    juce::AudioBuffer<float> buffer (2, delaySamples + 16);
    buffer.clear();
    for (int ch = 0; ch < 2; ++ch)
        for (int i = 0; i < buffer.getNumSamples(); ++i)
            buffer.setSample (ch, i, 1.0f);

    engine.process (buffer, makeParams (delayMs, 1.0f));

    // Buffers were initialised to zero, so the first delaySamples outputs are 0
    for (int i = 0; i < delaySamples; ++i)
    {
        REQUIRE (buffer.getSample (0, i) == Catch::Approx (0.0f));
        REQUIRE (buffer.getSample (1, i) == Catch::Approx (0.0f));
    }

    // After the delay has elapsed, DC passes through
    REQUIRE (buffer.getSample (0, delaySamples) == Catch::Approx (1.0f));
}

TEST_CASE ("Pre-delay - left and right channels are independent", "[PreDelay]")
{
    DSPEngine engine;
    engine.prepare (44100.0, 512);

    juce::AudioBuffer<float> buffer (2, 32);
    buffer.clear();
    buffer.setSample (0, 0, 1.0f);  // impulse on L only, R stays silent

    engine.process (buffer, makeParams (0.0f, 1.0f));

    REQUIRE (buffer.getSample (0, 0) == Catch::Approx (1.0f)); // L has signal
    REQUIRE (buffer.getSample (1, 0) == Catch::Approx (0.0f)); // R is silent
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
