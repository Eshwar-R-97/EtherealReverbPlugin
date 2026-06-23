#pragma once
#include <JuceHeader.h>
#include "PluginProcessor.h"

struct Preset
{
    const char* name;
    float preDelay, roomSize, decay, damping, diffusion;
    float modRate, modDepth, tiltEQ, mix, decayColor;
    float shimmer, shimmerPitch, shimmerChar, shimmerShiftHz;
    int   shimmerVoices;
    bool  freeze;
    bool  reverse;
    float reverseMix;
    float dream, swim, modShape, shimmerDrift, shimmerDir;
    float glow, cloud, vox, realm, chaos;
};

// ─────────────────────────────────────────────────────────────────────────────
//  Hardware-style LookAndFeel
// ─────────────────────────────────────────────────────────────────────────────
class EtherealLookAndFeel : public juce::LookAndFeel_V4
{
public:
    EtherealLookAndFeel();

    void drawRotarySlider (juce::Graphics&, int x, int y, int width, int height,
                           float sliderPosProportional, float rotaryStartAngle,
                           float rotaryEndAngle, juce::Slider&) override;

    void drawToggleButton (juce::Graphics&, juce::ToggleButton&,
                           bool shouldDrawButtonAsHighlighted,
                           bool shouldDrawButtonAsDown) override;

    void drawButtonBackground (juce::Graphics&, juce::Button&,
                               const juce::Colour& backgroundColour,
                               bool shouldDrawButtonAsHighlighted,
                               bool shouldDrawButtonAsDown) override;

    void drawButtonText (juce::Graphics&, juce::TextButton&,
                         bool shouldDrawButtonAsHighlighted,
                         bool shouldDrawButtonAsDown) override;

    void drawComboBox (juce::Graphics&, int width, int height, bool isButtonDown,
                       int buttonX, int buttonY, int buttonW, int buttonH,
                       juce::ComboBox&) override;

    void drawPopupMenuBackground (juce::Graphics&, int width, int height) override;

    void drawPopupMenuItem (juce::Graphics&, const juce::Rectangle<int>& area,
                            bool isSeparator, bool isActive, bool isHighlighted,
                            bool isTicked, bool hasSubMenu, const juce::String& text,
                            const juce::String& shortcutKeyText,
                            const juce::Drawable* icon,
                            const juce::Colour* textColourToUse) override;

    static constexpr juce::uint32 kBackground = 0xff080814;
    static constexpr juce::uint32 kPanel      = 0xff0e0e1c;
    static constexpr juce::uint32 kAccent     = 0xff00b4ff;
    static constexpr juce::uint32 kAccentDim  = 0xff004060;
    static constexpr juce::uint32 kFreeze     = 0xffff4040;
    static constexpr juce::uint32 kText       = 0xffe0e0ec;
    static constexpr juce::uint32 kTextDim    = 0xff484860;
    static constexpr juce::uint32 kTrack      = 0xff1a1a2c;

    float psychedelicBlend { 0.0f };  // 0 = normal, 1 = full psychedelic mode
};

// ─────────────────────────────────────────────────────────────────────────────
//  Editor
// ─────────────────────────────────────────────────────────────────────────────
class EtherealReverbEditor : public juce::AudioProcessorEditor,
                             private juce::Timer
{
public:
    explicit EtherealReverbEditor (EtherealReverbProcessor&);
    ~EtherealReverbEditor() override;

    void paint   (juce::Graphics&) override;
    void resized () override;

private:
    void setupKnob    (juce::Slider&, juce::Label&, const juce::String& labelText);
    void applyPreset  (int index);
    void timerCallback() override;

    EtherealReverbProcessor& processorRef;
    EtherealLookAndFeel      laf;

    // ── Knobs ──────────────────────────────────────────────────────────────
    juce::Slider preDelayKnob,   roomSizeKnob,  decayKnob;
    juce::Slider dampingKnob,    diffusionKnob, tiltEQKnob;
    juce::Slider modRateKnob,    modDepthKnob,  mixKnob;
    juce::Slider decayColorKnob, dreamKnob,     swimKnob, modShapeKnob;
    juce::Slider shimmerKnob, shimmerPitchKnob, shimmerCharKnob, shimmerShiftHzKnob;
    juce::Slider shimmerDriftKnob, shimmerDirKnob;
    juce::Slider glowKnob, cloudKnob, voxKnob, realmKnob, chaosKnob;

    // ── Knob labels ────────────────────────────────────────────────────────
    juce::Label  preDelayLabel,   roomSizeLabel,  decayLabel;
    juce::Label  dampingLabel,    diffusionLabel, tiltEQLabel;
    juce::Label  modRateLabel,    modDepthLabel,  mixLabel;
    juce::Label  decayColorLabel, dreamLabel,     swimLabel, modShapeLabel;
    juce::Label  shimmerLabel, shimmerPitchLabel, shimmerCharLabel, shimmerShiftHzLabel;
    juce::Label  shimmerDriftLabel, shimmerDirLabel;
    juce::Label  glowLabel, cloudLabel, voxLabel, realmLabel, chaosLabel;

    // ── Shimmer expand toggle ─────────────────────────────────────────────
    juce::TextButton   shimmerExpandBtn;
    bool               shimmerExpanded { true };

    // ── Shimmer voices dropdown ───────────────────────────────────────────
    juce::ComboBox shimmerVoicesBox;
    juce::Label    shimmerVoicesLabel;

    // ── Freeze / Reverse toggles + Reverse Mix knob ───────────────────────
    juce::ToggleButton freezeButton;
    juce::ToggleButton reverseButton;
    juce::Slider       reverseMixKnob;
    juce::Label        reverseMixLabel;

    // ── Preset dropdown ────────────────────────────────────────────────────
    juce::ComboBox presetBox;

    // ── APVTS attachments ──────────────────────────────────────────────────
    using SliderAttachment = juce::AudioProcessorValueTreeState::SliderAttachment;
    using ButtonAttachment = juce::AudioProcessorValueTreeState::ButtonAttachment;

    std::unique_ptr<SliderAttachment> preDelayAttach,  roomSizeAttach, decayAttach;
    std::unique_ptr<SliderAttachment> dampingAttach,   diffusionAttach, tiltEQAttach;
    std::unique_ptr<SliderAttachment> modRateAttach,   modDepthAttach,  mixAttach;
    std::unique_ptr<SliderAttachment> decayColorAttach, dreamAttach, swimAttach, modShapeAttach;
    std::unique_ptr<SliderAttachment>   shimmerAttach, shimmerPitchAttach,
                                        shimmerCharAttach, shimmerShiftHzAttach;
    std::unique_ptr<SliderAttachment> shimmerDriftAttach, shimmerDirAttach;
    std::unique_ptr<SliderAttachment> glowAttach, cloudAttach, voxAttach, realmAttach, chaosAttach;
    using ComboBoxAttachment = juce::AudioProcessorValueTreeState::ComboBoxAttachment;
    std::unique_ptr<ComboBoxAttachment> shimmerVoicesAttach;
    std::unique_ptr<ButtonAttachment>   freezeAttach;
    std::unique_ptr<ButtonAttachment>   reverseAttach;
    std::unique_ptr<SliderAttachment>   reverseMixAttach;

    // ── Psychedelic UI animation ───────────────────────────────────────────
    float psychedelicBlend { 0.0f };  // 0 = normal, 1 = full psychedelic

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (EtherealReverbEditor)
};
