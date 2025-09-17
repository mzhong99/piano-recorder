cmake_minimum_required(VERSION 3.15)
project(PianoRecorder VERSION 0.1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(CURL REQUIRED)

# JUCE
add_subdirectory(external/JUCE)
add_subdirectory(external/CLI11)
add_subdirectory(external/spdlog)

# Main executable
add_executable(piano-recorder src/main.cpp)

target_include_directories(piano-recorder
    PRIVATE
        include
        external/CLI11/include
        external/spdlog/include
)

target_link_libraries(piano-recorder
    PRIVATE
        juce::juce_audio_basics
        juce::juce_audio_devices
        juce::juce_audio_formats
        juce::juce_core
        CLI11::CLI11
        CURL::libcurl
)

