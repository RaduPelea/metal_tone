#pragma once
#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_dsp/juce_dsp.h>
#include <memory>
#include <atomic>

#include "NAM/dsp.h"
#include "NAM/get_dsp.h"
#include "ImpulseResponse.h"

class MetallicaToneProcessor : public juce::AudioProcessor
{
public:
    MetallicaToneProcessor();
    ~MetallicaToneProcessor() override = default;

    void prepareToPlay(double sampleRate, int samplesPerBlock) override;
    void releaseResources() override {}

    bool isBusesLayoutSupported(const BusesLayout& layouts) const override;
    void processBlock(juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }

    const juce::String getName() const override { return JucePlugin_Name; }
    bool  acceptsMidi()  const override { return false; }
    bool  producesMidi() const override { return false; }
    bool  isMidiEffect() const override { return false; }
    double getTailLengthSeconds() const override { return 0.0; }

    int getNumPrograms() override { return 1; }
    int getCurrentProgram() override { return 0; }
    void setCurrentProgram(int) override {}
    const juce::String getProgramName(int) override { return {}; }
    void changeProgramName(int, const juce::String&) override {}

    void getStateInformation(juce::MemoryBlock& destData) override;
    void setStateInformation(const void* data, int sizeInBytes) override;

    static juce::AudioProcessorValueTreeState::ParameterLayout createParameterLayout();
    juce::AudioProcessorValueTreeState apvts;

private:
    std::unique_ptr<nam::DSP> namModel;
    std::unique_ptr<dsp::ImpulseResponse> irProcessor;
    std::vector<double> namIn, namOut;

    using Filter    = juce::dsp::IIR::Filter<float>;
    using FilterCfs = juce::dsp::IIR::Coefficients<float>;
    using DupFilter = juce::dsp::ProcessorDuplicator<Filter, FilterCfs>;
    DupFilter bassFilter, midFilter, trebleFilter;   // user-controlled EQ only

    juce::File namFilePath, irFilePath;

    void loadNAM()
    {
        try
        {
            namModel = nam::get_dsp(
                std::filesystem::path(namFilePath.getFullPathName().toWideCharPointer()));
            juce::Logger::writeToLog("NAM loaded OK");
        }
        catch (const std::exception& e)
        {
            juce::Logger::writeToLog("NAM exception: " + juce::String(e.what()));
            namModel = nullptr;
        }
    }

    // Noise gate state
    float gateEnv        = 0.f;
    float gateAttackCoef = 0.f;
    float gateRelCoef    = 0.f;

    double currentSR  = 44100.0;
    int    currentBSz = 512;
    float  lastBass = -999.f, lastMid = -999.f, lastTreble = -999.f;

    void updateEQ();

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(MetallicaToneProcessor)
};
