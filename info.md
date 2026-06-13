# Ethereal Reverb — Technical Reference

Author: Eshwar Rajasekar  
Version: 1.1.0  
Formats: VST3 · AU · Standalone  
Framework: JUCE 8.0.7

---

## Overview

Ethereal Reverb is an algorithmic reverb built around a Feedback Delay Network (FDN) with a Householder mixing matrix. It adds harmonic shimmer, multi-stage diffusion, spectral decay shaping, and an LFO-modulated tail. The design targets long, ethereal reverb tails suited to ambient and psychedelic music.

---

## Architecture (signal flow)

```
Input
  │
  ▼
[Pre-delay]         — fixed delay, 0–150ms
  │
  ▼
[FDN × 8]           — 8 delay lines, Householder cross-coupling
  │     ▲
  │     └── [Shimmer feedback] ─ [Granular pitch shift × N voices]
  │                            └─ [SSB frequency shift]
  ▼
[All-pass chain]    — 6 stages per channel (12 total)
  │
  ▼
[Tilt EQ]           — 1-pole shelf, brightens or darkens the tail
  │
  ▼
[Bloom envelope]    — 40ms attack on wet onset, 3s release
  │
  ▼
[Dry/wet blend]     — Mix knob
  │
  ▼
Output
```

---

## Stage 1 — Pre-delay

A simple circular buffer delay of `preDelay ms` (0–150ms). Separates the dry signal from the onset of the reverb tail, giving the source sound space before the room appears.

```
delaySamples = preDelay × 0.001 × sampleRate
```

---

## Stage 2 — Feedback Delay Network (FDN)

### Why FDN over Schroeder combs

The original Schroeder reverb uses parallel comb filters + all-pass diffusers. This produces audible "ringing" at the comb frequencies and a tone-colored tail. An FDN cross-couples all delay lines every sample via a mixing matrix, which destroys the periodicity and produces a much smoother, more natural-sounding decay.

### Delay line configuration

8 delay lines with mutually prime lengths (no common factors → no periodicity):

| Tap | Base time (ms) |
|-----|---------------|
| 0   | 19.7 |
| 1   | 22.3 |
| 2   | 29.1 |
| 3   | 33.7 |
| 4   | 41.1 |
| 5   | 47.3 |
| 6   | 53.9 |
| 7   | 61.1 |

Actual delay length = `baseTime × roomSize × sampleRate` samples.  
Taps 0–3 are driven by the left input; taps 4–7 by the right input. Stereo decorrelation is a natural consequence of the cross-coupling — no separate stereo processing needed.

### Householder mixing matrix

After reading all 8 taps, the Householder reflection is applied:

```
v[i] = y[i] − (2/N) × Σ(y[j])
```

Where `N = 8`. This is equivalent to the matrix `A = I − (2/N) × 1·1ᵀ`.

Properties:
- **Orthogonal**: `AᵀA = I` — the matrix preserves energy, no gain change
- **Diagonal entry**: `(N−2)/N = 0.75`
- **Off-diagonal entry**: `−2/N = −0.25`
- **O(N) cost**: only one sum needed, all off-diagonal elements are identical

### RT60 feedback gain

Each tap's feedback coefficient is derived from the RT60 decay time:

```
g[i] = 10^(−3 × delayLength[i] / (decay × sampleRate))
```

This gives exactly −60 dB of attenuation over `decay` seconds, per tap. Longer taps decay faster (shorter RT60 in samples) than shorter taps at the same decay setting, which matches the physics of real rooms.

### Decay color (frequency-dependent decay)

Two additional 1-pole lowpass filter states are maintained per tap:

- `fdnDampState[i]`: HF damping LP (cutoff controlled by the Damping knob)
- `fdnLFState[i]`: LF tracking LP at ~300 Hz

The `decayColor` parameter blends between three feedback signals:

```
if decayColor ≥ 0:
    fbSig = fdnDampState × (1 − decayColor) + v[i] × decayColor
    → +1 (bright): HF bypassed, highs sustain as long as lows

if decayColor < 0:
    fbSig = fdnDampState × (1 + decayColor) + fdnLFState × (−decayColor)
    → −1 (dark): only bass content survives in the feedback loop
```

### HF air shelf (v1.1)

A subtle high-frequency boost is applied to the feedback signal before the decay gain:

```
fbSig += 0.10 × (v[i] − fdnDampState[i])
```

`v[i] − fdnDampState[i]` approximates the high-frequency content (what the damping LP removed). Adding 10% of it back gives the tail a gentle "air" quality — the reverb gets slightly brighter as it decays, which is unnatural but sounds ethereal. The effect is cumulative over many feedback cycles, reaching approximately +0.8 dB at the damping crossover frequency.

### LFO modulation

Each FDN tap has an independent LFO that modulates its read position (fractional delay):

```
delay[i] = baseDelay[i] + sin(lfoPhase[i] × 2π) × modDepthSamples
lfoPhase[i] += modRate / sampleRate
```

`modDepthSamples = modDepth × 24` (max ~0.5 ms at 48 kHz). The LFOs are initialized at evenly spaced phases `(i/N)` so they never all align — this prevents periodic amplitude modulation and creates a smooth, chorusing width to the tail.

### Freeze mode

When Freeze is active, the feedback bypasses the decay gain entirely:

```
fb = freeze ? v[i] : fbSig × fdnG[i]
```

Signal circulates at unity gain, sustaining indefinitely. The input still feeds in, so new notes continue to blur into the sustained tail.

---

## Stage 3 — Shimmer

Shimmer creates pitch-shifted harmonics that recirculate inside the reverb, building an ascending harmonic series in the tail. It is designed for psychedelic, ambient, and pad sounds.

### Architecture: additive injection

The pitch-shifted feedback is injected **additively** at low level:

```
fdnInput = wetSignal + tanh(shimFeedback) × 0.15 × shimmer
```

A 1-pole HP filter at ~120 Hz removes sub-bass before injection to prevent low-frequency accumulation. The `tanh()` hard-limits the feedback to (−1, 1) regardless of reverb tail amplitude — this is the stability guarantee. An earlier blend-based design (`fdnInput = wet×(1−b) + shimFeedback×b`) caused exponential gain runaway because it fed back the FDN *output* (which accumulates to 100–1000× the dry input amplitude at long decay times) rather than the dry input.

### Voice architecture (up to 5 voices)

Each voice is a granular pitch shifter at a harmonic ratio of the base pitch:

| Voice | Ratio × shimmerPitch | Harmonic (with shimmerPitch=2.0) |
|-------|---------------------|----------------------------------|
| 1 | 1.0× | Octave (harmonic 2) |
| 2 | 1.5× | Octave + fifth (harmonic 3) |
| 3 | 2.0× | Two octaves (harmonic 4) |
| 4 | 2.5× | Two octaves + major third (harmonic 5) |
| 5 | 3.0× | Two octaves + fifth (harmonic 6) |

With `shimmerPitch = 2.0`, the 5 voices land exactly on harmonics 2–6 of the input note — the natural overtone series.

Amplitude weights (normalised): `[1.0, 0.65, 0.45, 0.30, 0.20]`. Weights are normalised by their sum so total shimmer energy stays constant as voices are added.

### Granular pitch shifter

Each voice uses a 16384-sample (341 ms at 48 kHz) circular buffer with two overlapping Hann-windowed grains:

```
w0 = 0.5 − 0.5 × cos(2π × phase0 / kBufSize)
w1 = 0.5 − 0.5 × cos(2π × phase1 / kBufSize)   where phase1 = phase0 + kBufSize/2

out = w0 × readAt(phase0) + w1 × readAt(phase1)

phase0 += pitchRatio
```

Because `phase1 = phase0 + kBufSize/2`, the two Hann windows are offset by exactly half a period. Their sum is always exactly 1.0:

```
w0 + w1 = (0.5 − 0.5cos θ) + (0.5 + 0.5cos θ) = 1.0
```

This guarantees constant-power crossfade — no amplitude pumping at the grain boundary.

### Harmonic naturalisation (v1.1)

Three enhancements make the harmonics sound less metallic:

**1. Per-voice lowpass filter**  
Each voice output is filtered before summing. Higher voices are progressively darker, matching how real acoustic spaces absorb high-frequency overtones:

| Voice | LP cutoff |
|-------|----------|
| 1 | 7000 Hz |
| 2 | 5000 Hz |
| 3 | 3500 Hz |
| 4 | 2500 Hz |
| 5 | 1800 Hz |

**2. Micro-detune per voice**  
Each voice is offset by a small fixed number of cents. This creates beating between voices that sounds organic rather than digitally perfect:

| Voice | Detune | Ratio |
|-------|--------|-------|
| 1 | 0 ¢ | 1.000000 |
| 2 | +4 ¢ | 1.002313 |
| 3 | −3 ¢ | 0.998268 |
| 4 | +6 ¢ | 1.003467 |
| 5 | −5 ¢ | 0.997114 |

Conversion: `ratio = 2^(cents/1200)`

**3. Slow pitch LFO per voice**  
Each voice has an independent slow LFO (±0.5% depth) that gently wobbles the pitch ratio. Rates are chosen to be mutually non-locking:

| Voice | LFO rate |
|-------|---------|
| 1 | 0.07 Hz |
| 2 | 0.11 Hz |
| 3 | 0.05 Hz |
| 4 | 0.13 Hz |
| 5 | 0.09 Hz |

### SSB frequency shifter (Character knob)

The `shimmerChar` knob blends in a single-sideband (SSB) frequency shifter alongside the granular voices. Unlike granular pitch shifting (which maintains harmonic intervals), SSB shifts all frequencies up by a fixed Hz amount — creating inharmonic beating and drift.

Implementation: Berners-Abel IIR Hilbert pair — two first-order allpass sections per branch:

```
Branch A coefficients: [0.4788, 0.8762]
Branch B coefficients: [0.1617, 0.7330]

Upper sideband: out = sigA × cos(phase) − sigB × sin(phase)
phase += 2π × shiftHz / sampleRate
```

Phase error is < 2° across 80 Hz–18 kHz at 48 kHz. The SSB gives a subtle "alive" quality distinct from the clean harmonic shimmer — particularly effective at 10–20 Hz shift amounts.

---

## Stage 4 — All-pass diffusion

6 all-pass filter stages per channel (12 total), run in series. Delay times are mutually non-harmonic to prevent comb filtering:

```
Times (ms): 1.7, 3.1, 5.0, 8.9, 12.3, 15.7
```

Each stage implements the Schroeder all-pass structure:

```
out = −g × in + delayed
write: in + g × delayed
```

Where `g = diffusion × 0.7` (Schroeder's canonical coefficient, scaled by the Diffusion knob). The all-pass has flat magnitude response but adds complex phase — increasing echo density without tonal coloration. 6 stages in series creates a much denser, smoother wash than the original 2 stages.

---

## Stage 5 — Tilt EQ

A 1-pole shelf applied post-diffusion:

```
wetOut += tiltEQ × (tiltState − wetOut)
tiltState = (1 − tiltCoeff) × wetOut + tiltCoeff × tiltState
```

Where `tiltCoeff = 1 − 2π × 1000 / sampleRate` (shelf at ~1 kHz).

- `tiltEQ > 0`: output blends toward lowpass state → darker tail  
- `tiltEQ < 0`: subtracts lowpass state from output → brighter tail

---

## Stage 6 — Bloom onset envelope (v1.1)

The wet signal is gated through a smooth attack envelope:

```
inputAbs = (|dryL| + |dryR|) × 0.5
bloomTarget = inputAbs > 0.001 ? 1.0 : 0.0

if bloomTarget > bloomEnv:
    bloomEnv = attackCoeff × bloomEnv + (1−attackCoeff) × bloomTarget   [40ms attack]
else:
    bloomEnv = releaseCoeff × bloomEnv                                    [3s release]

output = dry + (wet − dry) × mix × bloomEnv
```

Attack: 40 ms — the reverb fades in underneath each note rather than appearing as an instant wall. Release: 3 s — long enough to sustain the reverb tail after the input stops without abrupt cutoff. This creates the "reverb emerging from silence" quality.

---

## Parameters reference

| Parameter | Range | Default | Description |
|-----------|-------|---------|-------------|
| Pre-Delay | 0–150 ms | 0 | Delay before reverb onset |
| Room Size | 0–1 | 0.5 | Scales all delay line lengths |
| Decay | 0.1–60 s | 2.0 | RT60 time (skewed curve, 0.35 exponent) |
| Damping | 0–1 | 0.5 | HF cutoff in feedback loop |
| Diffusion | 0–1 | 0.5 | All-pass gain (echo density) |
| Mod Rate | 0.1–8 Hz | 0.5 | LFO speed per FDN tap |
| Mod Depth | 0–1 | 0.1 | LFO displacement of read position |
| Tilt EQ | −1–+1 | 0 | Spectral tilt of tail |
| Mix | 0–1 | 0.3 | Dry/wet balance |
| Decay Color | −1–+1 | 0 | Dark (−1) / neutral (0) / bright (+1) |
| Shimmer | 0–1 | 0 | Shimmer amount |
| Shimmer Pitch | 0.5–3.0 | 2.0 | Granular pitch ratio (1.5=fifth, 2.0=octave) |
| Character | 0–1 | 0.3 | 0=pure granular, 1=50/50 granular+SSB |
| Shift Hz | 5–50 Hz | 15 | SSB frequency offset |
| Voices | 1–5 | 1 | Number of harmonic voices in shimmer |
| Freeze | on/off | off | Locks FDN feedback at unity gain |
| Reverse | on/off | off | Parallel reverse reverb; REV TIME knob sets buffer length |
| Rev Amt | 0–1 | 0.75 | Injection level of reversed signal into FDN (visible only when Reverse is active) |

---

## Stage 7 — Reverse reverb (v1.2)

When Reverse is active, a ping-pong buffer pair runs in parallel with the normal signal path.

### Algorithm

Two stereo buffer pairs (`A` and `B`) alternate roles each cycle:

```
Write side fills with post-predelay signal, sample by sample.
When write side reaches revBufLen samples:
    std::reverse(writeBuf, writeBuf + revBufLen)  → in-place reversal
    swap write/read sides
    revWritePos = 0; revReadPos = 0; revBufReady = true

Read side outputs: out[n] = readBuf[revReadPos++]  (advances until next swap)
```

The reversed signal is injected **additively** into the FDN input in parallel with the normal signal:

```
fdnInput = wetSignal + shimClip×shimScale + reversedSignal×reverseMix
```

`reverseMix` (Rev Amt knob, 0–1, default 0.75) controls injection level. Higher values produce a more prominent reverse swell; lower values blend it subtly under the forward tail. The tanh/HP shimmer path already bounds the total FDN injection, so stability is maintained.

### Buffer length

`revBufLen = clamp(preDelay × 0.001 × sampleRate, 1, 15000)`

The Pre-Delay knob doubles as the reverse window control (the UI relabels it "REV TIME" when Reverse is active). Longer values create a longer swell before the note.

### Latency compensation

The reversed chunk is delayed by one buffer length relative to the live signal (the buffer must fill before it can be reversed). The processor reports this via `setLatencySamples(revBufLen)` so DAW hosts can compensate automatically. In standalone/live use there is inherent latency equal to `preDelay` ms.

### What it sounds like

Both forward and reverse tails play simultaneously. The reverse path contributes a "swell" that builds up to the note, giving a backwards-underwater quality. The forward tail decays normally after the note. Together they create a symmetric bloom around each transient — characteristic of psychedelic and ambient music.

---

## Key design decisions

**FDN over Schroeder combs**: Schroeder combs produce audible metallic ringing at their resonant frequencies. FDN with Householder mixing destroys periodicity and produces modal density that grows as O(N!) with tap count, giving a dense, natural-sounding tail.

**Householder over random matrix**: A random orthogonal matrix requires O(N²) multiplications per sample. Householder is O(N) — only one sum needed — while remaining perfectly lossless (orthogonal). For N=8 this is a 64× speedup over a full matrix multiply.

**Shimmer as additive injection, not blend**: The FDN output amplitude at long decay times can be 500–1000× the dry input due to accumulation. Blending the pitch-shifted output back at any percentage caused instant saturation. Additive injection at fixed low level (-16 dB) with tanh limiting gives the correct behaviour used by professional shimmer reverbs (Valhalla, Eventide).

**Granular pitch shifting over phase vocoder**: Granular is simpler to implement, has O(1) per-sample cost, and produces a characteristic texture that suits ambient/psychedelic music. Phase vocoder would be higher quality but adds significant FFT complexity and latency.

**Natural harmonic ratios for voices**: Ratios 1×, 1.5×, 2×, 2.5×, 3× relative to the base pitch correspond to harmonics 2, 3, 4, 5, 6 of the fundamental when `shimmerPitch = 2.0`. This is the natural overtone series — any note played produces harmonically correct shimmer voices.

**Mono input support**: `isBusesLayoutSupported` accepts mono or stereo input. Mono input (e.g. standalone mic) is duplicated to stereo before DSP processing, ensuring the full stereo FDN is always active.

**Reverse as additive not replacement**: Blending the reversed signal in parallel (rather than replacing the normal signal) lets both forward decay and backward swell coexist. This is the psychedelic use case — the note blooms from silence, sustains, then trails off, all at once.

**Ping-pong over circular read**: A circular reverse buffer would require reading backwards from the write head, but the write head moves forward at sample rate while the read position must advance at sample rate too. Ping-pong avoids this contradiction: one buffer is a fixed snapshot that is read front-to-back (after reversal), while the other fills continuously. The audible artifact is a periodic "seam" every `revBufLen` samples as the buffers swap — at 100ms buffer length this is inaudible at normal listening levels.

**UI hardware redesign**: The plugin uses horizontal strip panels (SPACE, CENTER/MOTION, SHIMMER) to match the visual language of hardware units. Each strip has a subtle raised-panel edge highlight and corner rivets. In psychedelic (Reverse active) mode the background shifts from near-black-navy to a crimson→forest-green top-to-bottom gradient, each strip sampling that gradient at its y-position so the SPACE strip is red-tinted and the SHIMMER strip is green-tinted. Pulsing concentric rings animate outward from center. The transition animates over ~625ms via a `psychedelicBlend` float driven by a 20Hz timer.
