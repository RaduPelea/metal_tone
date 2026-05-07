#pragma once
#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_gui_basics/juce_gui_basics.h>
#include "PluginProcessor.h"

// ── Premium look & feel for rotary knobs ─────────────────────────────────────

class PremiumLnF : public juce::LookAndFeel_V4
{
public:
    PremiumLnF();
    void drawRotarySlider(juce::Graphics&, int x, int y, int w, int h,
                          float sliderPos, float startAngle, float endAngle,
                          juce::Slider&) override;
};

// ── Sidebar preset row ───────────────────────────────────────────────────────

class PresetRow : public juce::Component
{
public:
    using Preset = UltimateMetalToneProcessor::Preset;
    std::function<void(Preset)> onSelect;

    PresetRow(Preset p, const juce::String& displayName, bool available);

    void setActive(bool a) { active = a; repaint(); }
    bool isActive() const  { return active; }

    void paint(juce::Graphics&) override;
    void mouseEnter(const juce::MouseEvent&) override { hover = true;  repaint(); }
    void mouseExit (const juce::MouseEvent&) override { hover = false; repaint(); }
    void mouseUp   (const juce::MouseEvent&) override;

private:
    Preset       preset;
    juce::String name;
    bool         available;
    bool         active = false;
    bool         hover  = false;
};

// ── Pedal slot (empty placeholder) ───────────────────────────────────────────

class PedalSlot : public juce::Component
{
public:
    PedalSlot(int index);
    void paint(juce::Graphics&) override;
    void mouseEnter(const juce::MouseEvent&) override { hover = true;  repaint(); }
    void mouseExit (const juce::MouseEvent&) override { hover = false; repaint(); }
    void mouseUp   (const juce::MouseEvent&) override;
private:
    int  slotIndex;
    bool hover = false;
};

// ── Round + button ───────────────────────────────────────────────────────────

class AddButton : public juce::Component
{
public:
    std::function<void()> onClick;
    void paint(juce::Graphics&) override;
    void mouseEnter(const juce::MouseEvent&) override { hover = true;  repaint(); }
    void mouseExit (const juce::MouseEvent&) override { hover = false; repaint(); }
    void mouseDown (const juce::MouseEvent&) override { down  = true;  repaint(); }
    void mouseUp   (const juce::MouseEvent&) override;
private:
    bool hover = false, down = false;
};

// ── Editor ───────────────────────────────────────────────────────────────────

class UltimateMetalToneEditor : public juce::AudioProcessorEditor,
                                  public juce::FileDragAndDropTarget,
                                  private juce::Timer
{
public:
    explicit UltimateMetalToneEditor(UltimateMetalToneProcessor&);
    ~UltimateMetalToneEditor() override;

    void paint(juce::Graphics&) override;
    void resized() override;

    bool isInterestedInFileDrag(const juce::StringArray&) override;
    void filesDropped         (const juce::StringArray&, int x, int y) override;
    void fileDragEnter        (const juce::StringArray&, int x, int y) override;
    void fileDragExit         (const juce::StringArray&) override;

private:
    void timerCallback() override;
    void chooseNAM();
    void chooseIR();
    void onPresetChosen(UltimateMetalToneProcessor::Preset);
    void refreshPresetHighlights();
    void updateCustomVisibility();

    // Helper drawings
    void drawAmpHead     (juce::Graphics&, juce::Rectangle<int>);
    void drawPedalboardBg(juce::Graphics&, juce::Rectangle<int>);
    void drawScrew       (juce::Graphics&, float cx, float cy, float r);

    struct Knob : public juce::Component
    {
        juce::Slider slider;
        juce::Label  caption;
        Knob(const juce::String& name, juce::LookAndFeel& lnf);
        void resized() override;
    };

    UltimateMetalToneProcessor& processor;
    PremiumLnF lnf;

    Knob kGain, kGate, kBass, kMid, kTreble, kMaster;

    using Att = juce::AudioProcessorValueTreeState::SliderAttachment;
    Att aGain, aGate, aBass, aMid, aTreble, aMaster;

    juce::Label    titleLabel, subtitleLabel, statusLabel, sidebarLabel,
                    pedalboardLabel, ampLogoLabel;
    juce::Label    namLabel, irLabel;
    juce::TextButton btnLoadNam { "LOAD NAM" };
    juce::TextButton btnLoadIr  { "LOAD IR"  };

    juce::OwnedArray<PresetRow> presetRows;
    juce::OwnedArray<PedalSlot> pedalSlots;
    AddButton                    addPedalBtn;

    // Cached layout rectangles for paint() to draw chassis around components
    juce::Rectangle<int> ampHeadArea;
    juce::Rectangle<int> pedalboardArea;

    std::unique_ptr<juce::FileChooser> fileChooser;

    bool dragHighlight = false;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(UltimateMetalToneEditor)
};
