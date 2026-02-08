#include "CrossFader.h"

void CrossFader::prepare(double sampleRate, int fadeTimeMs)
{
    activeBus_ = requestedBus_ = 0;

    fader_.reset(sampleRate, fadeTimeMs / 1000.0);
    fader_.setCurrentAndTargetValue(1.0);
}

void CrossFader::requestBus(int requestedBus)
{
    requestedBus_ = requestedBus;
}

CrossFader::State CrossFader::getNextState()
{
    if (requestedBus_ != activeBus_ && !fader_.isSmoothing())
    {
        if (fader_.getCurrentValue() > 0.0f)
        {
            fader_.setTargetValue(0.0);
        }
        else
        {
            activeBus_ = requestedBus_;
            fader_.setTargetValue(1.0);
        }
    }

    return { activeBus_, fader_.getNextValue() };
}
