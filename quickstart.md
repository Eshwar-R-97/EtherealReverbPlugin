# Ethereal Reverb — Quickstart Guide

## Installation

| Format | Copy to |
|--------|---------|
| VST3 (FL Studio / Ableton / etc.) | `~/Library/Audio/Plug-Ins/VST3/` |
| AU (Logic Pro) | `~/Library/Audio/Plug-Ins/Components/` |
| Standalone | `/Applications/` |

After copying, rescan plugins in your DAW. Logic Pro detects AU automatically after a restart.

---

## Signal flow in one line

Dry signal → Pre-delay → FDN reverb → Diffusion → Tilt EQ → Bloom → Mix

---

## Knob-by-knob reference

### SPACE strip (top row)

| Knob | What it does | Sweet spots |
|------|-------------|-------------|
| **Pre-Delay** | Silence before reverb onset (0–150 ms). Gives vocals and instruments space before the room appears. In Reverse mode this becomes **REV TIME** — it sets how long the backwards swell is. | 15–40 ms for vocals; 80–120 ms for dramatic reverse swells |
| **Room Size** | Scales all FDN delay lines. Small = tight and intimate. Large = cavernous. | 0.3–0.5 for realism; 0.8–1.0 for infinite space |
| **Decay** | RT60 time: how long the tail takes to reach −60 dB (0.1–60 s). | 1–3 s for naturalistic; 10–30 s for ambient pads; max for drones |
| **Damping** | High-frequency rolloff in the feedback loop. High damping = warm, dark tail; low damping = bright, airy tail. | 0.4–0.7 for most material |
| **Diffusion** | Echo density in the all-pass chain. Low = you can hear discrete reflections; high = smooth, washed-out texture. | 0.5–0.8 for smooth; 0.1–0.3 for textured grit |
| **Tilt EQ** | Spectral tilt of the wet signal. Negative = darker tail; positive = brighter tail. | ±0.2–0.4 for subtle shaping |

### CENTER / MOTION panel (middle section)

| Knob | What it does | Sweet spots |
|------|-------------|-------------|
| **Mod Rate** | LFO speed modulating each FDN tap's delay time (0.1–8 Hz). | 0.2–0.6 Hz for natural chorus; 2–5 Hz for dramatic wobble |
| **Mod Depth** | How far the LFO displaces the read head (0–1 = 0–0.5 ms max). | 0.05–0.2 for subtle width; 0.5+ for deliberate warbling |
| **Mix** | Dry/wet balance. 0 = fully dry, 1 = fully wet. | 0.25–0.45 for most tracks; 1.0 for send/return |
| **Decay Color** | Shapes which frequencies decay fastest. −1 = only bass survives; 0 = neutral; +1 = highs sustain as long as lows. | −0.3 for warm vintage; +0.4 for shimmery brightness |

### SHIMMER strip (bottom collapsible)

Click **SHIMMER ▼** to expand/collapse. All knobs here affect the harmonic pitch-shift feedback loop that builds an overtone series inside the reverb.

| Knob / Control | What it does | Sweet spots |
|----------------|-------------|-------------|
| **Shimmer** | Overall shimmer amount. 0 = off; 1 = full shimmer injection. | 0.2–0.5 for subtle harmonic glow; 0.7–1.0 for wall-of-shimmer |
| **Pitch** | Pitch shift ratio (0.5–3.0). 1.0 = unison, 1.5 = fifth, 2.0 = octave. | 2.0 (octave up) is the classic shimmer sound |
| **Character** | Blends granular pitch shifting (0) with a single-sideband frequency shifter (1). SSB creates inharmonic drift and beating. | 0–0.3 for clean harmonics; 0.5–1.0 for psychedelic metallic texture |
| **Shift Hz** | Frequency offset for the SSB shifter (5–50 Hz). Only audible when Character > 0. | 10–20 Hz for slow alien shimmer; 40+ Hz for aggressive detuning |
| **Voices** | Number of harmonic voices (1–5). Each voice adds the next harmonic from the overtone series. | 1 for a single octave; 3–5 for a full harmonic choir |
| **FREEZE** | Locks the FDN at unity feedback — signal recirculates forever. New input still blurs in. | Hold a pad, freeze it, play over the sustained cloud |
| **REVERSE** | Enables parallel reverse reverb. The reversed signal is injected additively into the FDN alongside the normal tail. When active: Pre-Delay is relabelled **REV TIME** and a **REV AMT** knob appears. | Enable on percussive elements for backwards swells |
| **REV AMT** *(appears when REVERSE is on)* | Controls how strongly the reversed signal feeds into the FDN (0–1). | 0.5–0.75 for subtle blend; 1.0 for dominant reverse swell |

---

## Presets (preset dropdown)

| Preset | Character |
|--------|-----------|
| Default | Neutral starting point, all knobs near center |
| Cathedral | Large bright decay, high diffusion, slow mod |
| Shimmer Pad | Octave shimmer, long decay, high mix |
| Dark Room | Heavy damping, negative decay color, short decay |
| Psychedelic | Reverse enabled, high shimmer, SSB character |
| Vocal Plate | Short pre-delay, medium decay, darker tilt |
| Infinite Drone | Freeze enabled, long decay, 5 shimmer voices |
| Subtle | Low mix, short decay, minimal diffusion |

---

## Tips

**For send/return (most common DAW setup)**  
Set Mix to 1.0. Route a send from your track to the reverb, and control the wet amount from the send level. This keeps the dry signal clean.

**Making reverse reverb audible**  
Turn Pre-Delay (REV TIME) to 80–120 ms, Rev Amt to 0.75, Decay to 4–8 s, and Mix to 0.4. The swell will bloom in the first 80–120 ms before each transient. Best on sustained notes, not fast rhythmic material.

**Hearing shimmer clearly**  
Shimmer feeds back inside the reverb — it gets louder with longer decays. Set Decay > 5 s, Shimmer > 0.4, and Pitch to 2.0 (octave). Add Voices 3–5 for a full harmonic cloud.

**Freeze layering**  
Play a chord, hit Freeze. The chord sustains. Play a new melody over it. The melody's reverb also feeds the frozen tail, creating a thickening cloud. Toggle Freeze off to let it decay naturally.

---

For full DSP math, architecture details, and design decisions see [info.md](info.md).
