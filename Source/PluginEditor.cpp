#include "PluginEditor.h"

// ─────────────────────────────────────────────────────────────────────────────
//  Preset table
// ─────────────────────────────────────────────────────────────────────────────
static const std::array<Preset, 10> kPresets = {{
    //  name             preDly  rmSz   decay  damp   diff   mRate  mDep   tilt    mix    frz
    { "Init",            0.0f,  0.50f,  2.0f, 0.50f, 0.50f, 0.50f, 0.10f,  0.00f, 0.30f, false },
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
    const float radius  = (float) juce::jmin (width / 2, height / 2) - 5.0f;
    const float cx      = (float) x + (float) width  * 0.5f;
    const float cy      = (float) y + (float) height * 0.5f;
    const float angle   = rotaryStartAngle
                          + sliderPosProportional * (rotaryEndAngle - rotaryStartAngle);
    const float rx = cx - radius;
    const float ry = cy - radius;
    const float rw = radius * 2.0f;

    // Track arc
    juce::Path track;
    track.addArc (rx, ry, rw, rw, rotaryStartAngle, rotaryEndAngle, true);
    g.setColour (juce::Colour (kTrack));
    g.strokePath (track, juce::PathStrokeType (3.5f,
        juce::PathStrokeType::curved, juce::PathStrokeType::rounded));

    // Value arc
    juce::Path value;
    value.addArc (rx, ry, rw, rw, rotaryStartAngle, angle, true);
    g.setColour (juce::Colour (kAccent));
    g.strokePath (value, juce::PathStrokeType (3.5f,
        juce::PathStrokeType::curved, juce::PathStrokeType::rounded));

    // Knob body
    const float inner = radius * 0.58f;
    g.setColour (juce::Colour (kKnobBody));
    g.fillEllipse (cx - inner, cy - inner, inner * 2.0f, inner * 2.0f);

    // Subtle inner ring
    g.setColour (juce::Colour (kTrack));
    g.drawEllipse (cx - inner, cy - inner, inner * 2.0f, inner * 2.0f, 1.0f);

    // Pointer
    const float pLen   = inner * 0.65f;
    const float pThick = 2.0f;
    juce::Path pointer;
    pointer.addRectangle (-pThick * 0.5f, -inner + 4.0f, pThick, pLen);
    pointer.applyTransform (juce::AffineTransform::rotation (angle)
                                .translated (cx, cy));
    g.setColour (juce::Colour (kAccent));
    g.fillPath (pointer);
}

void EtherealLookAndFeel::drawToggleButton (juce::Graphics& g,
    juce::ToggleButton& button,
    bool shouldDrawButtonAsHighlighted, bool)
{
    const bool  on     = button.getToggleState();
    const auto  bounds = button.getLocalBounds().toFloat().reduced (1.0f);
    const float corner = bounds.getHeight() * 0.3f;

    g.setColour (on ? juce::Colour (kFreeze).withAlpha (0.85f)
                    : juce::Colour (kPanel));
    g.fillRoundedRectangle (bounds, corner);

    g.setColour (on ? juce::Colour (kFreeze)
                    : juce::Colour (shouldDrawButtonAsHighlighted ? kAccentDim
                                                                  : (juce::uint32) 0xff2a2a40));
    g.drawRoundedRectangle (bounds, corner, 1.5f);

    g.setFont (juce::Font (12.0f, juce::Font::bold));
    g.setColour (on ? juce::Colours::white : juce::Colour (kTextDim));
    g.drawText (button.getButtonText(), button.getLocalBounds(),
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

    // Arrow
    const float ax = (float) width - 15.0f;
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
    g.setFont (juce::Font (13.0f));
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
    setSize (700, 400);

    // ── Knobs ────────────────────────────────────────────────────────────
    setupKnob (preDelayKnob,  preDelayLabel,  "PRE-DELAY");
    setupKnob (roomSizeKnob,  roomSizeLabel,  "ROOM SIZE");
    setupKnob (decayKnob,     decayLabel,     "DECAY");
    setupKnob (dampingKnob,   dampingLabel,   "DAMPING");
    setupKnob (diffusionKnob, diffusionLabel, "DIFFUSION");
    setupKnob (tiltEQKnob,    tiltEQLabel,    "TILT EQ");
    setupKnob (modRateKnob,   modRateLabel,   "MOD RATE");
    setupKnob (modDepthKnob,  modDepthLabel,  "MOD DEPTH");
    setupKnob (mixKnob,       mixLabel,       "MIX");

    // ── Group headers ─────────────────────────────────────────────────────
    setupGroupLabel (spaceLabel,     "SPACE");
    setupGroupLabel (characterLabel, "CHARACTER");
    setupGroupLabel (motionLabel,    "MOTION");

    // ── Freeze ────────────────────────────────────────────────────────────
    freezeButton.setButtonText ("FREEZE");
    addAndMakeVisible (freezeButton);

    // ── Preset dropdown ───────────────────────────────────────────────────
    for (int i = 0; i < (int) kPresets.size(); ++i)
        presetBox.addItem (kPresets[(size_t) i].name, i + 1);
    presetBox.setSelectedId (1, juce::dontSendNotification);
    presetBox.setJustificationType (juce::Justification::centred);
    presetBox.onChange = [this] { applyPreset (presetBox.getSelectedId() - 1); };
    addAndMakeVisible (presetBox);

    // ── Attachments ───────────────────────────────────────────────────────
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
}

EtherealReverbEditor::~EtherealReverbEditor()
{
    setLookAndFeel (nullptr);
}

void EtherealReverbEditor::setupKnob (juce::Slider& knob, juce::Label& label,
                                       const juce::String& text)
{
    knob.setSliderStyle (juce::Slider::RotaryVerticalDrag);
    knob.setTextBoxStyle (juce::Slider::NoTextBox, false, 0, 0);
    addAndMakeVisible (knob);

    label.setText (text, juce::dontSendNotification);
    label.setFont (juce::Font (9.5f, juce::Font::bold));
    label.setColour (juce::Label::textColourId, juce::Colour (EtherealLookAndFeel::kTextDim));
    label.setJustificationType (juce::Justification::centred);
    addAndMakeVisible (label);
}

void EtherealReverbEditor::setupGroupLabel (juce::Label& label, const juce::String& text)
{
    label.setText (text, juce::dontSendNotification);
    label.setFont (juce::Font (10.5f, juce::Font::bold));
    label.setColour (juce::Label::textColourId, juce::Colour (EtherealLookAndFeel::kAccent));
    label.setJustificationType (juce::Justification::centred);
    addAndMakeVisible (label);
}

void EtherealReverbEditor::resized()
{
    const int W = getWidth();

    // Preset box — top right
    presetBox.setBounds (W - 220, 15, 200, 28);

    // Controls — 3 equal-width groups across the full width
    const int groupW    = W / 3;
    const int slotW     = groupW / 3;
    const int knobSz    = 68;

    const int groupLabelY = 190;
    const int knobY       = 213;
    const int nameLabelY  = knobY + knobSz + 4;
    const int freezeY     = nameLabelY + 16 + 12;

    spaceLabel.setBounds     (0,          groupLabelY, groupW, 18);
    characterLabel.setBounds (groupW,     groupLabelY, groupW, 18);
    motionLabel.setBounds    (groupW * 2, groupLabelY, groupW, 18);

    juce::Slider* knobs[]  = {
        &preDelayKnob, &roomSizeKnob, &decayKnob,
        &dampingKnob,  &diffusionKnob, &tiltEQKnob,
        &modRateKnob,  &modDepthKnob,  &mixKnob
    };
    juce::Label* labels[] = {
        &preDelayLabel, &roomSizeLabel, &decayLabel,
        &dampingLabel,  &diffusionLabel, &tiltEQLabel,
        &modRateLabel,  &modDepthLabel,  &mixLabel
    };

    for (int g = 0; g < 3; ++g)
    {
        for (int s = 0; s < 3; ++s)
        {
            const int idx   = g * 3 + s;
            const int slotX = g * groupW + s * slotW;
            const int knobX = slotX + (slotW - knobSz) / 2;

            knobs[idx]->setBounds  (knobX, knobY,      knobSz, knobSz);
            labels[idx]->setBounds (slotX, nameLabelY, slotW,  14);
        }
    }

    freezeButton.setBounds ((W - 112) / 2, freezeY, 112, 34);
}

void EtherealReverbEditor::paint (juce::Graphics& g)
{
    // Background
    g.fillAll (juce::Colour (EtherealLookAndFeel::kBackground));

    // ── Header ────────────────────────────────────────────────────────────
    g.setFont (juce::Font (20.0f, juce::Font::bold));
    g.setColour (juce::Colour (EtherealLookAndFeel::kAccent));
    g.drawText ("ETHEREAL", 18, 0, 130, 58, juce::Justification::centredLeft);
    g.setColour (juce::Colours::white.withAlpha (0.80f));
    g.drawText ("REVERB",   140, 0, 100, 58, juce::Justification::centredLeft);

    // ── Header divider ────────────────────────────────────────────────────
    g.setColour (juce::Colour (EtherealLookAndFeel::kAccentDim));
    g.drawLine (0.0f, 58.0f, (float) getWidth(), 58.0f, 1.0f);

    // ── Decay display ─────────────────────────────────────────────────────
    const float dX = 12.0f;
    const float dY = 64.0f;
    const float dW = (float) getWidth() - 24.0f;
    const float dH = 112.0f;

    g.setColour (juce::Colour (EtherealLookAndFeel::kPanel));
    g.fillRoundedRectangle (dX, dY, dW, dH, 6.0f);
    g.setColour (juce::Colour (EtherealLookAndFeel::kAccentDim));
    g.drawRoundedRectangle (dX, dY, dW, dH, 6.0f, 1.0f);

    // Grid lines
    g.setColour (juce::Colour (0xff1a1a2c));
    for (int i = 1; i < 4; ++i)
    {
        const float gx = dX + dW * (float) i / 4.0f;
        g.drawLine (gx, dY + 1.0f, gx, dY + dH - 1.0f, 1.0f);
    }
    for (int i = 1; i < 3; ++i)
    {
        const float gy = dY + dH * (float) i / 3.0f;
        g.drawLine (dX + 1.0f, gy, dX + dW - 1.0f, gy, 1.0f);
    }

    // Placeholder exponential decay curve
    const float margin = 10.0f;
    const float cW     = dW - margin * 2.0f;
    const float cH     = dH - margin * 2.0f;
    const int   steps  = 300;

    juce::Path curve;
    for (int i = 0; i <= steps; ++i)
    {
        const float t = (float) i / (float) steps;
        const float px = dX + margin + t * cW;
        const float py = dY + margin + cH * (1.0f - std::exp (-4.5f * t));

        if (i == 0)
            curve.startNewSubPath (px, dY + margin);
        else
            curve.lineTo (px, py);
    }

    g.setColour (juce::Colour (EtherealLookAndFeel::kAccent).withAlpha (0.65f));
    g.strokePath (curve, juce::PathStrokeType (1.5f));

    // Display label
    g.setFont (juce::Font (9.5f, juce::Font::bold));
    g.setColour (juce::Colour (EtherealLookAndFeel::kTextDim));
    g.drawText ("DECAY VISUALIZATION", (int) dX + 8, (int) dY + 5, 200, 12,
                juce::Justification::left, false);

    // ── Group separator lines ─────────────────────────────────────────────
    const int groupW = getWidth() / 3;
    g.setColour (juce::Colour (EtherealLookAndFeel::kAccentDim).withAlpha (0.4f));
    g.drawLine ((float) groupW,     186.0f, (float) groupW,     (float) getHeight() - 50.0f, 1.0f);
    g.drawLine ((float) groupW * 2, 186.0f, (float) groupW * 2, (float) getHeight() - 50.0f, 1.0f);

    // ── Version footer ────────────────────────────────────────────────────
    g.setFont (juce::Font (9.0f));
    g.setColour (juce::Colour (EtherealLookAndFeel::kTextDim));
    g.drawText ("v0.1.0", 0, getHeight() - 16, getWidth() - 10, 14,
                juce::Justification::right, false);
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
