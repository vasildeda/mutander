/*
  ==============================================================================

    This file contains the basic framework code for a JUCE plugin editor.

  ==============================================================================
*/

#include "PluginProcessor.h"
#include "PluginEditor.h"

//==============================================================================
SwichanderAudioProcessorEditor::SwichanderAudioProcessorEditor (SwichanderAudioProcessor& p)
    : AudioProcessorEditor (&p), audioProcessor (p)
{
    addAndMakeVisible(logo);
    addAndMakeVisible(switchButton);

    for (int i = 0; i < 5; ++i)
    {
        midiLabels[i].setText("MIDI " + juce::String(i + 1), juce::dontSendNotification);
        midiLabels[i].setJustificationType(juce::Justification::centred);
        addAndMakeVisible(midiLabels[i]);
    }

    setSize (704, 396);
}

SwichanderAudioProcessorEditor::~SwichanderAudioProcessorEditor()
{
}

//==============================================================================
void SwichanderAudioProcessorEditor::paint (juce::Graphics& g)
{
    g.fillAll (getLookAndFeel().findColour (juce::ResizableWindow::backgroundColourId));
}

void SwichanderAudioProcessorEditor::resized()
{
    using Track = juce::Grid::TrackInfo;
    using Fr = juce::Grid::Fr;

    juce::Grid grid;
    grid.templateRows = { Track(Fr(1)), Track(Fr(1)) };
    grid.templateColumns = { Track(Fr(1)), Track(Fr(1)), Track(Fr(1)),
                             Track(Fr(1)), Track(Fr(1)), Track(Fr(1)) };

    grid.items = {
        juce::GridItem(logo),
        juce::GridItem(),  // empty
        juce::GridItem(),  // empty
        juce::GridItem(),  // empty
        juce::GridItem(),  // empty
        juce::GridItem(),  // empty
        juce::GridItem(switchButton),
        juce::GridItem(midiLabels[0]),
        juce::GridItem(midiLabels[1]),
        juce::GridItem(midiLabels[2]),
        juce::GridItem(midiLabels[3]),
        juce::GridItem(midiLabels[4])
    };

    grid.performLayout(getLocalBounds());
}
