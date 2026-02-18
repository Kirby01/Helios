#include "PluginProcessor.h"
#include "PluginEditor.h"

HeliosAudioProcessor::HeliosAudioProcessor()
: AudioProcessor (BusesProperties()
                  .withInput  ("Input",  juce::AudioChannelSet::stereo(), true)
                  .withOutput ("Output", juce::AudioChannelSet::stereo(), true)),
  apvts (*this, nullptr, "PARAMS", createParameterLayout())
{
}

const juce::String HeliosAudioProcessor::getName() const { return JucePlugin_Name; }

bool HeliosAudioProcessor::acceptsMidi() const { return false; }
bool HeliosAudioProcessor::producesMidi() const { return false; }
bool HeliosAudioProcessor::isMidiEffect() const { return false; }
double HeliosAudioProcessor::getTailLengthSeconds() const { return 0.0; }

int HeliosAudioProcessor::getNumPrograms() { return 1; }
int HeliosAudioProcessor::getCurrentProgram() { return 0; }
void HeliosAudioProcessor::setCurrentProgram (int) {}
const juce::String HeliosAudioProcessor::getProgramName (int) { return {}; }
void HeliosAudioProcessor::changeProgramName (int, const juce::String&) {}

#ifndef JucePlugin_PreferredChannelConfigurations
bool HeliosAudioProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
    const auto mainOut = layouts.getMainOutputChannelSet();
    const auto mainIn  = layouts.getMainInputChannelSet();
    return (mainOut == juce::AudioChannelSet::stereo()
         && mainIn  == juce::AudioChannelSet::stereo());
}
#endif

juce::AudioProcessorValueTreeState::ParameterLayout HeliosAudioProcessor::createParameterLayout()
{
    using APF = juce::AudioParameterFloat;
    using APC = juce::AudioParameterChoice;

    std::vector<std::unique_ptr<juce::RangedAudioParameter>> p;

    p.push_back (std::make_unique<APF> (paramInputId,  "Input",
                                        juce::NormalisableRange<float> (0.0f, 4.0f, 0.01f),
                                        1.0f));

    p.push_back (std::make_unique<APF> (paramOutputId, "Output",
                                        juce::NormalisableRange<float> (0.0f, 4.0f, 0.01f),
                                        1.0f));

    p.push_back (std::make_unique<APF> (paramDriveId,  "Drive",
                                        juce::NormalisableRange<float> (0.0f, 200.0f, 1.0f),
                                        60.0f));

    p.push_back (std::make_unique<APF> (paramCutoffId, "LPF Hz",
                                        juce::NormalisableRange<float> (20.0f, 20000.0f, 1.0f, 0.5f),
                                        12000.0f));

    p.push_back (std::make_unique<APF> (paramResonanceId, "Resonance",
                                        juce::NormalisableRange<float> (0.0f, 0.98f, 0.001f),
                                        0.2f));

    p.push_back (std::make_unique<APC> (paramOSId, "Oversample",
                                        juce::StringArray { "1x", "2x", "4x" }, 0));

    return { p.begin(), p.end() };
}

void HeliosAudioProcessor::prepareToPlay (double sampleRate, int samplesPerBlock)
{
    // Reset core
    a = 1.0;
    b = 1.0;

    // (Re)build oversampler based on current parameter
    lastOSChoice = (int) apvts.getRawParameterValue (paramOSId)->load();
    rebuildOversamplingIfNeeded();

    // Prepare oversampler
    if (oversampling)
        oversampling->initProcessing ((size_t) samplesPerBlock);

    // Reset filters
    lpfL.reset();
    lpfR.reset();

    // Force coefficient refresh
    lastCutoff = -1.0f;
    lastRes    = -1.0f;

    updateDSPIfNeeded (sampleRate);
}

void HeliosAudioProcessor::releaseResources()
{
}

void HeliosAudioProcessor::rebuildOversamplingIfNeeded()
{
    const int choice = (int) apvts.getRawParameterValue (paramOSId)->load();
    if (choice == lastOSChoice && (choice == 0 ? !oversampling : (bool) oversampling))
        return;

    lastOSChoice = choice;

    osFactor = (choice == 0 ? 1 : (choice == 1 ? 2 : 4));

    // JUCE Oversampling uses "number of stages" where factor = 2^stages
    const int stages = (osFactor == 1 ? 0 : (osFactor == 2 ? 1 : 2));

    if (stages == 0)
    {
        oversampling.reset();
        // important: changing OS changes effective filter rate
        lastCutoff = -1.0f;
        lastRes    = -1.0f;
        return;
    }

    oversampling = std::make_unique<juce::dsp::Oversampling<float>>(
        2, stages,
        juce::dsp::Oversampling<float>::filterHalfBandPolyphaseIIR,
        true // more latency, better quality
    );

    // important: changing OS changes effective filter rate
    lastCutoff = -1.0f;
    lastRes    = -1.0f;
}

void HeliosAudioProcessor::updateFilterCoefficients (double osSampleRate)
{
    const float cutoff = apvts.getRawParameterValue (paramCutoffId)->load();
    const float res    = apvts.getRawParameterValue (paramResonanceId)->load();

    // Match your ReaJS mapping: Q = 0.5 + resonance * 12
    const float Q = 0.5f + res * 12.0f;

    // Clamp cutoff at Nyquist of OS rate
    const float maxCut = (float) (osSampleRate * 0.49);
    const float cutHz  = juce::jmin (cutoff, maxCut);

    lpfCoeffs = juce::dsp::IIR::Coefficients<float>::makeLowPass (osSampleRate, (double) cutHz, (double) Q);

    // Version-safe assignment (do NOT touch Filter::state)
    lpfL.coefficients = lpfCoeffs;
    lpfR.coefficients = lpfCoeffs;

    lastCutoff = cutoff;
    lastRes    = res;
}

void HeliosAudioProcessor::updateDSPIfNeeded (double currentSampleRate)
{
    rebuildOversamplingIfNeeded();

    const double osSampleRate = currentSampleRate * (double) osFactor;

    // Ensure coefficient pointers exist (version-safe)
    if (lpfL.coefficients == nullptr)
        lpfL.coefficients = juce::dsp::IIR::Coefficients<float>::makeLowPass (osSampleRate, 12000.0, 0.707);

    if (lpfR.coefficients == nullptr)
        lpfR.coefficients = juce::dsp::IIR::Coefficients<float>::makeLowPass (osSampleRate, 12000.0, 0.707);

    const float cutoff = apvts.getRawParameterValue (paramCutoffId)->load();
    const float res    = apvts.getRawParameterValue (paramResonanceId)->load();

    if (cutoff != lastCutoff || res != lastRes)
        updateFilterCoefficients (osSampleRate);
}

void HeliosAudioProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midi)
{
    juce::ignoreUnused (midi);
    juce::ScopedNoDenormals noDenormals;

    const auto totalNumInputChannels  = getTotalNumInputChannels();
    const auto totalNumOutputChannels = getTotalNumOutputChannels();
    for (auto i = totalNumInputChannels; i < totalNumOutputChannels; ++i)
        buffer.clear (i, 0, buffer.getNumSamples());

    jassert (buffer.getNumChannels() >= 2);

    const double sr = getSampleRate();
    updateDSPIfNeeded (sr);

    const float inGain  = apvts.getRawParameterValue (paramInputId)->load();
    const float outGain = apvts.getRawParameterValue (paramOutputId)->load();
    const float drv     = apvts.getRawParameterValue (paramDriveId)->load();

    juce::dsp::AudioBlock<float> block (buffer);

    if (oversampling)
    {
        // Upsample
        auto osBlock = oversampling->processSamplesUp (block);

        auto* chL = osBlock.getChannelPointer (0);
        auto* chR = osBlock.getChannelPointer (1);
        const int n = (int) osBlock.getNumSamples();

        for (int i = 0; i < n; ++i)
        {
            const double l = (double) chL[i] * (double) inGain;
            const double r = (double) chR[i] * (double) inGain;

            const double s = 0.5 * (l + r);

            // --- Kirby Core (explicit eps) at OS rate ---
            // a = (1-0.01)*b/(a+eps) + abs(s)*drv + 0.01*abs(b-a);
            // b = (1-0.01)*a + 0.01*(max(abs(b),eps) * min(abs(a/(b+eps)), 8));
            const double absS  = std::abs (s);
            const double absBA = std::abs (b - a);

            a = 0.99 * b / (a + eps) + absS * (double) drv + 0.01 * absBA;

            const double absB  = std::max (std::abs (b), eps);
            const double ratio = std::min (std::abs (a / (b + eps)), 8.0);
            b = 0.99 * a + 0.01 * (absB * ratio);

            const float xL = (float) ((l / (b + eps)) * (double) outGain);
            const float xR = (float) ((r / (b + eps)) * (double) outGain);

            // LPF at OS rate
            chL[i] = lpfL.processSample (xL);
            chR[i] = lpfR.processSample (xR);
        }

        // Downsample
        oversampling->processSamplesDown (block);
    }
    else
    {
        // 1x path
        auto* chL = buffer.getWritePointer (0);
        auto* chR = buffer.getWritePointer (1);
        const int n = buffer.getNumSamples();

        for (int i = 0; i < n; ++i)
        {
            const double l = (double) chL[i] * (double) inGain;
            const double r = (double) chR[i] * (double) inGain;

            const double s = 0.5 * (l + r);

            const double absS  = std::abs (s);
            const double absBA = std::abs (b - a);

            a = 0.99 * b / (a + eps) + absS * (double) drv + 0.01 * absBA;

            const double absB  = std::max (std::abs (b), eps);
            const double ratio = std::min (std::abs (a / (b + eps)), 8.0);
            b = 0.99 * a + 0.01 * (absB * ratio);

            const float xL = (float) ((l / (b + eps)) * (double) outGain);
            const float xR = (float) ((r / (b + eps)) * (double) outGain);

            chL[i] = lpfL.processSample (xL);
            chR[i] = lpfR.processSample (xR);
        }
    }
}

bool HeliosAudioProcessor::hasEditor() const { return true; }

juce::AudioProcessorEditor* HeliosAudioProcessor::createEditor()
{
    return new HeliosAudioProcessorEditor (*this);
}

void HeliosAudioProcessor::getStateInformation (juce::MemoryBlock& destData)
{
    auto state = apvts.copyState();
    std::unique_ptr<juce::XmlElement> xml (state.createXml());
    copyXmlToBinary (*xml, destData);
}

void HeliosAudioProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    std::unique_ptr<juce::XmlElement> xmlState (getXmlFromBinary (data, sizeInBytes));
    if (xmlState != nullptr)
        if (xmlState->hasTagName (apvts.state.getType()))
            apvts.replaceState (juce::ValueTree::fromXml (*xmlState));

    // Force refresh after loading
    lastCutoff = -1.0f;
    lastRes    = -1.0f;
}
#include "PluginProcessor.h"

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new HeliosAudioProcessor();
}
