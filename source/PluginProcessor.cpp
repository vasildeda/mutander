/*
  ==============================================================================

    This file contains the basic framework code for a JUCE plugin processor.

  ==============================================================================
*/

#include "PluginProcessor.h"
#include "PluginEditor.h"

//==============================================================================
PluginProcessor::PluginProcessor()
#ifndef JucePlugin_PreferredChannelConfigurations
     : AudioProcessor(
         BusesProperties()
             .withInput("Input", juce::AudioChannelSet::stereo(), true)
             .withOutput("Output", juce::AudioChannelSet::stereo(), true)
       )
#endif
{
}

PluginProcessor::~PluginProcessor()
{
}

//==============================================================================
const juce::String PluginProcessor::getName() const
{
    return JucePlugin_Name;
}

bool PluginProcessor::acceptsMidi() const
{
    return true;
}

bool PluginProcessor::producesMidi() const
{
    return false;
}

bool PluginProcessor::isMidiEffect() const
{
    return false;
}

double PluginProcessor::getTailLengthSeconds() const
{
    return 0.0;
}

int PluginProcessor::getNumPrograms()
{
    return 1;
}

int PluginProcessor::getCurrentProgram()
{
    return 0;
}

void PluginProcessor::setCurrentProgram(int index)
{
}

const juce::String PluginProcessor::getProgramName(int index)
{
    return {};
}

void PluginProcessor::changeProgramName(int index, const juce::String& newName)
{
}

//==============================================================================
void PluginProcessor::prepareToPlay(double sampleRate, int samplesPerBlock)
{
    midiDebouncer_.prepare(sampleRate, samplesPerBlock, 10);
    crossFader_.prepare(sampleRate, 50, isMuted() ? 0.0f : 1.0f);
}

void PluginProcessor::releaseResources()
{
}

#ifndef JucePlugin_PreferredChannelConfigurations
bool PluginProcessor::isBusesLayoutSupported(const BusesLayout& layouts) const
{
    if (layouts.getMainOutputChannelSet() != juce::AudioChannelSet::mono()
     && layouts.getMainOutputChannelSet() != juce::AudioChannelSet::stereo())
        return false;

   #if ! JucePlugin_IsSynth
    if (layouts.getMainOutputChannelSet() != layouts.getMainInputChannelSet())
        return false;
   #endif

    return true;
}
#endif

void PluginProcessor::processBlock(juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midiMessages)
{
    juce::ScopedNoDenormals noDenormals;

    handleMidi(midiMessages);
    processBuffer(buffer);
}

int32_t PluginProcessor::packMidiForMatch(const juce::MidiMessage& msg)
{
    const auto* data = msg.getRawData();
    return (static_cast<int32_t>(data[0]) << 8) | static_cast<int32_t>(data[1]);
}

bool PluginProcessor::midiMatches(const juce::MidiMessage& incoming, int32_t stored)
{
    if (stored == kUnassignedTrigger)
        return false;

    return packMidiForMatch(incoming) == stored;
}

void PluginProcessor::handleMidi(const juce::MidiBuffer& midi)
{
    if (auto msg = midiDebouncer_.processBlock(midi))
    {
        int target = midiLearnTarget_.load(std::memory_order_relaxed);
        if (target == 0 || target == 1)
        {
            // Learning mode: fill next empty slot
            auto& triggers = (target == 0) ? stopTriggers_ : goTriggers_;
            auto& oppositeTriggers = (target == 0) ? goTriggers_ : stopTriggers_;
            auto packed = packMidiForMatch(*msg);

            // Remove from opposite set if already assigned there
            for (int i = 0; i < kMaxTriggers; ++i)
            {
                if (oppositeTriggers[i].load(std::memory_order_relaxed) == packed)
                    oppositeTriggers[i].store(kUnassignedTrigger, std::memory_order_relaxed);
            }

            for (int i = 0; i < kMaxTriggers; ++i)
            {
                if (triggers[i].load(std::memory_order_relaxed) == kUnassignedTrigger)
                {
                    triggers[i].store(packed, std::memory_order_relaxed);
                    // Exit learn mode if that was the last slot
                    if (i == kMaxTriggers - 1)
                        midiLearnTarget_.store(-1, std::memory_order_relaxed);
                    triggerAsyncUpdate();
                    return;
                }
            }
        }
        else
        {
            // Normal mode: check stop triggers first (priority), then go
            for (int i = 0; i < kMaxTriggers; ++i)
            {
                if (midiMatches(*msg, stopTriggers_[i].load(std::memory_order_relaxed)))
                {
                    setMuted(true);
                    return;
                }
            }
            for (int i = 0; i < kMaxTriggers; ++i)
            {
                if (midiMatches(*msg, goTriggers_[i].load(std::memory_order_relaxed)))
                {
                    setMuted(false);
                    return;
                }
            }
        }
    }
}

void PluginProcessor::processBuffer(juce::AudioBuffer<float>& buffer)
{
    const int numChannels = buffer.getNumChannels();
    const int numSamples = buffer.getNumSamples();

    for (int s = 0; s < numSamples; ++s)
    {
        float gain = crossFader_.getNextGain();
        for (int ch = 0; ch < numChannels; ++ch)
            buffer.setSample(ch, s, buffer.getSample(ch, s) * gain);
    }
}

//==============================================================================
bool PluginProcessor::hasEditor() const
{
    return true;
}

juce::AudioProcessorEditor* PluginProcessor::createEditor()
{
    return new PluginEditor(*this);
}

//==============================================================================
void PluginProcessor::getStateInformation(juce::MemoryBlock& destData)
{
    auto xml = std::make_unique<juce::XmlElement>("Semaforte");
    xml->setAttribute("version", 1);

    auto* stopXml = xml->createNewChildElement("stopTriggers");
    for (int i = 0; i < kMaxTriggers; ++i)
    {
        auto* trigger = stopXml->createNewChildElement("trigger");
        trigger->addTextElement(juce::String(stopTriggers_[i].load(std::memory_order_relaxed)));
    }

    auto* goXml = xml->createNewChildElement("goTriggers");
    for (int i = 0; i < kMaxTriggers; ++i)
    {
        auto* trigger = goXml->createNewChildElement("trigger");
        trigger->addTextElement(juce::String(goTriggers_[i].load(std::memory_order_relaxed)));
    }

    xml->setAttribute("muted", isMuted());

    copyXmlToBinary(*xml, destData);
}

void PluginProcessor::setStateInformation(const void* data, int sizeInBytes)
{
    auto xml = getXmlFromBinary(data, sizeInBytes);

    if (xml != nullptr && xml->hasTagName("Semaforte"))
    {
        if (auto* stopXml = xml->getChildByName("stopTriggers"))
        {
            int i = 0;
            for (auto* trigger : stopXml->getChildIterator())
            {
                if (i >= kMaxTriggers)
                    break;
                stopTriggers_[i].store(trigger->getAllSubText().getIntValue(),
                                       std::memory_order_relaxed);
                ++i;
            }
        }

        if (auto* goXml = xml->getChildByName("goTriggers"))
        {
            int i = 0;
            for (auto* trigger : goXml->getChildIterator())
            {
                if (i >= kMaxTriggers)
                    break;
                goTriggers_[i].store(trigger->getAllSubText().getIntValue(),
                                     std::memory_order_relaxed);
                ++i;
            }
        }

        setMuted(xml->getBoolAttribute("muted", false));
    }
}

//==============================================================================
int32_t PluginProcessor::getStopTrigger(int slot) const
{
    if (slot < 0 || slot >= kMaxTriggers)
        return kUnassignedTrigger;
    return stopTriggers_[slot].load(std::memory_order_relaxed);
}

int32_t PluginProcessor::getGoTrigger(int slot) const
{
    if (slot < 0 || slot >= kMaxTriggers)
        return kUnassignedTrigger;
    return goTriggers_[slot].load(std::memory_order_relaxed);
}

void PluginProcessor::clearTriggers(int button)
{
    auto& triggers = (button == 0) ? stopTriggers_ : goTriggers_;
    for (int i = 0; i < kMaxTriggers; ++i)
        triggers[i].store(kUnassignedTrigger, std::memory_order_relaxed);
}

bool PluginProcessor::isMuted() const
{
    return muted_.load(std::memory_order_relaxed);
}

void PluginProcessor::setMuted(bool muted)
{
    muted_.store(muted, std::memory_order_relaxed);
    if (muted)
        crossFader_.mute();
    else
        crossFader_.unmute();
    triggerAsyncUpdate();
}

int PluginProcessor::getMidiLearnTarget() const
{
    return midiLearnTarget_.load(std::memory_order_relaxed);
}

void PluginProcessor::setMidiLearnTarget(int target)
{
    midiLearnTarget_.store(target, std::memory_order_relaxed);
}

void PluginProcessor::handleAsyncUpdate()
{
    if (onStateChanged)
        onStateChanged();
}

//==============================================================================
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new PluginProcessor();
}
