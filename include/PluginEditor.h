/*
  ==============================================================================

    This file contains the basic framework code for a JUCE plugin editor.

  ==============================================================================
*/

#pragma once

#include <juce_audio_processors/juce_audio_processors.h>
#include "BinaryData.h"
#include "LongPressButton.h"
#include "PluginProcessor.h"

//==============================================================================
class MutanderAudioProcessorEditor : public juce::AudioProcessorEditor
{
public:
    MutanderAudioProcessorEditor(MutanderAudioProcessor&);
    ~MutanderAudioProcessorEditor() override;

    //==============================================================================
    void paint(juce::Graphics& g) override;
    void resized() override;

private:
    MutanderAudioProcessor& audioProcessor_;

    std::unique_ptr<juce::Drawable> background_;
    std::array<LongPressButton, 5> channelButtons_;

    void updateChannelButtons();

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(MutanderAudioProcessorEditor)
};
