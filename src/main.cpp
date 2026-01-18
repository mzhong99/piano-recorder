#include "midi_device.hpp"
#include <cxxopts.hpp>
#include <httplib.h>
#include <iostream>
#include <spdlog/fmt/ostr.h>
#include <spdlog/spdlog.h>
#include <spdlog/version.h>

#include <alsa/asoundlib.h>

int main(int argc, char **argv) {
    // clang-format off
    cxxopts::Options options("piano-recorder", "MIDI recorder prototype");
    options.add_options()
        ("l,list", "List ALSA sequencer clients/ports (placeholder)")
        ("V,version", "Print library versions")
        ("h,help", "Print help");
    // clang-format on

    auto result = options.parse(argc, argv);
    if (result["help"].as<bool>()) {
        std::cout << options.help() << "\n";
        return 0;
    }

    if (result["list"].as<bool>()) {
        auto devices = pr::midi::enumerate_midi_sources();
        for (const auto &device : devices) {
            std::cout << fmt::format("Device: {}", fmt::streamed(device)) << std::endl;
        }
    }

    if (result["version"].as<bool>()) {
        spdlog::info("{}, using the following libs:", argv[0]);
        spdlog::info("    spdlog: {}.{}.{}", SPDLOG_VER_MAJOR, SPDLOG_VER_MINOR, SPDLOG_VER_PATCH);
        spdlog::info("    httplib: {}", CPPHTTPLIB_VERSION);
        spdlog::info("    alsa: {}", SND_LIB_VERSION_STR);
    }

    // Prove httplib compiles: create a server object (not started)
    httplib::Server server;
    (void)server;

    return 0;
}
