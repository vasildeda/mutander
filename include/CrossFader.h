#pragma once

#include "juce_audio_basics/juce_audio_basics.h"

class CrossFader
{
public:
    void prepare(double sampleRate, int fadeTimeMs);
    void requestBus(int newBus);
    float getNextValue();

    int getCurrentBus() { return currentBus; }
    int getTargetBus() { return targetBus; }

private:
    int currentBus = 0;
    int targetBus = 0;
    int requestedBus = 0;

    juce::SmoothedValue<float, juce::ValueSmoothingTypes::Linear> fader;

    void updateBusesWhenPossible();
};
