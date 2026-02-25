#include "CrossFader.h"

void CrossFader::prepare(double sampleRate, int fadeTimeMs, float initialGain)
{
    fader_.reset(sampleRate, fadeTimeMs / 1000.0);
    fader_.setCurrentAndTargetValue(initialGain);
}

void CrossFader::mute()
{
    fader_.setTargetValue(0.0f);
}

void CrossFader::unmute()
{
    fader_.setTargetValue(1.0f);
}

float CrossFader::getNextGain()
{
    return fader_.getNextValue();
}
