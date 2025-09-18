#include "audio-recorder.h"
#include "device-utils.h"

namespace piano_recorder {

AudioRecorder::AudioRecorder() :
    sample_rate(0.0),
    background_thread("Audio Recorder Thread"),
    active_writer(nullptr)
{
    background_thread.startThread();
}

void AudioRecorder::audioDeviceIOCallbackWithContext(
    const float* const* input_channel_data, int num_input_channels,
    float* const* output_channel_data, int num_output_channels,
    int num_samples, const juce::AudioIODeviceCallbackContext& context)
{
    juce::ignoreUnused(output_channel_data, num_output_channels, num_input_channels);

    if (auto* writer_ptr = active_writer.load())
        writer_ptr->write(input_channel_data, num_samples);
}

bool AudioRecorder::start_recording(const std::filesystem::path& file_path)
{
    stop_recording();

    juce::File file(file_path.string());
    file.deleteFile();

    std::unique_ptr<juce::OutputStream> output_stream = std::make_unique<juce::FileOutputStream>(file);
    if (!dynamic_cast<juce::FileOutputStream *>(output_stream.get())->openedOk()) {
        return false;
    }

    juce::WavAudioFormat wav_format;
    juce::AudioFormatWriterOptions options = juce::AudioFormatWriterOptions()
            .withSampleRate(sample_rate > 0 ? sample_rate : 44100.0)
            .withNumChannels(2)
            .withBitsPerSample(16);
    auto writer = wav_format.createWriterFor(output_stream, options);
    if (writer == nullptr) {
        return false;
    }

    threaded_writer = std::make_unique<juce::AudioFormatWriter::ThreadedWriter>(
        writer.release(), background_thread, 32768);

    active_writer.store(threaded_writer.get());
    return true;
}

std::filesystem::path AudioRecorder::get_output_file() const
{
    return to_std_path(output_file);
}

} // namespace piano_recorder

