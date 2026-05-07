#pragma once
#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_gui_basics/juce_gui_basics.h>
#include "PluginProcessor.h"

class MetalToneEditor : public juce::AudioProcessorEditor,
                         public juce::FileDragAndDropTarget,
                         private juce::Timer
{
public:
    explicit MetalToneEditor(MetalToneProcessor&);
    ~MetalToneEditor() override;

    void paint(juce::Graphics&) override;
    void resized() override;

    // Drag & drop
    bool isInterestedInFileDrag(const juce::StringArray&) override;
    void filesDropped         (const juce::StringArray&, int x, int y) override;
    void fileDragEnter        (const juce::StringArray&, int x, int y) override;
    void fileDragExit         (const juce::StringArray&) override;

private:
    void timerCallback() override;
    void chooseNAM();
    void chooseIR();

    // Helper: one labelled rotary knob
    struct Knob : public juce::Component
    {
        juce::Slider slider;
        juce::Label  caption;

        Knob(const juce::String& name);
        void resized() override;
    };

    MetalToneProcessor& processor;

    Knob kInput, kGate, kBass, kMid, kTreble, kMaster;

    using Att = juce::AudioProcessorValueTreeState::SliderAttachment;
    Att aInput, aGate, aBass, aMid, aTreble, aMaster;

    juce::Label    titleLabel;
    juce::Label    namLabel, irLabel;
    juce::TextButton btnLoadNam { "Load NAM..." };
    juce::TextButton btnLoadIr  { "Load IR..."  };

    std::unique_ptr<juce::FileChooser> fileChooser;

    bool dragHighlight = false;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(MetalToneEditor)
};
