/*
  ==============================================================================

    This file contains the basic framework code for a JUCE plugin editor.

  ==============================================================================
*/

#pragma once

#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_graphics/juce_graphics.h>
#include <juce_gui_basics/juce_gui_basics.h>
#include "PluginProcessor.h"

//==============================================================================
class LogoPlaceholder : public juce::Component
{
public:
    void paint(juce::Graphics& g) override
    {
        g.setColour(juce::Colours::darkgrey);
        g.fillRoundedRectangle(getLocalBounds().toFloat().reduced(4), 8.0f);
        g.setColour(juce::Colours::white);
        g.setFont(16.0f);
        g.drawText("LOGO", getLocalBounds(), juce::Justification::centred);
    }
};

//==============================================================================
class SwichanderAudioProcessorEditor  : public juce::AudioProcessorEditor
{
public:
    SwichanderAudioProcessorEditor (SwichanderAudioProcessor&);
    ~SwichanderAudioProcessorEditor() override;

    //==============================================================================
    void paint (juce::Graphics& g) override;
    void resized() override;

private:
    SwichanderAudioProcessor& audioProcessor;

    LogoPlaceholder logo;
    juce::ToggleButton switchButton { "Switch" };
    std::array<juce::Label, 5> midiLabels;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (SwichanderAudioProcessorEditor)
};
