#pragma once
#include "simple-recorder.h"
#include <juce_audio_devices/juce_audio_devices.h>
#include <juce_events/juce_events.h>

namespace piano_recorder {

class MidiRecorder : public SimpleRecorder, public juce::MidiInputCallback
{
public:
    MidiRecorder();
    ~MidiRecorder() override;

    bool start_recording(const std::filesystem::path& file_path) override;
    void stop_recording() override;
    bool is_recording() const override;
    std::filesystem::path get_output_file() const override;

    void handleIncomingMidiMessage(juce::MidiInput* source,
                                   const juce::MidiMessage& message) override;

private:
    std::filesystem::path output_path;
    std::unique_ptr<juce::FileOutputStream> output_stream;
    juce::MidiFile midi_file;
    juce::MidiMessageSequence midi_sequence;

    bool recording_in_progress{false};
    double start_time{0.0};
};

} // namespace piano_recorder

