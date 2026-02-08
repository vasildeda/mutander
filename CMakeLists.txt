cmake_minimum_required(VERSION 3.22)

project(Switchander VERSION 1.0.0)

# Add JUCE
add_subdirectory(libs/juce)

# Create a JUCE plugin
juce_add_plugin(Switchander
    COMPANY_NAME "Vasilovo"
    PLUGIN_MANUFACTURER_CODE VSLV
    PLUGIN_CODE Swch
    FORMATS AU VST3 Standalone
    PRODUCT_NAME "Switchander"
    PLUGIN_NAME "Switchander"
    VERSION 1.0.0
    IS_SYNTH FALSE
    NEEDS_MIDI_INPUT TRUE
    NEEDS_MIDI_OUTPUT FALSE
    IS_MIDI_EFFECT FALSE
    EDITOR_WANTS_KEYBOARD_FOCUS FALSE
    COPY_PLUGIN_AFTER_BUILD TRUE
    PLUGIN_MANUFACTURER "Vasilovo"
)

# Add resources
juce_add_binary_data(SwitchanderResources SOURCES
    resources/background.svg
)

# Add source files
target_sources(Switchander PRIVATE
    source/CrossFader.cpp
    source/LongPressButton.cpp
    source/MidiDebouncer.cpp
    source/PluginProcessor.cpp
    source/PluginEditor.cpp
)

# Add headers
target_include_directories(Switchander PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/include
    ${CMAKE_CURRENT_SOURCE_DIR}/source
)

# Add JUCE modules
target_link_libraries(Switchander PRIVATE
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
    SwitchanderResources
)

# Set C++ standard
target_compile_features(Switchander PUBLIC cxx_std_17)


# Disable VST2 to avoid parameter automation conflict with VST3
target_compile_definitions(Switchander PRIVATE
    JUCE_VST3_CAN_REPLACE_VST2=0
)
