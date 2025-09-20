#include "midi-recorder.h"
#include "device-utils.h"
#include <spdlog/spdlog.h>

namespace piano_recorder {

bool MidiRecorder::start_recording(const std::filesystem::path &file_path)
{
    _output_path = file_path;
    _output_path.replace_extension(".mid");

    juce::File file(_output_path.string());
    file.deleteFile();

    _output_stream = std::make_unique<juce::FileOutputStream>(file);
    if (!_output_stream->openedOk()) {
        spdlog::error("Could not create output stream");
        return false;
    }

    _midi_file = juce::MidiFile();
    _midi_file.setTicksPerQuarterNote(960); // standard PPQ
    _midi_sequence = juce::MidiMessageSequence();

    _start_time = juce::Time::getMillisecondCounterHiRes() * 0.001;

    _running = true;
    return true;
}

void MidiRecorder::stop_recording()
{
    if (!_running) {
        return;
    }

    // add the sequence to the MidiFile
    _midi_file.addTrack(_midi_sequence);

    if (_output_stream) {
        _midi_file.writeTo(*_output_stream);
        _output_stream->flush();
        _output_stream.reset();
    }

    _running = false;
}

void MidiRecorder::handleIncomingMidiMessage(juce::MidiInput *source, const juce::MidiMessage &message)
{
    if (!_running) {
        return;
    }

    // timestamp relative to recording start
    double now = juce::Time::getMillisecondCounterHiRes() * 0.001;
    double time_seconds = now - _start_time;

    juce::MidiMessage msg_copy = message;
    msg_copy.setTimeStamp(time_seconds);

    _midi_sequence.addEvent(msg_copy);
}

} // namespace piano_recorder

