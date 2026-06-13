#include "PluginEditor.h"

// ─────────────────────────────────────────────────────────────────────────────
//  Preset table
// ─────────────────────────────────────────────────────────────────────────────
static const std::array<Preset, 10> kPresets = {{
    //  name             preDly  rmSz   decay  damp   diff   mRate  mDep   tilt    mix   dColor  shimmer  shPitch shChar  shHz   shV  frz
    { "Default",         0.0f,  0.50f,  2.0f, 0.50f, 0.50f, 0.50f, 0.10f,  0.00f, 0.30f,  0.00f,  0.00f,  2.0f,  0.30f, 15.0f, 1, false },
    { "Studio Room",    10.0f,  0.30f,  1.2f, 0.60f, 0.40f, 0.30f, 0.05f,  0.00f, 0.25f, -0.20f,  0.00f,  2.0f,  0.30f, 15.0f, 1, false },
    { "Cathedral",      30.0f,  0.95f,  8.0f, 0.40f, 0.85f, 0.20f, 0.08f, -0.10f, 0.40f, -0.40f,  0.00f,  2.0f,  0.30f, 15.0f, 1, false },
    { "Plate",           5.0f,  0.50f,  2.5f, 0.20f, 0.90f, 0.40f, 0.10f,  0.10f, 0.35f,  0.30f,  0.00f,  2.0f,  0.30f, 15.0f, 1, false },
    { "Dark Cave",      20.0f,  0.75f,  5.0f, 0.90f, 0.60f, 0.20f, 0.05f, -0.80f, 0.45f, -0.70f,  0.00f,  2.0f,  0.30f, 15.0f, 1, false },
    { "Shimmer Pad",    15.0f,  0.80f,  6.0f, 0.30f, 0.80f, 1.00f, 0.30f,  0.70f, 0.50f,  0.60f,  0.65f,  2.0f,  0.50f, 15.0f, 4, false },
    { "Broken Spring",   0.0f,  0.40f,  1.5f, 0.50f, 0.20f, 6.00f, 0.80f,  0.20f, 0.40f,  0.00f,  0.00f,  2.0f,  0.30f, 15.0f, 1, false },
    { "Frozen Void",    25.0f,  0.90f, 15.0f, 0.30f, 0.95f, 0.50f, 0.20f,  0.40f, 0.60f, -0.30f,  0.40f,  2.0f,  0.60f, 20.0f, 5, true  },
    { "Snare Punch",    20.0f,  0.25f,  0.4f, 0.50f, 0.30f, 0.30f, 0.05f,  0.00f, 0.20f,  0.10f,  0.00f,  2.0f,  0.30f, 15.0f, 1, false },
    { "Deep Space",     40.0f,  1.00f, 18.0f, 0.50f, 0.95f, 0.15f, 0.15f, -0.20f, 0.55f, -0.50f,  0.30f,  1.5f,  0.20f, 10.0f, 3, false },
}};

// ─────────────────────────────────────────────────────────────────────────────
//  EtherealLookAndFeel
// ─────────────────────────────────────────────────────────────────────────────
EtherealLookAndFeel::EtherealLookAndFeel()
{
    setColour (juce::ResizableWindow::backgroundColourId,      juce::Colour (kBackground));
    setColour (juce::ComboBox::backgroundColourId,             juce::Colour (kPanel));
    setColour (juce::ComboBox::textColourId,                   juce::Colour (kText));
    setColour (juce::ComboBox::outlineColourId,                juce::Colour (kAccentDim));
    setColour (juce::ComboBox::arrowColourId,                  juce::Colour (kAccent));
    setColour (juce::PopupMenu::backgroundColourId,            juce::Colour (kPanel));
    setColour (juce::PopupMenu::textColourId,                  juce::Colour (kText));
    setColour (juce::PopupMenu::highlightedBackgroundColourId, juce::Colour (kAccentDim));
    setColour (juce::PopupMenu::highlightedTextColourId,       juce::Colour (kAccent));
}

void EtherealLookAndFeel::drawRotarySlider (juce::Graphics& g,
    int x, int y, int width, int height,
    float sliderPosProportional,
    float rotaryStartAngle, float rotaryEndAngle,
    juce::Slider&)
{
    const float cx      = (float) x + (float) width  * 0.5f;
    const float cy      = (float) y + (float) height * 0.5f;
    const float outerR  = (float) juce::jmin (width, height) * 0.5f - 1.0f;
    const float angle   = rotaryStartAngle
                        + sliderPosProportional * (rotaryEndAngle - rotaryStartAngle);

    // ── Tick marks around perimeter ──────────────────────────────────────
    for (int i = 0; i <= 10; ++i)
    {
        const float t    = (float) i / 10.0f;
        const float ta   = rotaryStartAngle + t * (rotaryEndAngle - rotaryStartAngle);
        const float sinA = std::sin (ta);
        const float cosA = std::cos (ta);

        const bool  major    = (i == 0 || i == 5 || i == 10);
        const float innerR   = major ? outerR - 6.0f : outerR - 4.0f;
        const float tickW    = major ? 1.5f : 1.0f;
        const juce::uint32 col = major ? 0xff606078u : 0xff383850u;

        g.setColour (juce::Colour (col));
        g.drawLine (cx + innerR * sinA, cy - innerR * cosA,
                    cx + outerR * sinA, cy - outerR * cosA, tickW);
    }

    // ── Value arc ────────────────────────────────────────────────────────
    {
        const float arcR = outerR - 8.0f;
        juce::Path valueArc;
        valueArc.addArc (cx - arcR, cy - arcR, arcR * 2.0f, arcR * 2.0f,
                         rotaryStartAngle, angle, true);
        const juce::Colour arcColour = juce::Colour (kAccent)
            .interpolatedWith (juce::Colour (0xff40ff40), psychedelicBlend);
        g.setColour (arcColour);
        g.strokePath (valueArc, juce::PathStrokeType (2.5f,
            juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
    }

    // ── Knob body ────────────────────────────────────────────────────────
    const float bodyR = outerR - 14.0f;

    // Drop shadow
    g.setColour (juce::Colours::black.withAlpha (0.55f));
    g.fillEllipse (cx - bodyR + 2.5f, cy - bodyR + 2.5f, bodyR * 2.0f, bodyR * 2.0f);

    // Body radial gradient — subtly redder in psychedelic mode
    {
        const juce::Colour bodyHi = juce::Colour (0xff3a3a50)
            .interpolatedWith (juce::Colour (0xff4a2828), psychedelicBlend);
        const juce::Colour bodyLo = juce::Colour (0xff161626)
            .interpolatedWith (juce::Colour (0xff200808), psychedelicBlend);
        juce::ColourGradient bodyGrad (
            bodyHi, cx - bodyR * 0.25f, cy - bodyR * 0.3f,
            bodyLo, cx + bodyR * 0.25f, cy + bodyR * 0.35f,
            true);
        g.setGradientFill (bodyGrad);
        g.fillEllipse (cx - bodyR, cy - bodyR, bodyR * 2.0f, bodyR * 2.0f);
    }

    // Specular highlight (top-left quadrant)
    {
        juce::ColourGradient specGrad (
            juce::Colour (0x38ffffff), cx - bodyR * 0.4f, cy - bodyR * 0.4f,
            juce::Colour (0x00ffffff), cx + bodyR * 0.15f, cy + bodyR * 0.15f,
            true);
        g.setGradientFill (specGrad);
        g.fillEllipse (cx - bodyR, cy - bodyR, bodyR * 2.0f, bodyR * 2.0f);
    }

    // Body ring
    g.setColour (juce::Colour (0xff28283c));
    g.drawEllipse (cx - bodyR, cy - bodyR, bodyR * 2.0f, bodyR * 2.0f, 1.5f);

    // ── White indicator pointer ───────────────────────────────────────────
    const float sinA  = std::sin (angle);
    const float cosA  = std::cos (angle);
    const float pEnd  = bodyR * 0.76f;
    const float pBase = bodyR * 0.14f;

    g.setColour (juce::Colours::white);
    g.drawLine (cx + pBase * sinA, cy - pBase * cosA,
                cx + pEnd  * sinA, cy - pEnd  * cosA, 2.2f);

    // Center cap
    g.setColour (juce::Colour (0xff080816));
    g.fillEllipse (cx - 3.0f, cy - 3.0f, 6.0f, 6.0f);
}

void EtherealLookAndFeel::drawToggleButton (juce::Graphics& g,
    juce::ToggleButton& button,
    bool shouldDrawButtonAsHighlighted, bool)
{
    const bool  on        = button.getToggleState();
    const bool  isReverse = button.getButtonText().contains ("REVERSE");
    const auto  bounds    = button.getLocalBounds().toFloat().reduced (2.5f);
    const float corner    = 9.0f;

    // On-state colours differ per button type
    const juce::Colour onHi  = isReverse ? juce::Colour (0xff0a3010) : juce::Colour (0xffb81c1c);
    const juce::Colour onLo  = isReverse ? juce::Colour (0xff051808) : juce::Colour (0xff7a0808);
    const juce::Colour onLED = isReverse ? juce::Colour (0xff28ff50) : juce::Colour (0xffff2828);
    const juce::Colour onOutline = isReverse ? juce::Colour (0xff20ff40).withAlpha (0.9f)
                                             : juce::Colour (kFreeze).withAlpha (0.9f);
    const juce::Colour onGlow1   = isReverse ? juce::Colour (0x3800ff40) : juce::Colour (0x38ff1a1a);
    const juce::Colour onGlow2   = isReverse ? juce::Colour (0x7000ff50) : juce::Colour (0x70ff3030);

    // ── Body gradient ─────────────────────────────────────────────────────
    {
        juce::ColourGradient bodyGrad (
            on ? onHi : juce::Colour (0xff222234),
            bounds.getX(), bounds.getY(),
            on ? onLo : juce::Colour (0xff0f0f1e),
            bounds.getX(), bounds.getBottom(), false);
        g.setGradientFill (bodyGrad);
        g.fillRoundedRectangle (bounds, corner);
    }

    // Top-edge highlight
    {
        auto topStrip = bounds.withHeight (bounds.getHeight() * 0.38f);
        juce::ColourGradient hl (
            juce::Colour (on ? 0x28ffffffu : 0x22ffffffu), topStrip.getX(), topStrip.getY(),
            juce::Colour (0x00000000u), topStrip.getX(), topStrip.getBottom(), false);
        g.setGradientFill (hl);
        g.fillRoundedRectangle (topStrip, corner);
    }

    // Outline
    g.setColour (on ? onOutline
                    : juce::Colour (shouldDrawButtonAsHighlighted ? 0xff4a4a62u : 0xff2e2e44u));
    g.drawRoundedRectangle (bounds, corner, 1.5f);

    // ── LED indicator ─────────────────────────────────────────────────────
    const float ledR  = 5.5f;
    const float ledCX = bounds.getX() + 18.0f;
    const float ledCY = bounds.getCentreY();

    if (on)
    {
        g.setColour (onGlow1);
        g.fillEllipse (ledCX - ledR * 2.4f, ledCY - ledR * 2.4f, ledR * 4.8f, ledR * 4.8f);
        g.setColour (onGlow2);
        g.fillEllipse (ledCX - ledR * 1.5f, ledCY - ledR * 1.5f, ledR * 3.0f, ledR * 3.0f);
    }

    g.setColour (on ? onLED : juce::Colour (0xff262636));
    g.fillEllipse (ledCX - ledR, ledCY - ledR, ledR * 2.0f, ledR * 2.0f);

    // LED specular dot
    g.setColour (juce::Colour (0x58ffffff));
    g.fillEllipse (ledCX - ledR * 0.5f, ledCY - ledR * 0.75f, ledR * 0.55f, ledR * 0.45f);

    // ── Text ──────────────────────────────────────────────────────────────
    g.setFont (juce::Font (juce::FontOptions{}.withHeight (13.0f).withStyle ("Bold")));
    g.setColour (on ? juce::Colours::white : juce::Colour (kTextDim));
    g.drawText (button.getButtonText(),
                button.getLocalBounds().withTrimmedLeft (30),
                juce::Justification::centred, false);
}

void EtherealLookAndFeel::drawButtonBackground (juce::Graphics& g, juce::Button& btn,
    const juce::Colour&, bool isHighlighted, bool)
{
    const auto b = btn.getLocalBounds().toFloat();
    // Subtle separator line across the top
    g.setColour (juce::Colour (0xff1a1a30));
    g.drawLine (b.getX() + 6.0f, b.getY(), b.getRight() - 6.0f, b.getY(), 0.75f);
    // Very faint highlight tint on hover
    if (isHighlighted)
    {
        g.setColour (juce::Colour (0xff8040ff).withAlpha (0.07f));
        g.fillRoundedRectangle (b, 3.0f);
    }
}

void EtherealLookAndFeel::drawButtonText (juce::Graphics& g, juce::TextButton& btn,
    bool, bool)
{
    g.setFont (juce::Font (juce::FontOptions{}.withHeight (8.5f).withStyle ("Bold")));
    g.setColour (juce::Colour (0xff8040ff).withAlpha (btn.isMouseOver() ? 0.85f : 0.55f));
    g.drawText (btn.getButtonText(), btn.getLocalBounds(), juce::Justification::centred, false);
}

void EtherealLookAndFeel::drawComboBox (juce::Graphics& g, int width, int height,
    bool, int, int, int, int, juce::ComboBox&)
{
    const juce::Rectangle<float> bounds (0.0f, 0.0f, (float) width, (float) height);
    g.setColour (juce::Colour (kPanel));
    g.fillRoundedRectangle (bounds, 4.0f);
    g.setColour (juce::Colour (kAccentDim));
    g.drawRoundedRectangle (bounds.reduced (0.5f), 4.0f, 1.0f);

    const float ax = (float) width - 14.0f;
    const float ay = (float) height * 0.5f - 3.0f;
    juce::Path arrow;
    arrow.addTriangle (ax - 5.0f, ay, ax + 5.0f, ay, ax, ay + 6.0f);
    g.setColour (juce::Colour (kAccent));
    g.fillPath (arrow);
}

void EtherealLookAndFeel::drawPopupMenuBackground (juce::Graphics& g, int width, int height)
{
    g.fillAll (juce::Colour (kPanel));
    g.setColour (juce::Colour (kAccentDim));
    g.drawRect (0, 0, width, height, 1);
}

void EtherealLookAndFeel::drawPopupMenuItem (juce::Graphics& g,
    const juce::Rectangle<int>& area,
    bool, bool isActive, bool isHighlighted,
    bool, bool, const juce::String& text,
    const juce::String&, const juce::Drawable*, const juce::Colour*)
{
    if (isHighlighted)
    {
        g.setColour (juce::Colour (kAccentDim));
        g.fillRect (area);
    }
    g.setFont (juce::Font (juce::FontOptions{}.withHeight (13.0f)));
    g.setColour (isActive ? juce::Colour (isHighlighted ? kAccent : kText)
                          : juce::Colour (kTextDim));
    g.drawText (text, area.reduced (10, 0), juce::Justification::centredLeft, true);
}

// ─────────────────────────────────────────────────────────────────────────────
//  EtherealReverbEditor
// ─────────────────────────────────────────────────────────────────────────────
EtherealReverbEditor::EtherealReverbEditor (EtherealReverbProcessor& p)
    : AudioProcessorEditor (&p), processorRef (p)
{
    setLookAndFeel (&laf);
    setSize (800, 570);

    setupKnob (preDelayKnob,      preDelayLabel,      "PRE-DELAY");
    setupKnob (roomSizeKnob,      roomSizeLabel,      "ROOM SIZE");
    setupKnob (decayKnob,         decayLabel,         "DECAY");
    setupKnob (dampingKnob,       dampingLabel,       "DAMPING");
    setupKnob (diffusionKnob,     diffusionLabel,     "DIFFUSION");
    setupKnob (tiltEQKnob,        tiltEQLabel,        "TILT EQ");
    setupKnob (modRateKnob,       modRateLabel,       "MOD RATE");
    setupKnob (modDepthKnob,      modDepthLabel,      "MOD DEPTH");
    setupKnob (mixKnob,           mixLabel,           "MIX");
    setupKnob (decayColorKnob,    decayColorLabel,    "DECAY COLOR");
    setupKnob (shimmerKnob,       shimmerLabel,       "SHIMMER");
    setupKnob (shimmerPitchKnob,  shimmerPitchLabel,  "PITCH");
    setupKnob (shimmerCharKnob,   shimmerCharLabel,   "CHARACTER");
    setupKnob (shimmerShiftHzKnob, shimmerShiftHzLabel, "SHIFT HZ");

    shimmerExpandBtn.setButtonText ("SHIMMER  \xe2\x96\xb6");
    shimmerExpandBtn.onClick = [this]
    {
        shimmerExpanded = !shimmerExpanded;
        shimmerExpandBtn.setButtonText (shimmerExpanded ? "SHIMMER  \xe2\x96\xbc"
                                                        : "SHIMMER  \xe2\x96\xb6");
        resized();
        repaint();
    };
    addAndMakeVisible (shimmerExpandBtn);

    shimmerVoicesBox.addItem ("1 Voice  — root only",               1);
    shimmerVoicesBox.addItem ("2 Voices — + fifth",                 2);
    shimmerVoicesBox.addItem ("3 Voices — + octave",                3);
    shimmerVoicesBox.addItem ("4 Voices — + major 10th",            4);
    shimmerVoicesBox.addItem ("5 Voices — full harmonic series",    5);
    shimmerVoicesBox.setSelectedId (1, juce::dontSendNotification);
    shimmerVoicesBox.setJustificationType (juce::Justification::centred);
    addAndMakeVisible (shimmerVoicesBox);

    shimmerVoicesLabel.setText ("VOICES", juce::dontSendNotification);
    shimmerVoicesLabel.setFont (juce::Font (juce::FontOptions{}.withHeight (9.0f).withStyle ("Bold")));
    shimmerVoicesLabel.setColour (juce::Label::textColourId, juce::Colour (EtherealLookAndFeel::kTextDim));
    shimmerVoicesLabel.setJustificationType (juce::Justification::centred);
    addAndMakeVisible (shimmerVoicesLabel);

    freezeButton.setButtonText ("  FREEZE");
    addAndMakeVisible (freezeButton);

    reverseButton.setButtonText ("  REVERSE");
    addAndMakeVisible (reverseButton);

    for (int i = 0; i < (int) kPresets.size(); ++i)
        presetBox.addItem (kPresets[(size_t) i].name, i + 1);
    presetBox.setSelectedId (1, juce::dontSendNotification);
    presetBox.setJustificationType (juce::Justification::centred);
    presetBox.onChange = [this] { applyPreset (presetBox.getSelectedId() - 1); };
    addAndMakeVisible (presetBox);

    auto& apvts = processorRef.apvts;
    preDelayAttach       = std::make_unique<SliderAttachment> (apvts, ParamID::preDelay,       preDelayKnob);
    roomSizeAttach       = std::make_unique<SliderAttachment> (apvts, ParamID::roomSize,       roomSizeKnob);
    decayAttach          = std::make_unique<SliderAttachment> (apvts, ParamID::decay,          decayKnob);
    dampingAttach        = std::make_unique<SliderAttachment> (apvts, ParamID::damping,        dampingKnob);
    diffusionAttach      = std::make_unique<SliderAttachment> (apvts, ParamID::diffusion,      diffusionKnob);
    tiltEQAttach         = std::make_unique<SliderAttachment> (apvts, ParamID::tiltEQ,         tiltEQKnob);
    modRateAttach        = std::make_unique<SliderAttachment> (apvts, ParamID::modRate,        modRateKnob);
    modDepthAttach       = std::make_unique<SliderAttachment> (apvts, ParamID::modDepth,       modDepthKnob);
    mixAttach            = std::make_unique<SliderAttachment> (apvts, ParamID::mix,            mixKnob);
    decayColorAttach     = std::make_unique<SliderAttachment> (apvts, ParamID::decayColor,     decayColorKnob);
    shimmerAttach        = std::make_unique<SliderAttachment> (apvts, ParamID::shimmer,        shimmerKnob);
    shimmerPitchAttach   = std::make_unique<SliderAttachment> (apvts, ParamID::shimmerPitch,   shimmerPitchKnob);
    shimmerCharAttach    = std::make_unique<SliderAttachment> (apvts, ParamID::shimmerChar,    shimmerCharKnob);
    shimmerShiftHzAttach = std::make_unique<SliderAttachment>   (apvts, ParamID::shimmerShiftHz, shimmerShiftHzKnob);
    shimmerVoicesAttach  = std::make_unique<ComboBoxAttachment> (apvts, ParamID::shimmerVoices,  shimmerVoicesBox);
    freezeAttach         = std::make_unique<ButtonAttachment>   (apvts, ParamID::freeze,         freezeButton);
    reverseAttach        = std::make_unique<ButtonAttachment>   (apvts, ParamID::reverse,        reverseButton);

    startTimerHz (20);
}

EtherealReverbEditor::~EtherealReverbEditor()
{
    stopTimer();
    setLookAndFeel (nullptr);
}

void EtherealReverbEditor::timerCallback()
{
    // Animate psychedelic blend toward target based on Reverse button state
    const bool revActive = processorRef.apvts
        .getRawParameterValue (ParamID::reverse)->load() > 0.5f;
    const float target = revActive ? 1.0f : 0.0f;
    const float step   = 0.08f;  // ~625ms full transition at 20Hz
    psychedelicBlend = juce::jlimit (0.0f, 1.0f,
        psychedelicBlend + (target > psychedelicBlend ? step : -step));
    laf.psychedelicBlend = psychedelicBlend;

    // Dynamic label for pre-delay knob
    preDelayLabel.setText (revActive ? "REV TIME" : "PRE-DELAY",
                           juce::dontSendNotification);

    repaint();
}

void EtherealReverbEditor::setupKnob (juce::Slider& knob, juce::Label& label,
                                       const juce::String& text)
{
    knob.setSliderStyle (juce::Slider::RotaryVerticalDrag);
    knob.setTextBoxStyle (juce::Slider::NoTextBox, false, 0, 0);
    addAndMakeVisible (knob);

    label.setText (text, juce::dontSendNotification);
    label.setFont (juce::Font (juce::FontOptions{}.withHeight (9.0f).withStyle ("Bold")));
    label.setColour (juce::Label::textColourId, juce::Colour (EtherealLookAndFeel::kTextDim));
    label.setJustificationType (juce::Justification::centred);
    addAndMakeVisible (label);
}

void EtherealReverbEditor::resized()
{
    // ── Preset box ────────────────────────────────────────────────────────
    presetBox.setBounds (570, 17, 215, 28);

    // ── Left panel — 3 knobs stacked (SPACE group) ────────────────────────
    constexpr int lKnobSz  = 80;
    constexpr int lPanelX  = 5;
    constexpr int lPanelW  = 205;
    constexpr int lKnobCX  = lPanelX + lPanelW / 2;
    constexpr int lKnobX   = lKnobCX - lKnobSz / 2;

    const int leftKnobY[] = { 88, 228, 368 };
    juce::Slider* leftKnobs[]  = { &preDelayKnob,  &roomSizeKnob,  &decayKnob  };
    juce::Label*  leftLabels[] = { &preDelayLabel,  &roomSizeLabel, &decayLabel };

    for (int i = 0; i < 3; ++i)
    {
        leftKnobs[i]->setBounds  (lKnobX, leftKnobY[i], lKnobSz, lKnobSz);
        leftLabels[i]->setBounds (lPanelX, leftKnobY[i] + lKnobSz + 4, lPanelW, 14);
    }

    // ── Right panel ───────────────────────────────────────────────────────
    constexpr int rPanelX = 590;
    constexpr int rColW   = 69;   // 3 cols × 69 = 207px total

    const int rColCX[] = {
        rPanelX + rColW / 2,
        rPanelX + rColW + rColW / 2,
        rPanelX + 2 * rColW + rColW / 2
    };

    // Row 1 — Damping, Diffusion, TiltEQ (knobSz=58)
    {
        constexpr int ky = 88, sz = 58;
        juce::Slider* knobs[]  = { &dampingKnob,  &diffusionKnob, &tiltEQKnob  };
        juce::Label*  labels[] = { &dampingLabel, &diffusionLabel, &tiltEQLabel };
        for (int col = 0; col < 3; ++col)
        {
            knobs[col]->setBounds  (rColCX[col] - sz / 2, ky, sz, sz);
            labels[col]->setBounds (rPanelX + col * rColW, ky + sz + 4, rColW, 14);
        }
    }

    // Row 2 — ModRate, ModDepth, Mix (knobSz=58)
    {
        constexpr int ky = 168, sz = 58;
        juce::Slider* knobs[]  = { &modRateKnob,  &modDepthKnob, &mixKnob  };
        juce::Label*  labels[] = { &modRateLabel, &modDepthLabel, &mixLabel };
        for (int col = 0; col < 3; ++col)
        {
            knobs[col]->setBounds  (rColCX[col] - sz / 2, ky, sz, sz);
            labels[col]->setBounds (rPanelX + col * rColW, ky + sz + 4, rColW, 14);
        }
    }

    // Row 3 — Decay Color (centred, knobSz=52)
    {
        constexpr int ky = 258, sz = 52;
        const int kcx = rPanelX + 207 / 2;
        decayColorKnob.setBounds  (kcx - sz / 2, ky, sz, sz);
        decayColorLabel.setBounds (rPanelX, ky + sz + 4, 207, 14);
    }

    // SHIMMER expand button (always visible as section header)
    shimmerExpandBtn.setBounds (rPanelX, 334, 207, 18);

    // Shimmer controls — only visible when expanded
    {
        const bool show = shimmerExpanded;

        // Row 4 — Shimmer, ShimmerPitch, ShimmerChar
        constexpr int ky4 = 358, sz4 = 50;
        juce::Slider* knobs4[]  = { &shimmerKnob,  &shimmerPitchKnob, &shimmerCharKnob  };
        juce::Label*  labels4[] = { &shimmerLabel, &shimmerPitchLabel, &shimmerCharLabel };
        for (int col = 0; col < 3; ++col)
        {
            knobs4[col]->setVisible (show);
            labels4[col]->setVisible (show);
            if (show)
            {
                knobs4[col]->setBounds  (rColCX[col] - sz4 / 2, ky4, sz4, sz4);
                labels4[col]->setBounds (rPanelX + col * rColW, ky4 + sz4 + 4, rColW, 14);
            }
        }

        // Row 5 — ShimmerShiftHz centred
        constexpr int ky5 = 428, sz5 = 44;
        shimmerShiftHzKnob.setVisible (show);
        shimmerShiftHzLabel.setVisible (show);
        if (show)
        {
            const int kcx = rPanelX + 207 / 2;
            shimmerShiftHzKnob.setBounds  (kcx - sz5 / 2, ky5, sz5, sz5);
            shimmerShiftHzLabel.setBounds (rPanelX, ky5 + sz5 + 4, 207, 14);
        }

        // Row 6 — Voices ComboBox
        shimmerVoicesBox.setVisible (show);
        shimmerVoicesLabel.setVisible (show);
        if (show)
        {
            constexpr int ky6 = 492;
            shimmerVoicesBox.setBounds   (rPanelX + 6, ky6,      195, 22);
            shimmerVoicesLabel.setBounds (rPanelX,      ky6 - 13, 207, 11);
        }
    }

    // ── Freeze + Reverse buttons — side by side below the visualization ───
    constexpr int btnW = 175, btnH = 50, btnGap = 8;
    const int btnStartX = 400 - (btnW * 2 + btnGap) / 2;
    freezeButton.setBounds  (btnStartX,                400, btnW, btnH);
    reverseButton.setBounds (btnStartX + btnW + btnGap, 400, btnW, btnH);
}

void EtherealReverbEditor::paint (juce::Graphics& g)
{
    const int W = getWidth();
    const int H = getHeight();

    // Helper: interpolate between normal and psychedelic colours
    auto blendCol = [&] (juce::uint32 normal, juce::uint32 psycho) -> juce::Colour
    {
        return juce::Colour (normal).interpolatedWith (juce::Colour (psycho), psychedelicBlend);
    };

    // ── Background gradient ───────────────────────────────────────────────
    {
        const juce::Colour bgTop = blendCol (0xff090916, 0xff1a0608);
        const juce::Colour bgBot = blendCol (0xff040408, 0xff030c02);
        juce::ColourGradient bg (bgTop, 0.0f, 0.0f, bgBot, 0.0f, (float) H, false);
        g.setGradientFill (bg);
        g.fillAll();
    }

    // Radial vignette — in psychedelic mode takes on a red/green tint
    {
        const juce::Colour vigEdge = blendCol (0x66000000, 0x55100400);
        juce::ColourGradient vig (
            juce::Colours::transparentBlack, (float) W * 0.5f, (float) H * 0.4f,
            vigEdge, 0.0f, 0.0f, true);
        g.setGradientFill (vig);
        g.fillAll();
    }

    // Psychedelic pulsing rings (only visible when blend > 0)
    if (psychedelicBlend > 0.0f)
    {
        const double now = juce::Time::getMillisecondCounterHiRes() * 0.001;
        const float cx   = (float) W * 0.5f;
        const float cy   = (float) H * 0.47f;
        for (int ring = 0; ring < 4; ++ring)
        {
            const float phase = (float) std::fmod (now * 0.25 + ring * 0.25, 1.0);
            const float r     = 60.0f + phase * (float) W * 0.65f;
            const float alpha = psychedelicBlend * (1.0f - phase) * 0.055f;
            const juce::Colour rc = (ring % 2 == 0)
                ? juce::Colour (0xffff1a30).withAlpha (alpha)
                : juce::Colour (0xff20ff50).withAlpha (alpha);
            g.setColour (rc);
            g.drawEllipse (cx - r, cy - r * 0.55f, r * 2.0f, r * 1.1f, 1.2f);
        }
    }

    // ── Left panel background ─────────────────────────────────────────────
    {
        juce::ColourGradient pg (
            blendCol (0xff0f0f1e, 0xff1a0608), 5.0f, 65.0f,
            blendCol (0xff080810, 0xff0e0404), 210.0f, 65.0f, false);
        g.setGradientFill (pg);
        g.fillRoundedRectangle (5.0f, 65.0f, 205.0f, (float) H - 82.0f, 6.0f);
        g.setColour (blendCol (0xff1a1a30, 0xff400808));
        g.drawRoundedRectangle (5.0f, 65.0f, 205.0f, (float) H - 82.0f, 6.0f, 1.0f);
    }

    g.setFont (juce::Font (juce::FontOptions{}.withHeight (9.5f).withStyle ("Bold")));
    g.setColour (blendCol (EtherealLookAndFeel::kAccent, 0xff40ff40).withAlpha (0.65f));
    g.drawText ("SPACE", 5, 70, 205, 13, juce::Justification::centred, false);

    // ── Right panel background ────────────────────────────────────────────
    {
        juce::ColourGradient pg (
            blendCol (0xff0f0f1e, 0xff1a0608), 588.0f, 65.0f,
            blendCol (0xff080810, 0xff0e0404), 797.0f, 65.0f, false);
        g.setGradientFill (pg);
        g.fillRoundedRectangle (588.0f, 65.0f, 207.0f, (float) H - 82.0f, 6.0f);
        g.setColour (blendCol (0xff1a1a30, 0xff400808));
        g.drawRoundedRectangle (588.0f, 65.0f, 207.0f, (float) H - 82.0f, 6.0f, 1.0f);
    }

    g.setFont (juce::Font (juce::FontOptions{}.withHeight (9.5f).withStyle ("Bold")));
    g.setColour (blendCol (EtherealLookAndFeel::kAccent, 0xff40ff40).withAlpha (0.65f));
    g.drawText ("CHARACTER & MOTION", 588, 70, 207, 13, juce::Justification::centred, false);

    // Right panel section sub-headers
    g.setFont (juce::Font (juce::FontOptions{}.withHeight (8.5f).withStyle ("Bold")));
    g.setColour (blendCol (EtherealLookAndFeel::kAccent, 0xff40ff40).withAlpha (0.40f));
    g.drawText ("TEXTURE",  588, 244, 207, 11, juce::Justification::centred, false);
    g.setColour (blendCol (0xff1a1a30, 0xff300808));
    g.drawLine (594.0f, 242.0f, 789.0f, 242.0f, 0.75f);

    // VOICES sub-label (only when shimmer is expanded)
    if (shimmerExpanded)
    {
        g.setColour (juce::Colour (0xff8040ff).withAlpha (0.35f));
        g.setFont (juce::Font (juce::FontOptions{}.withHeight (8.5f).withStyle ("Bold")));
        g.drawText ("VOICES", 588, 479, 207, 11, juce::Justification::centred, false);
        g.setColour (blendCol (0xff1a1a30, 0xff300808));
        g.drawLine (594.0f, 477.0f, 789.0f, 477.0f, 0.75f);
    }

    // ── Header divider ────────────────────────────────────────────────────
    g.setColour (blendCol (EtherealLookAndFeel::kAccentDim, 0xff300808));
    g.drawLine (0.0f, 62.0f, (float) W, 62.0f, 1.0f);

    // ── Title "EtherealReverb" — sine-wave gradient fill inside letters ───
    {
        juce::Font titleFont (juce::FontOptions{}.withHeight (47.0f).withStyle ("Bold"));
        juce::GlyphArrangement ga;
        ga.addFittedText (titleFont, "EtherealReverb",
                          18.0f, 4.0f, 500.0f, 54.0f,
                          juce::Justification::centredLeft, 1);

        juce::Path textPath;
        ga.createPath (textPath);

        g.saveState();
        g.reduceClipRegion (textPath);

        const juce::Colour titleTop = blendCol (0xff1a52a8, 0xffb01030);
        const juce::Colour titleBot = blendCol (0xff000018, 0xff050e02);
        juce::ColourGradient titleGrad (titleTop, 18.0f, 4.0f, titleBot, 18.0f, 58.0f, false);
        g.setGradientFill (titleGrad);
        g.fillRect (8, 0, 530, 62);

        const juce::Colour waveCol = blendCol (0xff3a7ae0, 0xffcc2040);
        for (float fy = 1.0f; fy < 62.0f; fy += 3.2f)
        {
            juce::Path sinPath;
            bool started = false;
            for (float fx = 10.0f; fx <= 535.0f; fx += 1.5f)
            {
                const float py = fy
                               + 5.5f * std::sin (fx * 0.055f)
                               + 3.0f * std::sin (fx * 0.032f + 1.35f);
                if (!started) { sinPath.startNewSubPath (fx, py); started = true; }
                else sinPath.lineTo (fx, py);
            }
            const float t     = fy / 62.0f;
            const float alpha = 0.22f + 0.16f * std::sin (t * juce::MathConstants<float>::twoPi);
            g.setColour (waveCol.withAlpha (alpha));
            g.strokePath (sinPath, juce::PathStrokeType (0.75f));
        }

        g.restoreState();

        g.setColour (blendCol (0xff2050a0, 0xff801828).withAlpha (0.55f));
        g.strokePath (textPath, juce::PathStrokeType (0.5f));

        g.setFont (juce::Font (juce::FontOptions{}.withHeight (10.5f)));
        g.setColour (juce::Colour (0xff505068));
        g.drawText ("by Eshwar", 22, 47, 200, 13, juce::Justification::centredLeft, false);
    }

    // ── Decay visualization screen ────────────────────────────────────────
    {
        constexpr float scX = 220.0f, scY = 70.0f, scW = 360.0f, scH = 270.0f;
        constexpr float scCorner = 8.0f;

        // Outer shadow
        g.setColour (juce::Colours::black.withAlpha (0.75f));
        g.fillRoundedRectangle (scX - 4.0f, scY - 4.0f, scW + 8.0f, scH + 8.0f, scCorner + 3.0f);

        // Bezel
        {
            juce::ColourGradient bevel (
                juce::Colour (0xff252538), scX, scY,
                juce::Colour (0xff161625), scX, scY + scH, false);
            g.setGradientFill (bevel);
            g.fillRoundedRectangle (scX - 2.5f, scY - 2.5f, scW + 5.0f, scH + 5.0f, scCorner + 1.5f);
        }

        g.setColour (juce::Colour (0xff030509));
        g.fillRoundedRectangle (scX, scY, scW, scH, scCorner);

        // Scanlines — tint shifts from white to red/green in psychedelic mode
        {
            const juce::Colour scanCol = blendCol (0x07ffffff, 0x08ff2020);
            g.setColour (scanCol);
            for (float gy = scY + 2.0f; gy < scY + scH - 1.0f; gy += 3.0f)
                g.drawLine (scX + 2.0f, gy, scX + scW - 2.0f, gy, 1.0f);
        }

        // Grid
        g.setColour (juce::Colour (0x14004845));
        for (int i = 1; i < 4; ++i)
        {
            float gx = scX + scW * (float) i / 4.0f;
            g.drawLine (gx, scY + 2.0f, gx, scY + scH - 2.0f, 1.0f);
        }
        for (int i = 1; i < 3; ++i)
        {
            float gy = scY + scH * (float) i / 3.0f;
            g.drawLine (scX + 2.0f, gy, scX + scW - 2.0f, gy, 1.0f);
        }

        // Live visualization
        {
            auto& apvts = processorRef.apvts;
            const float decayTime    = apvts.getRawParameterValue (ParamID::decay)      ->load();
            const float dampingVal   = apvts.getRawParameterValue (ParamID::damping)    ->load();
            const float preDelayMs   = apvts.getRawParameterValue (ParamID::preDelay)   ->load();
            const float decayColorVal= apvts.getRawParameterValue (ParamID::decayColor) ->load();
            const float shimmerVal   = apvts.getRawParameterValue (ParamID::shimmer)    ->load();
            const bool  frozen       = apvts.getRawParameterValue (ParamID::freeze)     ->load() > 0.5f;

            const float margin  = 15.0f;
            const float cW      = scW - margin * 2.0f;
            const float cH      = scH - margin * 2.0f - 8.0f;
            const float topY    = scY + margin;
            const float midY    = topY + cH * 0.5f;
            const float botY    = topY + cH;

            const float windowSecs = juce::jmin (decayTime * 1.4f + 1.0f, 60.0f);
            const float logDecay   = 3.0f * std::log (10.0f);

            auto envelope = [&] (float tSecs) -> float
            {
                if (frozen) return 1.0f;
                if (tSecs < preDelayMs * 0.001f) return 0.0f;
                const float t = tSecs - preDelayMs * 0.001f;
                return std::exp (-logDecay * t / decayTime);
            };

            // Pre-delay dead zone
            const float preDelayFrac = (preDelayMs * 0.001f) / windowSecs;
            if (preDelayFrac > 0.005f)
            {
                g.setColour (juce::Colour (0xff000000).withAlpha (0.3f));
                g.fillRect (scX + margin, topY, preDelayFrac * cW, cH);
                g.setColour (juce::Colour (0xff004040).withAlpha (0.6f));
                g.drawLine (scX + margin + preDelayFrac * cW, topY,
                            scX + margin + preDelayFrac * cW, botY, 1.0f);
            }

            // IR waveform bars
            auto hash = [] (int x) -> float
            {
                int h = x * 1234567891 + 987654321;
                h ^= (h >> 16); h *= 0x45d9f3b; h ^= (h >> 16);
                return ((float) (h & 0xffff)) / 65535.0f;
            };

            const int barStep = 2;
            for (int px = 0; px < (int) cW; px += barStep)
            {
                const float t    = (float) px / cW;
                const float amp  = envelope (t * windowSecs);
                if (amp < 0.001f) break;

                const float n1   = hash (px)        * 2.0f - 1.0f;
                const float n2   = hash (px + 7919) * 2.0f - 1.0f;
                const float barH = amp * cH * 0.44f;
                const float bx   = scX + margin + (float) px;

                const float barAlpha = frozen ? 0.30f : amp * 0.45f;
                juce::Colour barCol;
                if (frozen)
                    barCol = juce::Colour (0xffffaa00).withAlpha (barAlpha);
                else
                    barCol = blendCol (0xff00b4ff, 0xffff2040).withAlpha (barAlpha);
                g.setColour (barCol);
                g.drawLine (bx, midY, bx, midY - barH * n1, 1.0f);
                g.drawLine (bx, midY, bx, midY + barH * n2, 1.0f);
            }

            // Reverse swell curve — visible when psychedelicBlend > 0
            if (psychedelicBlend > 0.0f && !frozen)
            {
                const float revTimeMs   = apvts.getRawParameterValue (ParamID::preDelay)->load();
                const float revTimeSecs = revTimeMs * 0.001f;
                juce::Path revCurve;
                bool revStarted = false;
                for (int i = 0; i <= 400; ++i)
                {
                    const float t     = (float) i / 400.0f;
                    const float tSecs = t * windowSecs;
                    float amp;
                    if (revTimeSecs < 0.001f)
                        amp = envelope (tSecs);
                    else if (tSecs < revTimeSecs)
                        amp = tSecs / revTimeSecs;
                    else
                        amp = envelope (tSecs);
                    const float px = scX + margin + t * cW;
                    const float py = botY - amp * cH;
                    if (!revStarted) { revCurve.startNewSubPath (px, botY); revStarted = true; }
                    revCurve.lineTo (px, py);
                }
                const float ra = psychedelicBlend * 0.90f;
                g.setColour (juce::Colour (0xffff2040).withAlpha (ra * 0.08f));
                g.strokePath (revCurve, juce::PathStrokeType (14.0f, juce::PathStrokeType::curved));
                g.setColour (juce::Colour (0xff40ff60).withAlpha (ra * 0.18f));
                g.strokePath (revCurve, juce::PathStrokeType (5.0f,  juce::PathStrokeType::curved));
                g.setColour (juce::Colour (0xffff3050).withAlpha (ra));
                g.strokePath (revCurve, juce::PathStrokeType (1.6f,  juce::PathStrokeType::curved));
            }

            // Frequency-dependent decay curves
            if (!frozen)
            {
                const float hfDecay = decayTime
                                    * (1.0f - dampingVal * 0.75f)
                                    * (1.0f + decayColorVal * 0.5f);

                if (dampingVal > 0.05f || decayColorVal < -0.05f)
                {
                    juce::Path hfCurve;
                    for (int i = 0; i <= 300; ++i)
                    {
                        const float t     = (float) i / 300.0f;
                        const float tSecs = t * windowSecs;
                        if (tSecs < preDelayMs * 0.001f) continue;
                        const float tAdj  = tSecs - preDelayMs * 0.001f;
                        const float amp   = std::exp (-logDecay * tAdj / juce::jmax (0.01f, hfDecay));
                        const float px    = scX + margin + t * cW;
                        const float py    = botY - amp * cH;
                        if (hfCurve.isEmpty()) hfCurve.startNewSubPath (px, botY);
                        hfCurve.lineTo (px, py);
                    }
                    g.setColour (juce::Colour (0xffff6060).withAlpha (0.50f));
                    g.strokePath (hfCurve, juce::PathStrokeType (1.0f, juce::PathStrokeType::curved));
                }

                if (decayColorVal < -0.05f)
                {
                    const float lfDecay = decayTime * (1.0f - decayColorVal * 0.5f);
                    juce::Path lfCurve;
                    for (int i = 0; i <= 300; ++i)
                    {
                        const float t     = (float) i / 300.0f;
                        const float tSecs = t * windowSecs;
                        if (tSecs < preDelayMs * 0.001f) continue;
                        const float tAdj  = tSecs - preDelayMs * 0.001f;
                        const float amp   = std::exp (-logDecay * tAdj / lfDecay);
                        const float px    = scX + margin + t * cW;
                        const float py    = botY - amp * cH;
                        if (lfCurve.isEmpty()) lfCurve.startNewSubPath (px, botY);
                        lfCurve.lineTo (px, py);
                    }
                    g.setColour (juce::Colour (0xff60c0ff).withAlpha (0.40f));
                    g.strokePath (lfCurve, juce::PathStrokeType (1.0f, juce::PathStrokeType::curved));
                }

                // Shimmer overlay: rising harmonic echoes visualised as purple arcs
                if (shimmerVal > 0.02f)
                {
                    const float shimAlpha = shimmerVal * 0.55f;
                    for (int echo = 1; echo <= 3; ++echo)
                    {
                        juce::Path shimPath;
                        for (int i = 0; i <= 300; ++i)
                        {
                            const float t     = (float) i / 300.0f;
                            const float tSecs = t * windowSecs;
                            if (tSecs < preDelayMs * 0.001f) continue;
                            const float tAdj  = tSecs - preDelayMs * 0.001f;
                            const float amp   = std::exp (-logDecay * tAdj / decayTime)
                                              * shimmerVal
                                              * (1.0f - (float) (echo - 1) * 0.28f);
                            const float riseFrac = (float) echo * 0.12f;
                            const float riseY    = cH * riseFrac * amp;
                            const float px       = scX + margin + t * cW;
                            const float py       = botY - amp * cH - riseY;
                            if (shimPath.isEmpty()) shimPath.startNewSubPath (px, botY);
                            shimPath.lineTo (px, py);
                        }
                        g.setColour (juce::Colour (0xff8040ff).withAlpha (shimAlpha * (1.0f - (float)(echo-1) * 0.3f)));
                        g.strokePath (shimPath, juce::PathStrokeType (1.0f, juce::PathStrokeType::curved));
                    }
                }
            }

            // Main envelope curve / frozen oscilloscope
            {
                juce::Path curve;

                if (frozen)
                {
                    const double now = juce::Time::getMillisecondCounterHiRes() * 0.001;
                    const float  amp = cH * 0.38f;
                    const int    pts = 300;
                    for (int i = 0; i <= pts; ++i)
                    {
                        const float t  = (float) i / (float) pts;
                        const float px = scX + margin + t * cW;
                        const float py = midY + amp * std::sin ((double) t * 6.0 * juce::MathConstants<double>::pi
                                                                 + now * 1.8);
                        if (i == 0) curve.startNewSubPath (px, py);
                        else        curve.lineTo (px, py);
                    }
                    g.setColour (juce::Colour (0xffffaa00).withAlpha (0.10f));
                    g.strokePath (curve, juce::PathStrokeType (14.0f, juce::PathStrokeType::curved));
                    g.setColour (juce::Colour (0xffffaa00).withAlpha (0.22f));
                    g.strokePath (curve, juce::PathStrokeType (5.0f,  juce::PathStrokeType::curved));
                    g.setColour (juce::Colour (0xffffaa00).withAlpha (0.92f));
                    g.strokePath (curve, juce::PathStrokeType (1.6f,  juce::PathStrokeType::curved));
                }
                else
                {
                    bool started = false;
                    for (int i = 0; i <= 400; ++i)
                    {
                        const float t     = (float) i / 400.0f;
                        const float tSecs = t * windowSecs;
                        const float amp   = envelope (tSecs);
                        const float px    = scX + margin + t * cW;
                        const float py    = botY - amp * cH;
                        if (!started && tSecs < preDelayMs * 0.001f) continue;
                        if (!started) { curve.startNewSubPath (px, botY); started = true; }
                        curve.lineTo (px, py);
                    }
                    g.setColour (juce::Colour (0xff00b4ff).withAlpha (0.08f));
                    g.strokePath (curve, juce::PathStrokeType (14.0f, juce::PathStrokeType::curved));
                    g.setColour (juce::Colour (0xff00b4ff).withAlpha (0.18f));
                    g.strokePath (curve, juce::PathStrokeType (5.0f,  juce::PathStrokeType::curved));
                    g.setColour (juce::Colour (0xff00b4ff).withAlpha (0.92f));
                    g.strokePath (curve, juce::PathStrokeType (1.6f,  juce::PathStrokeType::curved));
                }
            }

            // Centre baseline
            g.setColour (juce::Colour (0x20ffffff));
            g.drawLine (scX + margin, midY, scX + margin + cW, midY, 0.5f);

            // Labels
            g.setFont (juce::Font (juce::FontOptions{}.withHeight (9.0f).withStyle ("Bold")));
            g.setColour (blendCol (EtherealLookAndFeel::kTextDim, 0xff206820));
            const juce::String decayStr = frozen   ? "FROZEN"
                : (psychedelicBlend > 0.5f ? juce::String ((int) preDelayMs) + "ms REV"
                                           : juce::String (decayTime, 1) + "s RT60");
            g.drawText (decayStr, (int)(scX + scW - 72.0f), (int) scY + 6, 64, 10,
                        juce::Justification::right, false);

            if (!frozen && (dampingVal > 0.05f || decayColorVal < -0.05f))
            {
                g.setColour (juce::Colour (0xffff6060).withAlpha (0.65f));
                g.drawText ("HF", (int)(scX + 8.0f), (int)(botY - 22.0f), 20, 10,
                            juce::Justification::left, false);
            }
            if (!frozen && decayColorVal < -0.05f)
            {
                g.setColour (juce::Colour (0xff60c0ff).withAlpha (0.65f));
                g.drawText ("LF", (int)(scX + 8.0f), (int)(botY - 36.0f), 20, 10,
                            juce::Justification::left, false);
            }
            if (!frozen && shimmerVal > 0.02f)
            {
                g.setColour (juce::Colour (0xff8040ff).withAlpha (0.75f));
                g.drawText ("SHIMMER", (int)(scX + 8.0f), (int)(botY - 50.0f), 55, 10,
                            juce::Justification::left, false);
            }
        }

        // Screen border
        g.setColour (blendCol (EtherealLookAndFeel::kAccentDim, 0xff203820));
        g.drawRoundedRectangle (scX, scY, scW, scH, scCorner, 1.5f);

        g.setColour (blendCol (0x1affffff, 0x1a80ff80));
        g.drawRoundedRectangle (scX + 1.5f, scY + 1.5f, scW - 3.0f, scH - 3.0f, scCorner - 1.0f, 0.5f);

        g.setFont (juce::Font (juce::FontOptions{}.withHeight (9.0f).withStyle ("Bold")));
        g.setColour (blendCol (EtherealLookAndFeel::kTextDim, 0xff206820));
        const juce::String screenTitle = psychedelicBlend > 0.5f ? "REVERSE REALM" : "DECAY VISUALIZATION";
        g.drawText (screenTitle, (int) scX + 8, (int) scY + 6, 200, 10,
                    juce::Justification::left, false);
    }

    // ── Version footer ────────────────────────────────────────────────────
    g.setFont (juce::Font (juce::FontOptions{}.withHeight (9.0f)));
    g.setColour (juce::Colour (EtherealLookAndFeel::kTextDim));
    g.drawText ("v1.1.0", 0, H - 16, W - 10, 14, juce::Justification::right, false);
}

void EtherealReverbEditor::applyPreset (int index)
{
    if (index < 0 || index >= (int) kPresets.size())
        return;

    const auto& p     = kPresets[(size_t) index];
    auto&       apvts = processorRef.apvts;

    auto setFloat = [&apvts] (const juce::String& id, float val)
    {
        if (auto* param = dynamic_cast<juce::RangedAudioParameter*> (apvts.getParameter (id)))
            param->setValueNotifyingHost (param->convertTo0to1 (val));
    };

    setFloat (ParamID::preDelay,       p.preDelay);
    setFloat (ParamID::roomSize,       p.roomSize);
    setFloat (ParamID::decay,          p.decay);
    setFloat (ParamID::damping,        p.damping);
    setFloat (ParamID::diffusion,      p.diffusion);
    setFloat (ParamID::modRate,        p.modRate);
    setFloat (ParamID::modDepth,       p.modDepth);
    setFloat (ParamID::tiltEQ,         p.tiltEQ);
    setFloat (ParamID::mix,            p.mix);
    setFloat (ParamID::decayColor,     p.decayColor);
    setFloat (ParamID::shimmer,        p.shimmer);
    setFloat (ParamID::shimmerPitch,   p.shimmerPitch);
    setFloat (ParamID::shimmerChar,    p.shimmerChar);
    setFloat (ParamID::shimmerShiftHz, p.shimmerShiftHz);

    // shimmerVoices is AudioParameterChoice: index 0-4, preset stores count 1-5
    if (auto* param = apvts.getParameter (ParamID::shimmerVoices))
        param->setValueNotifyingHost (param->convertTo0to1 ((float)(p.shimmerVoices - 1)));

    if (auto* param = apvts.getParameter (ParamID::freeze))
        param->setValueNotifyingHost (p.freeze ? 1.0f : 0.0f);
}
