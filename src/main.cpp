#include <iostream>
#include <cxxopts.hpp>
#include <httplib.h>
#include <spdlog/spdlog.h>
#include <spdlog/fmt/ostr.h>
#include <spdlog/version.h>
#include "midi_device.hpp"

#include <alsa/asoundlib.h>

int main(int argc, char** argv) {
    cxxopts::Options options("piano-recorder", "MIDI recorder prototype");
    options.add_options()
        ("l,list", "List ALSA sequencer clients/ports (placeholder)")
        ("h,help", "Print help");

    auto result = options.parse(argc, argv);
    if (result["help"].as<bool>()) {
        std::cout << options.help() << "\n";
        return 0;
    }

    if (result["list"].as<bool>()) {
        auto devices = pr::midi::enumerate_midi_sources();
        for (const auto &device : devices) {
            spdlog::info("Device: {}", fmt::streamed(device));
        }
    }

    // Prove ALSA link works: print library version string macro
    spdlog::info("spdlog: {}.{}.{}", SPDLOG_VER_MAJOR, SPDLOG_VER_MINOR, SPDLOG_VER_PATCH);
    spdlog::info("httplib: {}", CPPHTTPLIB_VERSION);
    spdlog::info("alsa: {}", SND_LIB_VERSION_STR);

    // Prove httplib compiles: create a server object (not started)
    httplib::Server server;
    (void)server;

    return 0;
}

