/*
  ==============================================================================

    This file contains the basic framework code for a JUCE plugin processor.

  ==============================================================================
*/

#pragma once

#include "CrossFader.h"
#include "MidiDebouncer.h"
#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_core/juce_core.h>
#include <array>
#include <atomic>

//==============================================================================
class SwitchanderAudioProcessor : public juce::AudioProcessor,
                                 public juce::AsyncUpdater
{
public:
    //==============================================================================
    SwitchanderAudioProcessor();
    ~SwitchanderAudioProcessor() override;

    //==============================================================================
    void prepareToPlay(double sampleRate, int samplesPerBlock) override;
    void releaseResources() override;

   #ifndef JucePlugin_PreferredChannelConfigurations
    bool isBusesLayoutSupported(const BusesLayout& layouts) const override;
   #endif

    void processBlock(juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    //==============================================================================
    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override;

    //==============================================================================
    const juce::String getName() const override;

    bool acceptsMidi() const override;
    bool producesMidi() const override;
    bool isMidiEffect() const override;
    double getTailLengthSeconds() const override;

    //==============================================================================
    int getNumPrograms() override;
    int getCurrentProgram() override;
    void setCurrentProgram(int index) override;
    const juce::String getProgramName(int index) override;
    void changeProgramName(int index, const juce::String& newName) override;

    //==============================================================================
    void getStateInformation(juce::MemoryBlock& destData) override;
    void setStateInformation(const void* data, int sizeInBytes) override;

    //==============================================================================
    // MIDI learn: -1 = not learning, 0-4 = learning for that bus
    std::atomic<int> midiLearnTarget_ { -1 };

    // Currently selected bus (0-4)
    std::atomic<int> selectedBus_ { 0 };

    // Callback when state changes (bus selected or MIDI learned)
    std::function<void()> onStateChanged;

    // Read trigger value for display (returns -1 if unassigned)
    int32_t getMidiTrigger(int bus) const;

    // Clear a MIDI trigger assignment
    void clearMidiTrigger(int bus);

    // Select a bus (from GUI click)
    void selectBus(int bus);

    void handleAsyncUpdate() override;

private:
    //==============================================================================
    MidiDebouncer midiDebouncer_;
    CrossFader crossFader_;

    // MIDI triggers for each bus (0-4). -1 means unassigned.
    // Packed as (status << 8) | data1, ignoring velocity/value.
    static constexpr int32_t kUnassignedTrigger = -1;
    std::array<std::atomic<int32_t>, 5> midiTriggers_ {
        kUnassignedTrigger, kUnassignedTrigger, kUnassignedTrigger,
        kUnassignedTrigger, kUnassignedTrigger
    };

    // Pack MIDI message for matching (ignores velocity/value)
    static int32_t packMidiForMatch(const juce::MidiMessage& msg);

    // Check if incoming message matches a stored trigger
    static bool midiMatches(const juce::MidiMessage& incoming, int32_t stored);

    //==============================================================================
    void handleMidi(const juce::MidiBuffer& midi);
    void processBuffer(juce::AudioBuffer<float>& buffer);

    //==============================================================================
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(SwitchanderAudioProcessor)
};
