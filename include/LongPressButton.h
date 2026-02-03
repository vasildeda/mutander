#pragma once

#include <juce_gui_basics/juce_gui_basics.h>  // Component, DrawableButton, Timer
#include <juce_graphics/juce_graphics.h>      // Drawable
#include <functional>                          // std::function

/**
 * LongPressButton
 * A reusable SVG-based button that detects short clicks and long presses.
 * Supports lambda callbacks.
 */
class LongPressButton : public juce::DrawableButton,
                        private juce::Timer
{
public:
    enum class LongPressMode
    {
        TriggerOnRelease,   // long press triggers on mouse release
        TriggerDuringHold   // long press triggers automatically after threshold
    };

    /**
     * @param name                button name
     * @param normalSVGData       pointer to SVG binary data (BinaryData::xxx_svg)
     * @param normalSVGSize       size of the SVG binary data
     * @param pressedSVGData      optional SVG for pressed state
     * @param pressedSVGSize      size of pressed SVG
     * @param longPressThresholdMs milliseconds to trigger long press
     * @param mode                trigger mode for long press
     */
    LongPressButton(const juce::String& name,
                    const void* normalSVGData, int normalSVGSize,
                    const void* pressedSVGData = nullptr, int pressedSVGSize = 0,
                    double longPressThresholdMs = 800.0,
                    LongPressMode mode = LongPressMode::TriggerOnRelease);

    ~LongPressButton() override = default;

    void resized() override;

    // Lambda callbacks
    std::function<void()> onClick;
    std::function<void()> onLongPress;

protected:
    void mouseDown(const juce::MouseEvent& e) override;
    void mouseUp(const juce::MouseEvent& e) override;

private:
    juce::DrawableButton button;
    double mouseDownTime { 0.0 };
    bool isMouseDown { false };
    double longPressThreshold;
    LongPressMode longPressMode;

    void timerCallback() override;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(LongPressButton)
};
