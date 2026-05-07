#include "PluginEditor.h"

// ── Palette ──────────────────────────────────────────────────────────────────
namespace pal
{
    static const juce::Colour bgTop      { 0xff1c1c1f };
    static const juce::Colour bgBot      { 0xff0a0a0c };
    static const juce::Colour panel      { 0xff1f2024 };
    static const juce::Colour sidebar    { 0xff141518 };
    static const juce::Colour divider    { 0xff2a2b30 };
    static const juce::Colour accent     { 0xffe4a445 };  // warm gold
    static const juce::Colour accentDim  { 0xff5a4222 };
    static const juce::Colour accentGlow { 0xff7a4a18 };
    static const juce::Colour text       { 0xffe6e6e8 };
    static const juce::Colour textDim    { 0xff7a7d85 };

    // Amp chassis tones
    static const juce::Colour chassisHi  { 0xff2a2622 };
    static const juce::Colour chassisLo  { 0xff100c0a };
    static const juce::Colour metalHi    { 0xffb8b0a4 };
    static const juce::Colour metalMid   { 0xff7d756a };
    static const juce::Colour metalLo    { 0xff45403a };
    static const juce::Colour grilleA    { 0xff181614 };
    static const juce::Colour grilleB    { 0xff242120 };
}

// ── PremiumLnF ───────────────────────────────────────────────────────────────

PremiumLnF::PremiumLnF()
{
    setColour(juce::Slider::textBoxTextColourId, pal::accent);
    setColour(juce::Slider::textBoxBackgroundColourId, juce::Colours::transparentBlack);
    setColour(juce::Slider::textBoxOutlineColourId, juce::Colours::transparentBlack);
}

void PremiumLnF::drawRotarySlider(juce::Graphics& g, int x, int y, int w, int h,
                                   float sliderPos,
                                   float startAngle, float endAngle,
                                   juce::Slider&)
{
    const float diameter = (float)juce::jmin(w, h) - 6.0f;
    const float radius   = diameter * 0.5f;
    const float cx       = (float)x + (float)w * 0.5f;
    const float cy       = (float)y + (float)h * 0.5f - 1.0f;
    const float angle    = startAngle + sliderPos * (endAngle - startAngle);

    // Outer chrome bezel
    {
        juce::ColourGradient grad(juce::Colour(0xff5a5b60), cx, cy - radius,
                                  juce::Colour(0xff15161a), cx, cy + radius, false);
        g.setGradientFill(grad);
        g.fillEllipse(cx - radius, cy - radius, diameter, diameter);
    }

    // Inner dial face
    const float innerR = radius - 4.0f;
    {
        juce::ColourGradient grad(juce::Colour(0xff262729), cx, cy - innerR,
                                   juce::Colour(0xff0d0e10), cx, cy + innerR, false);
        g.setGradientFill(grad);
        g.fillEllipse(cx - innerR, cy - innerR, innerR * 2.0f, innerR * 2.0f);
    }

    // Track arc (dim)
    {
        juce::Path track;
        track.addCentredArc(cx, cy, radius + 1.5f, radius + 1.5f, 0.0f,
                            startAngle, endAngle, true);
        g.setColour(pal::accentDim);
        g.strokePath(track, juce::PathStrokeType(2.0f, juce::PathStrokeType::curved,
                                                  juce::PathStrokeType::rounded));
    }

    // Active arc + glow
    {
        juce::Path active;
        active.addCentredArc(cx, cy, radius + 1.5f, radius + 1.5f, 0.0f,
                             startAngle, angle, true);
        g.setColour(pal::accentGlow.withAlpha(0.45f));
        g.strokePath(active, juce::PathStrokeType(5.0f, juce::PathStrokeType::curved,
                                                    juce::PathStrokeType::rounded));
        g.setColour(pal::accent);
        g.strokePath(active, juce::PathStrokeType(2.4f, juce::PathStrokeType::curved,
                                                    juce::PathStrokeType::rounded));
    }

    // Pointer
    {
        const float r1 = innerR - 6.0f;
        const float r2 = innerR - 1.0f;
        const float sx = cx + r1 * std::sin(angle);
        const float sy = cy - r1 * std::cos(angle);
        const float ex = cx + r2 * std::sin(angle);
        const float ey = cy - r2 * std::cos(angle);
        g.setColour(pal::accent);
        g.drawLine(sx, sy, ex, ey, 2.4f);
    }

    // Centre cap with highlight
    {
        const float capR = innerR * 0.42f;
        juce::ColourGradient cap(juce::Colour(0xff2c2d31), cx, cy - capR,
                                  juce::Colour(0xff111114), cx, cy + capR, false);
        g.setGradientFill(cap);
        g.fillEllipse(cx - capR, cy - capR, capR * 2.0f, capR * 2.0f);

        g.setColour(juce::Colour(0xff44454a).withAlpha(0.6f));
        g.fillEllipse(cx - capR * 0.55f, cy - capR * 0.85f, capR, capR * 0.35f);
    }
}

// ── PresetRow ────────────────────────────────────────────────────────────────

PresetRow::PresetRow(Preset p, const juce::String& displayName, bool isAvailable)
    : preset(p), name(displayName), available(isAvailable)
{
    setMouseCursor(juce::MouseCursor::PointingHandCursor);
}

void PresetRow::paint(juce::Graphics& g)
{
    auto b = getLocalBounds().toFloat();

    if (active)
    {
        juce::ColourGradient grad(pal::accentDim.withAlpha(0.55f), 0, 0,
                                   pal::accentDim.withAlpha(0.0f),
                                   (float)getWidth(), 0, false);
        g.setGradientFill(grad);
        g.fillRect(b);
        g.setColour(pal::accent);
        g.fillRect(0.0f, 0.0f, 3.0f, (float)getHeight());
    }
    else if (hover)
    {
        g.setColour(juce::Colour(0xff222328));
        g.fillRect(b);
    }

    g.setFont(juce::Font(juce::FontOptions{}.withHeight(13.5f)
                          .withStyle(active ? "Bold" : "Regular")));
    juce::Colour col = !available ? pal::textDim
                                  : (active ? pal::accent : pal::text);
    g.setColour(col);
    g.drawText(name.toUpperCase(),
               b.reduced(14.0f, 0.0f),
               juce::Justification::centredLeft);

    if (!available)
    {
        g.setFont(juce::Font(juce::FontOptions{}.withHeight(9.5f).withStyle("Bold")));
        g.setColour(pal::textDim.withAlpha(0.7f));
        g.drawText("SOON", b.removeFromRight(50).reduced(6.0f, 0.0f),
                   juce::Justification::centredRight);
    }

    g.setColour(pal::divider);
    g.fillRect(0, getHeight() - 1, getWidth(), 1);
}

void PresetRow::mouseUp(const juce::MouseEvent&)
{
    if (onSelect) onSelect(preset);
}

// ── PedalSlot ────────────────────────────────────────────────────────────────

PedalSlot::PedalSlot(int idx) : slotIndex(idx)
{
    setMouseCursor(juce::MouseCursor::PointingHandCursor);
}

void PedalSlot::paint(juce::Graphics& g)
{
    auto b = getLocalBounds().toFloat().reduced(1.0f);

    // Recessed inner shadow
    {
        juce::ColourGradient inner(juce::Colour(0xff080809), b.getCentreX(), b.getY(),
                                    juce::Colour(0xff14151a), b.getCentreX(), b.getBottom(), false);
        g.setGradientFill(inner);
        g.fillRoundedRectangle(b, 6.0f);
    }

    // Dashed border
    juce::Path dash;
    dash.addRoundedRectangle(b.reduced(2.0f), 5.0f);
    juce::PathStrokeType ps(1.4f);
    float dashes[2] = { 4.0f, 4.0f };
    g.setColour(hover ? pal::accent.withAlpha(0.85f)
                      : pal::divider.withAlpha(0.9f));
    juce::Path dashed;
    ps.createDashedStroke(dashed, dash, dashes, 2);
    g.fillPath(dashed);

    // Slot label
    g.setFont(juce::Font(juce::FontOptions{}.withHeight(9.5f).withStyle("Bold")));
    g.setColour(pal::textDim);
    g.drawText("SLOT " + juce::String(slotIndex + 1),
               b.removeFromTop(16), juce::Justification::centred);

    g.setFont(juce::Font(juce::FontOptions{}.withHeight(13.0f).withStyle("Bold")));
    g.setColour(hover ? pal::accent : pal::textDim);
    g.drawText("EMPTY", b, juce::Justification::centred);
}

void PedalSlot::mouseUp(const juce::MouseEvent&)
{
    // Placeholder — pedal management not yet implemented
}

// ── AddButton ────────────────────────────────────────────────────────────────

void AddButton::paint(juce::Graphics& g)
{
    auto b = getLocalBounds().toFloat().reduced(2.0f);
    const float r  = juce::jmin(b.getWidth(), b.getHeight()) * 0.5f;
    const float cx = b.getCentreX();
    const float cy = b.getCentreY();

    // Outer ring
    juce::ColourGradient ring(juce::Colour(0xff5a5b60), cx, cy - r,
                                juce::Colour(0xff15161a), cx, cy + r, false);
    g.setGradientFill(ring);
    g.fillEllipse(cx - r, cy - r, r * 2, r * 2);

    // Inner face (highlighted on hover)
    const float ir = r - 3.0f;
    juce::Colour faceTop = hover ? juce::Colour(0xff44352a) : juce::Colour(0xff262729);
    juce::Colour faceBot = hover ? juce::Colour(0xff1a120a) : juce::Colour(0xff0d0e10);
    if (down) { faceTop = faceTop.darker(0.3f); faceBot = faceBot.darker(0.3f); }
    juce::ColourGradient face(faceTop, cx, cy - ir, faceBot, cx, cy + ir, false);
    g.setGradientFill(face);
    g.fillEllipse(cx - ir, cy - ir, ir * 2, ir * 2);

    // Plus sign
    const float armR = ir * 0.5f;
    const float armW = 2.6f;
    g.setColour(hover ? juce::Colours::white : pal::accent);
    g.fillRoundedRectangle(cx - armR, cy - armW * 0.5f, armR * 2, armW, armW * 0.5f);
    g.fillRoundedRectangle(cx - armW * 0.5f, cy - armR, armW, armR * 2, armW * 0.5f);
}

void AddButton::mouseUp(const juce::MouseEvent&)
{
    down = false; repaint();
    if (onClick) onClick();
}

// ── Knob ─────────────────────────────────────────────────────────────────────

UltimateMetalToneEditor::Knob::Knob(const juce::String& name, juce::LookAndFeel& lnfRef)
{
    slider.setLookAndFeel(&lnfRef);
    slider.setSliderStyle(juce::Slider::RotaryVerticalDrag);
    slider.setTextBoxStyle(juce::Slider::TextBoxBelow, false, 64, 16);
    addAndMakeVisible(slider);

    caption.setText(name.toUpperCase(), juce::dontSendNotification);
    caption.setJustificationType(juce::Justification::centred);
    caption.setFont(juce::Font(juce::FontOptions{}.withHeight(10.5f).withStyle("Bold")));
    caption.setColour(juce::Label::textColourId, juce::Colour(0xff1a1a1c));
    addAndMakeVisible(caption);
}

void UltimateMetalToneEditor::Knob::resized()
{
    auto b = getLocalBounds();
    caption.setBounds(b.removeFromBottom(14));
    slider.setBounds(b);
}

// ── Editor ───────────────────────────────────────────────────────────────────

UltimateMetalToneEditor::UltimateMetalToneEditor(UltimateMetalToneProcessor& p)
    : AudioProcessorEditor(p), processor(p),
      kGain  ("GAIN",   lnf), kGate  ("GATE",  lnf), kBass  ("BASS",   lnf),
      kMid   ("MID",    lnf), kTreble("TREBLE",lnf), kMaster("MASTER", lnf),
      aGain  (p.apvts, "gain",   kGain.slider),
      aGate  (p.apvts, "gate",   kGate.slider),
      aBass  (p.apvts, "bass",   kBass.slider),
      aMid   (p.apvts, "mid",    kMid.slider),
      aTreble(p.apvts, "treble", kTreble.slider),
      aMaster(p.apvts, "master", kMaster.slider)
{
    titleLabel.setText("ULTIMATE METAL TONE", juce::dontSendNotification);
    titleLabel.setFont(juce::Font(juce::FontOptions{}.withHeight(22.5f).withStyle("Bold")));
    titleLabel.setColour(juce::Label::textColourId, pal::text);
    titleLabel.setJustificationType(juce::Justification::centredLeft);
    addAndMakeVisible(titleLabel);

    subtitleLabel.setText("NAM AMP MODELLER  /  IR CABINET", juce::dontSendNotification);
    subtitleLabel.setFont(juce::Font(juce::FontOptions{}.withHeight(10.0f).withStyle("Bold")));
    subtitleLabel.setColour(juce::Label::textColourId, pal::accent);
    subtitleLabel.setJustificationType(juce::Justification::centredLeft);
    addAndMakeVisible(subtitleLabel);

    // File rows removed — Custom preset relies on drag-and-drop only.
    // (namLabel / irLabel / btnLoadNam / btnLoadIr kept as members but never shown)

    statusLabel.setFont(juce::Font(juce::FontOptions{}.withHeight(10.5f)));
    statusLabel.setColour(juce::Label::textColourId, pal::textDim);
    statusLabel.setJustificationType(juce::Justification::centredLeft);
    addAndMakeVisible(statusLabel);

    sidebarLabel.setText("PRESETS", juce::dontSendNotification);
    sidebarLabel.setFont(juce::Font(juce::FontOptions{}.withHeight(11.0f).withStyle("Bold")));
    sidebarLabel.setColour(juce::Label::textColourId, pal::accent);
    sidebarLabel.setJustificationType(juce::Justification::centredLeft);
    sidebarLabel.setBorderSize({ 0, 14, 0, 0 });
    addAndMakeVisible(sidebarLabel);

    pedalboardLabel.setText("PEDALBOARD", juce::dontSendNotification);
    pedalboardLabel.setFont(juce::Font(juce::FontOptions{}.withHeight(11.0f).withStyle("Bold")));
    pedalboardLabel.setColour(juce::Label::textColourId, pal::accent);
    pedalboardLabel.setJustificationType(juce::Justification::centredLeft);
    addAndMakeVisible(pedalboardLabel);

    ampLogoLabel.setText("ULTIMATE  METAL  TONE", juce::dontSendNotification);
    ampLogoLabel.setFont(juce::Font(juce::FontOptions{}.withHeight(11.0f).withStyle("Bold")));
    ampLogoLabel.setColour(juce::Label::textColourId, pal::accent);
    ampLogoLabel.setJustificationType(juce::Justification::centred);
    addAndMakeVisible(ampLogoLabel);

    using P = UltimateMetalToneProcessor;
    for (int i = 0; i < P::NumPresets; ++i)
    {
        auto pr = (P::Preset)i;
        const auto& info = P::getPresetInfo(pr);
        auto* row = new PresetRow(pr, info.displayName, info.available);
        row->onSelect = [this](P::Preset selected) { onPresetChosen(selected); };
        presetRows.add(row);
        addAndMakeVisible(row);
    }

    for (auto* k : { &kGain, &kGate, &kBass, &kMid, &kTreble, &kMaster })
        addAndMakeVisible(k);

    for (int i = 0; i < 4; ++i)
    {
        auto* slot = new PedalSlot(i);
        pedalSlots.add(slot);
        addAndMakeVisible(slot);
    }

    addPedalBtn.onClick = [] { /* TODO: open pedal picker */ };
    addAndMakeVisible(addPedalBtn);

    refreshPresetHighlights();
    updateCustomVisibility();

    setSize(900, 700);
    startTimerHz(8);
}

UltimateMetalToneEditor::~UltimateMetalToneEditor()
{
    stopTimer();
    for (auto* k : { &kGain, &kGate, &kBass, &kMid, &kTreble, &kMaster })
        k->slider.setLookAndFeel(nullptr);
}

void UltimateMetalToneEditor::onPresetChosen(UltimateMetalToneProcessor::Preset p)
{
    processor.loadPreset(p);
    refreshPresetHighlights();
    updateCustomVisibility();
}

void UltimateMetalToneEditor::refreshPresetHighlights()
{
    auto cur = processor.getCurrentPreset();
    for (int i = 0; i < presetRows.size(); ++i)
        presetRows[i]->setActive(i == (int)cur);
}

void UltimateMetalToneEditor::updateCustomVisibility()
{
    // Load buttons no longer shown anywhere — drag-drop only.
    btnLoadNam.setVisible(false);
    btnLoadIr .setVisible(false);
}

// ── Drawing helpers ─────────────────────────────────────────────────────────

void UltimateMetalToneEditor::drawScrew(juce::Graphics& g, float cx, float cy, float r)
{
    juce::ColourGradient grad(juce::Colour(0xff8a8580), cx, cy - r,
                                juce::Colour(0xff262220), cx, cy + r, false);
    g.setGradientFill(grad);
    g.fillEllipse(cx - r, cy - r, r * 2, r * 2);

    g.setColour(juce::Colour(0xff15110d));
    g.drawLine(cx - r * 0.6f, cy + r * 0.6f, cx + r * 0.6f, cy - r * 0.6f, 1.4f);

    g.setColour(juce::Colour(0xffa39e96).withAlpha(0.6f));
    g.fillEllipse(cx - r * 0.4f, cy - r * 0.7f, r * 0.5f, r * 0.25f);
}

void UltimateMetalToneEditor::drawAmpHead(juce::Graphics& g, juce::Rectangle<int> area)
{
    auto b = area.toFloat();

    // Chassis (tolex-like dark gradient)
    {
        juce::ColourGradient grad(pal::chassisHi, b.getX(), b.getY(),
                                    pal::chassisLo, b.getX(), b.getBottom(), false);
        g.setGradientFill(grad);
        g.fillRoundedRectangle(b, 8.0f);
    }

    // Outer thin highlight on top edge
    g.setColour(juce::Colour(0xff3a352e).withAlpha(0.7f));
    g.drawLine(b.getX() + 8, b.getY() + 1, b.getRight() - 8, b.getY() + 1, 1.0f);

    // Outer outline
    g.setColour(juce::Colour(0xff050404));
    g.drawRoundedRectangle(b, 8.0f, 1.2f);

    // Logo plate at top
    auto plate = b.withHeight(28.0f).reduced(b.getWidth() * 0.32f, 4.0f).translated(0, 6.0f);
    {
        juce::ColourGradient pg(pal::metalHi, plate.getCentreX(), plate.getY(),
                                  pal::metalLo, plate.getCentreX(), plate.getBottom(), false);
        g.setGradientFill(pg);
        g.fillRoundedRectangle(plate, 3.0f);
        g.setColour(juce::Colour(0xff050404));
        g.drawRoundedRectangle(plate, 3.0f, 1.0f);
    }
    ampLogoLabel.setBounds(plate.toNearestInt());

    // Brushed-metal control panel
    auto panel = juce::Rectangle<float>(
        b.getX() + 24.0f, plate.getBottom() + 12.0f,
        b.getWidth() - 48.0f, 138.0f);
    {
        juce::ColourGradient pg(pal::metalHi, panel.getCentreX(), panel.getY(),
                                  pal::metalLo, panel.getCentreX(), panel.getBottom(), false);
        g.setGradientFill(pg);
        g.fillRoundedRectangle(panel, 6.0f);

        // Subtle horizontal brushed-metal striations
        g.setColour(juce::Colour(0xff000000).withAlpha(0.05f));
        for (float y = panel.getY() + 2; y < panel.getBottom(); y += 3.0f)
            g.drawLine(panel.getX() + 4, y, panel.getRight() - 4, y, 0.5f);

        g.setColour(juce::Colour(0xff050404));
        g.drawRoundedRectangle(panel, 6.0f, 1.0f);
    }

    // Screws in the four corners of the panel
    const float scR = 4.5f;
    drawScrew(g, panel.getX() + 10,        panel.getY() + 10,        scR);
    drawScrew(g, panel.getRight() - 10,    panel.getY() + 10,        scR);
    drawScrew(g, panel.getX() + 10,        panel.getBottom() - 10,   scR);
    drawScrew(g, panel.getRight() - 10,    panel.getBottom() - 10,   scR);

    // Grille area below the panel — vintage cane mesh look
    auto grille = juce::Rectangle<float>(
        b.getX() + 14.0f, panel.getBottom() + 8.0f,
        b.getWidth() - 28.0f, b.getBottom() - panel.getBottom() - 14.0f);

    if (grille.getHeight() > 8.0f)
    {
        // Base
        juce::ColourGradient gg(pal::grilleA, grille.getCentreX(), grille.getY(),
                                 pal::grilleB, grille.getCentreX(), grille.getBottom(), false);
        g.setGradientFill(gg);
        g.fillRoundedRectangle(grille, 4.0f);

        // Cane-mesh: small dots on a diagonal grid
        g.setColour(juce::Colour(0xff35302a).withAlpha(0.55f));
        const float step = 6.0f;
        for (float y = grille.getY() + 4; y < grille.getBottom() - 2; y += step)
            for (float x = grille.getX() + 4; x < grille.getRight() - 2; x += step)
                g.fillEllipse(x + (((int)(y / step)) & 1 ? step * 0.5f : 0.0f), y, 1.6f, 1.6f);

        g.setColour(juce::Colour(0xff050404));
        g.drawRoundedRectangle(grille, 4.0f, 1.0f);
    }

    // Save panel rect for layout (so resized() positions knobs correctly)
    // We use a member but it's already cached via ampHeadArea + relative math in resized()
}

void UltimateMetalToneEditor::drawPedalboardBg(juce::Graphics& g, juce::Rectangle<int> area)
{
    auto b = area.toFloat();

    // Dark wooden planking feel — vertical gradient stripes
    {
        juce::ColourGradient bg(juce::Colour(0xff242122), b.getX(), b.getY(),
                                 juce::Colour(0xff141112), b.getX(), b.getBottom(), false);
        g.setGradientFill(bg);
        g.fillRoundedRectangle(b, 8.0f);

        g.setColour(juce::Colour(0xff000000).withAlpha(0.18f));
        for (float y = b.getY() + 6; y < b.getBottom() - 4; y += 9.0f)
            g.drawLine(b.getX() + 6, y, b.getRight() - 6, y, 0.5f);
    }

    // Subtle inner outline + outer
    g.setColour(juce::Colour(0xff332e2a).withAlpha(0.6f));
    g.drawRoundedRectangle(b.reduced(1.0f), 7.0f, 1.0f);
    g.setColour(juce::Colour(0xff050404));
    g.drawRoundedRectangle(b, 8.0f, 1.0f);

    // Corner screws
    const float scR = 4.0f;
    drawScrew(g, b.getX() + 10,     b.getY() + 10,     scR);
    drawScrew(g, b.getRight() - 10, b.getY() + 10,     scR);
    drawScrew(g, b.getX() + 10,     b.getBottom() - 10, scR);
    drawScrew(g, b.getRight() - 10, b.getBottom() - 10, scR);
}

// ── paint ───────────────────────────────────────────────────────────────────

void UltimateMetalToneEditor::paint(juce::Graphics& g)
{
    juce::ColourGradient bg(pal::bgTop, 0.0f, 0.0f,
                            pal::bgBot, 0.0f, (float)getHeight(), false);
    g.setGradientFill(bg);
    g.fillAll();

    g.setColour(pal::accent);
    g.fillRect(0, 0, getWidth(), 2);

    // Sidebar
    const int sidebarW = 200;
    auto sb = juce::Rectangle<int>(getWidth() - sidebarW, 0, sidebarW, getHeight());
    g.setColour(pal::sidebar);
    g.fillRect(sb);
    g.setColour(pal::divider);
    g.fillRect(sb.getX(), 0, 1, getHeight());

    // Amp head + pedalboard frames (must be drawn under their child components)
    if (!ampHeadArea.isEmpty())
        drawAmpHead(g, ampHeadArea);
    if (!pedalboardArea.isEmpty())
        drawPedalboardBg(g, pedalboardArea);

    // Drop highlight
    if (dragHighlight)
    {
        g.setColour(pal::accent.withAlpha(0.15f));
        g.fillRoundedRectangle(getLocalBounds().toFloat().reduced(2.f), 6.f);
        g.setColour(pal::accent);
        g.drawRoundedRectangle(getLocalBounds().toFloat().reduced(2.f), 6.f, 2.0f);
    }
}

// ── resized ─────────────────────────────────────────────────────────────────

void UltimateMetalToneEditor::resized()
{
    auto full = getLocalBounds();

    // Sidebar
    const int sidebarW = 200;
    auto sidebar = full.removeFromRight(sidebarW);
    {
        auto header = sidebar.removeFromTop(34);
        sidebarLabel.setBounds(header);
        sidebar.removeFromTop(2);
        for (auto* row : presetRows)
            row->setBounds(sidebar.removeFromTop(34));
    }

    auto main = full.reduced(16, 12);

    titleLabel.setBounds(main.removeFromTop(28));
    subtitleLabel.setBounds(main.removeFromTop(14));
    main.removeFromTop(14);

    // Status line at bottom
    auto status = main.removeFromBottom(20);
    statusLabel.setBounds(status);
    main.removeFromBottom(4);

    // Pedalboard (bottom)
    pedalboardArea = main.removeFromBottom(150);
    {
        auto pb = pedalboardArea.reduced(20, 14);
        pedalboardLabel.setBounds(pb.removeFromTop(18));
        pb.removeFromTop(4);

        const int addBtnW = 56;
        auto addArea = pb.removeFromRight(addBtnW + 6);
        addPedalBtn.setBounds(addArea.removeFromRight(addBtnW).reduced(0, 8));

        pb.removeFromRight(6);

        const int slotGap = 8;
        const int slotW   = (pb.getWidth() - 3 * slotGap) / 4;
        for (auto* slot : pedalSlots)
        {
            slot->setBounds(pb.removeFromLeft(slotW));
            pb.removeFromLeft(slotGap);
        }
    }

    main.removeFromBottom(10);

    // Amp head occupies the rest
    ampHeadArea = main;

    // Position knobs on the brushed-metal panel inside the amp head
    {
        // Mirror the panel calculation in drawAmpHead
        auto b = ampHeadArea.toFloat();
        auto plate = b.withHeight(28.0f).reduced(b.getWidth() * 0.32f, 4.0f).translated(0, 6.0f);
        auto panel = juce::Rectangle<float>(
            b.getX() + 24.0f, plate.getBottom() + 12.0f,
            b.getWidth() - 48.0f, 138.0f);

        auto panelI = panel.toNearestInt().reduced(20, 12);
        const int knobW = panelI.getWidth() / 6;
        for (auto* k : { &kGain, &kGate, &kBass, &kMid, &kTreble, &kMaster })
            k->setBounds(panelI.removeFromLeft(knobW).reduced(4, 0));
    }

    repaint();
}

// ── Timer ───────────────────────────────────────────────────────────────────

void UltimateMetalToneEditor::timerCallback()
{
    auto st = processor.getStatusText();
    if (statusLabel.getText() != st) statusLabel.setText(st, juce::dontSendNotification);

    refreshPresetHighlights();
    updateCustomVisibility();
}

// ── File chooser ────────────────────────────────────────────────────────────

void UltimateMetalToneEditor::chooseNAM()
{
    fileChooser = std::make_unique<juce::FileChooser>(
        "Select a .nam file", juce::File{}, "*.nam");

    fileChooser->launchAsync(
        juce::FileBrowserComponent::openMode | juce::FileBrowserComponent::canSelectFiles,
        [this](const juce::FileChooser& fc)
        {
            auto f = fc.getResult();
            if (f.existsAsFile()) processor.loadNAMFromFile(f);
        });
}

void UltimateMetalToneEditor::chooseIR()
{
    fileChooser = std::make_unique<juce::FileChooser>(
        "Select an IR (.wav)", juce::File{}, "*.wav;*.aif;*.aiff");

    fileChooser->launchAsync(
        juce::FileBrowserComponent::openMode | juce::FileBrowserComponent::canSelectFiles,
        [this](const juce::FileChooser& fc)
        {
            auto f = fc.getResult();
            if (f.existsAsFile()) processor.loadIRFromFile(f);
        });
}

// ── Drag & drop ─────────────────────────────────────────────────────────────

bool UltimateMetalToneEditor::isInterestedInFileDrag(const juce::StringArray& files)
{
    if (processor.getCurrentPreset() != UltimateMetalToneProcessor::Custom)
        return false;

    for (auto& f : files)
    {
        auto lc = f.toLowerCase();
        if (lc.endsWith(".nam") || lc.endsWith(".wav") ||
            lc.endsWith(".aif") || lc.endsWith(".aiff"))
            return true;
    }
    return false;
}

void UltimateMetalToneEditor::fileDragEnter(const juce::StringArray&, int, int)
{
    dragHighlight = true; repaint();
}

void UltimateMetalToneEditor::fileDragExit(const juce::StringArray&)
{
    dragHighlight = false; repaint();
}

void UltimateMetalToneEditor::filesDropped(const juce::StringArray& files, int, int)
{
    dragHighlight = false; repaint();
    for (auto& f : files)
    {
        juce::File file(f);
        auto lc = f.toLowerCase();
        if (lc.endsWith(".nam"))
            processor.loadNAMFromFile(file);
        else if (lc.endsWith(".wav") || lc.endsWith(".aif") || lc.endsWith(".aiff"))
            processor.loadIRFromFile(file);
    }
}
