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
    static const juce::Colour active     { 0xff992020 };
    static const juce::Colour knobBase1  { 0xff232427 };
    static const juce::Colour knobBase2  { 0xff0e0e10 };
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

    // ── Outer ring (chrome bezel) ───────────────────────────────────────────
    {
        juce::ColourGradient grad(juce::Colour(0xff44464a), cx, cy - radius,
                                  juce::Colour(0xff15161a), cx, cy + radius, false);
        g.setGradientFill(grad);
        g.fillEllipse(cx - radius, cy - radius, diameter, diameter);
    }

    // ── Inner dial face ─────────────────────────────────────────────────────
    const float innerR = radius - 4.0f;
    {
        juce::ColourGradient grad(pal::knobBase1, cx, cy - innerR,
                                   pal::knobBase2, cx, cy + innerR, false);
        g.setGradientFill(grad);
        g.fillEllipse(cx - innerR, cy - innerR, innerR * 2.0f, innerR * 2.0f);
    }

    // ── Track arc (dim) ─────────────────────────────────────────────────────
    {
        juce::Path track;
        track.addCentredArc(cx, cy, radius + 1.5f, radius + 1.5f, 0.0f,
                            startAngle, endAngle, true);
        g.setColour(pal::accentDim);
        g.strokePath(track, juce::PathStrokeType(2.0f, juce::PathStrokeType::curved,
                                                  juce::PathStrokeType::rounded));
    }

    // ── Active arc (bright accent + soft glow) ──────────────────────────────
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

    // ── Pointer line ────────────────────────────────────────────────────────
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

    // ── Centre cap highlight ────────────────────────────────────────────────
    {
        const float capR = innerR * 0.42f;
        juce::ColourGradient cap(juce::Colour(0xff2c2d31), cx, cy - capR,
                                  juce::Colour(0xff111114), cx, cy + capR, false);
        g.setGradientFill(cap);
        g.fillEllipse(cx - capR, cy - capR, capR * 2.0f, capR * 2.0f);

        // tiny top highlight
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

    // Background
    if (active)
    {
        juce::ColourGradient grad(pal::accentDim.withAlpha(0.55f), 0, 0,
                                   pal::accentDim.withAlpha(0.0f),
                                   (float)getWidth(), 0, false);
        g.setGradientFill(grad);
        g.fillRect(b);

        // Left accent bar
        g.setColour(pal::accent);
        g.fillRect(0.0f, 0.0f, 3.0f, (float)getHeight());
    }
    else if (hover)
    {
        g.setColour(juce::Colour(0xff222328));
        g.fillRect(b);
    }

    // Label text
    g.setFont(juce::Font(juce::FontOptions{}.withHeight(13.5f)
                          .withStyle(active ? "Bold" : "Regular")));
    juce::Colour col = !available ? pal::textDim
                                  : (active ? pal::accent : pal::text);
    g.setColour(col);
    g.drawText(name.toUpperCase(),
               b.reduced(14.0f, 0.0f),
               juce::Justification::centredLeft);

    // Right tag
    if (!available)
    {
        g.setFont(juce::Font(juce::FontOptions{}.withHeight(9.5f).withStyle("Bold")));
        g.setColour(pal::textDim.withAlpha(0.7f));
        g.drawText("SOON", b.removeFromRight(50).reduced(6.0f, 0.0f),
                   juce::Justification::centredRight);
    }

    // Bottom divider
    g.setColour(pal::divider);
    g.fillRect(0, getHeight() - 1, getWidth(), 1);
}

void PresetRow::mouseUp(const juce::MouseEvent&)
{
    if (onSelect) onSelect(preset);
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
    caption.setColour(juce::Label::textColourId, pal::textDim);
    addAndMakeVisible(caption);
}

void UltimateMetalToneEditor::Knob::resized()
{
    auto b = getLocalBounds();
    caption.setBounds(b.removeFromTop(14));
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
    // Title bar
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

    // File labels
    auto styleFileLabel = [](juce::Label& l, const juce::String& prefix)
    {
        l.setColour(juce::Label::textColourId, pal::text);
        l.setColour(juce::Label::backgroundColourId, pal::panel);
        l.setColour(juce::Label::outlineColourId, pal::divider);
        l.setFont(juce::Font(juce::FontOptions{}.withHeight(12.5f)));
        l.setJustificationType(juce::Justification::centredLeft);
        l.setBorderSize({ 4, 12, 4, 12 });
        l.setText(prefix + "  loading...", juce::dontSendNotification);
    };
    styleFileLabel(namLabel, "NAM");
    styleFileLabel(irLabel,  "IR");
    addAndMakeVisible(namLabel);
    addAndMakeVisible(irLabel);

    // Load buttons
    auto styleLoadBtn = [](juce::TextButton& b)
    {
        b.setColour(juce::TextButton::buttonColourId, pal::panel);
        b.setColour(juce::TextButton::buttonOnColourId, pal::accent);
        b.setColour(juce::TextButton::textColourOffId, pal::accent);
        b.setColour(juce::TextButton::textColourOnId, juce::Colours::black);
        b.setColour(juce::ComboBox::outlineColourId, pal::divider);
    };
    styleLoadBtn(btnLoadNam);
    styleLoadBtn(btnLoadIr);
    btnLoadNam.onClick = [this] { chooseNAM(); };
    btnLoadIr .onClick = [this] { chooseIR();  };
    addAndMakeVisible(btnLoadNam);
    addAndMakeVisible(btnLoadIr);

    // Status row at bottom
    statusLabel.setFont(juce::Font(juce::FontOptions{}.withHeight(10.5f)));
    statusLabel.setColour(juce::Label::textColourId, pal::textDim);
    statusLabel.setJustificationType(juce::Justification::centredLeft);
    addAndMakeVisible(statusLabel);

    // Sidebar header
    sidebarLabel.setText("PRESETS", juce::dontSendNotification);
    sidebarLabel.setFont(juce::Font(juce::FontOptions{}.withHeight(11.0f).withStyle("Bold")));
    sidebarLabel.setColour(juce::Label::textColourId, pal::accent);
    sidebarLabel.setJustificationType(juce::Justification::centredLeft);
    sidebarLabel.setBorderSize({ 0, 14, 0, 0 });
    addAndMakeVisible(sidebarLabel);

    // Preset rows
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

    refreshPresetHighlights();
    updateCustomVisibility();

    setSize(820, 480);
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
    const auto& info = UltimateMetalToneProcessor::getPresetInfo(p);
    if (!info.available)
    {
        // Just record the selection — the file labels show "coming soon"
        processor.loadPreset(p);
    }
    else
    {
        processor.loadPreset(p);
    }
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
    bool isCustom = processor.getCurrentPreset() == UltimateMetalToneProcessor::Custom;
    btnLoadNam.setVisible(isCustom);
    btnLoadIr .setVisible(isCustom);
}

// ── Layout / paint ──────────────────────────────────────────────────────────

void UltimateMetalToneEditor::paint(juce::Graphics& g)
{
    // Background gradient
    juce::ColourGradient bg(pal::bgTop, 0.0f, 0.0f,
                            pal::bgBot, 0.0f, (float)getHeight(), false);
    g.setGradientFill(bg);
    g.fillAll();

    // Top accent line
    g.setColour(pal::accent);
    g.fillRect(0, 0, getWidth(), 2);

    // Sidebar background
    const int sidebarW = 200;
    auto sb = juce::Rectangle<int>(getWidth() - sidebarW, 0, sidebarW, getHeight());
    g.setColour(pal::sidebar);
    g.fillRect(sb);
    g.setColour(pal::divider);
    g.fillRect(sb.getX(), 0, 1, getHeight());

    // Knob row backing panel
    {
        auto knobArea = juce::Rectangle<int>(16, getHeight() - 200,
                                              getWidth() - sidebarW - 32, 168);
        g.setColour(pal::panel.withAlpha(0.45f));
        g.fillRoundedRectangle(knobArea.toFloat(), 8.0f);
        g.setColour(pal::divider);
        g.drawRoundedRectangle(knobArea.toFloat(), 8.0f, 1.0f);
    }

    // Drag-and-drop highlight (full window border glow)
    if (dragHighlight)
    {
        g.setColour(pal::accent.withAlpha(0.18f));
        g.fillRoundedRectangle(getLocalBounds().toFloat().reduced(2.f), 6.f);
        g.setColour(pal::accent);
        g.drawRoundedRectangle(getLocalBounds().toFloat().reduced(2.f), 6.f, 2.0f);
    }
}

void UltimateMetalToneEditor::resized()
{
    auto full = getLocalBounds();

    // Sidebar
    const int sidebarW = 200;
    auto sidebar = full.removeFromRight(sidebarW);
    {
        auto header = sidebar.removeFromTop(34);
        sidebarLabel.setBounds(header);

        // Slim divider line below header
        // (drawn in paint via divider colour)
        sidebar.removeFromTop(2);

        for (auto* row : presetRows)
            row->setBounds(sidebar.removeFromTop(34));
    }

    auto main = full.reduced(16, 12);

    // Title
    titleLabel.setBounds(main.removeFromTop(28));
    subtitleLabel.setBounds(main.removeFromTop(14));
    main.removeFromTop(14);

    // File rows
    auto namRow = main.removeFromTop(28);
    btnLoadNam.setBounds(namRow.removeFromRight(96));
    namRow.removeFromRight(8);
    namLabel.setBounds(namRow);
    main.removeFromTop(6);

    auto irRow = main.removeFromTop(28);
    btnLoadIr.setBounds(irRow.removeFromRight(96));
    irRow.removeFromRight(8);
    irLabel.setBounds(irRow);
    main.removeFromTop(8);

    // Status line
    auto status = main.removeFromBottom(20);
    statusLabel.setBounds(status);

    // Knob row
    auto knobs = main.removeFromBottom(160).reduced(0, 8);
    const int knobW = knobs.getWidth() / 6;
    for (auto* k : { &kGain, &kGate, &kBass, &kMid, &kTreble, &kMaster })
        k->setBounds(knobs.removeFromLeft(knobW).reduced(4, 0));
}

// ── Timer ───────────────────────────────────────────────────────────────────

void UltimateMetalToneEditor::timerCallback()
{
    auto nm = "NAM    " + processor.getNAMName();
    if (namLabel.getText() != nm) namLabel.setText(nm, juce::dontSendNotification);

    auto ir = "IR        " + processor.getIRName();
    if (irLabel.getText() != ir)  irLabel.setText(ir,  juce::dontSendNotification);

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
    // Only accept drops when Custom preset is active
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
