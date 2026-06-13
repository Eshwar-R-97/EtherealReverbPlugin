#include <catch2/catch_test_macros.hpp>
#include <catch2/catch_approx.hpp>
#include "DSPEngine.h"
#include <cmath>

// ─────────────────────────────────────────────────────────────────────────────
//  Helpers
// ─────────────────────────────────────────────────────────────────────────────

// Params with all non-FDN stages neutralised: no shimmer, no reverse, mix=1,
// modDepth=0 (no LFO), diffusion=0 (all-pass transparent), tiltEQ=0.
static DSPParams fdnParams (float decaySeconds = 3.0f, float roomSize = 0.5f)
{
    DSPParams p;
    p.decay      = decaySeconds;
    p.roomSize   = roomSize;
    p.mix        = 1.0f;
    p.shimmer    = 0.0f;
    p.reverse    = false;
    p.modDepth   = 0.0f;
    p.diffusion  = 0.0f;
    p.tiltEQ     = 0.0f;
    p.decayColor = 0.0f;
    p.freeze     = false;
    return p;
}

static float blockRMS (const juce::AudioBuffer<float>& buf, int startSample, int numSamples)
{
    float sum = 0.0f;
    for (int i = startSample; i < startSample + numSamples; ++i)
        sum += buf.getSample (0, i) * buf.getSample (0, i);
    return std::sqrt (sum / (float) numSamples);
}

// ─────────────────────────────────────────────────────────────────────────────
//  Tests
// ─────────────────────────────────────────────────────────────────────────────

TEST_CASE ("FDN - reverb tail is non-zero after sustained input", "[FDN]")
{
    DSPEngine engine;
    engine.prepare (44100.0, 512);

    // A single impulse produces a valid reverb tail but bloom starts at ~0 and
    // only reaches 5.6e-4 after one sample, making the output near the noise floor.
    // Use 50ms of DC to bring bloom close to 1.0 and fill all FDN delay lines.
    {
        juce::AudioBuffer<float> warmup (2, 2205); // 50ms
        for (int i = 0; i < 2205; ++i)
        {
            warmup.setSample (0, i, 1.0f);
            warmup.setSample (1, i, 1.0f);
        }
        engine.process (warmup, fdnParams (3.0f));
    }

    // Run silence — FDN drains naturally. Bloom decays on the 3s release curve.
    // After 50ms of signal, bloom ≈ 1 - exp(-50/40) ≈ 0.71. Plenty to see the tail.
    juce::AudioBuffer<float> tail (2, 2205);
    tail.clear();
    engine.process (tail, fdnParams (3.0f));

    const float rms = blockRMS (tail, 0, 100);
    REQUIRE (rms > 1e-3f);
}

TEST_CASE ("FDN - longer decay parameter retains more energy at a later point", "[FDN]")
{
    const double sr = 44100.0;
    const int    N  = (int) (sr * 0.5); // 500ms buffer

    // Measure RMS at 100ms into the reverb tail
    auto measureTailRMS = [&] (float decaySeconds) -> float
    {
        DSPEngine engine;
        engine.prepare (sr, 512);

        juce::AudioBuffer<float> buf (2, N);
        buf.clear();
        buf.setSample (0, 0, 1.0f);
        buf.setSample (1, 0, 1.0f);

        engine.process (buf, fdnParams (decaySeconds));
        return blockRMS (buf, 4410, 200);
    };

    const float rmsShort = measureTailRMS (0.3f);
    const float rmsLong  = measureTailRMS (5.0f);

    // Longer decay must hold more energy at the same elapsed time
    REQUIRE (rmsLong > rmsShort);
}

TEST_CASE ("FDN - Householder feedback gain formula produces correct RT60", "[FDN]")
{
    // g = 10^(-3 * N / (decay * sr))
    // After decay*sr samples, the gain accumulated N-times should equal 10^-3 = -60dB
    const float sr    = 44100.0f;
    const float decay = 2.0f;
    // Use the shortest tap for a clean test: kFDNTimes[0] = 19.7ms
    const float tapMs = 19.7f;
    const int   N     = (int) (tapMs * 0.001f * sr * 0.5f); // roomSize=0.5
    const float g     = std::pow (10.0f, -3.0f * (float) N / (decay * sr));

    // After decay/tapLength_in_seconds round trips the amplitude should reach 10^-3
    const float numRoundTrips = (decay * sr) / (float) N;
    const float finalAmp      = std::pow (g, numRoundTrips);

    REQUIRE (finalAmp == Catch::Approx (1e-3f).epsilon (0.01f));
}

TEST_CASE ("FDN - freeze maintains energy across successive silent blocks", "[FDN]")
{
    DSPEngine engine;
    engine.prepare (44100.0, 512);

    // Warm up with sustained DC so the FDN fills AND bloom rises close to 1.0.
    // After 100ms (4410 samples) bloom ≈ 1 - exp(-100/40) ≈ 0.92 — well above noise.
    {
        juce::AudioBuffer<float> warm (2, 4410);
        for (int i = 0; i < 4410; ++i)
        {
            warm.setSample (0, i, 1.0f);
            warm.setSample (1, i, 1.0f);
        }
        engine.process (warm, fdnParams (3.0f));
    }

    // Switch to freeze + silence. Bloom now decays at the 3s release rate,
    // so it stays high enough to measure for many seconds.
    DSPParams fp = fdnParams (3.0f);
    fp.freeze    = true;

    auto measureBlock = [&]() -> float
    {
        juce::AudioBuffer<float> buf (2, 441); // 10ms
        buf.clear();
        engine.process (buf, fp);
        return blockRMS (buf, 0, 441);
    };

    const float e1 = measureBlock();
    const float e2 = measureBlock();
    const float e3 = measureBlock();

    REQUIRE (e1 > 1e-4f);      // significant tail is alive
    REQUIRE (e2 >= e1 * 0.8f); // freeze — no meaningful decay between 10ms blocks
    REQUIRE (e3 >= e2 * 0.8f);
}

TEST_CASE ("FDN - silence input produces silence output after reset", "[FDN]")
{
    DSPEngine engine;
    engine.prepare (44100.0, 512);

    // Build up energy
    {
        juce::AudioBuffer<float> warm (2, 1024);
        warm.clear();
        warm.setSample (0, 0, 1.0f);
        warm.setSample (1, 0, 1.0f);
        engine.process (warm, fdnParams (3.0f));
    }

    engine.reset();

    juce::AudioBuffer<float> silent (2, 1024);
    silent.clear();
    engine.process (silent, fdnParams (3.0f));

    for (int i = 0; i < silent.getNumSamples(); ++i)
        REQUIRE (silent.getSample (0, i) == Catch::Approx (0.0f).margin (1e-6f));
}
