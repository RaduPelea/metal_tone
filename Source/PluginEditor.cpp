#include "PluginEditor.h"

static const juce::Colour BG       { 0xff141414 };
static const juce::Colour PANEL    { 0xff262626 };
static const juce::Colour AMBER    { 0xffe8a020 };
static const juce::Colour AMBER_DIM{ 0xff7a5010 };
static const juce::Colour CREAM    { 0xffe0d8c8 };
static const juce::Colour ACTIVE   { 0xff8b1a1a };

// ─── Knob ─────────────────────────────────────────────────────────────────────

UltimateMetalToneEditor::Knob::Knob(const juce::String& name)
{
    slider.setSliderStyle(juce::Slider::RotaryVerticalDrag);
    slider.setTextBoxStyle(juce::Slider::TextBoxBelow, false, 60, 16);
    slider.setColour(juce::Slider::thumbColourId,            AMBER);
    slider.setColour(juce::Slider::rotarySliderFillColourId, AMBER);
    slider.setColour(juce::Slider::rotarySliderOutlineColourId, AMBER_DIM);
    slider.setColour(juce::Slider::textBoxTextColourId,      AMBER);
    slider.setColour(juce::Slider::textBoxBackgroundColourId, juce::Colours::transparentBlack);
    slider.setColour(juce::Slider::textBoxOutlineColourId,    juce::Colours::transparentBlack);
    addAndMakeVisible(slider);

    caption.setText(name, juce::dontSendNotification);
    caption.setJustificationType(juce::Justification::centred);
    caption.setFont(juce::Font(juce::FontOptions{}.withHeight(11.0f).withStyle("Bold")));
    caption.setColour(juce::Label::textColourId, CREAM);
    addAndMakeVisible(caption);
}

void UltimateMetalToneEditor::Knob::resized()
{
    auto b = getLocalBounds();
    caption.setBounds(b.removeFromTop(14));
    slider.setBounds(b);
}

// ─── Editor ───────────────────────────────────────────────────────────────────

UltimateMetalToneEditor::UltimateMetalToneEditor(UltimateMetalToneProcessor& p)
    : AudioProcessorEditor(p), processor(p),
      kGain  ("GAIN"),   kGate ("GATE"),  kBass  ("BASS"),
      kMid   ("MID"),    kTreble("TREBLE"), kMaster("MASTER"),
      aGain  (p.apvts, "gain",   kGain.slider),
      aGate  (p.apvts, "gate",   kGate.slider),
      aBass  (p.apvts, "bass",   kBass.slider),
      aMid   (p.apvts, "mid",    kMid.slider),
      aTreble(p.apvts, "treble", kTreble.slider),
      aMaster(p.apvts, "master", kMaster.slider)
{
    titleLabel.setText("ULTIMATE METAL TONE", juce::dontSendNotification);
    titleLabel.setFont(juce::Font(juce::FontOptions{}.withHeight(22.f).withStyle("Bold")));
    titleLabel.setJustificationType(juce::Justification::centred);
    titleLabel.setColour(juce::Label::textColourId, AMBER);
    addAndMakeVisible(titleLabel);

    auto styleFileLabel = [](juce::Label& l)
    {
        l.setColour(juce::Label::textColourId, CREAM);
        l.setColour(juce::Label::backgroundColourId, PANEL);
        l.setColour(juce::Label::outlineColourId, AMBER_DIM);
        l.setFont(juce::Font(juce::FontOptions{}.withHeight(13.f)));
        l.setJustificationType(juce::Justification::centredLeft);
        l.setBorderSize({ 4, 8, 4, 8 });
    };
    styleFileLabel(namLabel);
    styleFileLabel(irLabel);
    namLabel.setText("NAM:  (loading...)", juce::dontSendNotification);
    irLabel .setText("IR:   (loading...)", juce::dontSendNotification);
    addAndMakeVisible(namLabel);
    addAndMakeVisible(irLabel);

    auto styleButton = [](juce::TextButton& b)
    {
        b.setColour(juce::TextButton::buttonColourId, PANEL);
        b.setColour(juce::TextButton::textColourOffId, AMBER);
        b.setColour(juce::ComboBox::outlineColourId, AMBER_DIM);
    };
    styleButton(btnLoadNam);
    styleButton(btnLoadIr);
    btnLoadNam.onClick = [this] { chooseNAM(); };
    btnLoadIr .onClick = [this] { chooseIR();  };
    addAndMakeVisible(btnLoadNam);
    addAndMakeVisible(btnLoadIr);

    // Preset buttons
    auto stylePreset = [](juce::TextButton& b)
    {
        b.setColour(juce::TextButton::buttonColourId, PANEL);
        b.setColour(juce::TextButton::buttonOnColourId, ACTIVE);
        b.setColour(juce::TextButton::textColourOffId, CREAM);
        b.setColour(juce::TextButton::textColourOnId, juce::Colours::white);
        b.setColour(juce::ComboBox::outlineColourId, AMBER_DIM);
        b.setClickingTogglesState(true);
        b.setRadioGroupId(1);
    };
    stylePreset(btnPresetThrash);
    stylePreset(btnPresetGroove);
    btnPresetThrash.setToggleState(true, juce::dontSendNotification);

    btnPresetThrash.onClick = [this]
    {
        processor.loadPreset(UltimateMetalToneProcessor::ThrashPreset);
        refreshPresetButtons();
    };
    btnPresetGroove.onClick = [this]
    {
        processor.loadPreset(UltimateMetalToneProcessor::GroovePreset);
        refreshPresetButtons();
    };
    addAndMakeVisible(btnPresetThrash);
    addAndMakeVisible(btnPresetGroove);

    for (auto* k : { &kGain, &kGate, &kBass, &kMid, &kTreble, &kMaster })
        addAndMakeVisible(k);

    setSize(680, 360);
    startTimerHz(10);
}

UltimateMetalToneEditor::~UltimateMetalToneEditor() { stopTimer(); }

void UltimateMetalToneEditor::refreshPresetButtons()
{
    bool isThrash = processor.getCurrentPreset() == UltimateMetalToneProcessor::ThrashPreset;
    btnPresetThrash.setToggleState( isThrash, juce::dontSendNotification);
    btnPresetGroove.setToggleState(!isThrash, juce::dontSendNotification);
}

// ─── Layout / paint ──────────────────────────────────────────────────────────

void UltimateMetalToneEditor::paint(juce::Graphics& g)
{
    g.fillAll(BG);

    // Top accent line
    g.setColour(AMBER);
    g.fillRect(0, 0, getWidth(), 2);

    if (dragHighlight)
    {
        g.setColour(AMBER.withAlpha(0.3f));
        g.fillRoundedRectangle(getLocalBounds().toFloat().reduced(4.f), 6.f);
    }
}

void UltimateMetalToneEditor::resized()
{
    auto b = getLocalBounds().reduced(8);

    titleLabel.setBounds(b.removeFromTop(30));
    b.removeFromTop(6);

    // Preset row
    {
        auto row = b.removeFromTop(34);
        const int w = (row.getWidth() - 8) / 2;
        btnPresetThrash.setBounds(row.removeFromLeft(w));
        row.removeFromLeft(8);
        btnPresetGroove.setBounds(row.removeFromLeft(w));
        b.removeFromTop(8);
    }

    // File rows
    auto fileRow = [&](juce::Label& lbl, juce::TextButton& btn)
    {
        auto row = b.removeFromTop(26);
        btn.setBounds(row.removeFromRight(110));
        row.removeFromRight(6);
        lbl.setBounds(row);
        b.removeFromTop(4);
    };
    fileRow(namLabel, btnLoadNam);
    fileRow(irLabel,  btnLoadIr);

    b.removeFromTop(8);

    const int knobW = b.getWidth() / 6;
    for (auto* k : { &kGain, &kGate, &kBass, &kMid, &kTreble, &kMaster })
        k->setBounds(b.removeFromLeft(knobW).reduced(2));
}

// ─── Timer ───────────────────────────────────────────────────────────────────

void UltimateMetalToneEditor::timerCallback()
{
    auto nm = "NAM:  " + processor.getNAMName();
    if (namLabel.getText() != nm) namLabel.setText(nm, juce::dontSendNotification);

    auto ir = "IR:   " + processor.getIRName();
    if (irLabel.getText() != ir)  irLabel.setText(ir,  juce::dontSendNotification);

    refreshPresetButtons();
}

// ─── File chooser ────────────────────────────────────────────────────────────

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

// ─── Drag & drop ─────────────────────────────────────────────────────────────

bool UltimateMetalToneEditor::isInterestedInFileDrag(const juce::StringArray& files)
{
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
