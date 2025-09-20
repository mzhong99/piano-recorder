#pragma once
#include "simple-recorder.h"
#include <juce_audio_devices/juce_audio_devices.h>
#include <juce_events/juce_events.h>

namespace piano_recorder {

class MidiRecorder : public SimpleRecorder, public juce::MidiInputCallback
{
public:
    MidiRecorder() = default;
    virtual ~MidiRecorder() = default;

    bool start_recording(const std::filesystem::path &base_path) override;
    void stop_recording() override;

    void handleIncomingMidiMessage(juce::MidiInput* source, const juce::MidiMessage& message) override;

private:
    std::unique_ptr<juce::FileOutputStream> _output_stream;
    juce::MidiFile _midi_file;
    juce::MidiMessageSequence _midi_sequence;

    double _start_time{0.0};
};

} // namespace piano_recorder

