#include "MidiDebouncer.h"
#include <optional>

void MidiDebouncer::prepare(double sampleRate, int samplesPerBlock, int ignoreTimeMs)
{
    this->samplesPerBlock = samplesPerBlock;
    ignoreSamples = static_cast<juce::int64>(sampleRate * ignoreTimeMs * 0.001);
    printf("Ignore samples: %llD\n", ignoreSamples);
    samplesSinceLast = ignoreSamples;
}

std::optional<juce::MidiMessage> MidiDebouncer::processBlock(const juce::MidiBuffer& midi)
{
    for (const auto metadata : midi)
    {
        int samplePos = metadata.samplePosition;

        // accumulate sample counter across blocks
        juce::int64 samplesElapsed = samplesSinceLast + samplePos;

        if (samplesElapsed >= ignoreSamples)
        {
            printf("Samples elapsed: %lld\n", samplesElapsed);
            samplesSinceLast = 0; // reset after accepting message
            return metadata.getMessage();
        }
    }

    // nothing allowed this block
    samplesSinceLast += midi.getNumEvents() > 0 ? midi.getLastEventTime() : samplesPerBlock;
    return std::nullopt;
}
