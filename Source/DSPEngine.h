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
    float tiltEQ     { 0.0f };   // -1–1, shelving EQ inside feedback loop
    float mix        { 0.3f };   // 0–1, dry/wet
    float decayColor    { 0.0f };   // -1=dark (bass sustains), 0=neutral, +1=bright (HF sustains)
    float shimmer       { 0.0f };   // 0–1, shimmer wet amount
    float shimmerPitch  { 2.0f };   // granular pitch ratio (1.5=fifth, 2.0=octave, etc.)
    float shimmerChar   { 0.3f };   // 0=pure granular, 1=50/50 granular+SSB blend
    float shimmerShiftHz{ 15.0f };  // SSB frequency offset in Hz (5–50)
    int   shimmerVoices { 1 };      // 1–5 harmonic voices stacked above base pitch
    bool  freeze        { false };  // locks feedback at unity, tail sustains
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
//  GranularShimmer — two overlapping Hann-windowed grains, Hann crossfade
//  Buffer is power-of-2 for cheap modular arithmetic via bitmask
// ─────────────────────────────────────────────────────────────────────────────
struct GranularShimmer
{
    static constexpr int kBufSize = 16384;  // ~341ms at 48kHz; supports pitch down to 0.5x

    float buf[kBufSize] {};
    int   writeHead { 0 };
    float phase0    { 0.0f };                       // grain A read phase (0..kBufSize)
    float phase1    { (float) (kBufSize / 2) };     // grain B, 180° offset for seamless crossfade

    void  reset();
    float process (float in, float pitchRatio);
};

// ─────────────────────────────────────────────────────────────────────────────
//  SSBShimmer — single-sideband frequency shifter via IIR Hilbert pair
//  Shifts all frequencies up by shiftHz regardless of pitch relationship.
//  Two first-order allpass sections per branch (Berners-Abel design) give
//  <2° phase error across 80Hz–18kHz at 48kHz.
// ─────────────────────────────────────────────────────────────────────────────
struct SSBShimmer
{
    static constexpr float kA[2] = { 0.4788f, 0.8762f };  // Branch A allpass coefficients
    static constexpr float kB[2] = { 0.1617f, 0.7330f };  // Branch B (gives ~90° rel. to A)

    float xA[2] {}, yA[2] {};   // Branch A state: [x_prev, y_prev] per section
    float xB[2] {}, yB[2] {};   // Branch B state
    float phase { 0.0f };        // quadrature oscillator phase (0..2π)

    void  reset();
    float process (float in, float phaseInc);
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

    // 1-pole lowpass at ~300Hz per tap — tracks LF content for decay color
    std::array<float, kNumTaps> fdnLFState {};

    // ── All-pass filters: 6 per channel, run in series ───────────────────
    // More stages = denser, smoother tail; times are mutually non-harmonic
    static constexpr int kNumAllPass = 6;
    static constexpr float kAllPassTimes[kNumAllPass]
        { 1.7f, 3.1f, 5.0f, 8.9f, 12.3f, 15.7f }; // ms

    // Indexed as [channel * kNumAllPass + filterIndex]  (2 channels × 6 stages = 12)
    std::array<CircularBuffer, kNumAllPass * kNumChannels> allPassLines;

    // ── Bloom envelope — wet signal fades in over ~40ms on new note onset ──
    float bloomEnv { 0.0f };

    // ── Tilt EQ: one 1-pole lowpass state per channel ────────────────────
    // Shelf frequency ~1 kHz; tiltEQ>0 darkens, <0 brightens the tail
    std::array<float, kNumChannels> tiltState {};

    // ── LFO phase accumulators: one per FDN tap ───────────────────────────
    // Incremented each sample, wraps at 1.0
    std::array<float, kNumTaps> lfoPhases {};

    // ── Shimmer engines ───────────────────────────────────────────────────
    // Up to 5 harmonic voices per stereo channel (10 total). Voices are indexed
    // as granular[voice * 2 + channel], so voice 0L=0, 0R=1, 1L=2, 1R=3, etc.
    // Ratios relative to shimmerPitch: 1×, 1.5×, 2×, 2.5×, 3× — forms the
    // natural harmonic series (root, fifth, octave, 10th, 12th).
    static constexpr int   kMaxVoices       = 5;
    static constexpr float kVoiceRatios[5]  = { 1.0f, 1.5f, 2.0f, 2.5f, 3.0f };
    // Descending amplitude so overtones decay naturally like a real harmonic series
    static constexpr float kVoiceWeights[5] = { 1.0f, 0.65f, 0.45f, 0.30f, 0.20f };
    // Micro-detune per voice in ratio form (cents: 0, +4, -3, +6, -5)
    // Gives each voice a unique tuning so they beat against each other organically
    static constexpr float kVoiceDetuneRatios[5] = { 1.0f, 1.002313f, 0.998268f, 1.003467f, 0.997114f };
    // Slow LFO rates per voice (Hz) — different primes so they never phase-lock
    static constexpr float kVoiceLFORates[5]     = { 0.07f, 0.11f, 0.05f, 0.13f, 0.09f };
    // LP filter cutoffs per voice (Hz) — higher harmonics are progressively darker
    static constexpr float kVoiceLPCutoffs[5]    = { 7000.0f, 5000.0f, 3500.0f, 2500.0f, 1800.0f };

    std::array<GranularShimmer, kMaxVoices * 2> granular;
    std::array<SSBShimmer, 2>                   ssb;
    // Per-voice LP filter states (kMaxVoices × stereo)
    std::array<float, kMaxVoices * 2> voiceLPState {};
    // Per-voice slow pitch LFO phases (0..1)
    std::array<float, kMaxVoices>     shimVoiceLFOPhases {};
    float shimFeedbackL { 0.0f };
    float shimFeedbackR { 0.0f };
    // 1-pole HP states on the shimmer feedback path — removes sub-bass that
    // accumulates in the recirculation loop and causes low-end saturation
    float shimHPL { 0.0f };
    float shimHPR { 0.0f };

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (DSPEngine)
};
