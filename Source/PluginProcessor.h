#pragma once
#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_dsp/juce_dsp.h>
#include <memory>
#include <atomic>
#include <vector>

#include "NAM/dsp.h"
#include "NAM/get_dsp.h"
#include "ImpulseResponse.h"

class UltimateMetalToneProcessor : public juce::AudioProcessor
{
public:
    // ── Preset enum / metadata ─────────────────────────────────────────────
    enum Preset
    {
        Thrash = 0,
        GrooveMetal,
        HeavyMetal,
        DeathMetal,
        BlackMetal,
        DoomMetal,
        ProgressiveMetal,
        Custom,
        NumPresets
    };

    struct PresetInfo
    {
        const char* displayName;
        bool        available;       // false = placeholder, no NAM/IR yet
        bool        userLoadable;    // true only for Custom — enables drag-drop / load buttons
    };
    static const PresetInfo& getPresetInfo(Preset);

    UltimateMetalToneProcessor();
    ~UltimateMetalToneProcessor() override;

    void prepareToPlay(double sampleRate, int samplesPerBlock) override;
    void releaseResources() override {}

    bool isBusesLayoutSupported(const BusesLayout& layouts) const override;
    void processBlock(juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }

    const juce::String getName() const override { return JucePlugin_Name; }
    bool   acceptsMidi()  const override { return false; }
    bool   producesMidi() const override { return false; }
    bool   isMidiEffect() const override { return false; }
    double getTailLengthSeconds() const override { return 0.0; }

    int getNumPrograms() override { return 1; }
    int getCurrentProgram() override { return 0; }
    void setCurrentProgram(int) override {}
    const juce::String getProgramName(int) override { return {}; }
    void changeProgramName(int, const juce::String&) override {}

    void getStateInformation(juce::MemoryBlock&) override;
    void setStateInformation(const void*, int) override;

    static juce::AudioProcessorValueTreeState::ParameterLayout createParameterLayout();
    juce::AudioProcessorValueTreeState apvts;

    // ── Public API for editor ────────────────────────────────────────────
    bool loadNAMFromFile(const juce::File& file);
    bool loadIRFromFile (const juce::File& file);
    juce::String getNAMName() const { return namName; }
    juce::String getIRName () const { return irName;  }
    juce::String getStatusText() const { return statusText; }

    void   loadPreset(Preset p);
    Preset getCurrentPreset() const { return currentPreset; }

private:
    void writeDefaultsToDisk();

    // NAM + IR (staged-swap pattern)
    std::unique_ptr<nam::DSP>             namModel, stagedNam;
    std::unique_ptr<dsp::ImpulseResponse> irProcessor, stagedIr;
    std::atomic<bool> namPending {false}, irPending {false};
    juce::SpinLock    swapLock;
    juce::String      namName, irName, statusText;
    juce::File        irFileForReload;

    Preset currentPreset = Thrash;

    std::vector<double> namIn, namOut;

    using Filter    = juce::dsp::IIR::Filter<float>;
    using FilterCfs = juce::dsp::IIR::Coefficients<float>;
    using DupFilter = juce::dsp::ProcessorDuplicator<Filter, FilterCfs>;
    DupFilter bassFilter, midFilter, trebleFilter;
    float lastBass = 0.f, lastMid = 0.f, lastTreble = 0.f;
    void  updateEQ();

    float gateEnv = 0.f;
    float gateAttackCoef = 0.f, gateRelCoef = 0.f;

    double currentSR  = 44100.0;
    int    currentBSz = 512;

    juce::File appDataDir;
    juce::File thrashNamFile, thrashIrFile;
    juce::File grooveNamFile, grooveIrFile;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(UltimateMetalToneProcessor)
};
