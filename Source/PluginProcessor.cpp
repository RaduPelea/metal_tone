#include "PluginProcessor.h"
#include "PluginEditor.h"
#include "BinaryData.h"
#include <cmath>
#include <filesystem>

namespace ParamIDs
{
    static const juce::String inputGain = "inputGain";
    static const juce::String gate      = "gate";
    static const juce::String bass      = "bass";
    static const juce::String mid       = "mid";
    static const juce::String treble    = "treble";
    static const juce::String master    = "master";
}

// ── Parameter layout ────────────────────────────────────────────────────────

juce::AudioProcessorValueTreeState::ParameterLayout
MetalToneProcessor::createParameterLayout()
{
    std::vector<std::unique_ptr<juce::RangedAudioParameter>> p;

    auto db = [&](const juce::String& id, const juce::String& name,
                   float lo, float hi, float def)
    {
        p.push_back(std::make_unique<juce::AudioParameterFloat>(
            juce::ParameterID(id, 1), name,
            juce::NormalisableRange<float>(lo, hi, 0.1f), def,
            juce::AudioParameterFloatAttributes().withLabel("dB")));
    };

    db(ParamIDs::inputGain, "Input",  -24.0f,  24.0f, 0.0f);
    db(ParamIDs::gate,      "Gate",  -100.0f,   0.0f, -100.0f);   // -100 = off
    db(ParamIDs::bass,      "Bass",   -12.0f,  12.0f, 0.0f);
    db(ParamIDs::mid,       "Mid",    -12.0f,  12.0f, 0.0f);
    db(ParamIDs::treble,    "Treble", -12.0f,  12.0f, 0.0f);
    db(ParamIDs::master,    "Master", -24.0f,  24.0f, 0.0f);

    return { p.begin(), p.end() };
}

// ── Constructor ─────────────────────────────────────────────────────────────

MetalToneProcessor::MetalToneProcessor()
    : AudioProcessor(BusesProperties()
                     .withInput ("Input",  juce::AudioChannelSet::mono(),   true)
                     .withOutput("Output", juce::AudioChannelSet::stereo(), true)),
      apvts(*this, nullptr, "Parameters", createParameterLayout())
{
    appDataDir = juce::File::getSpecialLocation(
        juce::File::userApplicationDataDirectory).getChildFile("MetalTone");
    appDataDir.createDirectory();

    juce::Logger::setCurrentLogger(
        new juce::FileLogger(appDataDir.getChildFile("debug.log"),
                             "MetalTone debug log"));

    writeDefaultsToDisk();

    defaultNamFile = appDataDir.getChildFile("DethLead.nam");
    defaultIrFile  = appDataDir.getChildFile("sm57.wav");

    juce::Logger::writeToLog("Default NAM: " + defaultNamFile.getFullPathName());
    juce::Logger::writeToLog("Default IR:  " + defaultIrFile.getFullPathName());

    loadNAMFromFile(defaultNamFile);
    loadIRFromFile (defaultIrFile);
}

MetalToneProcessor::~MetalToneProcessor()
{
    juce::Logger::setCurrentLogger(nullptr);
}

void MetalToneProcessor::writeDefaultsToDisk()
{
    juce::File namFile = appDataDir.getChildFile("DethLead.nam");
    namFile.replaceWithData(BinaryData::DethLead_nam, BinaryData::DethLead_namSize);

    juce::File irFile = appDataDir.getChildFile("sm57.wav");
    irFile.replaceWithData(BinaryData::sm57_wav, BinaryData::sm57_wavSize);
}

// ── File loading ────────────────────────────────────────────────────────────

bool MetalToneProcessor::loadNAMFromFile(const juce::File& file)
{
    if (!file.existsAsFile())
    {
        juce::Logger::writeToLog("loadNAM: file missing " + file.getFullPathName());
        return false;
    }

    try
    {
        auto path = std::filesystem::path(file.getFullPathName().toWideCharPointer());
        auto newModel = nam::get_dsp(path);
        if (!newModel) return false;

        // Reset to current settings before staging so the audio thread can use it immediately
        newModel->Reset(currentSR, currentBSz);
        newModel->prewarm();

        {
            juce::SpinLock::ScopedLockType lk(swapLock);
            stagedNam = std::move(newModel);
            namPending.store(true);
            namName = file.getFileName();
        }
        juce::Logger::writeToLog("NAM loaded: " + namName);
        return true;
    }
    catch (const std::exception& e)
    {
        juce::Logger::writeToLog("loadNAM exception: " + juce::String(e.what()));
        return false;
    }
}

bool MetalToneProcessor::loadIRFromFile(const juce::File& file)
{
    if (!file.existsAsFile())
    {
        juce::Logger::writeToLog("loadIR: file missing " + file.getFullPathName());
        return false;
    }

    try
    {
        auto newIR = std::make_unique<dsp::ImpulseResponse>(
            file.getFullPathName().toRawUTF8(), currentSR);

        if (newIR->GetWavState() != dsp::wav::LoadReturnCode::SUCCESS)
        {
            juce::Logger::writeToLog("loadIR: bad wav state");
            return false;
        }

        {
            juce::SpinLock::ScopedLockType lk(swapLock);
            stagedIr = std::move(newIR);
            irPending.store(true);
            irName = file.getFileName();
            irFileForReload = file;
        }
        juce::Logger::writeToLog("IR loaded: " + irName);
        return true;
    }
    catch (const std::exception& e)
    {
        juce::Logger::writeToLog("loadIR exception: " + juce::String(e.what()));
        return false;
    }
}

// ── prepareToPlay ───────────────────────────────────────────────────────────

void MetalToneProcessor::prepareToPlay(double sampleRate, int samplesPerBlock)
{
    currentSR  = sampleRate;
    currentBSz = samplesPerBlock;

    namIn .resize(static_cast<size_t>(samplesPerBlock) * 2, 0.0);
    namOut.resize(static_cast<size_t>(samplesPerBlock) * 2, 0.0);

    if (namModel)
    {
        namModel->Reset(sampleRate, samplesPerBlock);
        namModel->prewarm();
    }

    // IR is sample-rate dependent — reload from the remembered file at the new SR
    if (irFileForReload.existsAsFile())
        loadIRFromFile(irFileForReload);

    // Gate envelope coefficients (1 ms attack, 80 ms release)
    gateAttackCoef = std::exp(-1.0f / (static_cast<float>(sampleRate) * 0.001f));
    gateRelCoef    = std::exp(-1.0f / (static_cast<float>(sampleRate) * 0.080f));
    gateEnv        = 0.f;

    juce::dsp::ProcessSpec spec;
    spec.sampleRate       = sampleRate;
    spec.maximumBlockSize = static_cast<juce::uint32>(samplesPerBlock);
    spec.numChannels      = static_cast<juce::uint32>(
        std::max(1, getTotalNumOutputChannels()));

    bassFilter.prepare(spec);   bassFilter.reset();
    midFilter.prepare(spec);    midFilter.reset();
    trebleFilter.prepare(spec); trebleFilter.reset();

    lastBass = lastMid = lastTreble = 1e9f;   // force coef recalc next block
    updateEQ();
}

// ── EQ ───────────────────────────────────────────────────────────────────────

void MetalToneProcessor::updateEQ()
{
    if (currentSR <= 0.0) return;

    float bassDb   = apvts.getRawParameterValue(ParamIDs::bass)->load();
    float midDb    = apvts.getRawParameterValue(ParamIDs::mid)->load();
    float trebleDb = apvts.getRawParameterValue(ParamIDs::treble)->load();

    if (bassDb == lastBass && midDb == lastMid && trebleDb == lastTreble)
        return;
    lastBass = bassDb; lastMid = midDb; lastTreble = trebleDb;

    auto bassCoefs = FilterCfs::makeLowShelf(currentSR, 250.0f, 0.71f,
                                              juce::Decibels::decibelsToGain(bassDb));
    auto midCoefs  = FilterCfs::makePeakFilter(currentSR, 800.0f, 1.0f,
                                                 juce::Decibels::decibelsToGain(midDb));
    auto trebCoefs = FilterCfs::makeHighShelf(currentSR, 4000.0f, 0.71f,
                                                juce::Decibels::decibelsToGain(trebleDb));
    if (bassCoefs)   *bassFilter.state   = *bassCoefs;
    if (midCoefs)    *midFilter.state    = *midCoefs;
    if (trebCoefs)   *trebleFilter.state = *trebCoefs;
}

// ── Buses ───────────────────────────────────────────────────────────────────

bool MetalToneProcessor::isBusesLayoutSupported(const BusesLayout& layouts) const
{
    auto mono   = juce::AudioChannelSet::mono();
    auto stereo = juce::AudioChannelSet::stereo();
    const auto in  = layouts.getMainInputChannelSet();
    const auto out = layouts.getMainOutputChannelSet();
    return (in == mono || in == stereo) && (out == mono || out == stereo);
}

// ── processBlock ────────────────────────────────────────────────────────────

void MetalToneProcessor::processBlock(juce::AudioBuffer<float>& buffer,
                                        juce::MidiBuffer&)
{
    juce::ScopedNoDenormals noDenormals;
    const int numSamples = buffer.getNumSamples();
    const int numOut     = getTotalNumOutputChannels();
    if (numSamples == 0 || numOut == 0) return;

    // ── 1. Apply pending NAM/IR swaps (under spin lock, only at block start) ──
    if (namPending.load() || irPending.load())
    {
        juce::SpinLock::ScopedLockType lk(swapLock);
        if (namPending.exchange(false))
            namModel = std::move(stagedNam);
        if (irPending.exchange(false))
            irProcessor = std::move(stagedIr);
    }

    // Clear all channels above 0
    for (int ch = 1; ch < numOut; ++ch)
        buffer.clear(ch, 0, numSamples);

    auto* ch0 = buffer.getWritePointer(0);

    // ── 2. Input gain ────────────────────────────────────────────────────────
    float inGainDb = apvts.getRawParameterValue(ParamIDs::inputGain)->load();
    if (std::abs(inGainDb) > 0.01f)
        buffer.applyGain(0, 0, numSamples, juce::Decibels::decibelsToGain(inGainDb));

    // ── 3. Noise gate (only if user enabled it) ──────────────────────────────
    {
        float threshDb = apvts.getRawParameterValue(ParamIDs::gate)->load();
        if (threshDb > -99.5f)
        {
            float threshLin = juce::Decibels::decibelsToGain(threshDb);
            for (int i = 0; i < numSamples; ++i)
            {
                float s = std::abs(ch0[i]);
                gateEnv = (s > gateEnv)
                            ? gateAttackCoef * gateEnv + (1.f - gateAttackCoef) * s
                            : gateRelCoef    * gateEnv;
                if (gateEnv < threshLin) ch0[i] = 0.f;
            }
        }
    }

    // ── 4. NAM (×10 pre, ×3 post — fixed calibration) ────────────────────────
    if (namModel)
    {
        const size_t n = static_cast<size_t>(numSamples);
        if (namIn.size() < n) { namIn.resize(n * 2, 0.0); namOut.resize(n * 2, 0.0); }

        constexpr double kPre  = 10.0;
        constexpr double kPost = 3.0;

        for (int i = 0; i < numSamples; ++i)
            namIn[(size_t)i] = static_cast<double>(ch0[i]) * kPre;

        double* in  = namIn.data();
        double* out = namOut.data();
        namModel->process(&in, &out, numSamples);

        for (int i = 0; i < numSamples; ++i)
            ch0[i] = static_cast<float>(namOut[(size_t)i] * kPost);
    }

    // ── 5. IR convolution (NAM-style) ─────────────────────────────────────────
    if (irProcessor)
    {
        const size_t n = static_cast<size_t>(numSamples);
        if (namIn.size() < n) namIn.resize(n * 2, 0.0);

        for (int i = 0; i < numSamples; ++i)
            namIn[(size_t)i] = static_cast<double>(ch0[i]);

        double*  irIn    = namIn.data();
        double** irInPtr = &irIn;
        double** irOut   = irProcessor->Process(irInPtr, 1, n);
        if (irOut && irOut[0])
        {
            for (int i = 0; i < numSamples; ++i)
                ch0[i] = static_cast<float>(irOut[0][i]);
        }
    }

    // ── 6. Tone stack (only does anything when knobs are off-centre) ─────────
    updateEQ();
    {
        juce::dsp::AudioBlock<float> block(buffer);
        juce::dsp::ProcessContextReplacing<float> ctx(block);
        bassFilter.process(ctx);
        midFilter.process(ctx);
        trebleFilter.process(ctx);
    }

    // ── 7. Master gain ───────────────────────────────────────────────────────
    float masterDb = apvts.getRawParameterValue(ParamIDs::master)->load();
    if (std::abs(masterDb) > 0.01f)
        buffer.applyGain(juce::Decibels::decibelsToGain(masterDb));

    // ── 8. Mono → stereo copy ────────────────────────────────────────────────
    for (int ch = 1; ch < numOut; ++ch)
        buffer.copyFrom(ch, 0, buffer, 0, 0, numSamples);
}

// ── State ────────────────────────────────────────────────────────────────────

void MetalToneProcessor::getStateInformation(juce::MemoryBlock& destData)
{
    auto state = apvts.copyState();
    std::unique_ptr<juce::XmlElement> xml(state.createXml());
    copyXmlToBinary(*xml, destData);
}

void MetalToneProcessor::setStateInformation(const void* data, int size)
{
    std::unique_ptr<juce::XmlElement> xml(getXmlFromBinary(data, size));
    if (xml && xml->hasTagName(apvts.state.getType()))
        apvts.replaceState(juce::ValueTree::fromXml(*xml));
}

// ── Editor / Plugin factory ──────────────────────────────────────────────────

juce::AudioProcessorEditor* MetalToneProcessor::createEditor()
{
    return new MetalToneEditor(*this);
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new MetalToneProcessor();
}
