#pragma once

#include <JuceHeader.h>

class HeliosAudioProcessor : public juce::AudioProcessor
{
public:
    HeliosAudioProcessor();
    ~HeliosAudioProcessor() override = default;

    //==============================================================================
    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override;

   #ifndef JucePlugin_PreferredChannelConfigurations
    bool isBusesLayoutSupported (const BusesLayout& layouts) const override;
   #endif

    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    //==============================================================================
    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override;

    //==============================================================================
    const juce::String getName() const override;

    bool acceptsMidi() const override;
    bool producesMidi() const override;
    bool isMidiEffect() const override;
    double getTailLengthSeconds() const override;

    //==============================================================================
    int getNumPrograms() override;
    int getCurrentProgram() override;
    void setCurrentProgram (int index) override;
    const juce::String getProgramName (int index) override;
    void changeProgramName (int index, const juce::String& newName) override;

    //==============================================================================
    void getStateInformation (juce::MemoryBlock& destData) override;
    void setStateInformation (const void* data, int sizeInBytes) override;

    //==============================================================================
    juce::AudioProcessorValueTreeState apvts;
    static juce::AudioProcessorValueTreeState::ParameterLayout createParameterLayout();

    static constexpr const char* paramInputId      = "input";
    static constexpr const char* paramOutputId     = "output";
    static constexpr const char* paramDriveId      = "drive";
    static constexpr const char* paramCutoffId     = "cutoff";
    static constexpr const char* paramResonanceId  = "resonance";
    static constexpr const char* paramOSId         = "oversample";

private:
    void updateDSPIfNeeded (double currentSampleRate);
    void rebuildOversamplingIfNeeded();
    void updateFilterCoefficients (double osSampleRate);

    // Kirby core state (stereo-linked)
    double a = 1.0;
    double b = 1.0;
    static constexpr double eps = 1e-13;

    // Oversampling
    int lastOSChoice = 0; // 0=1x,1=2x,2=4x
    int osFactor = 1;

    std::unique_ptr<juce::dsp::Oversampling<float>> oversampling;

    // Per-channel IIR lowpass filters (run at OS rate)
    juce::dsp::IIR::Filter<float> lpfL, lpfR;
    juce::dsp::IIR::Coefficients<float>::Ptr lpfCoeffs;

    // Cached params to avoid constant rebuilds
    float lastCutoff = -1.0f;
    float lastRes = -1.0f;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (HeliosAudioProcessor)
};
