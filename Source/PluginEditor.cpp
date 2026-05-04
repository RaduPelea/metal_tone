#include "PluginEditor.h"

// ── Colours ───────────────────────────────────────────────────────────────────
static const juce::Colour BG_DARK   { 0xff1a1a1a };
static const juce::Colour BG_PANEL  { 0xff252525 };
static const juce::Colour AMBER     { 0xffe8a020 };
static const juce::Colour AMBER_DIM { 0xff7a5010 };
static const juce::Colour CREAM     { 0xffe0d8c8 };
static const juce::Colour RED_DARK  { 0xff6b0000 };

// ── MetallicaLookAndFeel ─────────────────────────────────────────────────────

MetallicaLookAndFeel::MetallicaLookAndFeel()
{
    setColour(juce::Slider::thumbColourId,            AMBER);
    setColour(juce::Slider::rotarySliderFillColourId, AMBER);
    setColour(juce::Slider::rotarySliderOutlineColourId, AMBER_DIM);
    setColour(juce::Label::textColourId, CREAM);
}

void MetallicaLookAndFeel::drawRotarySlider(
    juce::Graphics& g, int x, int y, int w, int h,
    float sliderPos, float startAngle, float endAngle, juce::Slider&)
{
    const float radius = (float)std::min(w, h) / 2.0f - 4.0f;
    const float cx = (float)x + (float)w / 2.0f;
    const float cy = (float)y + (float)h / 2.0f;

    // Background disc
    g.setColour(BG_PANEL);
    g.fillEllipse(cx - radius, cy - radius, radius * 2, radius * 2);

    // Arc track
    juce::Path track;
    track.addCentredArc(cx, cy, radius - 3, radius - 3,
                        0.0f, startAngle, endAngle, true);
    g.setColour(AMBER_DIM);
    g.strokePath(track, juce::PathStrokeType(3.0f));

    // Filled arc
    juce::Path fill;
    fill.addCentredArc(cx, cy, radius - 3, radius - 3,
                       0.0f, startAngle, startAngle + (endAngle - startAngle) * sliderPos, true);
    g.setColour(AMBER);
    g.strokePath(fill, juce::PathStrokeType(3.0f));

    // Pointer
    const float angle = startAngle + sliderPos * (endAngle - startAngle);
    const float px = cx + (radius - 6) * std::sin(angle);
    const float py = cy - (radius - 6) * std::cos(angle);
    g.setColour(CREAM);
    g.fillEllipse(px - 3.5f, py - 3.5f, 7.0f, 7.0f);

    // Tick marks at 0, 5, 10
    for (int t = 0; t <= 10; t += 5)
    {
        float ta = startAngle + ((float)t / 10.0f) * (endAngle - startAngle);
        float ox = cx + (radius + 2) * std::sin(ta);
        float oy = cy - (radius + 2) * std::cos(ta);
        g.setColour(t == 5 ? AMBER : AMBER_DIM);
        g.fillEllipse(ox - 2.0f, oy - 2.0f, 4.0f, 4.0f);
    }
}

void MetallicaLookAndFeel::drawLabel(juce::Graphics& g, juce::Label& label)
{
    g.setColour(label.findColour(juce::Label::textColourId));
    g.setFont(label.getFont());
    g.drawFittedText(label.getText(), label.getLocalBounds(), label.getJustificationType(), 1);
}

// ── AmpKnob ───────────────────────────────────────────────────────────────────

AmpKnob::AmpKnob(const juce::String& name)
{
    slider.setSliderStyle(juce::Slider::RotaryVerticalDrag);
    slider.setRange(0.0, 10.0, 0.01);
    slider.setTextBoxStyle(juce::Slider::NoTextBox, false, 0, 0);
    addAndMakeVisible(slider);

    nameLabel.setText(name, juce::dontSendNotification);
    nameLabel.setJustificationType(juce::Justification::centred);
    nameLabel.setFont(juce::Font(juce::FontOptions{}.withHeight(11.0f).withStyle("Bold")));
    nameLabel.setColour(juce::Label::textColourId, CREAM);
    addAndMakeVisible(nameLabel);

    valueLabel.setJustificationType(juce::Justification::centred);
    valueLabel.setFont(juce::Font(juce::FontOptions{}.withHeight(13.0f).withStyle("Bold")));
    valueLabel.setColour(juce::Label::textColourId, AMBER);
    addAndMakeVisible(valueLabel);

    slider.onValueChange = [this]
    {
        valueLabel.setText(juce::String(slider.getValue(), 1), juce::dontSendNotification);
    };
    valueLabel.setText(juce::String(slider.getValue(), 1), juce::dontSendNotification);
}

void AmpKnob::resized()
{
    auto b = getLocalBounds();
    nameLabel.setBounds(b.removeFromTop(16));
    valueLabel.setBounds(b.removeFromBottom(18));
    slider.setBounds(b);
}

// ── MetallicaToneEditor ───────────────────────────────────────────────────────

MetallicaToneEditor::MetallicaToneEditor(MetallicaToneProcessor& p)
    : AudioProcessorEditor(p), processor(p),
      kGain("GAIN"), kBass("BASS"), kMid("MID"),
      kTreble("TREBLE"), kMaster("MASTER"), kCabMix("CAB MIX"),
      attGain(p.apvts, "gain",   kGain.slider),
      attBass(p.apvts, "bass",   kBass.slider),
      attMid   (p.apvts, "mid",       kMid.slider),
      attTreble(p.apvts, "treble",    kTreble.slider),
      attMaster(p.apvts, "master",    kMaster.slider),
      attCabMix(p.apvts, "cabMix",    kCabMix.slider)
{
    setLookAndFeel(&laf);

    for (auto* k : { &kGain, &kBass, &kMid, &kTreble, &kMaster, &kCabMix })
        addAndMakeVisible(k);

    // Title
    titleLabel.setText("METALLICA TONE", juce::dontSendNotification);
    titleLabel.setFont(juce::Font(juce::FontOptions{}.withHeight(22.0f).withStyle("Bold")));
    titleLabel.setColour(juce::Label::textColourId, AMBER);
    titleLabel.setJustificationType(juce::Justification::centred);
    addAndMakeVisible(titleLabel);

    // Preset subtitle
    presetLabel.setText("THRASH PRESET  |  Mesa Boogie IIC+  |  4x12 Rectifier sm57",
                        juce::dontSendNotification);
    presetLabel.setFont(juce::Font(juce::FontOptions{}.withHeight(11.0f)));
    presetLabel.setColour(juce::Label::textColourId, CREAM);
    presetLabel.setJustificationType(juce::Justification::centred);
    addAndMakeVisible(presetLabel);

    setSize(680, 280);
}

MetallicaToneEditor::~MetallicaToneEditor()
{
    setLookAndFeel(nullptr);
}

void MetallicaToneEditor::paint(juce::Graphics& g)
{
    g.fillAll(BG_DARK);

    // Panel behind knobs
    g.setColour(BG_PANEL);
    g.fillRoundedRectangle(getLocalBounds().reduced(8).toFloat().withTrimmedTop(54), 8.0f);

    // Thin amber top border line
    g.setColour(AMBER);
    g.fillRect(0, 0, getWidth(), 2);

    // Separator lines between knobs
    g.setColour(AMBER_DIM);
    const int knobW = (getWidth() - 16) / 6;
    for (int i = 1; i < 6; ++i)
    {
        int lx = 8 + i * knobW;
        g.fillRect(lx, 60, 1, getHeight() - 68);
    }
}

void MetallicaToneEditor::resized()
{
    auto b = getLocalBounds().reduced(8);

    titleLabel.setBounds(b.removeFromTop(28));
    presetLabel.setBounds(b.removeFromTop(18));
    b.removeFromTop(4);  // gap

    const int knobW = b.getWidth() / 6;
    for (auto* k : { &kGain, &kBass, &kMid, &kTreble, &kMaster, &kCabMix })
    {
        k->setBounds(b.removeFromLeft(knobW));
    }
}
