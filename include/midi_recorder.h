#pragma once
#include "simple_recorder.h"
#include <RtMidi.h>

namespace piano_recorder {

class MidiRecorder : public SimpleRecorder
{
public:
    MidiRecorder() = default;

    bool start_recording(const std::filesystem::path &base_path) override;
    void stop_recording() override;

private:
    RtMidiIn _rt_midi;
    double _start_time{0.0};
};

} // namespace piano_recorder

