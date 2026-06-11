#pragma once
#include <JuceHeader.h>
#include "PluginProcessor.h"

struct Preset
{
    const char* name;
    float preDelay, roomSize, decay, damping, diffusion;
    float modRate, modDepth, tiltEQ, mix;
    bool  freeze;
};

// ─────────────────────────────────────────────────────────────────────────────
//  Custom LookAndFeel — dark navy palette, cyan accent
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

    static constexpr juce::uint32 kBackground  = 0xff0a0a14;
    static constexpr juce::uint32 kPanel       = 0xff12121f;
    static constexpr juce::uint32 kAccent      = 0xff00c8ff;
    static constexpr juce::uint32 kAccentDim   = 0xff005870;
    static constexpr juce::uint32 kFreeze      = 0xffff6b6b;
    static constexpr juce::uint32 kText        = 0xffe8e8f0;
    static constexpr juce::uint32 kTextDim     = 0xff555570;
    static constexpr juce::uint32 kTrack       = 0xff1e1e30;
    static constexpr juce::uint32 kKnobBody    = 0xff1a1a2e;
};

// ─────────────────────────────────────────────────────────────────────────────
//  Editor
// ─────────────────────────────────────────────────────────────────────────────
class EtherealReverbEditor : public juce::AudioProcessorEditor
{
public:
    explicit EtherealReverbEditor (EtherealReverbProcessor&);
    ~EtherealReverbEditor() override;

    void paint   (juce::Graphics&) override;
    void resized () override;

private:
    void setupKnob  (juce::Slider&, juce::Label&, const juce::String& labelText);
    void setupGroupLabel (juce::Label&, const juce::String& text);
    void applyPreset (int index);

    EtherealReverbProcessor& processorRef;
    EtherealLookAndFeel      laf;

    // ── Knobs ──────────────────────────────────────────────────────────────
    juce::Slider preDelayKnob,  roomSizeKnob,  decayKnob;
    juce::Slider dampingKnob,   diffusionKnob, tiltEQKnob;
    juce::Slider modRateKnob,   modDepthKnob,  mixKnob;

    // ── Knob name labels ───────────────────────────────────────────────────
    juce::Label  preDelayLabel,  roomSizeLabel,  decayLabel;
    juce::Label  dampingLabel,   diffusionLabel, tiltEQLabel;
    juce::Label  modRateLabel,   modDepthLabel,  mixLabel;

    // ── Group section headers ──────────────────────────────────────────────
    juce::Label  spaceLabel, characterLabel, motionLabel;

    // ── Freeze toggle ──────────────────────────────────────────────────────
    juce::ToggleButton freezeButton;

    // ── Preset dropdown ────────────────────────────────────────────────────
    juce::ComboBox presetBox;

    // ── APVTS attachments (keep these alive for the component lifetime) ────
    using SliderAttachment = juce::AudioProcessorValueTreeState::SliderAttachment;
    using ButtonAttachment = juce::AudioProcessorValueTreeState::ButtonAttachment;

    std::unique_ptr<SliderAttachment> preDelayAttach,  roomSizeAttach, decayAttach;
    std::unique_ptr<SliderAttachment> dampingAttach,   diffusionAttach, tiltEQAttach;
    std::unique_ptr<SliderAttachment> modRateAttach,   modDepthAttach,  mixAttach;
    std::unique_ptr<ButtonAttachment> freezeAttach;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (EtherealReverbEditor)
};
