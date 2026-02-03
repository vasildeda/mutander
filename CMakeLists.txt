cmake_minimum_required(VERSION 3.22)

project(Swichander VERSION 1.0.0)

# Add JUCE
add_subdirectory(libs/juce)

# Create a JUCE plugin
juce_add_plugin(Swichander
    COMPANY_NAME "Vasilovo"
    PLUGIN_MANUFACTURER_CODE VSLV
    PLUGIN_CODE Swch
    FORMATS AU VST3 Standalone
    PRODUCT_NAME "Swichander"
    PLUGIN_NAME "Swichander"
    VERSION 1.0.0
    IS_SYNTH FALSE
    NEEDS_MIDI_INPUT TRUE
    NEEDS_MIDI_OUTPUT FALSE
    IS_MIDI_EFFECT FALSE
    EDITOR_WANTS_KEYBOARD_FOCUS FALSE
    COPY_PLUGIN_AFTER_BUILD TRUE
    PLUGIN_MANUFACTURER "Vasilovo"
)

# Add source files
target_sources(Swichander PRIVATE
    Source/CrossFader.cpp
    Source/LongPressButton.cpp
    Source/MidiDebouncer.cpp
    Source/PluginProcessor.cpp
    Source/PluginEditor.cpp
)

# Add JUCE modules
target_link_libraries(Swichander PRIVATE
    juce::juce_audio_basics
    juce::juce_audio_devices
    juce::juce_audio_formats
    juce::juce_audio_processors
    juce::juce_audio_utils
    juce::juce_core
    juce::juce_data_structures
    juce::juce_events
    juce::juce_graphics
    juce::juce_gui_basics
    juce::juce_gui_extra
)

# Set C++ standard
target_compile_features(Swichander PUBLIC cxx_std_17)


# Disable VST2 to avoid parameter automation conflict with VST3
target_compile_definitions(Swichander PRIVATE
    JUCE_VST3_CAN_REPLACE_VST2=0
)
