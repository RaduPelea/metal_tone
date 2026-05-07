#pragma once
#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_dsp/juce_dsp.h>
#include <memory>
#include <atomic>
#include <vector>

#include "NAM/dsp.h"
#include "NAM/get_dsp.h"
#include "ImpulseResponse.h"

class MetalToneProcessor : public juce::AudioProcessor
{
public:
    MetalToneProcessor();
    ~MetalToneProcessor() override;

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

    // Public API for editor
    bool loadNAMFromFile(const juce::File& file);
    bool loadIRFromFile (const juce::File& file);
    juce::String getNAMName() const { return namName; }
    juce::String getIRName () const { return irName;  }

private:
    void writeDefaultsToDisk();

    // NAM + IR (staged-swap pattern)
    std::unique_ptr<nam::DSP>             namModel, stagedNam;
    std::unique_ptr<dsp::ImpulseResponse> irProcessor, stagedIr;
    std::atomic<bool> namPending {false}, irPending {false};
    juce::SpinLock    swapLock;
    juce::String      namName, irName;
    juce::File        irFileForReload;   // remembered so prepareToPlay can re-init at new SR

    // NAM I/O buffers (double precision required by the API)
    std::vector<double> namIn, namOut;

    // 3-band tone stack
    using Filter    = juce::dsp::IIR::Filter<float>;
    using FilterCfs = juce::dsp::IIR::Coefficients<float>;
    using DupFilter = juce::dsp::ProcessorDuplicator<Filter, FilterCfs>;
    DupFilter bassFilter, midFilter, trebleFilter;
    float lastBass = 0.f, lastMid = 0.f, lastTreble = 0.f;
    void  updateEQ();

    // Noise gate
    float gateEnv = 0.f;
    float gateAttackCoef = 0.f, gateRelCoef = 0.f;

    // Audio state
    double currentSR  = 44100.0;
    int    currentBSz = 512;

    // Default file paths in %APPDATA%/MetalTone/
    juce::File appDataDir;
    juce::File defaultNamFile, defaultIrFile;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(MetalToneProcessor)
};
