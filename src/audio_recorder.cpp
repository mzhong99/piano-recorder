#include "audio_recorder.h"
#include "device_utils.h"
#include <spdlog/fmt/ranges.h>
#include <spdlog/spdlog.h>

namespace piano_recorder {

AudioRecorder::AudioRecorder(uint32_t device_id, uint32_t channels, uint32_t sample_rate, uint32_t buflen_ms) :
    _device_id(device_id), _channels(channels), _sample_rate(sample_rate), _buflen_ms(buflen_ms)
{
    compute_buflen();
}

bool AudioRecorder::start_recording(const std::filesystem::path& base_path)
{
    if (_running.load(std::memory_order_acquire)) {
        spdlog::error("AudioRecorder is already running");
        return true;
    }

    _output_path = base_path;
    _output_path.replace_extension(".wav");

    _audio_buffer = std::make_unique<rigtorp::MPMCQueue<uint8_t>>(1);

    RtAudio::DeviceInfo info = _rt_audio.getDeviceInfo(_device_id);
    RtAudio::StreamParameters params{_device_id, info.inputChannels};

    RtAudio::StreamOptions option{.flags = RTAUDIO_HOG_DEVICE};

    spdlog::info("sample rates: {}, prefer {}", info.sampleRates, info.preferredSampleRate);
    _rt_audio.openStream(
        nullptr, &params, RTAUDIO_SINT24, info.preferredSampleRate, &_buflen_nsamp, &AudioRecorder::receive_frames, this);
    _rt_audio.startStream();

    _running.store(true, std::memory_order_release);

    return true;
}

void AudioRecorder::stop_recording(void)
{
    _running.store(false, std::memory_order_release);
}

int AudioRecorder::receive_frames(
    void *out_vp, void *in_vp, unsigned int nsamp, double elapsed_sec, RtAudioStreamStatus status, void *ctx)
{
    AudioRecorder *self = static_cast<AudioRecorder *>(ctx);

    // self->add_samples(in_vp, nsamp)
    // int32_t *samples = static_cast<int32_t *>(in_vp);

    // for (unsigned int i = 0; i < nsamp; i++) {
    //     while (self->_audio_buffer.size() >= self->_buflen_nsamp) {
    //         float discard = 0;
    //         self->_audio_buffer.pop(discard);
    //     }
    //     self->_audio_buffer.push(samples[i]);
    // }
    const int32_t *samples = static_cast<const int32_t *>(in_vp);
    size_t nonzero = 0;
    for (size_t i = 0; i < nsamp; i++) {
        if (samples[i] != 0) {
            nonzero++;
        }
    }
    spdlog::info("Received {} frames, err={}, nz={}", nsamp, status, nonzero);

    return 0;
}

void AudioRecorder::add_samples(const void *buffer_in, size_t num_samples)
{
//     // pop stale data so that you can push more stuff in
//     size_t push_size = num_samples * _buflen_sample_size;
// 
//     size_t bytes_before = _audio_buffer.size();
//     size_t bytes_after = _audio_buffer.size() + push_size;
//     if (bytes_after > _buflen_size_bytes) {
//         size_t overshoot = bytes_after - _buflen_size_bytes;
//         for (size_t i = 0; i < overshoot; i++) {
//             uint8_t discard;
//             _audio_buffer.pop(discard);
//         }
//     }
// 
//     // now, actually push new data
//     const uint8_t *buffer_u8 = static_cast<const uint8_t *>(buffer_in);
//     for (size_t i = 0; i < push_size; i++) {
//         _audio_buffer.push(buffer_u8[i]);
//     }
}

std::vector<RtAudio::DeviceInfo> AudioRecorder::get_device_choices(void)
{
    std::vector<unsigned int> device_ids = _rt_audio.getDeviceIds();
    std::vector<RtAudio::DeviceInfo> device_info;
    for (unsigned int device_id : device_ids) {
        RtAudio::DeviceInfo info = _rt_audio.getDeviceInfo(device_id);
        if (info.inputChannels > 0) {
            device_info.push_back(info);
        }
    }
    return device_info;
}

} // namespace piano_recorder

