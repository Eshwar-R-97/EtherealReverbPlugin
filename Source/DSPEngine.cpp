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

    // Compute once per block — parameter doesn't change mid-block
    const int preDelaySamples = (int) (params.preDelay * 0.001f * (float) sampleRate);

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
        //
        // feedbackGain is derived from params.decay and the delay time:
        //   feedbackGain = pow(10.0f, -3.0f * delaySamples / (params.decay * sampleRate))
        // This gives -60dB at t=decay seconds (the standard RT60 definition).
        //
        // For each line [ch * kNumCombs + i]:
        //   float out  = combLines[idx].read(delaySamples)
        //   float damp = combDampState[idx] += damping * (out - combDampState[idx])
        //   float fb   = params.freeze ? out : damp * feedbackGain
        //   combLines[idx].write(wetInput + fb)
        //   sum += out
        //
        // Sum all 4 outputs for L and R separately, then scale by 0.25f.
        //
        // YOUR CODE HERE

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
