#pragma once

#include <memory>
#include <atomic>
#include <string>
#include <vector>
#include <filesystem>

#include "audio_recorder.h"
#include "midi_recorder.h"

namespace piano_recorder {

class InstrumentRecorder
{
public:
    InstrumentRecorder(const std::filesystem::path &path = "");
    bool set_audio_device(const std::string &device_name);
    bool set_output_path(const std::filesystem::path &path);

    bool start_recording(void);
    void stop_recording(void);

    std::vector<RtAudio::DeviceInfo> get_audio_device_choices(void) {
        return _audio.get_device_choices();
    }

private:
    AudioRecorder _audio;
    MidiRecorder _midi;

    std::atomic<bool> _running{false};

    std::filesystem::path _output_base_path;
};

} // namespace piano_recorder

