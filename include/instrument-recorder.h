#pragma once
#include <juce_audio_utils/juce_audio_utils.h>

#include <memory>
#include <atomic>
#include <vector>
#include <filesystem>

#include "audio-recorder.h"
#include "midi-recorder.h"

namespace piano_recorder {

class InstrumentRecorder
{
public:
    InstrumentRecorder(std::weak_ptr<juce::AudioDeviceManager> device_manager, const std::filesystem::path &path = "");
    bool set_audio_source(const std::string &name);
    bool set_midi_source(const std::string &name);
    bool set_output_path(const std::filesystem::path &path);

    bool start_recording(void);
    void stop_recording(void);

private:
    std::weak_ptr<juce::AudioDeviceManager> _device_manager;
    AudioRecorder _audio_recorder;
    MidiRecorder _midi_recorder;

    std::atomic<bool> _running{false};

    std::string _audio_source_name;
    std::string _midi_source_name;

    std::filesystem::path _output_base_path;
};

} // namespace piano_recorder

