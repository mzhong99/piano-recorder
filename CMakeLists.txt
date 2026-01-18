cmake_minimum_required(VERSION 3.20)
project(piano_recorder VERSION 0.1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

option(PR_ENABLE_WARNINGS "Enable compiler warnings" ON)

# third party libraries from submodules
add_subdirectory(third_party/cxxopts EXCLUDE_FROM_ALL)
add_subdirectory(third_party/cpp-httplib EXCLUDE_FROM_ALL)
add_subdirectory(third_party/spdlog EXCLUDE_FROM_ALL)

# midifile tells you to copy and paste + statically compile stuff yourself so that's what we do
add_library(midifile STATIC
  third_party/midifile/src/MidiFile.cpp
  third_party/midifile/src/MidiEvent.cpp
  third_party/midifile/src/MidiMessage.cpp
)

target_include_directories(midifile PUBLIC
    ${CMAKE_SOURCE_DIR}/third_party/midifile/include
)

add_library(midifile::midifile ALIAS midifile)

# ALSA via pkg-config (system dependency)
find_package(PkgConfig REQUIRED)
pkg_check_modules(ALSA REQUIRED IMPORTED_TARGET alsa)

add_executable(piano-recorder
    src/main.cpp
    src/midi_device.cpp
    src/alsa_midi_in.cpp
)

target_include_directories(piano-recorder
    PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src
)

# Link dependencies
target_link_libraries(piano-recorder PRIVATE
    PkgConfig::ALSA
    cxxopts
    httplib::httplib
    spdlog::spdlog_header_only
    midifile::midifile
)

# Optional warnings
if(PR_ENABLE_WARNINGS)
    include(cmake/warnings.cmake)
    pr_apply_warnings(piano-recorder)
endif()

