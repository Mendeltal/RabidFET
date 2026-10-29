cmake_minimum_required(VERSION 3.22)
project(RabidFET VERSION 0.1.0)

add_subdirectory(JUCE)

juce_add_plugin(RabidFET
    COMPANY_NAME "Tal Mendel"
    PLUGIN_MANUFACTURER_CODE Ynam
    PLUGIN_CODE Rfet
    FORMATS VST3 AU Standalone
    PRODUCT_NAME "Rabid FET"
    IS_SYNTH FALSE
    NEEDS_MIDI_INPUT FALSE
    NEEDS_MIDI_OUTPUT FALSE)

target_sources(RabidFET PRIVATE
    RabidFET/Source/PluginProcessor.cpp
    RabidFET/Source/PluginEditor.cpp)

target_compile_definitions(RabidFET PUBLIC
    JUCE_WEB_BROWSER=0 JUCE_USE_CURL=0 JUCE_VST3_CAN_REPLACE_VST2=0)

target_link_libraries(RabidFET
    PRIVATE juce::juce_audio_utils juce::juce_dsp
    PUBLIC juce::juce_recommended_config_flags juce::juce_recommended_warning_flags)
