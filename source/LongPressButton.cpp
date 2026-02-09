#include "LongPressButton.h"

LongPressButton::LongPressButton()
{
}

void LongPressButton::setText(const juce::String& text)
{
    text_ = text;
    repaint();
}

void LongPressButton::setSelected(bool selected)
{
    selected_ = selected;
    repaint();
}

void LongPressButton::setLearning(bool learning)
{
    learning_ = learning;
    repaint();
}

void LongPressButton::setActiveColour(juce::Colour colour)
{
    activeColour_ = colour;
    repaint();
}

void LongPressButton::paint(juce::Graphics& g)
{
    auto bounds = getLocalBounds().toFloat().reduced(2.0f);

    // Background
    g.setColour(juce::Colours::black);
    g.fillRoundedRectangle(bounds, 6.0f);

    // Outline based on state
    if (learning_)
        g.setColour(juce::Colours::yellow);
    else if (selected_)
        g.setColour(activeColour_);
    else
        g.setColour(juce::Colours::grey);

    g.drawRoundedRectangle(bounds, 16.0f, 8.0f);

    // Text
    g.setColour(juce::Colours::white);
    g.setFont(28.0f);
    g.drawFittedText(text_, getLocalBounds(), juce::Justification::centred, 2);
}

void LongPressButton::mouseDown(const juce::MouseEvent& e)
{
    mouseDownTime_ = juce::Time::getMillisecondCounterHiRes();
    isMouseDown_ = true;
    startTimer(10);
}

void LongPressButton::mouseUp(const juce::MouseEvent& e)
{
    stopTimer();

    if (!isMouseDown_)
        return;  // long press already triggered

    auto duration = juce::Time::getMillisecondCounterHiRes() - mouseDownTime_;

    if (duration >= kLongPressThreshold_)
    {
        if (onLongPress)
            onLongPress();
    }
    else
    {
        if (onClick)
            onClick();
    }

    isMouseDown_ = false;
}

void LongPressButton::timerCallback()
{
    if (!isMouseDown_)
        return;

    auto duration = juce::Time::getMillisecondCounterHiRes() - mouseDownTime_;
    if (duration >= kLongPressThreshold_)
    {
        stopTimer();
        isMouseDown_ = false;
        if (onLongPress)
            onLongPress();
    }
}
