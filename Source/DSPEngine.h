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
    void prepare (double sampleRate, int maxBlockSize);
    void reset();
    void process (juce::AudioBuffer<float>& buffer, const DSPParams& params);

private:
    double sampleRate { 44100.0 };

    static constexpr int kNumChannels = 2;

    // ── Pre-delay (one line per channel, max 150ms) ───────────────────────
    std::array<CircularBuffer, kNumChannels> preDelayLines;

    // ── Feedback comb filters: 4 per channel, run in parallel ────────────
    static constexpr int kNumCombs = 4;

    // Classic Schroeder delay times (ms), slightly detuned L vs R for stereo width
    static constexpr float kCombTimesL[kNumCombs] { 29.7f, 37.1f, 41.1f, 43.7f };
    static constexpr float kCombTimesR[kNumCombs] { 30.1f, 37.5f, 41.5f, 44.1f };

    // Indexed as [channel * kNumCombs + filterIndex]
    std::array<CircularBuffer, kNumCombs * kNumChannels> combLines;

    // 1-pole lowpass state per comb line — implements the damping parameter
    std::array<float, kNumCombs * kNumChannels> combDampState {};

    // ── All-pass filters: 2 per channel, run in series ───────────────────
    static constexpr int kNumAllPass = 2;
    static constexpr float kAllPassTimes[kNumAllPass] { 5.0f, 1.7f }; // ms

    // Indexed as [channel * kNumAllPass + filterIndex]
    std::array<CircularBuffer, kNumAllPass * kNumChannels> allPassLines;

    // ── LFO phase accumulators: one per comb line ─────────────────────────
    // Incremented each sample, wraps at 1.0
    // Indexed same as combLines: [channel * kNumCombs + filterIndex]
    std::array<float, kNumCombs * kNumChannels> lfoPhases {};

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (DSPEngine)
};
