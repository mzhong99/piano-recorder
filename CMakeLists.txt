cmake_minimum_required(VERSION 3.15)
project(PianoRecorder VERSION 0.1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(CURL REQUIRED)

# JUCE
add_subdirectory(external/JUCE)
add_subdirectory(external/CLI11)
add_subdirectory(external/spdlog)

set(PIANO_RECORDER_COMMON_SOURCE
    src/device-utils.cpp
    src/audio-recorder.cpp
    src/midi-recorder.cpp
)

set(PIANO_RECORDER_EXTERNAL_LIBRARIES
    juce::juce_audio_basics
    juce::juce_audio_devices
    juce::juce_audio_formats
    juce::juce_core
    CLI11::CLI11
    CURL::libcurl
)

set(PIANO_RECORDER_INCLUDES
    include
    external/CLI11/include
    external/spdlog/include
)


# Main executable
add_library(piano-recorder-common ${PIANO_RECORDER_COMMON_SOURCE})
add_executable(piano-file-recorder app/piano-file-recorder.cpp)

target_include_directories(piano-recorder-common PUBLIC ${PIANO_RECORDER_INCLUDES})
target_link_libraries(piano-recorder-common PUBLIC ${PIANO_RECORDER_EXTERNAL_LIBRARIES})

target_include_directories(piano-file-recorder PRIVATE ${PIANO_RECORDER_INCLUDES})
target_link_libraries(piano-file-recorder PRIVATE piano-recorder-common)

