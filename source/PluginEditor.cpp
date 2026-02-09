/*
  ==============================================================================

    This file contains the basic framework code for a JUCE plugin editor.

  ==============================================================================
*/

#include "PluginProcessor.h"
#include "PluginEditor.h"

//==============================================================================
MutanderAudioProcessorEditor::MutanderAudioProcessorEditor(MutanderAudioProcessor& p)
    : AudioProcessorEditor(&p), audioProcessor_(p)
{
    // Stop button (red)
    stopButton_.setActiveColour(juce::Colours::red);
    stopButton_.onClick = [this] {
        int learning = audioProcessor_.midiLearnTarget_.load(std::memory_order_relaxed);
        if (learning >= 0)
            audioProcessor_.midiLearnTarget_.store(-1, std::memory_order_relaxed);
        audioProcessor_.setMuted(true);
    };
    stopButton_.onLongPress = [this] {
        if (audioProcessor_.midiLearnTarget_.load(std::memory_order_relaxed) == 0)
            audioProcessor_.midiLearnTarget_.store(-1, std::memory_order_relaxed);
        else
        {
            audioProcessor_.clearTriggers(0);
            audioProcessor_.midiLearnTarget_.store(0, std::memory_order_relaxed);
        }
        updateButtons();
    };
    addAndMakeVisible(stopButton_);

    // Go button (green)
    goButton_.setActiveColour(juce::Colours::green);
    goButton_.onClick = [this] {
        int learning = audioProcessor_.midiLearnTarget_.load(std::memory_order_relaxed);
        if (learning >= 0)
            audioProcessor_.midiLearnTarget_.store(-1, std::memory_order_relaxed);
        audioProcessor_.setMuted(false);
    };
    goButton_.onLongPress = [this] {
        if (audioProcessor_.midiLearnTarget_.load(std::memory_order_relaxed) == 1)
            audioProcessor_.midiLearnTarget_.store(-1, std::memory_order_relaxed);
        else
        {
            audioProcessor_.clearTriggers(1);
            audioProcessor_.midiLearnTarget_.store(1, std::memory_order_relaxed);
        }
        updateButtons();
    };
    addAndMakeVisible(goButton_);

    // Register callback for processor -> GUI updates
    audioProcessor_.onStateChanged = [this] {
        updateButtons();
    };

    // Initial state
    updateButtons();

    setSize(200, 396);
}

MutanderAudioProcessorEditor::~MutanderAudioProcessorEditor()
{
    audioProcessor_.onStateChanged = nullptr;
}

juce::String MutanderAudioProcessorEditor::formatTrigger(int32_t trigger)
{
    if (trigger < 0)
        return {};

    int status = (trigger >> 8) & 0xFF;
    int data1 = trigger & 0xFF;
    int channel = (status & 0x0F) + 1;
    int type = status & 0xF0;

    juce::String typeName;
    if (type == 0x90)
        typeName = juce::MidiMessage::getMidiNoteName(data1, true, true, 3);
    else if (type == 0xB0)
        typeName = "CC " + juce::String(data1);
    else if (type == 0xC0)
        typeName = "Prog " + juce::String(data1);
    else
        typeName = juce::String::toHexString(status) + ":" + juce::String::toHexString(data1);

    return "Ch " + juce::String(channel) + " " + typeName;
}

juce::String MutanderAudioProcessorEditor::formatTriggers(std::function<int32_t(int)> getter, int count)
{
    juce::StringArray lines;
    for (int i = 0; i < count; ++i)
    {
        auto text = formatTrigger(getter(i));
        if (text.isNotEmpty())
            lines.add(text);
    }
    return lines.isEmpty() ? "--" : lines.joinIntoString("\n");
}

void MutanderAudioProcessorEditor::updateButtons()
{
    bool muted = audioProcessor_.isMuted();
    int learning = audioProcessor_.midiLearnTarget_.load(std::memory_order_relaxed);

    stopButton_.setSelected(muted);
    stopButton_.setLearning(learning == 0);
    stopButton_.setText(formatTriggers(
        [this](int i) { return audioProcessor_.getStopTrigger(i); },
        MutanderAudioProcessor::kMaxTriggers));

    goButton_.setSelected(!muted);
    goButton_.setLearning(learning == 1);
    goButton_.setText(formatTriggers(
        [this](int i) { return audioProcessor_.getGoTrigger(i); },
        MutanderAudioProcessor::kMaxTriggers));
}

//==============================================================================
void MutanderAudioProcessorEditor::paint(juce::Graphics& g)
{
    g.fillAll(getLookAndFeel().findColour(juce::ResizableWindow::backgroundColourId));
}

void MutanderAudioProcessorEditor::resized()
{
    auto area = getLocalBounds().reduced(16);
    auto half = area.getHeight() / 2;

    stopButton_.setBounds(area.removeFromTop(half).reduced(0, 8));
    goButton_.setBounds(area.reduced(0, 8));
}
