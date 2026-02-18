#pragma once

#include <JuceHeader.h>
#include "PluginProcessor.h"

class HeliosAudioProcessorEditor : public juce::AudioProcessorEditor
{
public:
    explicit HeliosAudioProcessorEditor (HeliosAudioProcessor&);
    ~HeliosAudioProcessorEditor() override = default;

    void paint (juce::Graphics&) override;
    void resized() override;

private:
    HeliosAudioProcessor& audioProcessor;

    juce::Slider inputSlider, outputSlider, driveSlider, cutoffSlider, resSlider;
    juce::ComboBox osBox;

    juce::Label titleLabel;
    juce::Label inputLabel, outputLabel, driveLabel, cutoffLabel, resLabel, osLabel;

    using SliderAttachment = juce::AudioProcessorValueTreeState::SliderAttachment;
    using ComboAttachment  = juce::AudioProcessorValueTreeState::ComboBoxAttachment;

    std::unique_ptr<SliderAttachment> inputAtt, outputAtt, driveAtt, cutoffAtt, resAtt;
    std::unique_ptr<ComboAttachment> osAtt;

    void styleSlider (juce::Slider& s);

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (HeliosAudioProcessorEditor)
};
