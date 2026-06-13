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
    const auto  b      = btn.getLocalBounds().toFloat();
    const bool  isWide = btn.getWidth() > 400;

    if (isWide)
    {
        // Full-width section header — solid panel background with top/bottom borders
        juce::ColourGradient hdr (
            juce::Colour (0xff14142a), b.getX(), b.getY(),
            juce::Colour (0xff0c0c1a), b.getX(), b.getBottom(), false);
        g.setGradientFill (hdr);
        g.fillRect (b);
        g.setColour (juce::Colour (0xff252540));
        g.drawLine (b.getX(), b.getY(), b.getRight(), b.getY(), 1.0f);
        g.setColour (juce::Colour (0xff1a1a30));
        g.drawLine (b.getX(), b.getBottom() - 0.5f, b.getRight(), b.getBottom() - 0.5f, 0.5f);
        if (isHighlighted)
        {
            g.setColour (juce::Colour (0xff8040ff).withAlpha (0.05f));
            g.fillRect (b);
        }
    }
    else
    {
        g.setColour (juce::Colour (0xff1a1a30));
        g.drawLine (b.getX() + 6.0f, b.getY(), b.getRight() - 6.0f, b.getY(), 0.75f);
        if (isHighlighted)
        {
            g.setColour (juce::Colour (0xff8040ff).withAlpha (0.07f));
            g.fillRoundedRectangle (b, 3.0f);
        }
    }
}

void EtherealLookAndFeel::drawButtonText (juce::Graphics& g, juce::TextButton& btn,
    bool, bool)
{
    const bool isWide = btn.getWidth() > 400;
    g.setFont (juce::Font (juce::FontOptions{}.withHeight (isWide ? 9.5f : 8.5f).withStyle ("Bold")));
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
    presetBox.setBounds (548, 14, 234, 28);

    // ─────────────────────────────────────────────────────────────────────
    //  SPACE strip  (y=58..153)  — all 6 primary reverb knobs in one row
    // ─────────────────────────────────────────────────────────────────────
    constexpr int spaceY  = 62;
    constexpr int spaceKz = 64;
    constexpr int spaceColW = 800 / 6;   // 133px per column

    {
        juce::Slider* knobs[]  = { &preDelayKnob, &roomSizeKnob, &decayKnob,
                                    &dampingKnob, &diffusionKnob, &tiltEQKnob };
        juce::Label*  labels[] = { &preDelayLabel, &roomSizeLabel, &decayLabel,
                                    &dampingLabel, &diffusionLabel, &tiltEQLabel };
        for (int i = 0; i < 6; ++i)
        {
            const int cx = spaceColW * i + spaceColW / 2;
            knobs[i]->setBounds  (cx - spaceKz / 2, spaceY + 8, spaceKz, spaceKz);
            labels[i]->setBounds (spaceColW * i,    spaceY + 8 + spaceKz + 3, spaceColW, 13);
        }
    }

    // ─────────────────────────────────────────────────────────────────────
    //  CENTER section  (y=158..378)
    //  Left:  Visualization screen  (x=8, w=400)
    //  Right: MOTION panel          (x=416, w=376)
    // ─────────────────────────────────────────────────────────────────────
    constexpr int motionX  = 416;
    constexpr int motionW  = 376;
    constexpr int motionY  = 163;
    constexpr int mKnobSz  = 54;
    constexpr int mColW    = motionW / 3;   // ~125px

    // Row 1 — ModRate, ModDepth, Mix
    {
        juce::Slider* knobs[]  = { &modRateKnob,  &modDepthKnob, &mixKnob  };
        juce::Label*  labels[] = { &modRateLabel, &modDepthLabel, &mixLabel };
        for (int col = 0; col < 3; ++col)
        {
            const int cx = motionX + mColW * col + mColW / 2;
            knobs[col]->setBounds  (cx - mKnobSz / 2, motionY, mKnobSz, mKnobSz);
            labels[col]->setBounds (motionX + mColW * col, motionY + mKnobSz + 3, mColW, 13);
        }
    }

    // Row 2 — DecayColor (centred in motion panel)
    {
        constexpr int ky = motionY + mKnobSz + 24, sz = 50;
        const int cx = motionX + motionW / 2;
        decayColorKnob.setBounds  (cx - sz / 2, ky, sz, sz);
        decayColorLabel.setBounds (motionX, ky + sz + 3, motionW, 13);
    }

    // ─────────────────────────────────────────────────────────────────────
    //  SHIMMER section header + controls  (y=382+)
    // ─────────────────────────────────────────────────────────────────────
    constexpr int shimHeaderY = 382;
    shimmerExpandBtn.setBounds (0, shimHeaderY, 800, 20);

    {
        const bool show       = shimmerExpanded;
        constexpr int ky      = shimHeaderY + 26;
        constexpr int sz      = 48;
        constexpr int shimColW = 800 / 5;   // 160px per column (4 knobs + voices)

        juce::Slider* knobs[]  = { &shimmerKnob, &shimmerPitchKnob,
                                    &shimmerCharKnob, &shimmerShiftHzKnob };
        juce::Label*  labels[] = { &shimmerLabel, &shimmerPitchLabel,
                                    &shimmerCharLabel, &shimmerShiftHzLabel };
        for (int i = 0; i < 4; ++i)
        {
            knobs[i]->setVisible (show);
            labels[i]->setVisible (show);
            if (show)
            {
                const int cx = shimColW * i + shimColW / 2;
                knobs[i]->setBounds  (cx - sz / 2, ky, sz, sz);
                labels[i]->setBounds (shimColW * i, ky + sz + 3, shimColW, 13);
            }
        }

        shimmerVoicesBox.setVisible (show);
        shimmerVoicesLabel.setVisible (show);
        if (show)
        {
            const int col4cx = shimColW * 4 + shimColW / 2;
            shimmerVoicesLabel.setBounds (shimColW * 4, ky, shimColW, 13);
            shimmerVoicesBox.setBounds   (col4cx - 72, ky + 17, 144, 22);
        }
    }

    // ── Freeze + Reverse buttons ──────────────────────────────────────────
    constexpr int btnW = 175, btnH = 46, btnGap = 8;
    const int btnY      = shimmerExpanded ? 476 : 412;
    const int btnStartX = 400 - (btnW * 2 + btnGap) / 2;
    freezeButton.setBounds  (btnStartX,                btnY, btnW, btnH);
    reverseButton.setBounds (btnStartX + btnW + btnGap, btnY, btnW, btnH);
}

void EtherealReverbEditor::paint (juce::Graphics& g)
{
    const int   W  = getWidth();
    const int   H  = getHeight();
    const float fW = (float) W;
    const float fH = (float) H;

    // ── Colour helpers ────────────────────────────────────────────────────
    // blendCol: interpolate between normal and psychedelic palette colours
    auto blendCol = [&] (juce::uint32 normal, juce::uint32 psycho) -> juce::Colour
    {
        return juce::Colour (normal).interpolatedWith (juce::Colour (psycho), psychedelicBlend);
    };
    // psychoAt: red (top) → green (bottom) gradient at a given y fraction
    auto psychoAt = [&] (float yFrac) -> juce::Colour
    {
        return juce::Colour (0xff2a0808).interpolatedWith (juce::Colour (0xff082a08), yFrac);
    };

    // ── Background — full crimson→green gradient in psycho mode ──────────
    {
        const juce::Colour bgTop = juce::Colour (0xff090916)
            .interpolatedWith (psychoAt (0.0f), psychedelicBlend);
        const juce::Colour bgBot = juce::Colour (0xff040408)
            .interpolatedWith (psychoAt (1.0f), psychedelicBlend);
        juce::ColourGradient bg (bgTop, 0.0f, 0.0f, bgBot, 0.0f, fH, false);
        g.setGradientFill (bg);
        g.fillAll();
    }

    // Radial vignette
    {
        const juce::Colour vigEdge = blendCol (0x66000000, 0x44000800);
        juce::ColourGradient vig (juce::Colours::transparentBlack,
                                  fW * 0.5f, fH * 0.42f,
                                  vigEdge, 0.0f, 0.0f, true);
        g.setGradientFill (vig);
        g.fillAll();
    }

    // Psychedelic pulsing rings
    if (psychedelicBlend > 0.0f)
    {
        const double now = juce::Time::getMillisecondCounterHiRes() * 0.001;
        const float  cx  = fW * 0.5f, cy = fH * 0.47f;
        for (int ring = 0; ring < 4; ++ring)
        {
            const float phase = (float) std::fmod (now * 0.22 + ring * 0.25, 1.0);
            const float r     = 55.0f + phase * fW * 0.70f;
            const float alpha = psychedelicBlend * (1.0f - phase) * 0.07f;
            g.setColour ((ring % 2 == 0 ? juce::Colour (0xffff1a30)
                                        : juce::Colour (0xff20ff50)).withAlpha (alpha));
            g.drawEllipse (cx - r, cy - r * 0.55f, r * 2.0f, r * 1.1f, 1.3f);
        }
    }

    // ── Header divider ────────────────────────────────────────────────────
    g.setColour (blendCol (EtherealLookAndFeel::kAccentDim, 0xff3a0808));
    g.drawLine (0.0f, 57.0f, fW, 57.0f, 1.0f);

    // ── Rivet helper — small hardware screw dot ───────────────────────────
    auto drawRivet = [&] (float rx, float ry)
    {
        g.setColour (blendCol (0xff1c1c30, 0xff2a1010));
        g.fillEllipse (rx - 3.5f, ry - 3.5f, 7.0f, 7.0f);
        g.setColour (blendCol (0xff303048, 0xff401818));
        g.drawEllipse (rx - 3.5f, ry - 3.5f, 7.0f, 7.0f, 0.8f);
        g.setColour (juce::Colour (0x22ffffff));
        g.fillEllipse (rx - 2.2f, ry - 3.0f, 2.5f, 2.0f);
    };

    // ── SPACE strip  (y=58..153) ──────────────────────────────────────────
    {
        const float sY = 58.0f, sH = 95.0f;
        const juce::Colour stripNorm  = juce::Colour (0xff0e0e1e);
        const juce::Colour stripPsyco = psychoAt (sY / fH).darker (0.25f);
        juce::ColourGradient sg (
            stripNorm.interpolatedWith (stripPsyco, psychedelicBlend), 0.0f, sY,
            stripNorm.darker (0.1f).interpolatedWith (stripPsyco.darker (0.1f), psychedelicBlend),
            0.0f, sY + sH, false);
        g.setGradientFill (sg);
        g.fillRect (0.0f, sY, fW, sH);

        // Top edge highlight (raised panel feel)
        g.setColour (juce::Colour (0x18ffffff));
        g.drawLine (0.0f, sY, fW, sY, 1.0f);
        // Bottom shadow
        g.setColour (juce::Colour (0x28000000));
        g.drawLine (0.0f, sY + sH, fW, sY + sH, 1.5f);

        // Section label
        g.setFont (juce::Font (juce::FontOptions{}.withHeight (9.0f).withStyle ("Bold")));
        g.setColour (blendCol (EtherealLookAndFeel::kAccent, 0xff40ff40).withAlpha (0.55f));
        g.drawText ("SPACE", 0, (int) sY + 1, W, 10, juce::Justification::centred, false);

        // Corner rivets
        drawRivet (10.0f, sY + sH * 0.5f);
        drawRivet (fW - 10.0f, sY + sH * 0.5f);
    }

    // ── CENTER section  (y=158..380) — divider between space and center ───
    {
        g.setColour (blendCol (0xff1a1a30, 0xff350808));
        g.drawLine (0.0f, 157.0f, fW, 157.0f, 1.0f);
    }

    // MOTION panel background (right of visualization screen)
    {
        const float mpX = 414.0f, mpY = 158.0f, mpW = fW - mpX, mpH = 222.0f;
        const juce::Colour mpNorm  = juce::Colour (0xff0c0c1a);
        const juce::Colour mpPsyco = psychoAt (0.5f).darker (0.3f);
        juce::ColourGradient mg (
            mpNorm.interpolatedWith (mpPsyco, psychedelicBlend), mpX, mpY,
            mpNorm.darker (0.08f).interpolatedWith (mpPsyco.darker (0.08f), psychedelicBlend),
            mpX + mpW, mpY, false);
        g.setGradientFill (mg);
        g.fillRect (mpX, mpY, mpW, mpH);

        // Left separator
        g.setColour (blendCol (0xff1a1a30, 0xff350808));
        g.drawLine (mpX, mpY, mpX, mpY + mpH, 1.0f);

        // MOTION label
        g.setFont (juce::Font (juce::FontOptions{}.withHeight (9.0f).withStyle ("Bold")));
        g.setColour (blendCol (EtherealLookAndFeel::kAccent, 0xff40ff40).withAlpha (0.55f));
        g.drawText ("MOTION", (int) mpX, (int) mpY + 2, (int) mpW, 10,
                    juce::Justification::centred, false);

        drawRivet (mpX + 8.0f,  mpY + mpH * 0.5f);
        drawRivet (fW - 8.0f,   mpY + mpH * 0.5f);
    }

    // ── Title "EtherealReverb" — sine-wave gradient fill inside letters ───
    {
        juce::Font titleFont (juce::FontOptions{}.withHeight (44.0f).withStyle ("Bold"));
        juce::GlyphArrangement ga;
        ga.addFittedText (titleFont, "EtherealReverb",
                          16.0f, 4.0f, 520.0f, 52.0f,
                          juce::Justification::centredLeft, 1);

        juce::Path textPath;
        ga.createPath (textPath);

        g.saveState();
        g.reduceClipRegion (textPath);

        const juce::Colour titleTop = blendCol (0xff1a52a8, 0xffb01030);
        const juce::Colour titleBot = blendCol (0xff000018, 0xff050e02);
        juce::ColourGradient titleGrad (titleTop, 18.0f, 4.0f, titleBot, 18.0f, 58.0f, false);
        g.setGradientFill (titleGrad);
        g.fillRect (6, 0, 540, 57);

        const juce::Colour waveCol = blendCol (0xff3a7ae0, 0xffcc2040);
        for (float fy = 1.0f; fy < 57.0f; fy += 3.2f)
        {
            juce::Path sinPath;
            bool started = false;
            for (float fx = 8.0f; fx <= 548.0f; fx += 1.5f)
            {
                const float py = fy
                               + 5.5f * std::sin (fx * 0.055f)
                               + 3.0f * std::sin (fx * 0.032f + 1.35f);
                if (!started) { sinPath.startNewSubPath (fx, py); started = true; }
                else sinPath.lineTo (fx, py);
            }
            const float t     = fy / 57.0f;
            const float alpha = 0.22f + 0.16f * std::sin (t * juce::MathConstants<float>::twoPi);
            g.setColour (waveCol.withAlpha (alpha));
            g.strokePath (sinPath, juce::PathStrokeType (0.75f));
        }

        g.restoreState();

        g.setColour (blendCol (0xff2050a0, 0xff801828).withAlpha (0.55f));
        g.strokePath (textPath, juce::PathStrokeType (0.5f));

        g.setFont (juce::Font (juce::FontOptions{}.withHeight (10.5f)));
        g.setColour (juce::Colour (0xff505068));
        g.drawText ("by Eshwar", 20, 43, 200, 13, juce::Justification::centredLeft, false);
    }

    // ── Decay visualization screen ────────────────────────────────────────
    {
        constexpr float scX = 10.0f, scY = 160.0f, scW = 396.0f, scH = 215.0f;
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
