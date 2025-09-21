#include <spdlog/fmt/fmt.h>
#include <spdlog/spdlog.h>
#include <CLI/CLI.hpp>

#include "juce-audio-context.h"
#include "instrument-recorder.h"

#include <iostream>
#include <vector>
#include <string>
#include <memory>

#include <spdlog/spdlog.h>

using namespace piano_recorder;

void print_devices(JuceAudioContext &context)
{
    std::cout << "Audio Input Devices:" << std::endl;
    std::vector<std::string> audio_names = context.get_audio_device_names();
    for (const std::string &name : audio_names) {
        std::cout << " - " << name << std::endl;
    }

    std::cout << "MIDI Input Devices:" << std::endl;
    std::vector<std::string> midi_names = context.get_midi_device_names();
    for (const std::string &name : midi_names) {
        std::cout << " - " << name << std::endl;
    }
}

int record_audio(InstrumentRecorder &recorder)
{
    recorder.start_recording();

    std::cout << "Press ENTER to stop recording..." << std::endl;
    std::cin.get();

    recorder.stop_recording();
    return EXIT_SUCCESS;
}

int main(int argc, char **argv)
{
    CLI::App app{fmt::format("{} - record audio and/or MIDI", argv[0])};
    spdlog::set_pattern("[%H:%M:%S.%e] [%t] %s:%# %! %v");

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

    piano_recorder::JuceAudioContext juce_context{2, 0};
    // piano_recorder::InstrumentRecorder recorder(juce_context.get_device_manager());

    if (list_devices) {
        print_devices(juce_context);
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

    // recorder.set_audio_source(audio_device);
    // recorder.set_midi_source(midi_device);
    // recorder.set_output_path(output_base);

    // return record_audio(recorder);
    return 0;
}

