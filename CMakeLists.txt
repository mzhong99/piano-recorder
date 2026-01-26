cmake_minimum_required(VERSION 3.20)
project(piano_recorder VERSION 0.1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

option(PR_ENABLE_WARNINGS "Enable compiler warnings" ON)
add_compile_options("-fdiagnostics-color=always")
add_compile_options("-Wno-missing-field-initializers")

# third party libraries from submodules
add_subdirectory(third_party/cxxopts EXCLUDE_FROM_ALL)
add_subdirectory(third_party/cpp-httplib EXCLUDE_FROM_ALL)
add_subdirectory(third_party/spdlog EXCLUDE_FROM_ALL)
add_subdirectory(third_party/magic_enum EXCLUDE_FROM_ALL)
add_subdirectory(third_party/PlatformFolders EXCLUDE_FROM_ALL)

# midifile tells you to copy and paste + statically compile stuff yourself so that's what we do
add_library(midifile STATIC
    third_party/midifile/src/Binasc.cpp
    third_party/midifile/src/MidiFile.cpp
    third_party/midifile/src/MidiEvent.cpp
    third_party/midifile/src/MidiEventList.cpp
    third_party/midifile/src/MidiMessage.cpp
    third_party/midifile/src/Options.cpp
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
    src/midi_recorder.cpp
    src/midi_device.cpp
    src/alsa_sequencer.cpp
)

target_include_directories(piano-recorder
    PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src
    PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/third_party/PlatformFolders
)

# Link dependencies
target_link_libraries(piano-recorder PRIVATE
    PkgConfig::ALSA
    cxxopts
    httplib::httplib
    spdlog::spdlog_header_only
    midifile::midifile
    magic_enum::magic_enum
    sago::platform_folders
)

install(TARGETS piano-recorder RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})

set(CPACK_PACKAGE_NAME "piano-recorder")
set(CPACK_PACKAGE_VENDOR "Tarediiran Industries")
set(CPACK_PACKAGE_CONTACT "tarediiran@gmail.com")
set(CPACK_PACKAGE_DESCRIPTION_SUMMARY "System utilities for recording midi and audio sources")
set(CPACK_OUTPUT_FILE_PREFIX "${CMAKE_BINARY_DIR}/dist")

set(CPACK_PACKAGE_VERSION ${PROJECT_VERSION})

set(CPACK_DEBIAN_PACKAGE_DEPENDS "libasound2 (>= 1.0.25), libc6 (>= 2.31)")
set(CPACK_RPM_PACKAGE_REQUIRES "alsa-lib >= 1.0.25, glibc >= 2.31")

include(CPack)

# Optional warnings
if(PR_ENABLE_WARNINGS)
    include(cmake/warnings.cmake)
    pr_apply_warnings(piano-recorder)
endif()

