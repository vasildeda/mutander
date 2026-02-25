#pragma once

#include "juce_audio_basics/juce_audio_basics.h"

class CrossFader
{
public:
    void prepare(double sampleRate, int fadeTimeMs, float initialGain = 1.0f);
    void mute();
    void unmute();
    float getNextGain();

private:
    juce::SmoothedValue<float, juce::ValueSmoothingTypes::Linear> fader_;
};
