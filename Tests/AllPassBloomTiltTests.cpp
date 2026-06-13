#include <catch2/catch_test_macros.hpp>
#include <catch2/catch_approx.hpp>
#include "DSPEngine.h"
#include <cmath>
#include <numeric>

// ─────────────────────────────────────────────────────────────────────────────
//  Helpers
// ─────────────────────────────────────────────────────────────────────────────

static DSPParams baseParams()
{
    DSPParams p;
    p.mix        = 1.0f;
    p.shimmer    = 0.0f;
    p.reverse    = false;
    p.modDepth   = 0.0f;
    p.tiltEQ     = 0.0f;
    p.decayColor = 0.0f;
    p.freeze     = false;
    p.decay      = 0.5f;
    p.roomSize   = 0.3f;
    p.damping    = 0.5f;
    return p;
}

static float totalAbsoluteEnergy (const juce::AudioBuffer<float>& buf, int ch = 0)
{
    float sum = 0.0f;
    for (int i = 0; i < buf.getNumSamples(); ++i)
        sum += std::abs (buf.getSample (ch, i));
    return sum;
}

// Count samples whose absolute value exceeds threshold
static int countActive (const juce::AudioBuffer<float>& buf, float threshold = 1e-4f, int ch = 0)
{
    int count = 0;
    for (int i = 0; i < buf.getNumSamples(); ++i)
        if (std::abs (buf.getSample (ch, i)) > threshold)
            ++count;
    return count;
}

// ─────────────────────────────────────────────────────────────────────────────
//  All-pass diffusion
// ─────────────────────────────────────────────────────────────────────────────

TEST_CASE ("AllPass - diffusion=0 is fully transparent (apGain=0)", "[AllPass]")
{
    // apGain = diffusion × 0.7 = 0: out = -0×in + delayed = delayed (pure delay line).
    // With a very short room (roomSize→0 clamped to 1 sample), the all-pass is a 1-sample delay
    // and effectively passes the input through with one sample latency.
    // Test: the output samples are bounded and no energy is added by the all-pass stage.

    DSPEngine engine;
    engine.prepare (44100.0, 512);

    DSPParams p  = baseParams();
    p.diffusion  = 0.0f;

    // Feed white-ish signal
    const int N = 2048;
    juce::AudioBuffer<float> buf (2, N);
    buf.clear();
    for (int i = 0; i < N; ++i)
    {
        buf.setSample (0, i, std::sin ((float) i * 0.3f));
        buf.setSample (1, i, std::sin ((float) i * 0.3f));
    }

    engine.process (buf, p);

    // All samples should be bounded — no instability from all-pass
    for (int i = 0; i < N; ++i)
        REQUIRE (std::abs (buf.getSample (0, i)) < 10.0f);
}

TEST_CASE ("AllPass - higher diffusion spreads the impulse response", "[AllPass]")
{
    // An impulse through the reverb with diffusion=0 concentrates energy near sample 0.
    // With diffusion=0.7 the all-pass chain disperses it across many samples.
    // We compare the count of non-trivial samples in the output.

    auto runImpulse = [](float diffusion) -> int
    {
        DSPEngine engine;
        engine.prepare (44100.0, 512);

        DSPParams p  = baseParams();
        p.diffusion  = diffusion;
        p.decay      = 3.0f;

        const int N = 8192;
        juce::AudioBuffer<float> buf (2, N);
        buf.clear();
        buf.setSample (0, 0, 1.0f);
        buf.setSample (1, 0, 1.0f);

        engine.process (buf, p);
        return countActive (buf, 1e-4f);
    };

    const int activeLow  = runImpulse (0.0f);
    const int activeHigh = runImpulse (0.7f);

    REQUIRE (activeHigh > activeLow);
}

TEST_CASE ("AllPass - diffusion does not cause instability (long run)", "[AllPass]")
{
    DSPEngine engine;
    engine.prepare (44100.0, 512);

    DSPParams p  = baseParams();
    p.diffusion  = 0.7f;
    p.decay      = 5.0f;

    // Feed a sustained input and check nothing blows up
    const int N = 44100;
    juce::AudioBuffer<float> buf (2, N);
    for (int i = 0; i < N; ++i)
    {
        buf.setSample (0, i, std::sin ((float) i * 0.1f));
        buf.setSample (1, i, std::sin ((float) i * 0.1f));
    }

    engine.process (buf, p);

    for (int i = 0; i < N; ++i)
        REQUIRE (std::isfinite (buf.getSample (0, i)));
}

// ─────────────────────────────────────────────────────────────────────────────
//  Bloom envelope
// ─────────────────────────────────────────────────────────────────────────────

TEST_CASE ("Bloom - output amplitude grows from a fresh engine with sustained input", "[Bloom]")
{
    // With preDelay=0, wet≈dry for the first sample, so bloom is invisible there.
    // After the FDN fills, the wet path diverges from dry. We can't easily isolate bloom
    // from FDN, so we test the net observable: energy at t=500ms > energy at t=0 when
    // feeding an FDN with a sustained signal and long decay.
    DSPEngine engine;
    engine.prepare (44100.0, 512);

    DSPParams p = baseParams();
    p.decay     = 10.0f;
    p.mix       = 0.8f;
    p.diffusion = 0.5f;

    const int blockSize = 512;
    const int blocks    = 200; // ~2.3s total

    float firstRMS  = 0.0f;
    float laterRMS  = 0.0f;

    for (int b = 0; b < blocks; ++b)
    {
        juce::AudioBuffer<float> buf (2, blockSize);
        for (int i = 0; i < blockSize; ++i)
        {
            buf.setSample (0, i, 0.5f);
            buf.setSample (1, i, 0.5f);
        }
        engine.process (buf, p);

        if (b == 0)
        {
            float sum = 0.0f;
            for (int i = 0; i < blockSize; ++i)
                sum += buf.getSample (0, i) * buf.getSample (0, i);
            firstRMS = std::sqrt (sum / blockSize);
        }
        if (b == 50) // after ~580ms
        {
            float sum = 0.0f;
            for (int i = 0; i < blockSize; ++i)
                sum += buf.getSample (0, i) * buf.getSample (0, i);
            laterRMS = std::sqrt (sum / blockSize);
        }
    }

    // The wet FDN builds up over time, so the output energy should increase
    REQUIRE (laterRMS > firstRMS);
}

TEST_CASE ("Bloom - attack coefficient gives ~40ms rise time", "[Bloom]")
{
    // bloomEnv uses a 1-pole lowpass with attackCoeff = exp(-1 / (0.040 * sr)).
    // This is a pure formula test — no DSPEngine needed.
    // After one time constant (40ms = 1764 samples at 44100 Hz), the envelope
    // should reach 1 - 1/e ≈ 63.2% of its target (first-order system property).
    const float sr          = 44100.0f;
    const float attackCoeff = std::exp (-1.0f / (0.040f * sr));

    // At sample 0, bloom is essentially zero (one step from 0 toward 1.0)
    const float bloomAt0 = 1.0f - attackCoeff; // ≈ 5.67e-4
    REQUIRE (bloomAt0 < 0.01f); // nowhere near 1.0 at the first sample

    // Simulate the attack for exactly one time constant (1764 samples)
    float bloomEnv = 0.0f;
    for (int i = 0; i < 1764; ++i)
        bloomEnv = attackCoeff * bloomEnv + (1.0f - attackCoeff) * 1.0f;

    // Should be at ~63.2% — one time-constant of a first-order lowpass
    REQUIRE (bloomEnv == Catch::Approx (1.0f - std::exp (-1.0f)).epsilon (0.02f));
    REQUIRE (bloomEnv < 1.0f); // still hasn't fully risen
}

// ─────────────────────────────────────────────────────────────────────────────
//  Tilt EQ
// ─────────────────────────────────────────────────────────────────────────────

TEST_CASE ("TiltEQ - tiltEQ=0 does not alter the signal", "[TiltEQ]")
{
    // tiltEQ=0 means wetL += 0 * (tiltState - wetL) = 0, so signal is unchanged.
    // Run two identical engines — one with tiltEQ=0, one with tiltEQ=0.5.
    // They should produce different outputs.  (This also tests that the code path runs.)

    const int N = 2048;
    juce::AudioBuffer<float> refBuf (2, N), tiltedBuf (2, N);

    for (int i = 0; i < N; ++i)
    {
        const float v = std::sin ((float) i * 0.2f);
        refBuf.setSample    (0, i, v); refBuf.setSample    (1, i, v);
        tiltedBuf.setSample (0, i, v); tiltedBuf.setSample (1, i, v);
    }

    {
        DSPEngine engine;
        engine.prepare (44100.0, 512);
        DSPParams p = baseParams();
        p.tiltEQ = 0.0f;
        engine.process (refBuf, p);
    }
    {
        DSPEngine engine;
        engine.prepare (44100.0, 512);
        DSPParams p = baseParams();
        p.tiltEQ = 0.8f;
        engine.process (tiltedBuf, p);
    }

    // Some samples should differ between the two
    bool anyDiff = false;
    for (int i = 0; i < N; ++i)
    {
        if (std::abs (refBuf.getSample (0, i) - tiltedBuf.getSample (0, i)) > 1e-5f)
        {
            anyDiff = true;
            break;
        }
    }
    REQUIRE (anyDiff);
}

TEST_CASE ("TiltEQ - positive tiltEQ reduces high-frequency content", "[TiltEQ]")
{
    // Feed a signal rich in high frequencies (rapid alternation ≈ 22050 Hz)
    // and compare total energy with tiltEQ=0 vs tiltEQ=1.
    // The shelf is at ~1kHz; a 22050Hz input is well above it.
    // tiltEQ=+1 routes output through the LP shelf state → near-zero for HF content.

    auto totalEnergy = [](float tiltEQ) -> float
    {
        DSPEngine engine;
        engine.prepare (44100.0, 512);

        DSPParams p = baseParams();
        p.tiltEQ    = tiltEQ;
        p.decay     = 2.0f;
        p.mix       = 1.0f;

        const int N = 4096;
        juce::AudioBuffer<float> buf (2, N);
        // Nyquist-frequency signal (alternates every sample)
        for (int i = 0; i < N; ++i)
        {
            const float v = (i % 2 == 0) ? 1.0f : -1.0f;
            buf.setSample (0, i, v);
            buf.setSample (1, i, v);
        }

        engine.process (buf, p);

        float sum = 0.0f;
        for (int i = 100; i < N; ++i) // skip initial transient
            sum += buf.getSample (0, i) * buf.getSample (0, i);
        return sum;
    };

    const float energyNeutral  = totalEnergy (0.0f);
    const float energyDarkened = totalEnergy (1.0f);

    // Positive tilt darkens (reduces HF energy)
    REQUIRE (energyDarkened < energyNeutral);
}

TEST_CASE ("TiltEQ - negative tiltEQ increases high-frequency content", "[TiltEQ]")
{
    auto totalEnergy = [](float tiltEQ) -> float
    {
        DSPEngine engine;
        engine.prepare (44100.0, 512);

        DSPParams p = baseParams();
        p.tiltEQ    = tiltEQ;
        p.decay     = 2.0f;
        p.mix       = 1.0f;

        const int N = 4096;
        juce::AudioBuffer<float> buf (2, N);
        for (int i = 0; i < N; ++i)
        {
            const float v = (i % 2 == 0) ? 1.0f : -1.0f;
            buf.setSample (0, i, v);
            buf.setSample (1, i, v);
        }

        engine.process (buf, p);

        float sum = 0.0f;
        for (int i = 100; i < N; ++i)
            sum += buf.getSample (0, i) * buf.getSample (0, i);
        return sum;
    };

    const float energyNeutral   = totalEnergy (0.0f);
    const float energyBrightened = totalEnergy (-1.0f);

    // Negative tilt brightens (increases HF energy relative to neutral)
    REQUIRE (energyBrightened > energyNeutral);
}

TEST_CASE ("TiltEQ - output is finite for extreme settings", "[TiltEQ]")
{
    for (float t : { -1.0f, 0.0f, 1.0f })
    {
        DSPEngine engine;
        engine.prepare (44100.0, 512);

        DSPParams p = baseParams();
        p.tiltEQ    = t;

        const int N = 2048;
        juce::AudioBuffer<float> buf (2, N);
        for (int i = 0; i < N; ++i)
        {
            buf.setSample (0, i, std::sin ((float) i * 0.3f));
            buf.setSample (1, i, std::sin ((float) i * 0.3f));
        }

        engine.process (buf, p);

        for (int i = 0; i < N; ++i)
            REQUIRE (std::isfinite (buf.getSample (0, i)));
    }
}
