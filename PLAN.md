# Ethereal Reverb — DSP Upgrade Plan

---

## Where We Are: Schroeder Architecture

The current plugin uses the **Schroeder reverb** (1962), which is the foundational algorithm that every reverb is descended from. Here's exactly what it does:

```
Input → [4 parallel comb filters] → [Hadamard mix] → [2 series all-pass filters] → Output
```

Each **comb filter** is a delay line with feedback. The feedback gain `g` is derived from RT60:

```
g = 10^(-3N / (decay * sampleRate))
```

The **all-pass filters** add phase smearing without tone coloring — this increases echo density.

### What Schroeder sounds like
- Starts relatively sparse, builds density over time
- Has a characteristic "metallic" or "ringy" quality at longer decays — the individual comb resonances are audible
- The tail is somewhat periodic and artificial-sounding on pitched material (you can hear the individual delay times)
- Works well for drums, percussion, short tails — starts to fall apart on long ambient tails or sustained notes

---

## The Next Step: Feedback Delay Network (FDN)

An **FDN** replaces the parallel comb bank with N delay lines in a fully interconnected feedback loop. Every delay line feeds into every other delay line on every sample, through a **mixing matrix**.

```
          ┌─────────────── A (N×N matrix) ──────────────────┐
          │                                                  │
Input → [+] → [delay 1] → [filter 1] ──┐                   │
        [+] → [delay 2] → [filter 2] ──┤── A ──────────────┘→ Output
        [+] → [delay 3] → [filter 3] ──┤
        [+] → [delay 4] → [filter 4] ──┘
```

The matrix `A` must be **lossless** (unitary — orthogonal with norm 1) to preserve energy perfectly when decay=∞. A Householder or Hadamard matrix works. We already use a Hadamard for the Schroeder mix step — in an FDN it becomes the core feedback matrix.

### Schroeder vs FDN: side by side

| | Schroeder | FDN |
|---|---|---|
| **Delay lines** | Parallel, independent | Fully cross-coupled every sample |
| **Feedback** | Each comb feeds only itself | Every line feeds into every other line |
| **Density** | Builds slowly, can sound sparse | Dense from the start |
| **Metallic coloring** | Yes — individual combs ring | No — cross-coupling smears resonances |
| **Frequency-dependent decay** | Hard to add | Natural — put a filter on each tap |
| **Stereo** | Two separate banks (L/R) | Single matrix covers both channels |
| **Complexity** | Simple | Moderately complex — matrix math |
| **Sound** | Classic, lo-fi, retro | Smooth, natural, modern |

### What they share
- Both are built from delay lines as the fundamental unit
- Both use feedback gain to control RT60
- Both can use all-pass filters for additional diffusion
- Both support LFO modulation on delay read positions
- Both support freeze (set feedback gain = 1.0)

---

## Upgrade Plan

### Stage 7 — Replace Schroeder with an 8-tap FDN

**What changes:** Replace the 4+4 parallel comb banks with a single 8-tap FDN using an 8×8 Householder feedback matrix.

**Why 8 taps:** More taps = denser tail. 4 taps is audibly sparse for ambient/pad material. 8 is the standard for high-quality algorithmic reverb.

**Delay times:** Use mutually prime values (no common factors) to avoid periodic resonances:
```
{19.7, 22.3, 29.1, 33.7, 41.1, 47.3, 53.9, 61.1} ms
```

**Per-tap filter:** Each tap gets a one-pole lowpass in the feedback path. The cutoff is controlled by the Damping knob. This gives frequency-dependent decay for free — HF decays faster than LF naturally, which is how real rooms work.

**Feedback matrix:** 8×8 Householder reflection:
```
H = I - (2/N) * v * v^T   where v = [1,1,1,...,1]
```
This is lossless, cheap to compute (O(N) not O(N²)), and well-studied.

**You own:** the delay time choices, the per-tap filter design, the matrix selection rationale.  
**Framework:** the CircularBuffer infrastructure already exists and works.

---

### Stage 8 — Frequency-Dependent Decay

**What it enables:** Different RT60 at low, mid, and high frequencies — the single most impactful thing separating budget reverbs from high-end ones.

Split the signal into 3 bands (LF/MF/HF) using Linkwitz-Riley crossovers. Run each band through the FDN with a different decay multiplier. Recombine.

New parameter: **Decay Shape** — a single knob that morphs from "bright decay" (HF lives longer than LF) to "dark decay" (HF dies fast). Center = natural room behavior.

**You own:** crossover frequencies, the decay curve per band, the Decay Shape morphing formula.

---

### Stage 9 — Shimmer Mode

Shimmer is pitch-shifted feedback — the reverb tail slowly rises in pitch, creating an ethereal, choir-like wash. Made famous by the Eventide H3000 and Valhalla Shimmer.

**Implementation:** In the FDN feedback path, add a pitch shifter on 2 of the 8 taps. A simple granular pitch shifter (overlap-add, grain size ~80ms) is sufficient and sounds great.

New parameter: **Shimmer** — 0 = off, 1 = full octave-up shimmer. At intermediate values it blends between pure reverb and shimmer.

This is the single biggest differentiator. No free reverb plugin does shimmer well. This would make Ethereal Reverb genuinely distinct.

**You own:** the grain window function, the pitch ratio (1 octave up is 2x, fifth is 1.5x), the blend amount.

---

### Stage 10 — Nonlinear Feedback (Saturation)

Add soft saturation in the feedback path of the FDN. This models tape and tube warmth — the reverb tail gets slightly harmonically enriched as it recirculates.

**Implementation:** `tanh(x * drive) / drive` per tap, where `drive` is scaled by a new parameter.

New parameter: **Character** — 0 = clean linear, 1 = warm saturated. Even subtle amounts (0.1–0.2) make a dramatic difference on sustained material.

**You own:** the saturation curve shape and the drive scaling formula.

---

### Stage 11 — Reverse Mode

Capture a buffer of the reverb tail, reverse it, and play it back. The effect: the reverb swells before the transient rather than after — iconic on vocals and snares.

**Implementation:** Double-buffer approach. While one buffer plays back reversed, the other is filling. Switch on zero crossings to avoid clicks.

New parameter: **Reverse** toggle. When active, the Decay knob controls the buffer length.

---

## Summary: What Would Make This Genuinely Unique

Right now Ethereal Reverb sounds like a textbook plugin. Here's the stack that would make it stand out:

1. **FDN core** → removes the metallic Schroeder coloration (Stage 7)
2. **Frequency-dependent decay** → sounds like a real room (Stage 8)
3. **Shimmer** → the one thing that makes people reach for it specifically (Stage 9)
4. **Saturation** → warmth and character that's hard to describe but immediately felt (Stage 10)
5. **Reverse mode** → creative tool, not just a utility reverb (Stage 11)

Stages 7+8 make it a *good* reverb. Stages 9+10+11 make it a reverb with a *personality*.

The DSP math for all of these is yours to design. The JUCE wiring, parameter routing, UI controls, and build system will be handled in the framework layer.
