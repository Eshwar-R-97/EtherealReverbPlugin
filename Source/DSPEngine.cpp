#include "DSPEngine.h"

void DSPEngine::prepare (double sr, int /*maxBlockSize*/)
{
    sampleRate = sr;

    // Pre-delay: worst case is 150ms
    const int maxPreDelay = (int) (0.150 * sampleRate) + 2;
    for (auto& line : preDelayLines)
        line.allocate (maxPreDelay);

    // Comb filters: allocate at full roomSize (1.0) + 10% headroom for mod depth
    // At runtime, roomSize scales the actual read offset — buffer stays this size
    for (int ch = 0; ch < kNumChannels; ++ch)
    {
        for (int i = 0; i < kNumCombs; ++i)
        {
            const float baseMs = (ch == 0) ? kCombTimesL[i] : kCombTimesR[i];
            const int   samples = (int) (baseMs * 0.001 * sampleRate * 1.1) + 4;
            combLines[ch * kNumCombs + i].allocate (samples);
        }
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
    for (auto& line : combLines)     line.reset();
    for (auto& line : allPassLines)  line.reset();
    combDampState.fill (0.0f);
    tiltState.fill (0.0f);
    lfoPhases.fill (0.0f);
}

void DSPEngine::process (juce::AudioBuffer<float>& buffer, const DSPParams& params)
{
    const int numSamples = buffer.getNumSamples();
    float* const left    = buffer.getWritePointer (0);
    float* const right   = buffer.getWritePointer (1);

    // ── Per-block pre-computation (no pow/multiply in the hot path) ─────────────
    const float sr = (float) sampleRate;
    const int preDelaySamples = (int) (params.preDelay * 0.001f * sr);

    // Delay lengths and feedback gains for all comb lines
    // N scales with roomSize; g is the RT60 formula: -60dB at params.decay seconds
    int   combN[kNumCombs * kNumChannels];
    float combG[kNumCombs * kNumChannels];

    for (int ch = 0; ch < kNumChannels; ++ch)
    {
        for (int i = 0; i < kNumCombs; ++i)
        {
            const int   idx    = ch * kNumCombs + i;
            const float baseMs = (ch == 0) ? kCombTimesL[i] : kCombTimesR[i];
            const int   N      = juce::jmax (1, (int) (baseMs * 0.001f * sr * params.roomSize));
            combN[idx] = N;
            combG[idx] = std::pow (10.0f, -3.0f * (float) N / (params.decay * sr));
        }
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

        // ── Stage 2: Feedback comb filter bank (4 per channel, parallel) ─
        // Store individual outputs — Stage 3 needs them before summing.
        float combOutsL[kNumCombs], combOutsR[kNumCombs];

        for (int i = 0; i < kNumCombs; ++i)
        {
            // Left
            const int   idxL = i;
            const float outL = combLines[idxL].read (combN[idxL]);
            combDampState[idxL] = params.damping         * combDampState[idxL]
                                + (1.0f - params.damping) * outL;
            const float fbL = params.freeze ? outL : combDampState[idxL] * combG[idxL];
            combLines[idxL].write (wetL + fbL);
            combOutsL[i] = outL;

            // Right
            const int   idxR = kNumCombs + i;
            const float outR = combLines[idxR].read (combN[idxR]);
            combDampState[idxR] = params.damping         * combDampState[idxR]
                                + (1.0f - params.damping) * outR;
            const float fbR = params.freeze ? outR : combDampState[idxR] * combG[idxR];
            combLines[idxR].write (wetR + fbR);
            combOutsR[i] = outR;
        }

        // ── Stage 3: Hadamard mixing matrix (4x4) ────────────────────────
        // Apply H4 to each channel's 4 comb outputs — produces 4 distinct
        // linear combinations. h0 is the vanilla sum; h1 is the alternating
        // combination. Cross-coupling h1 from the opposite channel into wetL/wetR
        // decorrelates L and R proportional to diffusion.
        //
        // diffusion=0 → vanilla Schroeder summing (h0L * 0.25, h0R * 0.25)
        // diffusion=1 → full cross-coupling (h1 from opposite channel blended in)
        const float h0L = combOutsL[0] + combOutsL[1] + combOutsL[2] + combOutsL[3];
        const float h1L = combOutsL[0] - combOutsL[1] + combOutsL[2] - combOutsL[3];

        const float h0R = combOutsR[0] + combOutsR[1] + combOutsR[2] + combOutsR[3];
        const float h1R = combOutsR[0] - combOutsR[1] + combOutsR[2] - combOutsR[3];

        // ± asymmetry between L/R ensures they are decorrelated, not just scaled
        wetL = (h0L + params.diffusion * h1R) * 0.25f;
        wetR = (h0R - params.diffusion * h1L) * 0.25f;

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
