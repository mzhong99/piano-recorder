#pragma once
#include "simple-recorder.h"
#include <juce_audio_utils/juce_audio_utils.h>

#include <memory>
#include <atomic>

namespace piano_recorder {

class AudioRecorder : public SimpleRecorder, private juce::AudioIODeviceCallback
{
public:
    AudioRecorder();

    bool start_recording(const std::filesystem::path& file) override;
    void stop_recording() override;
    bool is_recording() const override;
    std::filesystem::path get_output_file() const override;

    // JUCE callbacks
    void audioDeviceIOCallbackWithContext(
        const float* const* input_channel_data, int num_input_channels,
        float* const* output_channel_data, int num_output_channels,
        int num_samples, const juce::AudioIODeviceCallbackContext& context) override;

    void audioDeviceAboutToStart(juce::AudioIODevice* device) override;
    void audioDeviceStopped() override;

private:
    juce::File output_file;
    juce::AudioBuffer<float> audio_buffer;
    double sample_rate;

    std::unique_ptr<juce::AudioFormatWriter> writer;
    std::unique_ptr<juce::AudioFormatWriter::ThreadedWriter> threaded_writer;

    juce::TimeSliceThread background_thread;
    std::atomic<juce::AudioFormatWriter::ThreadedWriter*> active_writer;
};

} // namespace piano_recorder

