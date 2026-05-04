cmake_minimum_required(VERSION 3.22)
project(MetallicaTone VERSION 1.0.0)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# ── JUCE ─────────────────────────────────────────────────────────────────────
add_subdirectory("C:/stringotter/JUCE_8"
                 ${CMAKE_CURRENT_BINARY_DIR}/JUCE)

# ── NeuralAmpModelerCore ──────────────────────────────────────────────────────
set(NAM_CORE_PATH  "C:/stringotter/NeuralAmpModelerCore")
set(NAM_DEPS_PATH  "${NAM_CORE_PATH}/Dependencies")
set(ADSP_PATH      "${NAM_DEPS_PATH}/AudioDSPTools/dsp")

file(GLOB_RECURSE NAM_SOURCES "${NAM_CORE_PATH}/NAM/*.cpp")
set(ADSP_SOURCES
    "${ADSP_PATH}/dsp.cpp"
    "${ADSP_PATH}/ImpulseResponse.cpp"
    "${ADSP_PATH}/wav.cpp"
)

add_library(NAMCore STATIC ${NAM_SOURCES} ${ADSP_SOURCES})
target_include_directories(NAMCore SYSTEM PUBLIC
    ${NAM_CORE_PATH}
    ${NAM_DEPS_PATH}/eigen
    ${NAM_DEPS_PATH}/nlohmann
    ${ADSP_PATH}
)
set_target_properties(NAMCore PROPERTIES POSITION_INDEPENDENT_CODE ON)
if(MSVC)
    target_compile_options(NAMCore PRIVATE /W0)
else()
    target_compile_options(NAMCore PRIVATE -w)
endif()

# ── Plugin formats ────────────────────────────────────────────────────────────
if(APPLE)
    set(PLUGIN_FORMATS AU VST3 Standalone)
else()
    set(PLUGIN_FORMATS VST3 Standalone)
endif()

juce_add_plugin(MetallicaTone
    COMPANY_NAME             "MetallicaTone"
    IS_SYNTH                 FALSE
    NEEDS_MIDI_INPUT         FALSE
    NEEDS_MIDI_OUTPUT        FALSE
    IS_MIDI_EFFECT           FALSE
    EDITOR_WANTS_KEYBOARD_FOCUS FALSE
    COPY_PLUGIN_AFTER_BUILD  TRUE
    PLUGIN_MANUFACTURER_CODE Mtal
    PLUGIN_CODE              Mtal
    FORMATS                  ${PLUGIN_FORMATS}
    PRODUCT_NAME             "MetallicaTone"
    MICROPHONE_PERMISSION_ENABLED TRUE
    MICROPHONE_PERMISSION_TEXT "MetallicaTone needs audio input for your guitar."
)

# ── Embedded assets ───────────────────────────────────────────────────────────
juce_add_binary_data(MetallicaBinaryData
    SOURCES
        Resources/DethLead.nam
        Resources/sm57.wav
)

# ── Sources ───────────────────────────────────────────────────────────────────
target_sources(MetallicaTone PRIVATE
    Source/PluginProcessor.cpp
    Source/PluginEditor.cpp
)

target_include_directories(MetallicaTone PRIVATE
    ${NAM_CORE_PATH}
    ${NAM_DEPS_PATH}/eigen
    ${NAM_DEPS_PATH}/nlohmann
    ${ADSP_PATH}
)
target_include_directories(MetallicaTone SYSTEM PRIVATE
    ${NAM_CORE_PATH}
    ${NAM_DEPS_PATH}/eigen
    ${NAM_DEPS_PATH}/nlohmann
    ${ADSP_PATH}
)

target_compile_definitions(MetallicaTone PUBLIC
    JUCE_WEB_BROWSER=0
    JUCE_USE_CURL=0
    JUCE_VST3_CAN_REPLACE_VST2=0
    JUCE_DISPLAY_SPLASH_SCREEN=0
)

target_link_libraries(MetallicaTone
    PRIVATE
        MetallicaBinaryData
        $<$<CXX_COMPILER_ID:MSVC>:-WHOLEARCHIVE:NAMCore>
        NAMCore
        juce::juce_audio_utils
        juce::juce_audio_processors
        juce::juce_audio_formats
        juce::juce_dsp
        juce::juce_gui_basics
    PUBLIC
        juce::juce_recommended_config_flags
        juce::juce_recommended_lto_flags
        juce::juce_recommended_warning_flags
)
