#include "instrument_recorder.h"
#include <sstream>
#include <spdlog/spdlog.h>

namespace piano_recorder {

InstrumentRecorder::InstrumentRecorder(const std::filesystem::path &path) {}

bool InstrumentRecorder::start_recording(void)
{
    if (_running) {
        spdlog::error("start_recording() attempted to be called while already running");
        return false;
    }

    if (!_audio.start_recording(_output_base_path)) {
        return false;
    }

    if (!_midi.start_recording(_output_base_path)) {
        return false;
    }

    _running = true;
    return true;
}

void InstrumentRecorder::stop_recording(void)
{
    if (!_running) {
        return;
    }

    _running = false;
}

bool InstrumentRecorder::set_output_path(const std::filesystem::path &path)
{
    if (_running) {
        return false;
    }

    _output_base_path = path;
    spdlog::info("Set output base: '{}'", _output_base_path.string());
    return true;
}

bool InstrumentRecorder::set_audio_device(const std::string &device_name)
{
    if (_running) {
        return false;
    }


    std::vector<RtAudio::DeviceInfo> options = get_audio_device_choices();
    for (const RtAudio::DeviceInfo &info: options) {
        if (info.name.find(device_name) == 0) {
            _audio.set_device_id(info.ID);
            spdlog::info("Set input device to {}: {}", info.ID, info.name);
            return true;
        }
    }

    return false;
}

}  // namespace piano_recorder
