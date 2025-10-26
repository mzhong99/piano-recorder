#include <spdlog/fmt/fmt.h>
#include <spdlog/spdlog.h>
#include <cxxopts.hpp>

#include "instrument_recorder.h"

#include <iostream>
#include <vector>
#include <string>
#include <memory>

#include <spdlog/spdlog.h>

using namespace piano_recorder;

void print_devices(InstrumentRecorder &recorder)
{
    std::vector<RtAudio::DeviceInfo> audio_choices = recorder.get_audio_device_choices();
    for (size_t i = 0; i < audio_choices.size(); i++) {
        RtAudio::DeviceInfo info = audio_choices[i];
        spdlog::info(" {} - {} (fmt=0b{:08b}) ({} inputs)", info.ID, info.name, info.nativeFormats, info.inputChannels);
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

    cxxopts::Options options(argv[0], "Record audio and MIDI inputs to disk");
    options.add_options()
        ("o,output", "Base output file name (without extension)", cxxopts::value<std::string>())
        ("l,list", "List available audio / MIDI input devices", cxxopts::value<bool>()->default_value("false"))
        ("a,audio", "Audio input device index", cxxopts::value<std::string>())
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

    piano_recorder::InstrumentRecorder recorder;
    if (result["list"].as<bool>()) {
        print_devices(recorder);
        return EXIT_SUCCESS;
    }

    if (result.count("output") == 0) {
        std::cerr << "--output must be specified." << std::endl;
        return EXIT_FAILURE;
    }

    std::string audio_name = result["audio"].as<std::string>();
    std::string output_base = result["output"].as<std::string>();

    recorder.set_output_path(output_base);
    recorder.set_audio_device(audio_name);

    return record_audio(recorder);
}

