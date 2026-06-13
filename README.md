# Ethereal Reverb

An algorithmic reverb plugin by Eshwar Rajasekar — available for **FL Studio** (Mac) and **Logic Pro**.

Made for learning how to build audio plugins, if you have any suggestions open an issue and I'll try to specialize this plugin to that!

---

## How to Install

### What you need
- A Mac (macOS 10.15 or later)
- FL Studio or Logic Pro already installed

### Step 1 — Download the plugin files

1. On this page, click the green **"<> Code"** button near the top right
2. Click **"Download ZIP"**
3. Once downloaded, double-click the ZIP file in your Downloads folder — it will unzip into a folder called `etherealreverbplugin-main`

Inside that folder, open the **`releases`** folder. You'll see two sub-folders:
- `FL Studio (VST3)` — contains **Ethereal Reverb.vst3**
- `Logic Pro (AU)` — contains **Ethereal Reverb.component**

---

### Installing for FL Studio

**Step 2 — Copy the plugin to the right place**

1. Open **Finder**
2. In the menu bar at the top of your screen, click **Go → Go to Folder...**
3. Type this exactly and press Enter:
   ```
   ~/Library/Audio/Plug-Ins/VST3
   ```
4. Drag **Ethereal Reverb.vst3** from the `releases/FL Studio (VST3)` folder into this VST3 folder

**Step 3 — Remove the macOS security lock**

Because this plugin isn't sold through the App Store, macOS puts a security lock on it. You need to remove that lock using an app called **Terminal**.

To open Terminal:
- Press **Command + Space** to open Spotlight Search
- Type **Terminal** and press Enter

Once Terminal is open, copy and paste this entire line and press Enter:

```
sudo xattr -rd com.apple.quarantine ~/Library/Audio/Plug-Ins/VST3/"Ethereal Reverb.vst3"
```

It will ask for your **Mac login password**. Type it and press Enter. (The password won't show as you type — that's normal.)

**Step 4 — Load in FL Studio**

1. Open FL Studio
2. Go to **Options → Manage Plugins**
3. Make sure the VST3 folder path is listed — if not, click **"Find more plugins"** and add `/Users/[yourname]/Library/Audio/Plug-Ins/VST3`
4. Click **Rescan** — Ethereal Reverb will now appear in your plugin browser
5. To use it, drag it onto a **mixer insert** as an effect

---

### Installing for Logic Pro

**Step 2 — Copy the plugin to the right place**

1. Open **Finder**
2. In the menu bar at the top of your screen, click **Go → Go to Folder...**
3. Type this exactly and press Enter:
   ```
   ~/Library/Audio/Plug-Ins/Components
   ```
4. Drag **Ethereal Reverb.component** from the `releases/Logic Pro (AU)` folder into this Components folder

**Step 3 — Remove the macOS security lock**

Open Terminal (press **Command + Space**, type **Terminal**, press Enter), then copy and paste this line and press Enter:

```
sudo xattr -rd com.apple.quarantine ~/Library/Audio/Plug-Ins/Components/"Ethereal Reverb.component"
```

It will ask for your **Mac login password**. Type it and press Enter. (The password won't show as you type — that's normal.)

**Step 4 — Load in Logic**

1. Open Logic Pro
2. Create an **Audio** or **Software Instrument** track
3. Click the plugin slot on the channel strip
4. Navigate to **Audio Units → EshwarRajasekar → Ethereal Reverb**

> **If Logic says "validation failed":** Quit Logic, re-run the Terminal command from Step 3, then reopen Logic. If it still doesn't appear, go to **Logic Pro → Settings → Plug-in Manager**, find Ethereal Reverb, and click **Reset & Rescan**.

---

## Controls

| Knob | What it does |
|---|---|
| **Pre-Delay** | Silence before the reverb starts. Higher = more space between the original sound and the room effect. |
| **Room Size** | How big the virtual room is. Small = tight and close, large = cavernous. |
| **Decay** | How long the reverb tail lasts (0.1 to 60 seconds). High values create huge, ambient washes. |
| **Damping** | How quickly high frequencies fade. High damping = warmer, darker reverb (like carpet and curtains). |
| **Diffusion** | How dense the reverb is. Low = you can hear distinct echoes, high = smooth wash. |
| **Tilt EQ** | Shifts the tone of the reverb tail. Turn left for darker, turn right for brighter. |
| **Mod Rate** | Speed of the subtle wobble/movement in the reverb. Adds shimmer and life. |
| **Mod Depth** | How strong the movement is. High values give a more chorus-like or dreamy quality. |
| **Mix** | Blend between the dry (original) signal and the wet (reverb) signal. |
| **Freeze** | Locks the reverb tail so it sustains forever — great for ambient swells and drones. |

---

## Presets

| Preset | Sound |
|---|---|
| Default | Neutral starting point |
| Studio Room | Small, tight, natural — good for drums or vocals |
| Cathedral | Huge, dark, slow — cinematic and spiritual |
| Plate | Dense, bright, classic — the sound of classic records |
| Dark Cave | Deep low-end, heavy damping — cavernous and moody |
| Shimmer Pad | Long, bright, modulated — ethereal and evolving |
| Broken Spring | Fast LFO chaos — wobbly and unpredictable |
| Frozen Void | Maximum decay, freeze-ready — infinite ambience |
| Snare Punch | Short, punchy — designed for drums |
| Deep Space | Extreme tail, wide stereo — vast and cinematic |

---

## Troubleshooting

**The plugin doesn't show up in my DAW**
- Make sure you completed Step 3 (the Terminal quarantine removal command)
- Try rescanning plugins in your DAW's settings
- Restart your DAW after copying the plugin file

**Terminal says "Permission denied"**
- Make sure you typed `sudo` at the beginning of the command
- When it asks for your password, type your Mac login password (it won't show dots or characters — just type and press Enter)

**I accidentally closed Terminal before it finished**
- Just reopen Terminal and run the command again — it's safe to run more than once

---

## Building from Source (advanced)

This section is for developers only. You'll need CMake 3.22+, Ninja, and Xcode command-line tools installed.

```bash
git clone https://github.com/Eshwar-R-97/EtherealReverbPlugin.git
cd EtherealReverbPlugin

# Pre-clone JUCE to avoid network issues
mkdir -p build/_deps
git clone --depth=1 --branch 8.0.7 https://github.com/juce-framework/JUCE.git build/_deps/juce-src

# Configure and build
cmake -G Ninja -B build
cmake --build build --target EtherealReverb_AU          # Logic Pro
cmake --build build --target EtherealReverb_VST3        # FL Studio
cmake --build build --target EtherealReverb_Standalone  # Standalone app
```
