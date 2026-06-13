# Ethereal Reverb

An algorithmic reverb plugin by Eshwar Rajasekar. Available as AU (Logic Pro) and VST3 (FL Studio).

Made for learning how to build audio plugins, if you have any suggestions open an issue and I'll try to specialize this plugin to that!

---

## Installing for Logic Pro (AU)

**1. Copy the plugin**

```bash
cp -R "Ethereal Reverb.component" ~/Library/Audio/Plug-Ins/Components/
```

**2. Remove the macOS quarantine flag** (required for unsigned builds)

```bash
sudo xattr -rd com.apple.quarantine ~/Library/Audio/Plug-Ins/Components/"Ethereal Reverb.component"
```

**3. Load in Logic**

- Open Logic Pro
- Create an Audio or Software Instrument track
- Open the plugin slot → Audio Units → EshwarRajasekar → Ethereal Reverb

---

## Installing for FL Studio (VST3)

**1. Copy the plugin**

```bash
cp -R "Ethereal Reverb.vst3" ~/Library/Audio/Plug-Ins/VST3/
```

**2. Remove the macOS quarantine flag** (required for unsigned builds)

```bash
sudo xattr -rd com.apple.quarantine ~/Library/Audio/Plug-Ins/VST3/"Ethereal Reverb.vst3"
```

**3. Load in FL Studio**

- Open FL Studio
- Go to Options → Manage Plugins
- Click **Find more plugins** and make sure `~/Library/Audio/Plug-Ins/VST3/` is in the search paths
- Hit **Rescan** — Ethereal Reverb will appear in your plugin list
- Drag it onto a mixer insert as an effect

---

## Controls

| Knob | Range | What it does |
|---|---|---|
| **Pre-Delay** | 0–150ms | Silence before the reverb tail starts. Adds space between the dry signal and the room. |
| **Room Size** | 0–1 | Scales all internal delay lengths. Small = tight, large = cavernous. |
| **Decay** | 0.1–60s | RT60 time — how long it takes the tail to fall 60dB. |
| **Damping** | 0–1 | High frequencies die faster at higher values. Simulates absorptive surfaces. |
| **Diffusion** | 0–1 | Echo density. Low = sparse early reflections, high = smooth wash. |
| **Tilt EQ** | -1 to +1 | Tilts the frequency balance of the tail. Negative = darker, positive = brighter. |
| **Mod Rate** | 0–10Hz | LFO speed applied to the comb filters. Adds movement and shimmer. |
| **Mod Depth** | 0–1 | How much the LFO wobbles the delay lines. Higher = more chorus-like. |
| **Mix** | 0–1 | Dry/wet blend. |
| **Freeze** | On/Off | Locks the reverb tail at unity gain — it sustains indefinitely. |

---

## Presets

| Preset | Character |
|---|---|
| Default | Neutral starting point |
| Studio Room | Small, tight, natural |
| Cathedral | Huge, dark, slow |
| Plate | Dense, bright, classic |
| Dark Cave | Deep low-end, heavy damping |
| Shimmer Pad | Long, bright, modulated |
| Broken Spring | Fast LFO chaos |
| Frozen Void | Maximum decay, freeze-ready |
| Snare Punch | Short, punchy, gated feel |
| Deep Space | Extreme tail, wide diffusion |

---

## Building from Source

Requires CMake 3.22+, Ninja, and Xcode command line tools.

```bash
git clone <repo>
cd etherealreverbplugin

# Clone JUCE into the expected location
mkdir -p build/_deps
git clone --depth=1 --branch 8.0.7 https://github.com/juce-framework/JUCE.git build/_deps/juce-src

# Configure and build
cmake -G Ninja -B build
cmake --build build --target EtherealReverb_AU          # Logic Pro
cmake --build build --target EtherealReverb_VST3        # FL Studio
cmake --build build --target EtherealReverb_Standalone  # Standalone app
```
