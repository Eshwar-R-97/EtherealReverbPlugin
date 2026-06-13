#include <catch2/catch_test_macros.hpp>
#include <catch2/catch_approx.hpp>
#include "DSPEngine.h"
#include <cmath>
#include <algorithm>

// ─────────────────────────────────────────────────────────────────────────────
//  Helpers
// ─────────────────────────────────────────────────────────────────────────────

static DSPParams reverseParams (float preDelayMs = 20.0f, float revMix = 1.0f)
{
    DSPParams p;
    p.preDelay   = preDelayMs;
    p.roomSize   = 0.3f;
    p.decay      = 2.0f;
    p.damping    = 0.5f;
    p.diffusion  = 0.0f;
    p.modDepth   = 0.0f;
    p.tiltEQ     = 0.0f;
    p.mix        = 1.0f;
    p.shimmer    = 0.0f;
    p.decayColor = 0.0f;
    p.freeze     = false;
    p.reverse    = true;
    p.reverseMix = revMix;
    return p;
}

// ─────────────────────────────────────────────────────────────────────────────
//  Tests
// ─────────────────────────────────────────────────────────────────────────────

TEST_CASE ("Reverse - with reverse=false, output is identical to baseline", "[Reverse]")
{
    const int N = 2048;
    juce::AudioBuffer<float> forward (2, N), noReverse (2, N);

    for (int i = 0; i < N; ++i)
    {
        const float v = std::sin ((float) i * 0.3f);
        forward.setSample    (0, i, v); forward.setSample    (1, i, v);
        noReverse.setSample  (0, i, v); noReverse.setSample  (1, i, v);
    }

    {
        DSPEngine engine;
        engine.prepare (44100.0, 512);
        DSPParams p = reverseParams();
        p.reverse   = false;
        engine.process (forward, p);
    }
    {
        DSPEngine engine;
        engine.prepare (44100.0, 512);
        DSPParams p = reverseParams();
        p.reverse   = false;
        engine.process (noReverse, p);
    }

    // Two identical runs with reverse=false should produce identical output
    for (int i = 0; i < N; ++i)
        REQUIRE (forward.getSample (0, i) == Catch::Approx (noReverse.getSample (0, i)).margin (1e-6f));
}

TEST_CASE ("Reverse - enabling reverse changes the output", "[Reverse]")
{
    const int N = 4410; // 100ms at 44100 Hz

    juce::AudioBuffer<float> fwdBuf (2, N), revBuf (2, N);

    for (int i = 0; i < N; ++i)
    {
        const float v = (i < 441) ? 1.0f : 0.0f; // 10ms pulse
        fwdBuf.setSample (0, i, v); fwdBuf.setSample (1, i, v);
        revBuf.setSample (0, i, v); revBuf.setSample (1, i, v);
    }

    {
        DSPEngine engine;
        engine.prepare (44100.0, 512);
        DSPParams p = reverseParams (20.0f);
        p.reverse   = false;
        engine.process (fwdBuf, p);
    }
    {
        DSPEngine engine;
        engine.prepare (44100.0, 512);
        engine.process (revBuf, reverseParams (20.0f));
    }

    bool anyDiff = false;
    for (int i = 0; i < N; ++i)
    {
        if (std::abs (fwdBuf.getSample (0, i) - revBuf.getSample (0, i)) > 1e-5f)
        {
            anyDiff = true;
            break;
        }
    }
    REQUIRE (anyDiff);
}

TEST_CASE ("Reverse - reverseMix=0 produces same output as reverse=false", "[Reverse]")
{
    const int N = 2048;

    juce::AudioBuffer<float> noRevBuf (2, N), zeroMixBuf (2, N);

    for (int i = 0; i < N; ++i)
    {
        const float v = std::sin ((float) i * 0.2f);
        noRevBuf.setSample  (0, i, v); noRevBuf.setSample  (1, i, v);
        zeroMixBuf.setSample(0, i, v); zeroMixBuf.setSample(1, i, v);
    }

    {
        DSPEngine engine;
        engine.prepare (44100.0, 512);
        DSPParams p = reverseParams (20.0f, 0.0f);
        p.reverse   = false;
        engine.process (noRevBuf, p);
    }
    {
        DSPEngine engine;
        engine.prepare (44100.0, 512);
        // reverse=true but reverseMix=0 → revL/revR are always 0 → identical to reverse=false
        engine.process (zeroMixBuf, reverseParams (20.0f, 0.0f));
    }

    for (int i = 0; i < N; ++i)
        REQUIRE (noRevBuf.getSample (0, i) == Catch::Approx (zeroMixBuf.getSample (0, i)).margin (1e-5f));
}

TEST_CASE ("Reverse - ping-pong produces non-zero output after two buffer cycles", "[Reverse]")
{
    // With preDelay=5ms: revBufLen = 220 samples.
    // The pre-delay outputs signal starting at sample 220 (when the delay buffer fills).
    // Cycle 1 (samples 0-219): reverse buffer A fills with zeros (pre-delay buffer is empty).
    //   At sample 219: A is reversed (all zeros), swapped. Reading A starts (all zeros).
    // Cycle 2 (samples 220-439): reverse buffer B fills with the pre-delay output (signal).
    //   At sample 439: B is reversed in-place, swapped. Reading B starts.
    // Cycle 3 (samples 440-659): read reversed B (non-zero). This is injected into FDN.
    //   FDN outputs it after ~shortest_tap (434 samples) → first contribution at ~874.
    // So check for energy from sample revBufLen*3 (660) onwards (after FDN roundtrip).
    const double sr         = 44100.0;
    const float  preDelayMs = 5.0f;
    const int    revBufLen  = (int) (preDelayMs * 0.001f * (float) sr); // 220

    DSPEngine engine;
    engine.prepare (sr, 512);

    // Sustained signal for all samples so the pre-delay output is non-zero
    const int N = revBufLen * 8; // ~8.8ms * 8 ≈ 40ms
    juce::AudioBuffer<float> buf (2, N);
    for (int i = 0; i < N; ++i)
    {
        const float v = std::sin ((float) i * 0.1f);
        buf.setSample (0, i, v);
        buf.setSample (1, i, v);
    }

    engine.process (buf, reverseParams (preDelayMs, 1.0f));

    // Energy should be non-zero in the window where reversed content has fed
    // through the FDN and appeared at the output
    float energy = 0.0f;
    for (int i = revBufLen * 4; i < N; ++i)
        energy += std::abs (buf.getSample (0, i));

    REQUIRE (energy > 0.0f);
}

TEST_CASE ("Reverse - higher reverseMix produces more energy in the output", "[Reverse]")
{
    // reverseMix scales the injection of reversed signal into the FDN.
    // Higher mix → more reversed content → more total energy in the output.
    // Use the same sustained-signal approach as the cycle test above.
    const double sr         = 44100.0;
    const float  preDelayMs = 5.0f;
    const int    revBufLen  = (int) (preDelayMs * 0.001f * (float) sr); // 220
    const int    N          = revBufLen * 8;

    auto runAndMeasure = [&] (float revMix) -> float
    {
        DSPEngine engine;
        engine.prepare (sr, 512);

        juce::AudioBuffer<float> buf (2, N);
        for (int i = 0; i < N; ++i)
        {
            const float v = std::sin ((float) i * 0.1f);
            buf.setSample (0, i, v);
            buf.setSample (1, i, v);
        }

        engine.process (buf, reverseParams (preDelayMs, revMix));

        // Measure energy in the region where reversed content has arrived
        float energy = 0.0f;
        for (int i = revBufLen * 4; i < N; ++i)
            energy += std::abs (buf.getSample (0, i));
        return energy;
    };

    const float lowMix  = runAndMeasure (0.0f);
    const float highMix = runAndMeasure (1.0f);

    REQUIRE (highMix > lowMix);
}

TEST_CASE ("Reverse - output is finite for all parameter combinations", "[Reverse]")
{
    for (float mix : { 0.0f, 0.5f, 1.0f })
    {
        DSPEngine engine;
        engine.prepare (44100.0, 512);

        const int N = 4096;
        juce::AudioBuffer<float> buf (2, N);
        for (int i = 0; i < N; ++i)
        {
            buf.setSample (0, i, std::sin ((float) i * 0.1f));
            buf.setSample (1, i, std::sin ((float) i * 0.1f));
        }

        engine.process (buf, reverseParams (20.0f, mix));

        for (int i = 0; i < N; ++i)
            REQUIRE (std::isfinite (buf.getSample (0, i)));
    }
}
