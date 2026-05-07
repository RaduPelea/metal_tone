#include "PluginProcessor.h"
#include "PluginEditor.h"
#include "BinaryData.h"
#include <cmath>
#include <filesystem>

namespace ParamIDs
{
    static const juce::String gain   = "gain";       // pre-NAM drive (dB)
    static const juce::String gate   = "gate";
    static const juce::String bass   = "bass";
    static const juce::String mid    = "mid";
    static const juce::String treble = "treble";
    static const juce::String master = "master";
}

// ── Parameter layout ────────────────────────────────────────────────────────

juce::AudioProcessorValueTreeState::ParameterLayout
UltimateMetalToneProcessor::createParameterLayout()
{
    std::vector<std::unique_ptr<juce::RangedAudioParameter>> p;

    // 0.1-precision dial that displays 1.0..10.0
    auto dial = [&](const juce::String& id, const juce::String& name, float def)
    {
        auto fmt = [](float v, int) { return juce::String(v, 1); };
        p.push_back(std::make_unique<juce::AudioParameterFloat>(
            juce::ParameterID(id, 1), name,
            juce::NormalisableRange<float>(1.0f, 10.0f, 0.1f), def,
            juce::AudioParameterFloatAttributes().withStringFromValueFunction(fmt)));
    };

    dial(ParamIDs::gain,   "Gain",   5.5f);
    // Gate keeps its dB scale — user explicitly asked to leave it alone
    p.push_back(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID(ParamIDs::gate, 1), "Gate",
        juce::NormalisableRange<float>(-100.0f, 0.0f, 0.1f), -100.0f,
        juce::AudioParameterFloatAttributes().withLabel("dB")));
    dial(ParamIDs::bass,   "Bass",   5.5f);
    dial(ParamIDs::mid,    "Mid",    5.5f);
    dial(ParamIDs::treble, "Treble", 5.5f);
    dial(ParamIDs::master, "Master", 5.5f);

    return { p.begin(), p.end() };
}

// ── Dial → dB helpers ───────────────────────────────────────────────────────
// Knob shows 1.0..10.0; centre 5.5 = 0 dB, 1.0 = -range, 10.0 = +range.
static inline float dialToDb(float v, float rangeDb)
{
    return (v - 5.5f) / 4.5f * rangeDb;
}

// ── Preset metadata table ───────────────────────────────────────────────────

const UltimateMetalToneProcessor::PresetInfo&
UltimateMetalToneProcessor::getPresetInfo(Preset p)
{
    static const PresetInfo infos[NumPresets] = {
        { "Thrash",            true,  false },  // Mesa IIC+ / sm57
        { "Groove Metal",      true,  false },  // Walk / Pantera Cowboys
        { "Heavy Metal",       false, false },  // coming soon
        { "Death Metal",       false, false },
        { "Black Metal",       false, false },
        { "Doom Metal",        false, false },
        { "Progressive Metal", false, false },
        { "Custom",            true,  true  }   // user loads files
    };
    return infos[(int)p];
}

// ── Constructor ─────────────────────────────────────────────────────────────

UltimateMetalToneProcessor::UltimateMetalToneProcessor()
    : AudioProcessor(BusesProperties()
                     .withInput ("Input",  juce::AudioChannelSet::mono(),   true)
                     .withOutput("Output", juce::AudioChannelSet::stereo(), true)),
      apvts(*this, nullptr, "Parameters", createParameterLayout())
{
    appDataDir = juce::File::getSpecialLocation(
        juce::File::userApplicationDataDirectory).getChildFile("UltimateMetalTone");
    appDataDir.createDirectory();

    juce::Logger::setCurrentLogger(
        new juce::FileLogger(appDataDir.getChildFile("debug.log"),
                             "Ultimate Metal Tone — debug log"));

    writeDefaultsToDisk();

    thrashNamFile = appDataDir.getChildFile("DethLead.nam");
    thrashIrFile  = appDataDir.getChildFile("sm57.wav");
    grooveNamFile = appDataDir.getChildFile("Walk.nam");
    grooveIrFile  = appDataDir.getChildFile("PanteraCowboys.wav");

    // Boot with the Thrash preset
    loadPreset(Thrash);
}

UltimateMetalToneProcessor::~UltimateMetalToneProcessor()
{
    juce::Logger::setCurrentLogger(nullptr);
}

void UltimateMetalToneProcessor::writeDefaultsToDisk()
{
    auto write = [this](const juce::String& name, const char* data, int size)
    {
        appDataDir.getChildFile(name).replaceWithData(data, static_cast<size_t>(size));
    };

    write("DethLead.nam",       BinaryData::DethLead_nam,       BinaryData::DethLead_namSize);
    write("sm57.wav",           BinaryData::sm57_wav,           BinaryData::sm57_wavSize);
    write("Walk.nam",           BinaryData::Walk_nam,           BinaryData::Walk_namSize);
    write("PanteraCowboys.wav", BinaryData::PanteraCowboys_wav, BinaryData::PanteraCowboys_wavSize);
}

// ── Presets ─────────────────────────────────────────────────────────────────

void UltimateMetalToneProcessor::loadPreset(Preset p)
{
    currentPreset = p;
    const auto& info = getPresetInfo(p);

    juce::Logger::writeToLog("Preset selected: " + juce::String(info.displayName));

    switch (p)
    {
        case Thrash:
            loadNAMFromFile(thrashNamFile);
            loadIRFromFile (thrashIrFile);
            statusText = "Mesa Boogie Mark IIC+  /  4x12 Mesa Rectifier sm57";
            break;
        case GrooveMetal:
            loadNAMFromFile(grooveNamFile);
            loadIRFromFile (grooveIrFile);
            statusText = "Pantera Walk  /  Pantera Cowboys From Hell IR";
            break;
        case Custom:
            // Don't change loaded NAM/IR — user manages via Load buttons / drag-drop
            statusText = "Custom — drag your own .nam and .wav files onto the window";
            break;
        default:
            statusText = juce::String(info.displayName) + " — coming soon (drop files via Custom)";
            break;
    }
}

// ── File loading ────────────────────────────────────────────────────────────

bool UltimateMetalToneProcessor::loadNAMFromFile(const juce::File& file)
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

bool UltimateMetalToneProcessor::loadIRFromFile(const juce::File& file)
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

void UltimateMetalToneProcessor::prepareToPlay(double sampleRate, int samplesPerBlock)
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

    if (irFileForReload.existsAsFile())
        loadIRFromFile(irFileForReload);

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

    lastBass = lastMid = lastTreble = 1e9f;
    updateEQ();
}

// ── EQ ──────────────────────────────────────────────────────────────────────

void UltimateMetalToneProcessor::updateEQ()
{
    if (currentSR <= 0.0) return;

    // Knobs 1..10 → ±12 dB
    float bassRaw   = apvts.getRawParameterValue(ParamIDs::bass)->load();
    float midRaw    = apvts.getRawParameterValue(ParamIDs::mid)->load();
    float trebleRaw = apvts.getRawParameterValue(ParamIDs::treble)->load();

    if (bassRaw == lastBass && midRaw == lastMid && trebleRaw == lastTreble)
        return;
    lastBass = bassRaw; lastMid = midRaw; lastTreble = trebleRaw;

    float bassDb   = dialToDb(bassRaw,   12.0f);
    float midDb    = dialToDb(midRaw,    12.0f);
    float trebleDb = dialToDb(trebleRaw, 12.0f);

    auto bc = FilterCfs::makeLowShelf  (currentSR, 250.0f, 0.71f, juce::Decibels::decibelsToGain(bassDb));
    auto mc = FilterCfs::makePeakFilter(currentSR, 800.0f, 1.0f,  juce::Decibels::decibelsToGain(midDb));
    auto tc = FilterCfs::makeHighShelf (currentSR, 4000.0f,0.71f, juce::Decibels::decibelsToGain(trebleDb));
    if (bc) *bassFilter.state   = *bc;
    if (mc) *midFilter.state    = *mc;
    if (tc) *trebleFilter.state = *tc;
}

// ── Buses ───────────────────────────────────────────────────────────────────

bool UltimateMetalToneProcessor::isBusesLayoutSupported(const BusesLayout& layouts) const
{
    auto mono   = juce::AudioChannelSet::mono();
    auto stereo = juce::AudioChannelSet::stereo();
    const auto in  = layouts.getMainInputChannelSet();
    const auto out = layouts.getMainOutputChannelSet();
    return (in == mono || in == stereo) && (out == mono || out == stereo);
}

// ── processBlock ────────────────────────────────────────────────────────────

void UltimateMetalToneProcessor::processBlock(juce::AudioBuffer<float>& buffer,
                                                juce::MidiBuffer&)
{
    juce::ScopedNoDenormals noDenormals;
    const int numSamples = buffer.getNumSamples();
    const int numOut     = getTotalNumOutputChannels();
    if (numSamples == 0 || numOut == 0) return;

    if (namPending.load() || irPending.load())
    {
        juce::SpinLock::ScopedLockType lk(swapLock);
        if (namPending.exchange(false)) namModel    = std::move(stagedNam);
        if (irPending.exchange(false))  irProcessor = std::move(stagedIr);
    }

    for (int ch = 1; ch < numOut; ++ch)
        buffer.clear(ch, 0, numSamples);

    auto* ch0 = buffer.getWritePointer(0);

    // Gain (pre-NAM drive) — knob 1..10, ±24 dB swing
    float gainDb = dialToDb(apvts.getRawParameterValue(ParamIDs::gain)->load(), 24.0f);
    if (std::abs(gainDb) > 0.01f)
        buffer.applyGain(0, 0, numSamples, juce::Decibels::decibelsToGain(gainDb));

    // Noise gate
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

    // NAM (×10 / ×3 fixed calibration)
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

    // IR convolution
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
            for (int i = 0; i < numSamples; ++i)
                ch0[i] = static_cast<float>(irOut[0][i]);
    }

    // Tone stack
    updateEQ();
    {
        juce::dsp::AudioBlock<float> block(buffer);
        juce::dsp::ProcessContextReplacing<float> ctx(block);
        bassFilter.process(ctx);
        midFilter.process(ctx);
        trebleFilter.process(ctx);
    }

    // Master — knob 1..10, ±24 dB
    float masterDb = dialToDb(apvts.getRawParameterValue(ParamIDs::master)->load(), 24.0f);
    if (std::abs(masterDb) > 0.01f)
        buffer.applyGain(juce::Decibels::decibelsToGain(masterDb));

    // Mono → stereo
    for (int ch = 1; ch < numOut; ++ch)
        buffer.copyFrom(ch, 0, buffer, 0, 0, numSamples);
}

// ── State ───────────────────────────────────────────────────────────────────

void UltimateMetalToneProcessor::getStateInformation(juce::MemoryBlock& destData)
{
    auto state = apvts.copyState();
    std::unique_ptr<juce::XmlElement> xml(state.createXml());
    copyXmlToBinary(*xml, destData);
}

void UltimateMetalToneProcessor::setStateInformation(const void* data, int size)
{
    std::unique_ptr<juce::XmlElement> xml(getXmlFromBinary(data, size));
    if (xml && xml->hasTagName(apvts.state.getType()))
        apvts.replaceState(juce::ValueTree::fromXml(*xml));
}

// ── Editor / Plugin factory ─────────────────────────────────────────────────

juce::AudioProcessorEditor* UltimateMetalToneProcessor::createEditor()
{
    return new UltimateMetalToneEditor(*this);
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new UltimateMetalToneProcessor();
}
