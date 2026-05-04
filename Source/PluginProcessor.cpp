#include "PluginProcessor.h"
#include "PluginEditor.h"
#include "BinaryData.h"
#include <cmath>

namespace ParamIDs
{
    static const juce::String gain      = "gain";       // 0-10  preamp drive into NAM
    static const juce::String gate      = "gate";       // 0-10  noise gate threshold
    static const juce::String bass      = "bass";       // 0-10
    static const juce::String mid       = "mid";        // 0-10
    static const juce::String treble    = "treble";     // 0-10
    static const juce::String master    = "master";     // 0-10
    static const juce::String cabMix    = "cabMix";     // 0-10
}

// ─── Parameter layout ────────────────────────────────────────────────────────

juce::AudioProcessorValueTreeState::ParameterLayout MetallicaToneProcessor::createParameterLayout()
{
    std::vector<std::unique_ptr<juce::RangedAudioParameter>> p;
    auto dial = [&](const juce::String& id, const juce::String& name, float def)
    {
        p.push_back(std::make_unique<juce::AudioParameterFloat>(
            juce::ParameterID(id, 1), name,
            juce::NormalisableRange<float>(0.0f, 10.0f, 0.01f), def));
    };
    dial(ParamIDs::gain,   "Gain",   8.0f);
    dial(ParamIDs::gate,   "Gate",   4.0f);   // default 4 = -48 dB threshold
    dial(ParamIDs::bass,   "Bass",   6.0f);
    dial(ParamIDs::mid,       "Mid",        2.0f);
    dial(ParamIDs::treble,    "Treble",     7.0f);
    dial(ParamIDs::master,    "Master",     7.0f);
    dial(ParamIDs::cabMix,    "Cab Mix",   10.0f);
    return { p.begin(), p.end() };
}

// ─── Helpers: 0-10 knob → parameter ─────────────────────────────────────────
// GAIN: exponential  0→1×  5→~7×  10→50×
static double dialToPreBoost(float v)    { return std::pow(50.0, static_cast<double>(v) / 10.0); }
// GATE: 0→-inf (off)  1→-80dB  10→-20dB  (threshold before NAM)
static float dialToGateThreshLin(float v)
{
    if (v < 0.05f) return 0.f;                          // 0 = gate fully off
    float db = -80.0f + (v / 10.0f) * 60.0f;           // -80 dB .. -20 dB
    return juce::Decibels::decibelsToGain(db);
}
static float dialToEqDb(float v)         { return (v / 5.0f - 1.0f) * 12.0f; }
static float dialToMasterDb(float v)     { return (v / 5.0f - 1.0f) * 18.0f; }
static float dialToCabMix(float v)       { return v / 10.0f; }

// ─── Constructor ─────────────────────────────────────────────────────────────

MetallicaToneProcessor::MetallicaToneProcessor()
    : AudioProcessor(BusesProperties()
                     .withInput ("Input",  juce::AudioChannelSet::mono(),   true)
                     .withOutput("Output", juce::AudioChannelSet::stereo(), true)),
      apvts(*this, nullptr, "Parameters", createParameterLayout())
{
    // Write embedded assets to AppData so they survive across sessions
    juce::File dir = juce::File::getSpecialLocation(
        juce::File::userApplicationDataDirectory).getChildFile("MetallicaTone");
    dir.createDirectory();

    namFilePath = dir.getChildFile("DethLead.nam");
    namFilePath.replaceWithData(BinaryData::DethLead_nam, BinaryData::DethLead_namSize);

    irFilePath = dir.getChildFile("sm57.wav");
    irFilePath.replaceWithData(BinaryData::sm57_wav, BinaryData::sm57_wavSize);

    // Set up file logger so we can see what's happening
    juce::File logFile = dir.getChildFile("debug.log");
    juce::Logger::setCurrentLogger(
        new juce::FileLogger(logFile, "MetallicaTone Debug Log"));

    juce::Logger::writeToLog("NAM path:   " + namFilePath.getFullPathName());
    juce::Logger::writeToLog("NAM exists: " + juce::String((int)namFilePath.existsAsFile()));
    juce::Logger::writeToLog("NAM size:   " + juce::String(namFilePath.getSize()));
    juce::Logger::writeToLog("IR path:    " + irFilePath.getFullPathName());

    loadNAM();
}

// ─── prepareToPlay ────────────────────────────────────────────────────────────

void MetallicaToneProcessor::prepareToPlay(double sampleRate, int samplesPerBlock)
{
    currentSR  = sampleRate;
    currentBSz = samplesPerBlock;

    namIn .resize(static_cast<size_t>(samplesPerBlock * 2), 0.0);
    namOut.resize(static_cast<size_t>(samplesPerBlock * 2), 0.0);

    // Noise gate envelope coefficients (1 ms attack, 80 ms release)
    gateAttackCoef = std::exp(-1.0f / (static_cast<float>(sampleRate) * 0.001f));
    gateRelCoef    = std::exp(-1.0f / (static_cast<float>(sampleRate) * 0.080f));
    gateEnv        = 0.f;

    if (!namModel)
        loadNAM();   // retry if constructor load failed

    if (namModel)
    {
        namModel->Reset(sampleRate, samplesPerBlock);
        namModel->prewarm();
        juce::Logger::writeToLog("NAM reset OK at " + juce::String(sampleRate) + " Hz");
    }
    else
    {
        juce::Logger::writeToLog("NAM model is NULL in prepareToPlay!");
    }

    // Load IR with the actual device sample rate
    if (irFilePath.existsAsFile())
    {
        try
        {
            auto ir = std::make_unique<dsp::ImpulseResponse>(
                irFilePath.getFullPathName().toRawUTF8(), sampleRate);
            if (ir->GetWavState() == dsp::wav::LoadReturnCode::SUCCESS)
            {
                irProcessor = std::move(ir);
                juce::Logger::writeToLog("IR loaded OK");
            }
            else
                juce::Logger::writeToLog("IR wav state failed");
        }
        catch (const std::exception& e)
        {
            juce::Logger::writeToLog("IR exception: " + juce::String(e.what()));
        }
    }
    else
    {
        juce::Logger::writeToLog("IR file missing: " + irFilePath.getFullPathName());
    }

    juce::dsp::ProcessSpec spec;
    spec.sampleRate       = sampleRate;
    spec.maximumBlockSize = static_cast<juce::uint32>(samplesPerBlock);
    spec.numChannels      = static_cast<juce::uint32>(
        std::max(1, getTotalNumOutputChannels()));

    hpfFilter.prepare(spec);    hpfFilter.reset();
    bassFilter.prepare(spec);   bassFilter.reset();
    midFilter.prepare(spec);    midFilter.reset();
    trebleFilter.prepare(spec); trebleFilter.reset();

    // Fixed 80 Hz high-pass — tightens low notes, cuts sub-bass rumble
    *hpfFilter.state = *FilterCfs::makeHighPass(sampleRate, 80.0f, 0.71f);

    updateEQ();
}

// ─── EQ coefficients ─────────────────────────────────────────────────────────

void MetallicaToneProcessor::updateEQ()
{
    if (currentSR <= 0.0) return;

    float bassDb   = dialToEqDb(apvts.getRawParameterValue(ParamIDs::bass)->load());
    float midDb    = dialToEqDb(apvts.getRawParameterValue(ParamIDs::mid)->load());
    float trebleDb = dialToEqDb(apvts.getRawParameterValue(ParamIDs::treble)->load());

    float bassG   = juce::Decibels::decibelsToGain(bassDb);
    float midG    = juce::Decibels::decibelsToGain(midDb);
    float trebleG = juce::Decibels::decibelsToGain(trebleDb);

    // Bass : low shelf  250 Hz — controls body/warmth
    // Mid  : peak      1000 Hz Q=1.5 — scoop here preserves 200-600Hz "chunk"
    //        (849Hz was cutting too low, making low notes muffled)
    // Treble: high shelf 3500 Hz — presence/bite
    auto bassCoefs = FilterCfs::makeLowShelf  (currentSR, 250.0f,  0.71f, bassG);
    auto midCoefs  = FilterCfs::makePeakFilter(currentSR, 1000.0f, 1.5f,  midG);
    auto trebCoefs = FilterCfs::makeHighShelf  (currentSR, 3500.0f, 0.71f, trebleG);

    if (bassCoefs)   *bassFilter.state   = *bassCoefs;
    if (midCoefs)    *midFilter.state    = *midCoefs;
    if (trebCoefs)   *trebleFilter.state = *trebCoefs;
}

// ─── Buses ───────────────────────────────────────────────────────────────────

bool MetallicaToneProcessor::isBusesLayoutSupported(const BusesLayout& layouts) const
{
    auto mono   = juce::AudioChannelSet::mono();
    auto stereo = juce::AudioChannelSet::stereo();
    const auto in  = layouts.getMainInputChannelSet();
    const auto out = layouts.getMainOutputChannelSet();
    return (in == mono || in == stereo) && (out == mono || out == stereo);
}

// ─── processBlock ────────────────────────────────────────────────────────────

void MetallicaToneProcessor::processBlock(juce::AudioBuffer<float>& buffer,
                                           juce::MidiBuffer&)
{
    juce::ScopedNoDenormals noDenormals;
    const int numSamples = buffer.getNumSamples();
    const int numOut     = getTotalNumOutputChannels();

    // Clear channels > 0 (we process mono on ch0)
    for (int ch = 1; ch < numOut; ++ch)
        buffer.clear(ch, 0, numSamples);

    // ── EQ update (only when a knob changed) ─────────────────────────────────
    {
        float b = apvts.getRawParameterValue(ParamIDs::bass)->load();
        float m = apvts.getRawParameterValue(ParamIDs::mid)->load();
        float t = apvts.getRawParameterValue(ParamIDs::treble)->load();
        if (b != lastBass || m != lastMid || t != lastTreble)
        {
            lastBass = b; lastMid = m; lastTreble = t;
            updateEQ();
        }
    }

    auto* ch0 = buffer.getWritePointer(0);

    if (!namModel)
    {
        for (int ch = 1; ch < numOut; ++ch)
            buffer.copyFrom(ch, 0, buffer, 0, 0, numSamples);
        return;
    }

    // ── 1. Fixed HPF 80 Hz — tighten low end before NAM ─────────────────────
    {
        juce::dsp::AudioBlock<float> blk(buffer.getArrayOfWritePointers(), 1, (size_t)numSamples);
        hpfFilter.process(juce::dsp::ProcessContextReplacing<float>(blk));
    }

    // ── 2. Noise gate ────────────────────────────────────────────────────────
    {
        const float thresh = dialToGateThreshLin(
            apvts.getRawParameterValue(ParamIDs::gate)->load());
        if (thresh > 0.f)
        {
            for (int i = 0; i < numSamples; ++i)
            {
                float s = std::abs(ch0[i]);
                gateEnv = (s > gateEnv) ? gateAttackCoef * gateEnv + (1.f - gateAttackCoef) * s
                                        : gateRelCoef    * gateEnv;
                // Hard gate: below threshold → silence; above → pass
                float g = (gateEnv >= thresh) ? 1.f : 0.f;
                ch0[i] *= g;
            }
        }
    }

    // ── 2. NAM + IR ──────────────────────────────────────────────────────────
    {
        const size_t n = static_cast<size_t>(numSamples);
        if (namIn.size() < n * 2) { namIn.resize(n * 2, 0.0); namOut.resize(n * 2, 0.0); }

        const double preBoost = dialToPreBoost(apvts.getRawParameterValue(ParamIDs::gain)->load());
        for (int i = 0; i < numSamples; ++i)
            namIn[static_cast<size_t>(i)] = static_cast<double>(ch0[i]) * preBoost;

        double* inp  = namIn.data();
        double* outp = namOut.data();
        namModel->process(&inp, &outp, numSamples);
        double* finalOut = namOut.data();

        float cabMix = dialToCabMix(apvts.getRawParameterValue(ParamIDs::cabMix)->load());
        if (irProcessor && cabMix > 0.0f)
        {
            double* irIn = namOut.data();
            double** irInPtr = &irIn;
            double** irOut = irProcessor->Process(irInPtr, 1,
                                                   static_cast<size_t>(numSamples));
            if (irOut && irOut[0])
            {
                constexpr float postBoost = 3.0f;
                for (int i = 0; i < numSamples; ++i)
                {
                    float wet = static_cast<float>(irOut[0][i]) * postBoost;
                    float dry = static_cast<float>(finalOut[i]) * postBoost;
                    ch0[i] = dry * (1.0f - cabMix) + wet * cabMix;
                }
                finalOut = nullptr;
            }
        }

        if (finalOut)
        {
            constexpr float postBoost = 3.0f;
            for (int i = 0; i < numSamples; ++i)
                ch0[i] = static_cast<float>(finalOut[i]) * postBoost;
        }
    }

    // ── 3. Bass / Mid / Treble EQ ─────────────────────────────────────────────
    {
        juce::dsp::AudioBlock<float> block(buffer);
        juce::dsp::ProcessContextReplacing<float> ctx(block);
        bassFilter.process(ctx);
        midFilter.process(ctx);
        trebleFilter.process(ctx);
    }

    // ── 4. Master volume ──────────────────────────────────────────────────────
    float masterDb = dialToMasterDb(apvts.getRawParameterValue(ParamIDs::master)->load());
    buffer.applyGain(juce::Decibels::decibelsToGain(masterDb));

    // ── 5. Mono → stereo copy ────────────────────────────────────────────────
    for (int ch = 1; ch < numOut; ++ch)
        buffer.copyFrom(ch, 0, buffer, 0, 0, numSamples);
}

// ─── State ───────────────────────────────────────────────────────────────────

void MetallicaToneProcessor::getStateInformation(juce::MemoryBlock& destData)
{
    auto state = apvts.copyState();
    std::unique_ptr<juce::XmlElement> xml(state.createXml());
    copyXmlToBinary(*xml, destData);
}

void MetallicaToneProcessor::setStateInformation(const void* data, int sizeInBytes)
{
    std::unique_ptr<juce::XmlElement> xml(getXmlFromBinary(data, sizeInBytes));
    if (xml && xml->hasTagName(apvts.state.getType()))
        apvts.replaceState(juce::ValueTree::fromXml(*xml));
}

// ─── Editor factory ──────────────────────────────────────────────────────────

juce::AudioProcessorEditor* MetallicaToneProcessor::createEditor()
{
    return new MetallicaToneEditor(*this);
}

// ─── Plugin factory ──────────────────────────────────────────────────────────

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new MetallicaToneProcessor();
}
