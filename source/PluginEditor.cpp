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
    background_ = juce::Drawable::createFromImageData(BinaryData::background_svg,
                                                      BinaryData::background_svgSize);

    for (int i = 0; i < 5; ++i)
    {
        channelButtons_[i].setText("--");
        channelButtons_[i].onClick = [this, i] {
            if (audioProcessor_.midiLearnTarget_.load(std::memory_order_relaxed) == i)
            {
                audioProcessor_.midiLearnTarget_.store(-1, std::memory_order_relaxed);
                updateChannelButtons();
            }
            else
            {
                audioProcessor_.selectBus(i);
            }
        };
        channelButtons_[i].onLongPress = [this, i] {
            audioProcessor_.clearMidiTrigger(i);
            audioProcessor_.midiLearnTarget_.store(i, std::memory_order_relaxed);
            updateChannelButtons();
        };
        addAndMakeVisible(channelButtons_[i]);
    }

    // Register callback for processor → GUI updates
    audioProcessor_.onStateChanged = [this] {
        updateChannelButtons();
    };

    // Initial state
    updateChannelButtons();

    setSize(704, 396);
}

MutanderAudioProcessorEditor::~MutanderAudioProcessorEditor()
{
    audioProcessor_.onStateChanged = nullptr;
}

void MutanderAudioProcessorEditor::updateChannelButtons()
{
    int selectedBus = audioProcessor_.selectedBus_.load(std::memory_order_relaxed);
    int learningBus = audioProcessor_.midiLearnTarget_.load(std::memory_order_relaxed);

    for (int i = 0; i < 5; ++i)
    {
        channelButtons_[i].setSelected(i == selectedBus);
        channelButtons_[i].setLearning(i == learningBus);

        // Update text based on trigger assignment
        int32_t trigger = audioProcessor_.getMidiTrigger(i);
        if (trigger < 0)
        {
            channelButtons_[i].setText("--");
        }
        else
        {
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

            channelButtons_[i].setText("Ch " + juce::String(channel) + "\n" + typeName);
        }
    }
}

//==============================================================================
void MutanderAudioProcessorEditor::paint(juce::Graphics& g)
{
    g.fillAll(getLookAndFeel().findColour(juce::ResizableWindow::backgroundColourId));

    if (background_)
        background_->drawWithin(g, getLocalBounds().toFloat(),
                                juce::RectanglePlacement::centred, 1.0f);
}

void MutanderAudioProcessorEditor::resized()
{
    using Track = juce::Grid::TrackInfo;
    using Fr = juce::Grid::Fr;

    constexpr float gap = 16.0f;

    juce::Grid grid;
    grid.templateRows = { Track(Fr(1)), Track(Fr(1)) };
    grid.templateColumns = { Track(Fr(1)), Track(Fr(1)), Track(Fr(1)),
                             Track(Fr(1)), Track(Fr(1)) };
    grid.columnGap = juce::Grid::Px(gap);
    grid.rowGap = juce::Grid::Px(gap);

    grid.items = {
        juce::GridItem(channelButtons_[0]).withColumn({ 1, 2 }).withRow({ 2, 3 }),
        juce::GridItem(channelButtons_[1]).withColumn({ 2, 3 }).withRow({ 2, 3 }),
        juce::GridItem(channelButtons_[2]).withColumn({ 3, 4 }).withRow({ 2, 3 }),
        juce::GridItem(channelButtons_[3]).withColumn({ 4, 5 }).withRow({ 2, 3 }),
        juce::GridItem(channelButtons_[4]).withColumn({ 5, 6 }).withRow({ 2, 3 })
    };

    auto area = getLocalBounds().reduced(static_cast<int>(gap));
    grid.performLayout(area);
}
