#pragma once
#include <JuceHeader.h>
#include <array>
#include <vector>

// ─────────────────────────────────────────────────────────────────────────────
//  Parameter bundle — assembled each block in processBlock, handed to DSPEngine
// ─────────────────────────────────────────────────────────────────────────────
struct DSPParams
{
    float preDelay  { 0.0f };   // ms,  0–150
    float roomSize  { 0.5f };   // 0–1, scales all delay line lengths
    float decay     { 2.0f };   // seconds, drives comb feedback coefficient
    float damping   { 0.5f };   // 0–1, 1-pole LP cutoff inside comb feedback
    float diffusion { 0.5f };   // 0–1, all-pass gain (echo density)
    float modRate   { 0.5f };   // Hz,  LFO speed per comb line
    float modDepth  { 0.1f };   // 0–1, LFO displacement of delay read position
    float tiltEQ    { 0.0f };   // -1–1, shelving EQ inside feedback loop
    float mix       { 0.3f };   // 0–1, dry/wet
    bool  freeze    { false };  // locks feedback at unity, tail sustains
};

// ─────────────────────────────────────────────────────────────────────────────
//  Circular buffer — O(1) delay line with integer and interpolated read
// ─────────────────────────────────────────────────────────────────────────────
struct CircularBuffer
{
    std::vector<float> data;
    int writeHead { 0 };
    int length    { 0 };

    void allocate (int numSamples)
    {
        length = numSamples;
        data.assign ((size_t) numSamples, 0.0f);
        writeHead = 0;
    }

    void reset()
    {
        std::fill (data.begin(), data.end(), 0.0f);
        writeHead = 0;
    }

    // Write one sample and advance the head
    void write (float sample)
    {
        data[(size_t) writeHead] = sample;
        if (++writeHead >= length)
            writeHead = 0;
    }

    // Read the sample that was written N positions ago
    // Call read() BEFORE write() each sample so delaySamples=1 gives one-sample delay
    float read (int delaySamples) const
    {
        int index = writeHead - delaySamples;
        if (index < 0)
            index += length;
        return data[(size_t) index];
    }

    // Linear interpolation for fractional delay — needed once you implement mod depth
    // You will own the fractional math here; this is the correct hook point
    float readInterpolated (float delaySamples) const
    {
        const int   base = (int) delaySamples;
        const float frac = delaySamples - (float) base;
        return read (base) + frac * (read (base + 1) - read (base));
    }
};

// ─────────────────────────────────────────────────────────────────────────────
//  DSPEngine — owns all reverb state, lives on the audio thread exclusively
// ─────────────────────────────────────────────────────────────────────────────
class DSPEngine
{
public:
    DSPEngine() = default;
    void prepare (double sampleRate, int maxBlockSize);
    void reset();
    void process (juce::AudioBuffer<float>& buffer, const DSPParams& params);

private:
    double sampleRate { 44100.0 };

    static constexpr int kNumChannels = 2;

    // ── Pre-delay (one line per channel, max 150ms) ───────────────────────
    std::array<CircularBuffer, kNumChannels> preDelayLines;

    // ── FDN: 8 delay lines, cross-coupled via Householder matrix ─────────
    // Taps 0-3 are fed by the left input, taps 4-7 by the right input.
    // The Householder matrix cross-couples all 8 every sample, so L and R
    // decorrelate naturally without a separate stereo bank.
    static constexpr int kNumTaps = 8;

    // Mutually prime delay times (ms) — no common factors avoids periodicity
    static constexpr float kFDNTimes[kNumTaps]
        { 19.7f, 22.3f, 29.1f, 33.7f, 41.1f, 47.3f, 53.9f, 61.1f };

    std::array<CircularBuffer, kNumTaps> fdnLines;

    // 1-pole lowpass state per tap — implements the damping parameter
    std::array<float, kNumTaps> fdnDampState {};

    // ── All-pass filters: 2 per channel, run in series ───────────────────
    static constexpr int kNumAllPass = 2;
    static constexpr float kAllPassTimes[kNumAllPass] { 5.0f, 1.7f }; // ms

    // Indexed as [channel * kNumAllPass + filterIndex]
    std::array<CircularBuffer, kNumAllPass * kNumChannels> allPassLines;

    // ── Tilt EQ: one 1-pole lowpass state per channel ────────────────────
    // Shelf frequency ~1 kHz; tiltEQ>0 darkens, <0 brightens the tail
    std::array<float, kNumChannels> tiltState {};

    // ── LFO phase accumulators: one per FDN tap ───────────────────────────
    // Incremented each sample, wraps at 1.0
    std::array<float, kNumTaps> lfoPhases {};

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (DSPEngine)
};
