#pragma once
#include "simple_recorder.h"
#include "spsc_ring_buffer.h"
#include <RtAudio.h>
#include <rigtorp/MPMCQueue.h>

#include <memory>
#include <atomic>

namespace piano_recorder {

class AudioRecorder : public SimpleRecorder
{
public:
    AudioRecorder(
        uint32_t device_id = 0, uint32_t channels = 2, uint32_t sample_rate = 44100, uint32_t buflen_ms = 1000);

    bool start_recording(const std::filesystem::path &base_path) override;
    void stop_recording(void) override;

    bool set_device_id(uint32_t device_id) {
        if (_running) {
            return false;
        }

        _device_id = device_id;
        return true;
    }

    std::vector<RtAudio::DeviceInfo> get_device_choices(void);

private:
    uint32_t _device_id;
    uint32_t _channels;
    uint32_t _sample_rate;
    uint32_t _buflen_ms;

    unsigned int _buflen_nsamp;
    size_t _buflen_size_bytes;
    size_t _buflen_sample_size;

    RtAudio _rt_audio;
    std::unique_ptr<rigtorp::MPMCQueue<uint8_t>> _audio_buffer;

    constexpr unsigned int compute_buflen(void) {
        return _sample_rate * _buflen_ms;
    }

    void add_samples(const void *buffer_in, size_t num_samples);

    static int receive_frames(
        void *out_vp, void *in_vp, unsigned int nsamp, double elapsed_sec, RtAudioStreamStatus status, void *ctx);
};

} // namespace piano_recorder

