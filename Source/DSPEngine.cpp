#include "DSPEngine.h"

namespace
{
    float triangleLFO (float phase01)
    {
        return 4.0f * std::abs (phase01 - 0.5f) - 1.0f;
    }

    uint32_t chaosRand (uint32_t& state)
    {
        state = state * 1664525u + 1013904223u;
        return state;
    }

    float chaosRandFloat (uint32_t& state)
    {
        return (float) (chaosRand (state) & 0x00ffffffu) / (float) 0x01000000u;
    }

    // Single peaking biquad (RBJ), coefficient update per block
    void updatePeakingCoeffs (float freq, float gainDb, float Q, float sr,
                              float& b0, float& b1, float& b2, float& a1, float& a2)
    {
        const float A     = std::pow (10.0f, gainDb / 40.0f);
        const float w0    = juce::MathConstants<float>::twoPi * freq / sr;
        const float cosw0 = std::cos (w0);
        const float sinw0 = std::sin (w0);
        const float alpha = sinw0 / (2.0f * Q);

        const float a0 = 1.0f + alpha / A;
        b0 = (1.0f + alpha * A) / a0;
        b1 = (-2.0f * cosw0) / a0;
        b2 = (1.0f - alpha * A) / a0;
        a1 = (-2.0f * cosw0) / a0;
        a2 = (1.0f - alpha / A) / a0;
    }

    float processBiquad (float in, float b0, float b1, float b2, float a1, float a2,
                         BiquadState& s)
    {
        const float out = b0 * in + b1 * s.x1 + b2 * s.x2 - a1 * s.y1 - a2 * s.y2;
        s.x2 = s.x1; s.x1 = in;
        s.y2 = s.y1; s.y1 = out;
        return out;
    }
}

// ─────────────────────────────────────────────────────────────────────────────
//  GranularShimmer
// ─────────────────────────────────────────────────────────────────────────────

void GranularShimmer::reset()
{
    std::fill (buf, buf + kBufSize, 0.0f);
    writeHead = 0;
    phase0    = 0.0f;
    phase1    = (float) (kBufSize / 2);
}

float GranularShimmer::process (float in, float pitchRatio)
{
    buf[writeHead] = in;

    // Read at (writeHead - kBufSize + phase) which is (kBufSize - phase) samples ago.
    // Linear interpolation for sub-sample accuracy.
    auto readAt = [&] (float ph) -> float
    {
        const int   i    = (int) ph;
        const float frac = ph - (float) i;
        const int   r0   = (writeHead - kBufSize + i) & (kBufSize - 1);
        const int   r1   = (r0 + 1) & (kBufSize - 1);
        return buf[r0] + frac * (buf[r1] - buf[r0]);
    };

    // Hann window: w(ph) = 0.5 - 0.5*cos(2π*ph/kBufSize)
    // Two grains 180° apart always sum to 1.0 (constant-power crossfade)
    const float scale = juce::MathConstants<float>::twoPi / (float) kBufSize;
    const float w0  = 0.5f - 0.5f * std::cos (scale * phase0);
    const float w1  = 0.5f - 0.5f * std::cos (scale * phase1);
    const float out = w0 * readAt (phase0) + w1 * readAt (phase1);

    phase0 += pitchRatio;
    if (phase0 >= (float) kBufSize) phase0 -= (float) kBufSize;
    phase1 = phase0 + (float) (kBufSize / 2);
    if (phase1 >= (float) kBufSize) phase1 -= (float) kBufSize;

    writeHead = (writeHead + 1) & (kBufSize - 1);
    return out;
}

// ─────────────────────────────────────────────────────────────────────────────
//  SSBShimmer
// ─────────────────────────────────────────────────────────────────────────────

void SSBShimmer::reset()
{
    std::fill (xA, xA + 2, 0.0f); std::fill (yA, yA + 2, 0.0f);
    std::fill (xB, xB + 2, 0.0f); std::fill (yB, yB + 2, 0.0f);
    phase = 0.0f;
}

float SSBShimmer::process (float in, float phaseInc)
{
    // Branch A: two first-order allpass sections in series
    // Difference equation: y[n] = a*(x[n] - y[n-1]) + x[n-1]
    float sigA = in;
    for (int i = 0; i < 2; ++i)
    {
        const float y = kA[i] * (sigA - yA[i]) + xA[i];
        xA[i] = sigA; yA[i] = y; sigA = y;
    }

    // Branch B: same structure, different coefficients → ~90° relative to Branch A
    float sigB = in;
    for (int i = 0; i < 2; ++i)
    {
        const float y = kB[i] * (sigB - yB[i]) + xB[i];
        xB[i] = sigB; yB[i] = y; sigB = y;
    }

    // Upper-sideband modulation: shifts all frequencies up by shiftHz
    const float cosP = std::cos (phase);
    const float sinP = std::sin (phase);
    phase += phaseInc;
    if (phase >= juce::MathConstants<float>::twoPi)
        phase -= juce::MathConstants<float>::twoPi;

    return sigA * cosP - sigB * sinP;
}

// ─────────────────────────────────────────────────────────────────────────────
//  DSPEngine
// ─────────────────────────────────────────────────────────────────────────────

void DSPEngine::prepare (double sr, int /*maxBlockSize*/)
{
    sampleRate = sr;

    // Pre-delay: worst case is 150ms
    const int maxPreDelay = (int) (0.150 * sampleRate) + 2;
    for (auto& line : preDelayLines)
        line.allocate (maxPreDelay);

    // FDN taps: allocate at full roomSize (1.0) + 10% headroom for mod depth
    for (int i = 0; i < kNumTaps; ++i)
    {
        const int samples = (int) (kFDNTimes[i] * 0.001 * sampleRate * 1.1) + 4;
        fdnLines[i].allocate (samples);
    }

    // All-pass filters
    for (int ch = 0; ch < kNumChannels; ++ch)
    {
        for (int i = 0; i < kNumAllPass; ++i)
        {
            const int samples = (int) (kAllPassTimes[i] * 0.001 * sampleRate) + 4;
            allPassLines[ch * kNumAllPass + i].allocate (samples);
        }
    }

    for (auto& line : chorusLines)
        line.allocate (kMaxChorusSamples);

    reset();
}

void DSPEngine::reset()
{
    for (auto& line : preDelayLines) line.reset();
    for (auto& line : fdnLines)      line.reset();
    for (auto& line : allPassLines)  line.reset();
    fdnDampState.fill (0.0f);
    fdnLFState.fill (0.0f);
    tiltState.fill (0.0f);
    bloomEnv = 0.0f;
    for (int k = 0; k < kNumTaps; ++k)
        lfoPhases[(size_t) k] = (float) k / (float) kNumTaps;
    for (auto& g : granular)  g.reset();
    for (auto& s : ssb)       s.reset();
    voiceLPState.fill (0.0f);
    shimVoiceLFOPhases.fill (0.0f);
    shimFeedbackL = shimFeedbackR = 0.0f;
    shimHPL       = shimHPR       = 0.0f;
    revBufAL.fill (0.0f); revBufAR.fill (0.0f);
    revBufBL.fill (0.0f); revBufBR.fill (0.0f);
    revWriteSide = 0; revWritePos = 0; revReadPos = 0;
    revBufLen = 0; revBufReady = false;
    revReadFrac = 0.0f; revPitchRate = 1.0f;
    revCrossfade = 0;
    revCrossfadeLen = juce::jmax (1, (int) (sampleRate * 0.035));
    revPrevL = revPrevR = 0.0f;
    lfoRandomTarget.fill (0.0f);
    lfoRandomValue.fill (0.0f);
    lfoRandomHold = 0;
    for (auto& line : chorusLines) line.reset();
    chorusLfoPhaseL = 0.0f; chorusLfoPhaseR = 0.25f;
    realmLPState.fill (0.0f);
    realmLfoPhase = 0.0f;
    for (auto& g : cloudGranular) g.reset();
    cloudLfoPhase = 0.0f;
    for (auto& s : formantStates) { s = {}; }
    voxLfoPhase = 0.0f;
    chaosModBoost = 0.0f;
    chaosFreezeRemain = 0;
    chaosCooldown = 0;
}

void DSPEngine::process (juce::AudioBuffer<float>& buffer, const DSPParams& params)
{
    const int numSamples = buffer.getNumSamples();
    float* const left    = buffer.getWritePointer (0);
    float* const right   = buffer.getWritePointer (1);

    // ── Per-block pre-computation (no pow/multiply in the hot path) ─────────────
    const float sr = (float) sampleRate;
    const int preDelaySamples = (int) (params.preDelay * 0.001f * sr);

    // FDN tap delay lengths and RT60 feedback gains
    // N scales with roomSize; g derived from RT60: -60dB in params.decay seconds
    int   fdnN[kNumTaps];
    float fdnG[kNumTaps];
    for (int i = 0; i < kNumTaps; ++i)
    {
        fdnN[i] = juce::jmax (1, (int) (kFDNTimes[i] * 0.001f * sr * params.roomSize));
        fdnG[i] = std::pow (10.0f, -3.0f * (float) fdnN[i] / (params.decay * sr));
    }

    // All-pass delay lengths scale with roomSize for coherent room character
    int apN[kNumAllPass * kNumChannels];
    for (int ch = 0; ch < kNumChannels; ++ch)
        for (int i = 0; i < kNumAllPass; ++i)
            apN[ch * kNumAllPass + i] = juce::jmax (1, (int) (kAllPassTimes[i] * 0.001f * sr * params.roomSize));

    // Schroeder's canonical g=0.7 scaled by diffusion: 0 = transparent, 1 = full density
    const float apGain = params.diffusion * 0.7f;

    // Tilt EQ: 1-pole coefficient for shelf at ~1 kHz
    const float tiltCoeff = 1.0f - (juce::MathConstants<float>::twoPi * 1000.0f / sr);

    // Modulation: per-tap rates, deeper depth (up to ~4ms), swim boost
    const float swimBoost     = 1.0f + params.swim * 0.6f;
    const float modDepthSamples = params.modDepth * sr * 0.004f * swimBoost;
    const float chaosDepthBoost = 1.0f + chaosModBoost * 3.0f;
    const float effectiveModDepth = modDepthSamples * chaosDepthBoost;
    const float twoPi           = juce::MathConstants<float>::twoPi;
    float tapLfoInc[kNumTaps];
    for (int i = 0; i < kNumTaps; ++i)
        tapLfoInc[i] = params.modRate * (0.7f + 0.6f * (float) i / (float) (kNumTaps - 1)) / sr;
    // Householder scale factor: 2/N applied once to the summed tap outputs
    const float householderScale = 2.0f / (float) kNumTaps;
    // LF tracking filter: 1-pole LP at ~300Hz — isolates bass for decay color
    const float lfCoeff = 1.0f - (juce::MathConstants<float>::twoPi * 300.0f / sr);

    // Reverse reverb: update ping-pong buffer length if preDelay changed
    const int crossfadeSamples = juce::jmax (1, (int) (sr * 0.035f)); // 35ms chunk crossfade
    if (params.reverse)
    {
        const int newLen = juce::jlimit (1, kMaxRevSamples,
                                         juce::jmax (1, (int) (params.preDelay * 0.001f * sr)));
        if (newLen != revBufLen)
        {
            revBufLen       = newLen;
            revWritePos     = 0; revReadPos = 0; revBufReady = false;
            revReadFrac     = 0.0f;
            revCrossfade    = 0;
            revCrossfadeLen = crossfadeSamples;
            revBufAL.fill (0.0f); revBufAR.fill (0.0f);
            revBufBL.fill (0.0f); revBufBR.fill (0.0f);
        }
        // Pitch smear: subtle time-stretch through reversed chunk
        revPitchRate = 0.97f + 0.06f * params.dream;
    }

    // Bloom envelope coefficients — computed once per block
    const float bloomAttackCoeff  = std::exp (-1.0f / (0.040f * sr)); // 40ms attack
    const float bloomReleaseCoeff = std::exp (-1.0f / (3.0f  * sr)); // 3s release

    // Per-voice shimmer naturalisation — computed once per block
    // Method 1: progressive LP darkening of each harmonic voice
    float voiceLPCoeff[kMaxVoices];
    for (int v = 0; v < kMaxVoices; ++v)
        voiceLPCoeff[v] = 1.0f - (twoPi * kVoiceLPCutoffs[v] / sr);
    // Method 3: slow pitch LFO increments per voice
    float voiceLFOInc[kMaxVoices];
    for (int v = 0; v < kMaxVoices; ++v)
        voiceLFOInc[v] = kVoiceLFORates[v] / sr;

    // Chorus (melt) LFO rates — irrational ratio for non-repeating swim
    const float chorusIncL = 0.47f / sr;
    const float chorusIncR = 0.71f / sr;
    const float chorusDepthSamples = params.swim * sr * 0.018f; // up to 18ms
    const int   chorusBaseDelay    = juce::jmax (1, (int) (sr * 0.008f));

    // Dual realm crossover at ~400 Hz
    const float realmLPCoeff = 1.0f - (twoPi * 400.0f / sr);
    const float realmLfoInc  = 0.06f / sr;

    // Cloud granular slow pitch wander
    const float cloudLfoInc = 0.03f / sr;

    // Formant morph coefficients (updated per block from vox + slow LFO)
    const float voxMorph = params.vox;
    const float voxLfoInc = 0.04f / sr;
    float fB0[3], fB1[3], fB2[3], fA1[3], fA2[3];
    {
        const float vowel = 0.5f + 0.5f * std::sin (voxLfoPhase * twoPi);
        const float freqs[3] = { 500.0f, 1200.0f, 2800.0f };
        const float gains[3] = {
            voxMorph * juce::jmap (vowel, 0.0f, 1.0f, 4.0f, 8.0f),
            voxMorph * juce::jmap (vowel, 0.0f, 1.0f, 2.0f, 7.0f),
            voxMorph * juce::jmap (1.0f - vowel, 0.0f, 1.0f, -2.0f, 5.0f)
        };
        for (int f = 0; f < 3; ++f)
            updatePeakingCoeffs (freqs[f], gains[f], 2.5f, sr, fB0[f], fB1[f], fB2[f], fA1[f], fA2[f]);
    }

    // Smoothed-random LFO hold timer (~80ms)
    const int randomHoldSamples = juce::jmax (1, (int) (sr * 0.08f));

    // Chaos cooldown decay
    const float chaosDecay = std::exp (-1.0f / (0.15f * sr));
    const float glowDrive  = 1.0f + params.glow * 0.85f;
    const bool  chaosActive = params.chaos > 0.001f;
    const float driftAmt   = 0.005f + params.shimmerDrift * 0.075f;
    const float dirBlend   = (params.shimmerDir + 1.0f) * 0.5f; // 0=down, 1=up

    for (int n = 0; n < numSamples; ++n)
    {
        const float dryL = left[n];
        const float dryR = right[n];

        // ── Stage 1: Pre-delay ────────────────────────────────────────────
        // read(0) would return stale data at writeHead, so bypass the buffer
        // entirely when delay=0. Always write so the buffer stays current if
        // the knob is turned up mid-session.
        float wetL = (preDelaySamples > 0) ? preDelayLines[0].read (preDelaySamples) : dryL;
        float wetR = (preDelaySamples > 0) ? preDelayLines[1].read (preDelaySamples) : dryR;
        preDelayLines[0].write (dryL);
        preDelayLines[1].write (dryR);

        // ── Stage 2: FDN — read, Householder mix, damp, write ────────────
        // Advance per-tap LFO phases and smoothed-random targets
        if (--lfoRandomHold <= 0)
        {
            lfoRandomHold = randomHoldSamples;
            for (int i = 0; i < kNumTaps; ++i)
                lfoRandomTarget[(size_t) i] = chaosRandFloat (chaosRng) * 2.0f - 1.0f;
        }
        for (int i = 0; i < kNumTaps; ++i)
        {
            lfoPhases[(size_t) i] += tapLfoInc[i];
            if (lfoPhases[(size_t) i] >= 1.0f)
                lfoPhases[(size_t) i] -= 1.0f;
            // Slew random LFO toward target
            lfoRandomValue[(size_t) i] += 0.002f * (lfoRandomTarget[(size_t) i] - lfoRandomValue[(size_t) i]);
        }

        // Read all taps with shaped LFO-modulated fractional delay
        float y[kNumTaps];
        for (int i = 0; i < kNumTaps; ++i)
        {
            const float phase = lfoPhases[(size_t) i];
            const float sine  = std::sin (phase * twoPi);
            const float tri   = triangleLFO (phase);
            const float rnd   = lfoRandomValue[(size_t) i];
            float lfoVal = sine;
            if (params.modShape <= 0.5f)
            {
                const float t = params.modShape * 2.0f;
                lfoVal = sine * (1.0f - t) + tri * t;
            }
            else
            {
                const float t = (params.modShape - 0.5f) * 2.0f;
                lfoVal = tri * (1.0f - t) + rnd * t;
            }
            const float delay = juce::jmax (1.0f, (float) fdnN[i] + lfoVal * effectiveModDepth);
            y[i] = fdnLines[i].readInterpolated (delay);
        }

        // Householder reflection: v[i] = y[i] - (2/N) * sum(y)
        // This is O(N) — all off-diagonal values are identical so one sum suffices
        float tapSum = 0.0f;
        for (int i = 0; i < kNumTaps; ++i) tapSum += y[i];
        tapSum *= householderScale;

        float v[kNumTaps];
        for (int i = 0; i < kNumTaps; ++i) v[i] = y[i] - tapSum;

        // Shimmer: add pitch-shifted feedback ADDITIVELY at a bounded level.
        // The FDN output accumulates to much higher amplitude than the dry input
        // at long decay times, so a blend approach causes instability. Instead:
        //  1. HP-filter the feedback to prevent sub-bass buildup
        //  2. tanh-clip to a hard ceiling of ±1 — this is the stability guarantee
        //  3. Inject at shimmer×0.15 (≈ −16 dB at full), additive on top of dry input
        const float hpCoeff = 1.0f - (juce::MathConstants<float>::twoPi * 120.0f / sr);
        shimHPL = hpCoeff * shimHPL + (1.0f - hpCoeff) * shimFeedbackL;
        shimHPR = hpCoeff * shimHPR + (1.0f - hpCoeff) * shimFeedbackR;
        const float shimClipL = std::tanh (shimFeedbackL - shimHPL);
        const float shimClipR = std::tanh (shimFeedbackR - shimHPR);
        const float shimScale  = params.shimmer * 0.15f;

        // ── Reverse reverb: ping-pong with crossfade and pitch smear ───────
        float revL = 0.0f, revR = 0.0f;
        if (params.reverse && revBufLen > 0)
        {
            float* wBufL = (revWriteSide == 0) ? revBufAL.data() : revBufBL.data();
            float* wBufR = (revWriteSide == 0) ? revBufAR.data() : revBufBR.data();
            float* rBufL = (revWriteSide == 0) ? revBufBL.data() : revBufAL.data();
            float* rBufR = (revWriteSide == 0) ? revBufBR.data() : revBufAR.data();

            wBufL[revWritePos] = wetL;
            wBufR[revWritePos] = wetR;

            if (revBufReady)
            {
                const int   idx0 = revReadPos % revBufLen;
                const int   idx1 = (idx0 + 1) % revBufLen;
                const float frac = revReadFrac - (float) (int) revReadFrac;
                float sL = rBufL[idx0] + frac * (rBufL[idx1] - rBufL[idx0]);
                float sR = rBufR[idx0] + frac * (rBufR[idx1] - rBufR[idx0]);

                if (revCrossfade > 0)
                {
                    const float fade = (float) revCrossfade / (float) juce::jmax (1, revCrossfadeLen);
                    sL = sL * fade + revPrevL * (1.0f - fade);
                    sR = sR * fade + revPrevR * (1.0f - fade);
                    --revCrossfade;
                }

                revL = sL * params.reverseMix;
                revR = sR * params.reverseMix;

                revReadFrac += revPitchRate;
                while (revReadFrac >= 1.0f)
                {
                    revReadFrac -= 1.0f;
                    revReadPos = (revReadPos + 1) % revBufLen;
                }
            }

            if (++revWritePos >= revBufLen)
            {
                revPrevL = rBufL[revBufLen - 1];
                revPrevR = rBufR[revBufLen - 1];
                std::reverse (wBufL, wBufL + revBufLen);
                std::reverse (wBufR, wBufR + revBufLen);
                revWriteSide  = 1 - revWriteSide;
                revWritePos   = 0;
                revReadPos    = 0;
                revReadFrac   = 0.0f;
                revCrossfade  = revCrossfadeLen;
                revBufReady   = true;
            }
        }

        const float fdnInputL  = wetL + shimClipL * shimScale + revL;
        const float fdnInputR  = wetR + shimClipR * shimScale + revR;

        // Apply per-tap damping LP, LF tracking LP, decay color blend, then write back
        // Taps 0-3 are driven by fdnInputL, taps 4-7 by fdnInputR
        for (int i = 0; i < kNumTaps; ++i)
        {
            // HF damping LP (existing) — higher damping = more HF removed from feedback
            fdnDampState[i] = params.damping         * fdnDampState[i]
                            + (1.0f - params.damping) * v[i];

            // LF tracking LP at ~300Hz — captures the bass content of the signal
            fdnLFState[i] = lfCoeff * fdnLFState[i] + (1.0f - lfCoeff) * v[i];

            // Decay color blends between three feedback signals:
            //  +1 (bright):  feed back v[i] directly — HF rolloff bypassed, highs sustain
            //   0 (neutral): feed back fdnDampState[i] — existing damping behavior
            //  -1 (dark):    feed back fdnLFState[i] — only bass survives in the loop
            float fbSig;
            if (params.decayColor >= 0.0f)
                fbSig = fdnDampState[i] * (1.0f - params.decayColor) + v[i] * params.decayColor;
            else
                fbSig = fdnDampState[i] * (1.0f + params.decayColor) + fdnLFState[i] * (-params.decayColor);

            // HF air shelf: adds back ~10% of the high-frequency content that
            // damping removed. Makes the tail feel airier as it decays.
            // fdnDampState is a LP of v[i], so (v[i] - fdnDampState) ≈ HF content.
            fbSig += 0.10f * (v[i] - fdnDampState[i]);

            // Glow: gentle saturation in feedback for warm haze
            if (params.glow > 0.001f)
                fbSig = std::tanh (fbSig * glowDrive) / glowDrive;

            const bool microFreeze = chaosFreezeRemain > 0;
            if (microFreeze)
                --chaosFreezeRemain;

            const float fb    = (params.freeze || microFreeze) ? v[i] : fbSig * fdnG[i];
            const float input = (i < kNumTaps / 2) ? fdnInputL : fdnInputR;
            fdnLines[i].write (input + fb);
        }

        // Output: first half of taps → L, second half → R
        wetL = (y[0] + y[1] + y[2] + y[3]) * 0.25f;
        wetR = (y[4] + y[5] + y[6] + y[7]) * 0.25f;

        // ── Dual realm: LF/HF split with breathing crossfade ──────────────
        if (params.realm > 0.001f)
        {
            realmLPState[0] = realmLPCoeff * realmLPState[0] + (1.0f - realmLPCoeff) * wetL;
            realmLPState[1] = realmLPCoeff * realmLPState[1] + (1.0f - realmLPCoeff) * wetR;

            const float breathe = 0.5f + 0.5f * std::sin (realmLfoPhase * twoPi);
            const float hfMix   = params.realm * breathe;
            const float lfL     = realmLPState[0] * 1.08f;
            const float lfR     = realmLPState[1] * 1.08f;
            const float hfL     = (wetL - realmLPState[0]) * 1.12f;
            const float hfR     = (wetR - realmLPState[1]) * 1.12f;
            wetL = lfL * (1.0f - hfMix) + hfL * hfMix;
            wetR = lfR * (1.0f - hfMix) + hfR * hfMix;

            realmLfoPhase += realmLfoInc;
            if (realmLfoPhase >= 1.0f) realmLfoPhase -= 1.0f;
        }

        // ── Shimmer: compute pitch-shifted feedback for next sample ───────
        // Method 3: advance slow pitch LFOs for all voices every sample
        for (int v = 0; v < kMaxVoices; ++v)
        {
            shimVoiceLFOPhases[v] += voiceLFOInc[v];
            if (shimVoiceLFOPhases[v] >= 1.0f) shimVoiceLFOPhases[v] -= 1.0f;
        }

        if (params.shimmer > 0.001f)
        {
            const float phaseInc = twoPi * params.shimmerShiftHz / sr;
            const float charAmt  = params.shimmerChar;
            const int   numV     = params.shimmerVoices;

            float wSum = 0.0f;
            for (int v = 0; v < numV; ++v) wSum += kVoiceWeights[v];
            const float invW = 1.0f / wSum;

            float granL = 0.0f, granR = 0.0f;
            for (int v = 0; v < numV; ++v)
            {
                const float lfoMod = 1.0f + driftAmt * std::sin (shimVoiceLFOPhases[v] * twoPi);
                const float upMult = params.shimmerPitch * kVoiceRatios[v] * kVoiceDetuneRatios[v];
                const float dnMult = (params.shimmerPitch / kVoiceRatios[v]) * kVoiceDetuneRatios[v];
                const float ratio  = (upMult * dirBlend + dnMult * (1.0f - dirBlend)) * lfoMod;

                float outL = granular[(size_t)(v * 2 + 0)].process (wetL, ratio);
                float outR = granular[(size_t)(v * 2 + 1)].process (wetR, ratio);

                // Method 1: per-voice LP filter — higher voices progressively darker
                auto& lpL = voiceLPState[(size_t)(v * 2 + 0)];
                auto& lpR = voiceLPState[(size_t)(v * 2 + 1)];
                const float c = voiceLPCoeff[v];
                lpL = c * lpL + (1.0f - c) * outL;
                lpR = c * lpR + (1.0f - c) * outR;

                granL += lpL * kVoiceWeights[v];
                granR += lpR * kVoiceWeights[v];
            }
            granL *= invW;
            granR *= invW;

            // Advance inactive voices silently to keep granular buffers warm
            for (int v = numV; v < kMaxVoices; ++v)
            {
                const float ratio = params.shimmerPitch * kVoiceRatios[v];
                granular[(size_t)(v * 2 + 0)].process (wetL, ratio);
                granular[(size_t)(v * 2 + 1)].process (wetR, ratio);
            }

            const float ssbL = ssb[0].process (wetL, phaseInc);
            const float ssbR = ssb[1].process (wetR, phaseInc);

            shimFeedbackL = granL * (1.0f - charAmt * 0.5f) + ssbL * (charAmt * 0.5f);
            shimFeedbackR = granR * (1.0f - charAmt * 0.5f) + ssbR * (charAmt * 0.5f);
        }
        else
        {
            for (int v = 0; v < kMaxVoices; ++v)
            {
                const float ratio = params.shimmerPitch * kVoiceRatios[v];
                granular[(size_t)(v * 2 + 0)].process (wetL, ratio);
                granular[(size_t)(v * 2 + 1)].process (wetR, ratio);
            }
            shimFeedbackL = shimFeedbackR = 0.0f;
        }

        // ── Stage 4: All-pass filter chain (2 in series per channel) ──────
        // Each filter: flat magnitude, complex phase — increases echo density
        // without tonal coloration. Series connection compounds the effect.
        float sigL = wetL;
        for (int i = 0; i < kNumAllPass; ++i)
        {
            const float delayed = allPassLines[i].read (apN[i]);
            const float out     = -apGain * sigL + delayed;
            allPassLines[i].write (sigL + apGain * delayed);
            sigL = out;
        }
        wetL = sigL;

        float sigR = wetR;
        for (int i = 0; i < kNumAllPass; ++i)
        {
            const int   idx     = kNumAllPass + i;
            const float delayed = allPassLines[idx].read (apN[idx]);
            const float out     = -apGain * sigR + delayed;
            allPassLines[idx].write (sigR + apGain * delayed);
            sigR = out;
        }
        wetR = sigR;

        // ── Melt chorus (swim): modulated delay on wet path ───────────────
        if (params.swim > 0.001f)
        {
            chorusLfoPhaseL += chorusIncL;
            chorusLfoPhaseR += chorusIncR;
            if (chorusLfoPhaseL >= 1.0f) chorusLfoPhaseL -= 1.0f;
            if (chorusLfoPhaseR >= 1.0f) chorusLfoPhaseR -= 1.0f;

            const float modL = std::sin (chorusLfoPhaseL * twoPi) * chorusDepthSamples;
            const float modR = std::sin (chorusLfoPhaseR * twoPi) * chorusDepthSamples;
            const float dL   = (float) chorusBaseDelay + modL;
            const float dR   = (float) chorusBaseDelay + modR;

            chorusLines[0].write (wetL);
            chorusLines[1].write (wetR);
            const float cL = chorusLines[0].readInterpolated (dL);
            const float cR = chorusLines[1].readInterpolated (dR);
            const float mix = params.swim * 0.38f;
            wetL += cL * mix;
            wetR += cR * mix;
        }

        // ── Cloud: parallel granular scatter ────────────────────────────
        if (params.cloud > 0.001f)
        {
            cloudLfoPhase += cloudLfoInc;
            if (cloudLfoPhase >= 1.0f) cloudLfoPhase -= 1.0f;
            const float cloudRatio = 0.92f + 0.16f * std::sin (cloudLfoPhase * twoPi);
            const float cloudMix   = params.cloud * 0.28f;
            wetL += cloudGranular[0].process (wetL, cloudRatio) * cloudMix;
            wetR += cloudGranular[1].process (wetR, cloudRatio) * cloudMix;
        }

        // ── Vox: formant vowel morph (3 peaking filters per channel) ────
        if (params.vox > 0.001f)
        {
            float vL = wetL, vR = wetR;
            for (int f = 0; f < 3; ++f)
            {
                vL = processBiquad (vL, fB0[f], fB1[f], fB2[f], fA1[f], fA2[f], formantStates[(size_t)(f * 2 + 0)]);
                vR = processBiquad (vR, fB0[f], fB1[f], fB2[f], fA1[f], fA2[f], formantStates[(size_t)(f * 2 + 1)]);
            }
            const float voxMix = params.vox * 0.55f;
            wetL = wetL * (1.0f - voxMix) + vL * voxMix;
            wetR = wetR * (1.0f - voxMix) + vR * voxMix;
            voxLfoPhase += voxLfoInc;
            if (voxLfoPhase >= 1.0f) voxLfoPhase -= 1.0f;
        }

        // ── Stage 5: Tilt EQ ─────────────────────────────────────────────
        // 1-pole lowpass shelf: state tracks the low-frequency content.
        // tiltEQ > 0 blends toward LP output (darken); < 0 subtracts it (brighten).
        // Shelf frequency ~1 kHz via ω = 2π·f/sr approximation.
        wetL += params.tiltEQ * (tiltState[0] - wetL);
        tiltState[0] = (1.0f - tiltCoeff) * wetL + tiltCoeff * tiltState[0];

        wetR += params.tiltEQ * (tiltState[1] - wetR);
        tiltState[1] = (1.0f - tiltCoeff) * wetR + tiltCoeff * tiltState[1];

        // ── Stage 6: Dry/wet blend with bloom envelope ────────────────────
        // Bloom: wet signal fades in over ~40ms when a new note arrives,
        // preventing the reverb from appearing as an instant dense wall.
        // Release is 3s so the reverb tail is not gated when input stops.
        const float inputAbs   = (std::abs (dryL) + std::abs (dryR)) * 0.5f;
        const float bloomTarget = inputAbs > 0.001f ? 1.0f : 0.0f;
        bloomEnv = bloomTarget > bloomEnv
            ? bloomAttackCoeff  * bloomEnv + (1.0f - bloomAttackCoeff)  * bloomTarget
            : bloomReleaseCoeff * bloomEnv + (1.0f - bloomReleaseCoeff) * bloomTarget;

        left[n]  = dryL + (wetL - dryL) * params.mix * bloomEnv;
        right[n] = dryR + (wetR - dryR) * params.mix * bloomEnv;

        // ── Chaos: rare instability events ──────────────────────────────
        if (chaosActive)
        {
            chaosModBoost *= chaosDecay;
            if (chaosCooldown > 0)
                --chaosCooldown;

            if (chaosCooldown <= 0 && inputAbs > 0.002f)
            {
                const float triggerProb = params.chaos * 0.00008f;
                if (chaosRandFloat (chaosRng) < triggerProb)
                {
                    chaosModBoost     = params.chaos;
                    chaosFreezeRemain = (int) (sr * (0.05f + params.chaos * 0.15f));
                    chaosCooldown     = (int) (sr * (3.0f + (1.0f - params.chaos) * 12.0f));
                }
            }
        }
        else
        {
            chaosModBoost *= chaosDecay;
        }
    }
}
