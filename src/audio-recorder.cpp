#include "audio-recorder.h"
#include "device-utils.h"
#include <spdlog/spdlog.h>

namespace piano_recorder {

AudioRecorder::AudioRecorder() :
    _sample_rate(0.0), _background_thread("Audio Recorder Thread"), _active_writer(nullptr)
{
    _background_thread.startThread();
}

void AudioRecorder::audioDeviceIOCallbackWithContext(
    const float* const* input_channel_data, int num_input_channels,
    float* const* output_channel_data, int num_output_channels,
    int num_samples, const juce::AudioIODeviceCallbackContext& context)
{
    if (!_running) {
        return;
    }

    juce::AudioFormatWriter::ThreadedWriter *writer_ptr = _active_writer.load();
    if (writer_ptr != nullptr) {
        writer_ptr->write(input_channel_data, num_samples);
    }
}

void AudioRecorder::audioDeviceAboutToStart(juce::AudioIODevice* device)
{
    _sample_rate = device->getCurrentSampleRate();
}

bool AudioRecorder::start_recording(const std::filesystem::path& base_path)
{
    _output_path = base_path;
    _output_path.replace_extension(".wav");

    _output_file = juce::File(_output_path.string());
    _output_file.deleteFile();

    std::unique_ptr<juce::OutputStream> output_stream = std::make_unique<juce::FileOutputStream>(_output_file);
    if (!dynamic_cast<juce::FileOutputStream *>(output_stream.get())->openedOk()) {
        spdlog::error("OutputStream::openedOk() returned false");
        return false;
    }

    juce::WavAudioFormat wav_format;
    juce::AudioFormatWriterOptions options = juce::AudioFormatWriterOptions()
            .withSampleRate(_sample_rate > 0 ? _sample_rate : 44100.0)
            .withNumChannels(2)
            .withBitsPerSample(16);
    auto writer = wav_format.createWriterFor(output_stream, options);
    if (writer == nullptr) {
        spdlog::error("Could not create writer");
        return false;
    }

    _threaded_writer =
        std::make_unique<juce::AudioFormatWriter::ThreadedWriter>(_writer.release(), _background_thread, 32768);
    _active_writer.store(_threaded_writer.get());
    _running = true;

    return true;
}

void AudioRecorder::stop_recording(void)
{
    _writer.reset();
    _threaded_writer.reset();

    _running = false;
}

} // namespace piano_recorder

