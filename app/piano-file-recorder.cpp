#include <spdlog/fmt/fmt.h>
#include <spdlog/spdlog.h>
#include <cxxopts.hpp>

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
    for (size_t i = 0; i < audio_names.size(); i++) {
        std::cout << i << " - " << audio_names[i] << std::endl;
    }

    std::cout << "MIDI Input Devices:" << std::endl;
    std::vector<std::string> midi_names = context.get_midi_device_names();
    for (size_t i = 0; i < midi_names.size(); i++) {
        std::cout << i << " - " << midi_names[i] << std::endl;
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
    spdlog::set_pattern("[%H:%M:%S.%e] [%t] %s:%# %! %v");

    cxxopts::Options options("piano_recorder", "Record audio and MIDI inputs to disk");
    options.add_options()
        ("o,output", "Base output file name (without extension)", cxxopts::value<std::string>())
        ("l,list", "List available audio / MIDI input devices", cxxopts::value<bool>()->default_value("false"))
        ("a,audio", "Audio input device index", cxxopts::value<int>())
        ("m,midi", "MIDI input device index", cxxopts::value<int>())
        ("h,help", "Print usage");

    cxxopts::ParseResult result;
    try {
        result = options.parse(argc, argv);
    } catch (const std::exception &e) {
        std::cout << options.help() << std::endl;
        return EXIT_FAILURE;
    }

    if (result.count("help")) {
        std::cout << options.help() << std::endl;
        return EXIT_SUCCESS;
    }

    int audio_idx = result.count("audio") == 1 ? result["audio"].as<int>() : -1;
    int midi_idx = result.count("midi") == 1 ? result["midi"].as<int>() : -1;

    if (audio_idx == -1 && midi_idx == -1) {
        std::cerr << "At least one of --audio or --midi must be specified." << std::endl;
        return EXIT_FAILURE;
    }

    if (result.count("output") == 0) {
        std::cerr << "--output must be specified." << std::endl;
        return EXIT_FAILURE;
    }

    piano_recorder::JuceAudioContext juce_context{2, 0};
    spdlog::info("Starting juce context");
    if (result["list"].as<bool>()) {
        print_devices(juce_context);
        return EXIT_SUCCESS;
    }

    piano_recorder::InstrumentRecorder recorder(juce_context.get_device_manager());

    std::string output_base = result["output"].as<std::string>();
    recorder.set_output_path(output_base);

    if (audio_idx != -1) {
        recorder.set_audio_source(audio_idx);
    }
    if (midi_idx != -1) {
        recorder.set_midi_source(midi_idx);
    }

    // return record_audio(recorder);
    return 0;
}

