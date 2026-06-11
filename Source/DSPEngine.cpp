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
        // Each line: read delayed output, apply 1-pole LP (damping), feed back
        // scaled by g. Four outputs summed and normalised to [-1,1] by * 0.25.
        float combOutL = 0.0f, combOutR = 0.0f;

        for (int i = 0; i < kNumCombs; ++i)
        {
            // Left
            const int idxL  = i;
            const float outL = combLines[idxL].read (combN[idxL]);
            combDampState[idxL] = params.damping       * combDampState[idxL]
                                + (1.0f - params.damping) * outL;
            const float fbL = params.freeze ? outL : combDampState[idxL] * combG[idxL];
            combLines[idxL].write (wetL + fbL);
            combOutL += outL;

            // Right
            const int idxR   = kNumCombs + i;
            const float outR = combLines[idxR].read (combN[idxR]);
            combDampState[idxR] = params.damping       * combDampState[idxR]
                                + (1.0f - params.damping) * outR;
            const float fbR = params.freeze ? outR : combDampState[idxR] * combG[idxR];
            combLines[idxR].write (wetR + fbR);
            combOutR += outR;
        }

        wetL = combOutL * 0.25f;
        wetR = combOutR * 0.25f;

        // ── Stage 3: Hadamard mixing matrix (4x4) ────────────────────────
        //
        // Mix the 4 summed comb outputs using the normalised 4x4 Hadamard matrix.
        // This cross-couples all lines so echo density builds much faster than
        // vanilla Schroeder. The diffusion parameter scales the off-diagonal terms.
        //
        // H4 = 0.5 * [ 1  1  1  1 ]
        //             [ 1 -1  1 -1 ]
        //             [ 1  1 -1 -1 ]
        //             [ 1 -1 -1  1 ]
        //
        // YOUR CODE HERE

        // ── Stage 4: All-pass filter chain (2 in series per channel) ──────
        //
        // Standard Schroeder all-pass (diffusion gain g = params.diffusion * 0.7):
        //   float delayed  = allPassLines[idx].read(delaySamples)
        //   float passOut  = -g * input + delayed
        //   allPassLines[idx].write(input + g * delayed)
        //   input = passOut  (chain output into next filter)
        //
        // YOUR CODE HERE

        // ── Stage 5: Tilt EQ (inside feedback — applied per sample) ───────
        //
        // A first-order shelving filter whose tilt direction is controlled by
        // params.tiltEQ (-1 = darken, +1 = brighten).
        // This runs inside the feedback path, not after it.
        // Implement as a biquad or 1-pole shelf — your choice.
        //
        // YOUR CODE HERE

        // ── Stage 6: Dry/wet blend (framework territory) ──────────────────
        left[n]  = dryL + (wetL - dryL) * params.mix;
        right[n] = dryR + (wetR - dryR) * params.mix;
    }
}
