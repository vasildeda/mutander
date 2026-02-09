/*
  ==============================================================================

    This file contains the basic framework code for a JUCE plugin editor.

  ==============================================================================
*/

#pragma once

#include <juce_audio_processors/juce_audio_processors.h>
#include "LongPressButton.h"
#include "PluginProcessor.h"

//==============================================================================
class PluginEditor : public juce::AudioProcessorEditor
{
public:
    PluginEditor(PluginProcessor&);
    ~PluginEditor() override;

    //==============================================================================
    void paint(juce::Graphics& g) override;
    void resized() override;

private:
    PluginProcessor& audioProcessor_;

    LongPressButton stopButton_;
    LongPressButton goButton_;

    void updateButtons();
    static juce::String formatTrigger(int32_t trigger);
    static juce::String formatTriggers(std::function<int32_t(int)> getter, int count);

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(PluginEditor)
};
