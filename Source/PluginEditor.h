#pragma once
#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_gui_basics/juce_gui_basics.h>
#include "PluginProcessor.h"

// ── Custom look & feel ────────────────────────────────────────────────────────
class MetallicaLookAndFeel : public juce::LookAndFeel_V4
{
public:
    MetallicaLookAndFeel();
    void drawRotarySlider(juce::Graphics&, int x, int y, int w, int h,
                          float sliderPos, float startAngle, float endAngle,
                          juce::Slider&) override;
    void drawLabel(juce::Graphics&, juce::Label&) override;
};

// ── Knob + label combo ────────────────────────────────────────────────────────
class AmpKnob : public juce::Component
{
public:
    juce::Slider slider;
    juce::Label  nameLabel;
    juce::Label  valueLabel;

    explicit AmpKnob(const juce::String& name);
    void resized() override;
};

// ── Main editor ───────────────────────────────────────────────────────────────
class MetallicaToneEditor : public juce::AudioProcessorEditor,
                             public juce::Slider::Listener
{
public:
    explicit MetallicaToneEditor(MetallicaToneProcessor&);
    ~MetallicaToneEditor() override;

    void paint(juce::Graphics&) override;
    void resized() override;
    void sliderValueChanged(juce::Slider*) override {}  // handled by APVTS attachments

private:
    MetallicaToneProcessor& processor;
    MetallicaLookAndFeel laf;

    AmpKnob kInput, kBass, kMid, kTreble, kMaster, kCabMix;

    using Attachment = juce::AudioProcessorValueTreeState::SliderAttachment;
    Attachment attInput, attBass, attMid, attTreble, attMaster, attCabMix;

    juce::Label titleLabel;
    juce::Label presetLabel;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(MetallicaToneEditor)
};
