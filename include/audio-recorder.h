#pragma once
#include "simple-recorder.h"
#include <juce_audio_utils/juce_audio_utils.h>

#include <memory>
#include <atomic>

namespace piano_recorder {

class AudioRecorder : public SimpleRecorder, public juce::AudioIODeviceCallback
{
public:
    AudioRecorder();
    bool start_recording(const std::filesystem::path &base_path) override;
    void stop_recording(void) override;

    // JUCE callbacks
    void audioDeviceIOCallbackWithContext(
        const float* const* input_channel_data, int num_input_channels,
        float* const* output_channel_data, int num_output_channels,
        int num_samples, const juce::AudioIODeviceCallbackContext& context) override;

    void audioDeviceAboutToStart(juce::AudioIODevice *device) override;
    void audioDeviceStopped(void) override {};

private:
    juce::File _output_file;
    double _sample_rate;

    std::unique_ptr<juce::AudioFormatWriter> _writer;
    std::unique_ptr<juce::AudioFormatWriter::ThreadedWriter> _threaded_writer;

    juce::TimeSliceThread _background_thread;
    std::atomic<juce::AudioFormatWriter::ThreadedWriter*> _active_writer;
};

} // namespace piano_recorder

