/*
  ==============================================================================

    This file contains the basic framework code for a JUCE plugin editor.

  ==============================================================================
*/

#include "PluginProcessor.h"
#include "PluginEditor.h"
#include "BinaryData.h"

//==============================================================================
PluginEditor::PluginEditor(PluginProcessor& p)
    : AudioProcessorEditor(&p), audioProcessor_(p),
      background_(juce::Drawable::createFromImageData(BinaryData::background_svg, BinaryData::background_svgSize))
{
    if (background_)
    {
        std::function<juce::Component*(juce::Component*, juce::StringRef)> findRecursive =
            [&](juce::Component* parent, juce::StringRef id) -> juce::Component* {
            for (auto* c : parent->getChildren())
            {
                if (c->getComponentID() == id)
                    return c;
                if (auto* found = findRecursive(c, id))
                    return found;
            }
            return nullptr;
        };
        titlePath_ = dynamic_cast<juce::DrawableShape*>(findRecursive(background_.get(), "text1"));
    }
    // Stop button (red)
    stopButton_.setActiveColour(juce::Colours::red);
    stopButton_.onClick = [this] {
        if (audioProcessor_.getMidiLearnTarget() >= 0)
            audioProcessor_.setMidiLearnTarget(-1);
        audioProcessor_.setMuted(true);
    };
    stopButton_.onLongPress = [this] {
        if (audioProcessor_.getMidiLearnTarget() == 0)
            audioProcessor_.setMidiLearnTarget(-1);
        else
        {
            audioProcessor_.clearTriggers(0);
            audioProcessor_.setMidiLearnTarget(0);
        }
        updateButtons();
    };
    addAndMakeVisible(stopButton_);

    // Go button (green)
    goButton_.setActiveColour(juce::Colours::green);
    goButton_.onClick = [this] {
        if (audioProcessor_.getMidiLearnTarget() >= 0)
            audioProcessor_.setMidiLearnTarget(-1);
        audioProcessor_.setMuted(false);
    };
    goButton_.onLongPress = [this] {
        if (audioProcessor_.getMidiLearnTarget() == 1)
            audioProcessor_.setMidiLearnTarget(-1);
        else
        {
            audioProcessor_.clearTriggers(1);
            audioProcessor_.setMidiLearnTarget(1);
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

    setSize(200, 400);
}

PluginEditor::~PluginEditor()
{
    audioProcessor_.onStateChanged = nullptr;
}

juce::String PluginEditor::formatTrigger(int32_t trigger)
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

juce::String PluginEditor::formatTriggers(std::function<int32_t(int)> getter, int count)
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

void PluginEditor::updateButtons()
{
    bool muted = audioProcessor_.isMuted();
    int learning = audioProcessor_.getMidiLearnTarget();

    stopButton_.setSelected(muted);
    stopButton_.setLearning(learning == 0);
    stopButton_.setText(formatTriggers(
        [this](int i) { return audioProcessor_.getStopTrigger(i); },
        PluginProcessor::kMaxTriggers));

    goButton_.setSelected(!muted);
    goButton_.setLearning(learning == 1);
    goButton_.setText(formatTriggers(
        [this](int i) { return audioProcessor_.getGoTrigger(i); },
        PluginProcessor::kMaxTriggers));

    if (titlePath_)
    {
        auto colour = learning >= 0 ? juce::Colours::yellow
                    : muted         ? juce::Colours::red
                                    : juce::Colours::green;
        titlePath_->setFill(colour);
        repaint();
    }
}

//==============================================================================
void PluginEditor::paint(juce::Graphics& g)
{
    if (background_)
        background_->drawWithin(g, getLocalBounds().toFloat(), juce::RectanglePlacement::stretchToFit, 1.0f);
    else
        g.fillAll(getLookAndFeel().findColour(juce::ResizableWindow::backgroundColourId));
}

void PluginEditor::resized()
{
    auto area = getLocalBounds().reduced(16);
    constexpr int buttonHeight = 160;
    constexpr int gap = 16;

    stopButton_.setBounds(area.removeFromTop(buttonHeight));
    area.removeFromTop(gap);
    goButton_.setBounds(area.removeFromTop(buttonHeight));
}
