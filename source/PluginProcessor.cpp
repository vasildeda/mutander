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
             .withInput("Input 1", juce::AudioChannelSet::stereo(), true)
             .withInput("Input 2", juce::AudioChannelSet::stereo(), true)
             .withInput("Input 3", juce::AudioChannelSet::stereo(), true)
             .withInput("Input 4", juce::AudioChannelSet::stereo(), true)
             .withInput("Input 5", juce::AudioChannelSet::stereo(), true)
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
    return 1;   // NB: some hosts don't cope very well if you tell them there are 0 programs,
                // so this should be at least 1, even if you're not really implementing programs.
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
    // When playback stops, you can use this as an opportunity to free up any
    // spare memory, etc.
}

#ifndef JucePlugin_PreferredChannelConfigurations
bool MutanderAudioProcessor::isBusesLayoutSupported(const BusesLayout& layouts) const
{
    // This is the place where you check if the layout is supported.
    // In this template code we only support mono or stereo.
    // Some plugin hosts, such as certain GarageBand versions, will only
    // load plugins that support stereo bus layouts.
    if (layouts.getMainOutputChannelSet() != juce::AudioChannelSet::mono()
     && layouts.getMainOutputChannelSet() != juce::AudioChannelSet::stereo())
        return false;

    // This checks if the input layout matches the output layout
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
        if (target >= 0 && target < 5)
        {
            midiTriggers_[target].store(packMidiForMatch(*msg), std::memory_order_relaxed);
            midiLearnTarget_.store(-1, std::memory_order_relaxed);
            triggerAsyncUpdate();
        }
        else
        {
            for (int i = 0; i < 5; ++i)
            {
                auto stored = midiTriggers_[i].load(std::memory_order_relaxed);
                if (midiMatches(*msg, stored))
                {
                    crossFader_.requestBus(i);
                    selectedBus_.store(i, std::memory_order_relaxed);
                    triggerAsyncUpdate();
                    break;
                }
            }
        }
    }
}

void MutanderAudioProcessor::processBuffer(juce::AudioBuffer<float>& buffer)
{
    auto out = getBusBuffer(buffer, false, 0);
    const int numSamples = out.getNumSamples();

    for (int s = 0; s < numSamples; ++s)
    {
        auto [bus, gain] = crossFader_.getNextState();
        auto in = getBusBuffer(buffer, true, bus);

        for (int ch = 0; ch < juce::jmin(in.getNumChannels(), out.getNumChannels()); ++ch)
            out.setSample(ch, s, in.getSample(ch, s) * gain);
    }
}

//==============================================================================
bool MutanderAudioProcessor::hasEditor() const
{
    return true; // (change this to false if you choose to not supply an editor)
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

    auto* triggers = xml->createNewChildElement("triggers");
    for (int i = 0; i < 5; ++i)
    {
        auto* trigger = triggers->createNewChildElement("trigger");
        trigger->addTextElement(juce::String(midiTriggers_[i].load(std::memory_order_relaxed)));
    }

    copyXmlToBinary(*xml, destData);
}

void MutanderAudioProcessor::setStateInformation(const void* data, int sizeInBytes)
{
    auto xml = getXmlFromBinary(data, sizeInBytes);

    if (xml != nullptr && xml->hasTagName("Mutander"))
    {
        // int version = xml->getIntAttribute("version", 1);

        if (auto* triggers = xml->getChildByName("triggers"))
        {
            int i = 0;
            for (auto* trigger : triggers->getChildIterator())
            {
                if (i >= 5)
                    break;
                midiTriggers_[i].store(trigger->getAllSubText().getIntValue(),
                                       std::memory_order_relaxed);
                ++i;
            }
        }

        triggerAsyncUpdate();
    }
}

//==============================================================================
int32_t MutanderAudioProcessor::getMidiTrigger(int bus) const
{
    if (bus < 0 || bus >= 5)
        return kUnassignedTrigger;
    return midiTriggers_[bus].load(std::memory_order_relaxed);
}

void MutanderAudioProcessor::clearMidiTrigger(int bus)
{
    if (bus >= 0 && bus < 5)
        midiTriggers_[bus].store(kUnassignedTrigger, std::memory_order_relaxed);
}

void MutanderAudioProcessor::selectBus(int bus)
{
    if (bus < 0 || bus >= 5)
        return;
    crossFader_.requestBus(bus);
    selectedBus_.store(bus, std::memory_order_relaxed);
    triggerAsyncUpdate();
}

void MutanderAudioProcessor::handleAsyncUpdate()
{
    if (onStateChanged)
        onStateChanged();
}

//==============================================================================
// This creates new instances of the plugin..
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new MutanderAudioProcessor();
}
