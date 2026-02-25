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
class PluginProcessor : public juce::AudioProcessor,
                                 public juce::AsyncUpdater
{
public:
    //==============================================================================
    PluginProcessor();
    ~PluginProcessor() override;

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
    // Callback when state changes
    std::function<void()> onStateChanged;

    // Read trigger value for display (returns -1 if unassigned)
    int32_t getStopTrigger(int slot) const;
    int32_t getGoTrigger(int slot) const;

    // Clear all triggers for a button (0=stop, 1=go)
    void clearTriggers(int button);

    // Access muted state from GUI
    bool isMuted() const;
    void setMuted(bool muted);

    // MIDI learn: -1 = off, 0 = learning stop, 1 = learning go
    int getMidiLearnTarget() const;
    void setMidiLearnTarget(int target);

    void handleAsyncUpdate() override;

    static constexpr int kMaxTriggers = 5;

private:
    //==============================================================================
    MidiDebouncer midiDebouncer_;
    CrossFader crossFader_;
    // MIDI learn: -1 = off, 0 = learning stop, 1 = learning go
    std::atomic<int> midiLearnTarget_ { -1 };
    std::atomic<bool> muted_ { false };

    // MIDI triggers. -1 means unassigned.
    // Packed as (status << 8) | data1, ignoring velocity/value.
    static constexpr int32_t kUnassignedTrigger = -1;
    std::array<std::atomic<int32_t>, 5> stopTriggers_ {
        kUnassignedTrigger, kUnassignedTrigger, kUnassignedTrigger,
        kUnassignedTrigger, kUnassignedTrigger
    };
    std::array<std::atomic<int32_t>, 5> goTriggers_ {
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
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(PluginProcessor)
};
