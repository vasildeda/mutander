#pragma once

#include <juce_audio_basics/juce_audio_basics.h>

class MidiDebouncer
{
public:
    /** Initialize the debouncer */
    void prepare(double sampleRate, int samplesPerBlock, int ignoreTimeMs);

    /** Call this every block, returns pointer to first allowed MIDI message */
    std::optional<juce::MidiMessage> processBlock(const juce::MidiBuffer& midi);

private:
    int samplesPerBlock = 1;
    juce::int64 ignoreSamples = 0;     // number of samples to ignore after first message
    juce::int64 samplesSinceLast = 0;  // tracks how many samples passed since last allowed
};
