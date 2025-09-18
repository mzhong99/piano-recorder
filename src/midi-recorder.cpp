#include "midi-recorder.h"
#include "device-utils.h"

namespace piano_recorder {

MidiRecorder::MidiRecorder()
    : start_time(0.0)
{
}

bool MidiRecorder::start_recording(const std::filesystem::path& file_path)
{
    stop_recording();

    output_path = file_path;
    juce::File file(file_path.string());
    file.deleteFile();

    output_stream = std::make_unique<juce::FileOutputStream>(file);
    if (!output_stream->openedOk()) {
        return false;
    }

    midi_file = juce::MidiFile();
    midi_file.setTicksPerQuarterNote(960); // standard PPQ
    midi_sequence = juce::MidiMessageSequence();

    start_time = juce::Time::getMillisecondCounterHiRes() * 0.001;
    recording_in_progress = true;

    return true;
}

void MidiRecorder::stop_recording()
{
    if (!recording_in_progress) {
        return;
    }

    // add the sequence to the MidiFile
    midi_file.addTrack(midi_sequence);

    if (output_stream) {
        midi_file.writeTo(*output_stream);
        output_stream->flush();
        output_stream.reset();
    }

    recording_in_progress = false;
}

void MidiRecorder::handleIncomingMidiMessage(juce::MidiInput* /*source*/,
                                             const juce::MidiMessage& message)
{
    if (!recording_in_progress) {
        return;
    }

    // timestamp relative to recording start
    double now = juce::Time::getMillisecondCounterHiRes() * 0.001;
    double time_seconds = now - start_time;

    juce::MidiMessage msg_copy = message;
    msg_copy.setTimeStamp(time_seconds);

    midi_sequence.addEvent(msg_copy);
}

std::filesystem::path MidiRecorder::get_output_file() const
{
    return output_path;
}

} // namespace piano_recorder

