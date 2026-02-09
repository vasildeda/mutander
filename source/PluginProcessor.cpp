/*
  ==============================================================================

    This file contains the basic framework code for a JUCE plugin processor.

  ==============================================================================
*/

#include "PluginProcessor.h"
#include "PluginEditor.h"

//==============================================================================
MutanderAudioProcessor::MutanderAudioProcessor()
#ifndef JucePlugin_PreferredChannelConfigurations
     : AudioProcessor(
         BusesProperties()
             .withInput("Input", juce::AudioChannelSet::stereo(), true)
             .withOutput("Output", juce::AudioChannelSet::stereo(), true)
       )
#endif
{
}

MutanderAudioProcessor::~MutanderAudioProcessor()
{
}

//==============================================================================
const juce::String MutanderAudioProcessor::getName() const
{
    return JucePlugin_Name;
}

bool MutanderAudioProcessor::acceptsMidi() const
{
    return true;
}

bool MutanderAudioProcessor::producesMidi() const
{
    return false;
}

bool MutanderAudioProcessor::isMidiEffect() const
{
    return false;
}

double MutanderAudioProcessor::getTailLengthSeconds() const
{
    return 0.0;
}

int MutanderAudioProcessor::getNumPrograms()
{
    return 1;
}

int MutanderAudioProcessor::getCurrentProgram()
{
    return 0;
}

void MutanderAudioProcessor::setCurrentProgram(int index)
{
}

const juce::String MutanderAudioProcessor::getProgramName(int index)
{
    return {};
}

void MutanderAudioProcessor::changeProgramName(int index, const juce::String& newName)
{
}

//==============================================================================
void MutanderAudioProcessor::prepareToPlay(double sampleRate, int samplesPerBlock)
{
    midiDebouncer_.prepare(sampleRate, samplesPerBlock, 10);
    crossFader_.prepare(sampleRate, 50);
}

void MutanderAudioProcessor::releaseResources()
{
}

#ifndef JucePlugin_PreferredChannelConfigurations
bool MutanderAudioProcessor::isBusesLayoutSupported(const BusesLayout& layouts) const
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

void MutanderAudioProcessor::processBlock(juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midiMessages)
{
    juce::ScopedNoDenormals noDenormals;

    handleMidi(midiMessages);
    processBuffer(buffer);
}

int32_t MutanderAudioProcessor::packMidiForMatch(const juce::MidiMessage& msg)
{
    const auto* data = msg.getRawData();
    return (static_cast<int32_t>(data[0]) << 8) | static_cast<int32_t>(data[1]);
}

bool MutanderAudioProcessor::midiMatches(const juce::MidiMessage& incoming, int32_t stored)
{
    if (stored == kUnassignedTrigger)
        return false;

    // Ignore Note On with velocity 0 (treated as Note Off)
    if (incoming.isNoteOn() && incoming.getVelocity() == 0)
        return false;

    return packMidiForMatch(incoming) == stored;
}

void MutanderAudioProcessor::handleMidi(const juce::MidiBuffer& midi)
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
                    triggerAsyncUpdate();
                    return;
                }
            }
            // All slots full, exit learn mode
            midiLearnTarget_.store(-1, std::memory_order_relaxed);
            triggerAsyncUpdate();
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

void MutanderAudioProcessor::processBuffer(juce::AudioBuffer<float>& buffer)
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
bool MutanderAudioProcessor::hasEditor() const
{
    return true;
}

juce::AudioProcessorEditor* MutanderAudioProcessor::createEditor()
{
    return new MutanderAudioProcessorEditor(*this);
}

//==============================================================================
void MutanderAudioProcessor::getStateInformation(juce::MemoryBlock& destData)
{
    auto xml = std::make_unique<juce::XmlElement>("Mutander");
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

    copyXmlToBinary(*xml, destData);
}

void MutanderAudioProcessor::setStateInformation(const void* data, int sizeInBytes)
{
    auto xml = getXmlFromBinary(data, sizeInBytes);

    if (xml != nullptr && xml->hasTagName("Mutander"))
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

        triggerAsyncUpdate();
    }
}

//==============================================================================
int32_t MutanderAudioProcessor::getStopTrigger(int slot) const
{
    if (slot < 0 || slot >= kMaxTriggers)
        return kUnassignedTrigger;
    return stopTriggers_[slot].load(std::memory_order_relaxed);
}

int32_t MutanderAudioProcessor::getGoTrigger(int slot) const
{
    if (slot < 0 || slot >= kMaxTriggers)
        return kUnassignedTrigger;
    return goTriggers_[slot].load(std::memory_order_relaxed);
}

void MutanderAudioProcessor::clearTriggers(int button)
{
    auto& triggers = (button == 0) ? stopTriggers_ : goTriggers_;
    for (int i = 0; i < kMaxTriggers; ++i)
        triggers[i].store(kUnassignedTrigger, std::memory_order_relaxed);
}

void MutanderAudioProcessor::setMuted(bool muted)
{
    isMuted_.store(muted, std::memory_order_relaxed);
    if (muted)
        crossFader_.mute();
    else
        crossFader_.unmute();
    triggerAsyncUpdate();
}

bool MutanderAudioProcessor::isMuted() const
{
    return isMuted_.load(std::memory_order_relaxed);
}

void MutanderAudioProcessor::handleAsyncUpdate()
{
    if (onStateChanged)
        onStateChanged();
}

//==============================================================================
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new MutanderAudioProcessor();
}
