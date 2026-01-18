#include "midi_device.hpp"
#include <cxxopts.hpp>
#include <string>
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
        ("p,port", "Select source port as client:port (e.g., 24:0)", cxxopts::value<std::string>())
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

    if (result.count("port")) {
        std::string chosen_port = result["port"].as<std::string>();
        auto devices = pr::midi::enumerate_midi_sources();
        for (const auto &device : devices) {
            std::string possible_port = fmt::format("{}:{}", device.client_id, device.port_id);
            if (chosen_port == possible_port) {
                spdlog::info("Selected: {}", fmt::streamed(device));
            }
        }
    }

    return 0;
}
