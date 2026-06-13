#include "DSPEngine.h"

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
    for (int k = 0; k < kNumTaps; ++k)
        lfoPhases[(size_t) k] = (float) k / (float) kNumTaps;
    for (auto& g : granular)  g.reset();
    for (auto& s : ssb)       s.reset();
    shimFeedbackL = shimFeedbackR = 0.0f;
    shimHPL       = shimHPR       = 0.0f;
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

    // Modulation: LFO increment per sample, max depth in samples (~0.5ms)
    const float lfoInc          = params.modRate / sr;
    const float modDepthSamples = params.modDepth * 24.0f;
    const float twoPi           = juce::MathConstants<float>::twoPi;
    // Householder scale factor: 2/N applied once to the summed tap outputs
    const float householderScale = 2.0f / (float) kNumTaps;
    // LF tracking filter: 1-pole LP at ~300Hz — isolates bass for decay color
    const float lfCoeff = 1.0f - (juce::MathConstants<float>::twoPi * 300.0f / sr);

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
        // Advance LFO phases for all taps first
        for (int i = 0; i < kNumTaps; ++i)
        {
            lfoPhases[(size_t) i] += lfoInc;
            if (lfoPhases[(size_t) i] >= 1.0f)
                lfoPhases[(size_t) i] -= 1.0f;
        }

        // Read all taps with LFO-modulated fractional delay
        float y[kNumTaps];
        for (int i = 0; i < kNumTaps; ++i)
        {
            const float lfoVal = std::sin (lfoPhases[(size_t) i] * twoPi);
            const float delay  = juce::jmax (1.0f, (float) fdnN[i] + lfoVal * modDepthSamples);
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
        const float fdnInputL  = wetL + shimClipL * shimScale;
        const float fdnInputR  = wetR + shimClipR * shimScale;

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

            const float fb    = params.freeze ? v[i] : fbSig * fdnG[i];
            const float input = (i < kNumTaps / 2) ? fdnInputL : fdnInputR;
            fdnLines[i].write (input + fb);
        }

        // Output: first half of taps → L, second half → R
        wetL = (y[0] + y[1] + y[2] + y[3]) * 0.25f;
        wetR = (y[4] + y[5] + y[6] + y[7]) * 0.25f;

        // ── Shimmer: compute pitch-shifted feedback for next sample ───────
        // Active voices are weighted and summed; inactive voices advance silently
        // to prevent stale-data pops when the voices count is increased.
        if (params.shimmer > 0.001f)
        {
            const float phaseInc = twoPi * params.shimmerShiftHz / sr;
            const float charAmt  = params.shimmerChar;
            const int   numV     = params.shimmerVoices;

            // Compute normalised weight sum for the active voices
            float wSum = 0.0f;
            for (int v = 0; v < numV; ++v) wSum += kVoiceWeights[v];
            const float invW = 1.0f / wSum;

            float granL = 0.0f, granR = 0.0f;
            for (int v = 0; v < numV; ++v)
            {
                const float ratio = params.shimmerPitch * kVoiceRatios[v];
                granL += granular[(size_t)(v * 2 + 0)].process (wetL, ratio) * kVoiceWeights[v];
                granR += granular[(size_t)(v * 2 + 1)].process (wetR, ratio) * kVoiceWeights[v];
            }
            granL *= invW;
            granR *= invW;

            // Advance inactive voices silently to keep buffers warm
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
            // Keep all buffers advancing silently
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

        // ── Stage 5: Tilt EQ ─────────────────────────────────────────────
        // 1-pole lowpass shelf: state tracks the low-frequency content.
        // tiltEQ > 0 blends toward LP output (darken); < 0 subtracts it (brighten).
        // Shelf frequency ~1 kHz via ω = 2π·f/sr approximation.
        wetL += params.tiltEQ * (tiltState[0] - wetL);
        tiltState[0] = (1.0f - tiltCoeff) * wetL + tiltCoeff * tiltState[0];

        wetR += params.tiltEQ * (tiltState[1] - wetR);
        tiltState[1] = (1.0f - tiltCoeff) * wetR + tiltCoeff * tiltState[1];

        // ── Stage 6: Dry/wet blend (framework territory) ──────────────────
        left[n]  = dryL + (wetL - dryL) * params.mix;
        right[n] = dryR + (wetR - dryR) * params.mix;
    }
}
