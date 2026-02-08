#pragma once

#include "juce_audio_basics/juce_audio_basics.h"

class CrossFader
{
public:
    struct State
    {
        int bus;
        float gain;
    };

    void prepare(double sampleRate, int fadeTimeMs);
    void requestBus(int requestedBus);
    State getNextState();

private:
    int activeBus_ = 0;
    int requestedBus_ = 0;

    juce::SmoothedValue<float, juce::ValueSmoothingTypes::Linear> fader_;
};
