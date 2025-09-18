#include <spdlog/fmt/fmt.h>
#include <CLI/CLI.hpp>

#include "device-utils.h"

#include <iostream>
#include <string>

#include <spdlog/spdlog.h>

void print_devices(void)
{
    std::cout << "Audio Input Devices:" << std::endl;
    std::vector<std::string> audio_names = piano_recorder::get_audio_device_names();
    for (const std::string &name : audio_names) {
        std::cout << " - " << name << std::endl;
    }

    std::cout << "MIDI Input Devices:" << std::endl;
    std::vector<std::string> midi_names = piano_recorder::get_midi_device_names();
    for (const std::string &name : midi_names) {
        std::cout << " - " << name << std::endl;
    }
}

int record_audio(const std::string &audio_device, const std::string &midi_device, const std::string &output_base)
{
    spdlog::info("Start recording: audio='{}', midi='{}', output_base='{}'", audio_device, midi_device, output_base);

    return EXIT_SUCCESS;
}

int main(int argc, char **argv)
{
    CLI::App app{fmt::format("{} - record audio and/or MIDI", argv[0])};

    std::string output_base;
    std::string audio_device;
    std::string midi_device;

    bool list_devices = false;

    app.add_option("-o,--output", output_base, "Base output file name (without extension)");
    app.add_flag("-l,--list-devices", list_devices, "List available audio and MIDI input devices");

    app.add_option("-a,--audio", audio_device, "Audio input device name");
    app.add_option("-m,--midi", midi_device, "MIDI input device name");

    try {
        CLI11_PARSE(app, argc, argv);
    } catch (const CLI::ParseError &e) {
        return app.exit(e);
    }

    if (list_devices) {
        print_devices();
        return EXIT_SUCCESS;
    }

    if (audio_device.empty() && midi_device.empty()) {
        std::cerr << "At least one of --audio or --midi must be specified." << std::endl;
        return EXIT_FAILURE;
    }

    if (output_base.empty()) {
        std::cerr << "--output must be specified." << std::endl;
        return EXIT_FAILURE;
    }

    return record_audio(audio_device, midi_device, output_base);
}

