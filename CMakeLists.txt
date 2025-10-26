cmake_minimum_required(VERSION 3.15)
project(PianoRecorder VERSION 0.1.0 LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(SPDLOG_HEADER_ONLY ON CACHE BOOL "" FORCE)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wno-interference-size")

find_package(CURL REQUIRED)
find_package(Threads REQUIRED)

set(RTMIDI_BUILD_TESTING OFF)
set(RTAUDIO_BUILD_TESTING OFF)

add_subdirectory(external/rtaudio)
add_subdirectory(external/rtmidi)
add_subdirectory(external/cxxopts)
add_subdirectory(external/spdlog)

set(PIANO_RECORDER_COMMON_SOURCE
    src/audio_recorder.cpp
    src/midi_recorder.cpp
    src/instrument_recorder.cpp
)

set(PIANO_RECORDER_TEST_SRC
    tests/doctest_main.cpp
)

set(PIANO_RECORDER_EXTERNAL_LIBRARIES
    rtaudio
    rtmidi
    Threads::Threads
    cxxopts::cxxopts
    CURL::libcurl
    spdlog::spdlog_header_only
)

set(PIANO_RECORDER_INCLUDES
    include
    external/rtaudio
    external/rtmidi
    external/MPMCQueue/include
    external/cxxopts/include
    external/spdlog/include
)

# Main executable
add_library(piano-recorder-common ${PIANO_RECORDER_COMMON_SOURCE})
target_include_directories(piano-recorder-common PUBLIC ${PIANO_RECORDER_INCLUDES})
target_link_libraries(piano-recorder-common PUBLIC ${PIANO_RECORDER_EXTERNAL_LIBRARIES})

add_executable(piano-file-recorder app/piano-file-recorder.cpp)
target_include_directories(piano-file-recorder PRIVATE ${PIANO_RECORDER_INCLUDES})
target_link_libraries(piano-file-recorder PUBLIC piano-recorder-common)

add_executable(unittests ${PIANO_RECORDER_TEST_SRC})
target_link_libraries(unittests PUBLIC ${PIANO_RECORDER_EXTERNAL_LIBRARIES})
target_include_directories(unittests PRIVATE external/doctest/doctest ${PIANO_RECORDER_INCLUDES})

# Install rules
install(TARGETS
    # piano-recorder-common
    piano-file-recorder
    # piano-file-playback
    EXPORT piano-recorder-targets
    RUNTIME DESTINATION bin
    LIBRARY DESTINATION lib
    ARCHIVE DESTINATION lib
)

install(DIRECTORY include/ DESTINATION include)

# === Export + Config for find_package() ===
install(EXPORT piano-recorder-targets
    FILE piano-recorder-targets.cmake
    NAMESPACE piano::
    DESTINATION lib/cmake/piano-recorder
)

include(CPack)
set(CPACK_PACKAGE_NAME "piano-recorder")
set(CPACK_PACKAGE_VERSION ${PROJECT_VERSION})
set(CPACK_PACKAGE_CONTACT "Matthew Zhong <matthewzhong@logmethods.com>")
set(CPACK_DEBIAN_PACKAGE_MAINTAINER "Matthew Zhong") # required for DEB
