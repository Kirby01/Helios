#include "PluginEditor.h"

static void styleLabel (juce::Label& l)
{
    l.setColour (juce::Label::textColourId, juce::Colours::white);
    l.setJustificationType (juce::Justification::centred);
}

HeliosAudioProcessorEditor::HeliosAudioProcessorEditor (HeliosAudioProcessor& p)
: AudioProcessorEditor (&p), audioProcessor (p)
{
    setSize (520, 260);

    titleLabel.setText ("HELIOS", juce::dontSendNotification);
    titleLabel.setFont (juce::Font (22.0f, juce::Font::bold));
    styleLabel (titleLabel);
    addAndMakeVisible (titleLabel);

    auto setupSlider = [this](juce::Slider& s, juce::Label& l, const juce::String& text)
    {
        styleSlider (s);
        addAndMakeVisible (s);

        l.setText (text, juce::dontSendNotification);
        styleLabel (l);
        addAndMakeVisible (l);
    };

    setupSlider (inputSlider,  inputLabel,  "Input");
    setupSlider (outputSlider, outputLabel, "Output");
    setupSlider (driveSlider,  driveLabel,  "Drive");
    setupSlider (cutoffSlider, cutoffLabel, "LPF Hz");
    setupSlider (resSlider,    resLabel,    "Res");

    osLabel.setText ("OS", juce::dontSendNotification);
    styleLabel (osLabel);
    addAndMakeVisible (osLabel);

    osBox.addItem ("1x", 1);
    osBox.addItem ("2x", 2);
    osBox.addItem ("4x", 3);
    osBox.setColour (juce::ComboBox::backgroundColourId, juce::Colours::black);
    osBox.setColour (juce::ComboBox::textColourId, juce::Colours::white);
    osBox.setColour (juce::ComboBox::outlineColourId, juce::Colours::white);
    addAndMakeVisible (osBox);

    // Attachments
    inputAtt  = std::make_unique<SliderAttachment> (audioProcessor.apvts, HeliosAudioProcessor::paramInputId,     inputSlider);
    outputAtt = std::make_unique<SliderAttachment> (audioProcessor.apvts, HeliosAudioProcessor::paramOutputId,    outputSlider);
    driveAtt  = std::make_unique<SliderAttachment> (audioProcessor.apvts, HeliosAudioProcessor::paramDriveId,     driveSlider);
    cutoffAtt = std::make_unique<SliderAttachment> (audioProcessor.apvts, HeliosAudioProcessor::paramCutoffId,    cutoffSlider);
    resAtt    = std::make_unique<SliderAttachment> (audioProcessor.apvts, HeliosAudioProcessor::paramResonanceId, resSlider);
    osAtt     = std::make_unique<ComboAttachment>  (audioProcessor.apvts, HeliosAudioProcessor::paramOSId,        osBox);
}

void HeliosAudioProcessorEditor::styleSlider (juce::Slider& s)
{
    s.setSliderStyle (juce::Slider::RotaryHorizontalVerticalDrag);
    s.setTextBoxStyle (juce::Slider::TextBoxBelow, false, 70, 18);
    s.setColour (juce::Slider::rotarySliderFillColourId, juce::Colours::white);
    s.setColour (juce::Slider::rotarySliderOutlineColourId, juce::Colours::grey);
    s.setColour (juce::Slider::textBoxTextColourId, juce::Colours::white);
    s.setColour (juce::Slider::textBoxBackgroundColourId, juce::Colours::black);
    s.setColour (juce::Slider::textBoxOutlineColourId, juce::Colours::white);
}

void HeliosAudioProcessorEditor::paint (juce::Graphics& g)
{
    g.fillAll (juce::Colours::black);

    // subtle frame
    g.setColour (juce::Colours::white.withAlpha (0.15f));
    g.drawRoundedRectangle (getLocalBounds().toFloat().reduced (8.0f), 12.0f, 1.0f);
}

void HeliosAudioProcessorEditor::resized()
{
    auto r = getLocalBounds().reduced (16);
    titleLabel.setBounds (r.removeFromTop (34));

    r.removeFromTop (6);

    auto topRow = r.removeFromTop (150);
    auto bottomRow = r.removeFromTop (60);

    auto placeKnob = [](juce::Rectangle<int> area, juce::Slider& s, juce::Label& l)
    {
        auto labelArea = area.removeFromTop (18);
        l.setBounds (labelArea);
        s.setBounds (area.reduced (6));
    };

    const int w = topRow.getWidth() / 5;

    placeKnob (topRow.removeFromLeft (w), inputSlider,  inputLabel);
    placeKnob (topRow.removeFromLeft (w), outputSlider, outputLabel);
    placeKnob (topRow.removeFromLeft (w), driveSlider,  driveLabel);
    placeKnob (topRow.removeFromLeft (w), cutoffSlider, cutoffLabel);
    placeKnob (topRow,                 resSlider,    resLabel);

    auto osArea = bottomRow.removeFromLeft (140);
    osLabel.setBounds (osArea.removeFromTop (18));
    osBox.setBounds (osArea.removeFromTop (26).reduced (0, 2));
}
