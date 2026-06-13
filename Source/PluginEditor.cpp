#include "PluginEditor.h"

// ─────────────────────────────────────────────────────────────────────────────
//  Preset table
// ─────────────────────────────────────────────────────────────────────────────
static const std::array<Preset, 10> kPresets = {{
    //  name             preDly  rmSz   decay  damp   diff   mRate  mDep   tilt    mix    frz
    { "Default",         0.0f,  0.50f,  2.0f, 0.50f, 0.50f, 0.50f, 0.10f,  0.00f, 0.30f, false },
    { "Studio Room",    10.0f,  0.30f,  1.2f, 0.60f, 0.40f, 0.30f, 0.05f,  0.00f, 0.25f, false },
    { "Cathedral",      30.0f,  0.95f,  8.0f, 0.40f, 0.85f, 0.20f, 0.08f, -0.10f, 0.40f, false },
    { "Plate",           5.0f,  0.50f,  2.5f, 0.20f, 0.90f, 0.40f, 0.10f,  0.10f, 0.35f, false },
    { "Dark Cave",      20.0f,  0.75f,  5.0f, 0.90f, 0.60f, 0.20f, 0.05f, -0.80f, 0.45f, false },
    { "Shimmer Pad",    15.0f,  0.80f,  6.0f, 0.30f, 0.80f, 1.00f, 0.30f,  0.70f, 0.50f, false },
    { "Broken Spring",   0.0f,  0.40f,  1.5f, 0.50f, 0.20f, 6.00f, 0.80f,  0.20f, 0.40f, false },
    { "Frozen Void",    25.0f,  0.90f, 15.0f, 0.30f, 0.95f, 0.50f, 0.20f,  0.40f, 0.60f, true  },
    { "Snare Punch",    20.0f,  0.25f,  0.4f, 0.50f, 0.30f, 0.30f, 0.05f,  0.00f, 0.20f, false },
    { "Deep Space",     40.0f,  1.00f, 18.0f, 0.50f, 0.95f, 0.15f, 0.15f, -0.20f, 0.55f, false },
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
        g.setColour (juce::Colour (kAccent));
        g.strokePath (valueArc, juce::PathStrokeType (2.5f,
            juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
    }

    // ── Knob body ────────────────────────────────────────────────────────
    const float bodyR = outerR - 14.0f;

    // Drop shadow
    g.setColour (juce::Colours::black.withAlpha (0.55f));
    g.fillEllipse (cx - bodyR + 2.5f, cy - bodyR + 2.5f, bodyR * 2.0f, bodyR * 2.0f);

    // Body radial gradient (lighter upper-left, darker lower-right)
    {
        juce::ColourGradient bodyGrad (
            juce::Colour (0xff3a3a50), cx - bodyR * 0.25f, cy - bodyR * 0.3f,
            juce::Colour (0xff161626), cx + bodyR * 0.25f, cy + bodyR * 0.35f,
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
    const bool  on     = button.getToggleState();
    const auto  bounds = button.getLocalBounds().toFloat().reduced (2.5f);
    const float corner = 9.0f;

    // ── Body gradient ─────────────────────────────────────────────────────
    {
        juce::ColourGradient bodyGrad (
            on ? juce::Colour (0xffb81c1c) : juce::Colour (0xff222234),
            bounds.getX(), bounds.getY(),
            on ? juce::Colour (0xff7a0808) : juce::Colour (0xff0f0f1e),
            bounds.getX(), bounds.getBottom(), false);
        g.setGradientFill (bodyGrad);
        g.fillRoundedRectangle (bounds, corner);
    }

    // Top-edge highlight (physical raised 3-D effect)
    {
        auto topStrip = bounds.withHeight (bounds.getHeight() * 0.38f);
        juce::ColourGradient hl (
            juce::Colour (on ? 0x28ffffffu : 0x22ffffffu), topStrip.getX(), topStrip.getY(),
            juce::Colour (0x00000000u), topStrip.getX(), topStrip.getBottom(), false);
        g.setGradientFill (hl);
        g.fillRoundedRectangle (topStrip, corner);
    }

    // Outline
    g.setColour (on ? juce::Colour (kFreeze).withAlpha (0.9f)
                    : juce::Colour (shouldDrawButtonAsHighlighted ? 0xff4a4a62u : 0xff2e2e44u));
    g.drawRoundedRectangle (bounds, corner, 1.5f);

    // ── LED indicator ─────────────────────────────────────────────────────
    const float ledR  = 5.5f;
    const float ledCX = bounds.getX() + 18.0f;
    const float ledCY = bounds.getCentreY();

    if (on)
    {
        g.setColour (juce::Colour (0x38ff1a1a));
        g.fillEllipse (ledCX - ledR * 2.4f, ledCY - ledR * 2.4f, ledR * 4.8f, ledR * 4.8f);
        g.setColour (juce::Colour (0x70ff3030));
        g.fillEllipse (ledCX - ledR * 1.5f, ledCY - ledR * 1.5f, ledR * 3.0f, ledR * 3.0f);
    }

    g.setColour (on ? juce::Colour (0xffff2828) : juce::Colour (0xff262636));
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
    setSize (800, 480);

    setupKnob (preDelayKnob,  preDelayLabel,  "PRE-DELAY");
    setupKnob (roomSizeKnob,  roomSizeLabel,  "ROOM SIZE");
    setupKnob (decayKnob,     decayLabel,     "DECAY");
    setupKnob (dampingKnob,   dampingLabel,   "DAMPING");
    setupKnob (diffusionKnob, diffusionLabel, "DIFFUSION");
    setupKnob (tiltEQKnob,    tiltEQLabel,    "TILT EQ");
    setupKnob (modRateKnob,   modRateLabel,   "MOD RATE");
    setupKnob (modDepthKnob,  modDepthLabel,  "MOD DEPTH");
    setupKnob (mixKnob,       mixLabel,       "MIX");

    freezeButton.setButtonText ("  FREEZE");
    addAndMakeVisible (freezeButton);

    for (int i = 0; i < (int) kPresets.size(); ++i)
        presetBox.addItem (kPresets[(size_t) i].name, i + 1);
    presetBox.setSelectedId (1, juce::dontSendNotification);
    presetBox.setJustificationType (juce::Justification::centred);
    presetBox.onChange = [this] { applyPreset (presetBox.getSelectedId() - 1); };
    addAndMakeVisible (presetBox);

    auto& apvts = processorRef.apvts;
    preDelayAttach  = std::make_unique<SliderAttachment> (apvts, ParamID::preDelay,  preDelayKnob);
    roomSizeAttach  = std::make_unique<SliderAttachment> (apvts, ParamID::roomSize,  roomSizeKnob);
    decayAttach     = std::make_unique<SliderAttachment> (apvts, ParamID::decay,     decayKnob);
    dampingAttach   = std::make_unique<SliderAttachment> (apvts, ParamID::damping,   dampingKnob);
    diffusionAttach = std::make_unique<SliderAttachment> (apvts, ParamID::diffusion, diffusionKnob);
    tiltEQAttach    = std::make_unique<SliderAttachment> (apvts, ParamID::tiltEQ,    tiltEQKnob);
    modRateAttach   = std::make_unique<SliderAttachment> (apvts, ParamID::modRate,   modRateKnob);
    modDepthAttach  = std::make_unique<SliderAttachment> (apvts, ParamID::modDepth,  modDepthKnob);
    mixAttach       = std::make_unique<SliderAttachment> (apvts, ParamID::mix,       mixKnob);
    freezeAttach    = std::make_unique<ButtonAttachment> (apvts, ParamID::freeze,    freezeButton);

    startTimerHz (20);
}

EtherealReverbEditor::~EtherealReverbEditor()
{
    stopTimer();
    setLookAndFeel (nullptr);
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
    constexpr int lKnobCX  = lPanelX + lPanelW / 2;   // = 107
    constexpr int lKnobX   = lKnobCX - lKnobSz / 2;   // = 67

    const int leftKnobY[] = { 88, 203, 318 };
    juce::Slider* leftKnobs[]  = { &preDelayKnob,  &roomSizeKnob,  &decayKnob  };
    juce::Label*  leftLabels[] = { &preDelayLabel,  &roomSizeLabel, &decayLabel };

    for (int i = 0; i < 3; ++i)
    {
        leftKnobs[i]->setBounds  (lKnobX, leftKnobY[i], lKnobSz, lKnobSz);
        leftLabels[i]->setBounds (lPanelX, leftKnobY[i] + lKnobSz + 4, lPanelW, 14);
    }

    // ── Right panel — 3 cols × 2 rows ─────────────────────────────────────
    constexpr int rKnobSz = 62;
    constexpr int rPanelX = 590;
    constexpr int rColW   = 69;                        // 3 cols × 69 = 207

    const int rColCX[] = {
        rPanelX + rColW / 2,
        rPanelX + rColW + rColW / 2,
        rPanelX + 2 * rColW + rColW / 2
    };                                                 // { 624, 693, 762 }

    const int rightKnobY[] = { 93, 220 };
    juce::Slider* rightKnobs[2][3] = {
        { &dampingKnob,  &diffusionKnob, &tiltEQKnob },
        { &modRateKnob,  &modDepthKnob,  &mixKnob    }
    };
    juce::Label* rightLabels[2][3] = {
        { &dampingLabel,  &diffusionLabel, &tiltEQLabel },
        { &modRateLabel,  &modDepthLabel,  &mixLabel    }
    };

    for (int row = 0; row < 2; ++row)
    {
        for (int col = 0; col < 3; ++col)
        {
            const int kx = rColCX[col] - rKnobSz / 2;
            const int ky = rightKnobY[row];
            rightKnobs [row][col]->setBounds (kx, ky, rKnobSz, rKnobSz);
            rightLabels[row][col]->setBounds (rPanelX + col * rColW,
                                              ky + rKnobSz + 4, rColW, 14);
        }
    }

    // ── Freeze button — centred below the screen ──────────────────────────
    // Screen centre X = 220 + 360/2 = 400
    constexpr int freezeW = 230, freezeH = 50;
    freezeButton.setBounds (400 - freezeW / 2, 338, freezeW, freezeH);
}

void EtherealReverbEditor::paint (juce::Graphics& g)
{
    const int W = getWidth();
    const int H = getHeight();

    // ── Background gradient ───────────────────────────────────────────────
    {
        juce::ColourGradient bg (
            juce::Colour (0xff090916), 0.0f, 0.0f,
            juce::Colour (0xff040408), 0.0f, (float) H, false);
        g.setGradientFill (bg);
        g.fillAll();
    }

    // Radial vignette (darkens corners)
    {
        juce::ColourGradient vig (
            juce::Colours::transparentBlack, (float) W * 0.5f, (float) H * 0.4f,
            juce::Colours::black.withAlpha (0.40f), 0.0f, 0.0f, true);
        g.setGradientFill (vig);
        g.fillAll();
    }

    // ── Left panel background ─────────────────────────────────────────────
    {
        juce::ColourGradient pg (
            juce::Colour (0xff0f0f1e), 5.0f, 65.0f,
            juce::Colour (0xff080810), 210.0f, 65.0f, false);
        g.setGradientFill (pg);
        g.fillRoundedRectangle (5.0f, 65.0f, 205.0f, (float) H - 82.0f, 6.0f);
        g.setColour (juce::Colour (0xff1a1a30));
        g.drawRoundedRectangle (5.0f, 65.0f, 205.0f, (float) H - 82.0f, 6.0f, 1.0f);
    }

    g.setFont (juce::Font (juce::FontOptions{}.withHeight (9.5f).withStyle ("Bold")));
    g.setColour (juce::Colour (EtherealLookAndFeel::kAccent).withAlpha (0.65f));
    g.drawText ("SPACE", 5, 70, 205, 13, juce::Justification::centred, false);

    // ── Right panel background ────────────────────────────────────────────
    {
        juce::ColourGradient pg (
            juce::Colour (0xff0f0f1e), 588.0f, 65.0f,
            juce::Colour (0xff080810), 797.0f, 65.0f, false);
        g.setGradientFill (pg);
        g.fillRoundedRectangle (588.0f, 65.0f, 207.0f, (float) H - 82.0f, 6.0f);
        g.setColour (juce::Colour (0xff1a1a30));
        g.drawRoundedRectangle (588.0f, 65.0f, 207.0f, (float) H - 82.0f, 6.0f, 1.0f);
    }

    g.setFont (juce::Font (juce::FontOptions{}.withHeight (9.5f).withStyle ("Bold")));
    g.setColour (juce::Colour (EtherealLookAndFeel::kAccent).withAlpha (0.65f));
    g.drawText ("CHARACTER & MOTION", 588, 70, 207, 13, juce::Justification::centred, false);

    // ── Header divider ────────────────────────────────────────────────────
    g.setColour (juce::Colour (EtherealLookAndFeel::kAccentDim));
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

        // Clip all subsequent drawing to the letter interiors
        g.saveState();
        g.reduceClipRegion (textPath);

        // Blue-to-black gradient fill
        juce::ColourGradient titleGrad (
            juce::Colour (0xff1a52a8), 18.0f,  4.0f,
            juce::Colour (0xff000018), 18.0f, 58.0f, false);
        g.setGradientFill (titleGrad);
        g.fillRect (8, 0, 530, 62);

        // Overlapping sine wave lines (visible only inside letters)
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
            g.setColour (juce::Colour (0xff3a7ae0).withAlpha (alpha));
            g.strokePath (sinPath, juce::PathStrokeType (0.75f));
        }

        g.restoreState();

        // Subtle path outline for sharpness
        g.setColour (juce::Colour (0xff2050a0).withAlpha (0.55f));
        g.strokePath (textPath, juce::PathStrokeType (0.5f));
    }

    // ── Decay visualization screen ────────────────────────────────────────
    {
        constexpr float scX = 220.0f, scY = 70.0f, scW = 360.0f, scH = 250.0f;
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

        // Screen dark background
        g.setColour (juce::Colour (0xff030509));
        g.fillRoundedRectangle (scX, scY, scW, scH, scCorner);

        // Scanlines
        g.setColour (juce::Colour (0x07ffffff));
        for (float gy = scY + 2.0f; gy < scY + scH - 1.0f; gy += 3.0f)
            g.drawLine (scX + 2.0f, gy, scX + scW - 2.0f, gy, 1.0f);

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

        // Live decay curve — reads actual parameter values each repaint
        {
            auto& apvts = processorRef.apvts;
            const float decayTime = apvts.getRawParameterValue (ParamID::decay)->load();
            const bool  frozen    = apvts.getRawParameterValue (ParamID::freeze)->load() > 0.5f;

            const float margin = 15.0f;
            const float cW     = scW - margin * 2.0f;
            const float cH     = scH - margin * 2.0f - 8.0f;
            const float topY   = scY + margin;
            const float botY   = topY + cH;

            // X-axis spans a fixed 20-second window so the slope changes visibly
            // with the decay knob: short decay = steep drop, long decay = gentle slope.
            constexpr float kWindowSecs = 20.0f;
            const float logDecay = 3.0f * std::log (10.0f); // ln(1000) — RT60 exponent

            juce::Path curve;
            constexpr int steps = 400;

            if (frozen)
            {
                // Flat line at full amplitude — tail is sustained forever
                curve.startNewSubPath (scX + margin, topY);
                curve.lineTo (scX + margin + cW, topY);
            }
            else
            {
                for (int i = 0; i <= steps; ++i)
                {
                    const float t      = (float) i / (float) steps;
                    const float tSecs  = t * kWindowSecs;
                    const float amp    = std::exp (-logDecay * tSecs / decayTime);
                    const float px     = scX + margin + t * cW;
                    const float py     = botY - amp * cH;
                    if (i == 0) curve.startNewSubPath (px, topY);
                    else        curve.lineTo (px, py);
                }
            }

            // Freeze uses warm amber glow; normal uses cyan
            const juce::Colour curveCol = frozen ? juce::Colour (0xffffaa00)
                                                 : juce::Colour (0xff00b4ff);

            g.setColour (curveCol.withAlpha (0.07f));
            g.strokePath (curve, juce::PathStrokeType (12.0f, juce::PathStrokeType::curved));
            g.setColour (curveCol.withAlpha (0.16f));
            g.strokePath (curve, juce::PathStrokeType (5.0f,  juce::PathStrokeType::curved));
            g.setColour (curveCol.withAlpha (0.90f));
            g.strokePath (curve, juce::PathStrokeType (1.6f,  juce::PathStrokeType::curved));

            // Time axis label — shows current decay time
            g.setFont (juce::Font (juce::FontOptions{}.withHeight (9.0f).withStyle ("Bold")));
            g.setColour (juce::Colour (EtherealLookAndFeel::kTextDim));
            const juce::String decayStr = frozen ? "FROZEN"
                                                 : juce::String (decayTime, 1) + "s RT60";
            g.drawText (decayStr, (int)(scX + scW - 70.0f), (int) scY + 6, 62, 10,
                        juce::Justification::right, false);
        }

        // Screen border
        g.setColour (juce::Colour (EtherealLookAndFeel::kAccentDim));
        g.drawRoundedRectangle (scX, scY, scW, scH, scCorner, 1.5f);

        // Inner bevel highlight
        g.setColour (juce::Colour (0x1affffff));
        g.drawRoundedRectangle (scX + 1.5f, scY + 1.5f, scW - 3.0f, scH - 3.0f, scCorner - 1.0f, 0.5f);

        // Screen label
        g.setFont (juce::Font (juce::FontOptions{}.withHeight (9.0f).withStyle ("Bold")));
        g.setColour (juce::Colour (EtherealLookAndFeel::kTextDim));
        g.drawText ("DECAY VISUALIZATION", (int) scX + 8, (int) scY + 6, 200, 10,
                    juce::Justification::left, false);
    }

    // ── Version footer ────────────────────────────────────────────────────
    g.setFont (juce::Font (juce::FontOptions{}.withHeight (9.0f)));
    g.setColour (juce::Colour (EtherealLookAndFeel::kTextDim));
    g.drawText ("v0.1.0", 0, H - 16, W - 10, 14, juce::Justification::right, false);
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

    setFloat (ParamID::preDelay,  p.preDelay);
    setFloat (ParamID::roomSize,  p.roomSize);
    setFloat (ParamID::decay,     p.decay);
    setFloat (ParamID::damping,   p.damping);
    setFloat (ParamID::diffusion, p.diffusion);
    setFloat (ParamID::modRate,   p.modRate);
    setFloat (ParamID::modDepth,  p.modDepth);
    setFloat (ParamID::tiltEQ,    p.tiltEQ);
    setFloat (ParamID::mix,       p.mix);

    if (auto* param = apvts.getParameter (ParamID::freeze))
        param->setValueNotifyingHost (p.freeze ? 1.0f : 0.0f);
}
