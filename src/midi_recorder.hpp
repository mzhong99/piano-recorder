#pragma once

#include <alsa/asoundlib.h>

#include <atomic>
#include <chrono>
#include <thread>
#include <string>
#include <vector>

#include "midi_device.hpp"

namespace pr::midi {

class MidiRecorder {
public:
    explicit MidiRecorder(MidiPortHandle src);
    ~MidiRecorder();

    MidiRecorder(const MidiRecorder&) = delete;
    MidiRecorder& operator=(const MidiRecorder&) = delete;

    void start();
    void stop();
    bool running() const noexcept { return running_.load(); }

private:
    void alsa_open_();
    void alsa_close_() noexcept;
    void alsa_create_input_port_();
    void alsa_subscribe_();
    void alsa_unsubscribe_best_effort_() noexcept;

    void record_loop_(void);

    static const char* event_type_name_(int t) noexcept;
    static void print_hex_(const unsigned char* p, size_t n) noexcept;

    static bool alsa_to_midi_bytes_(const snd_seq_event_t& ev, std::vector<unsigned char>& out);

private:
    MidiPortHandle src_;

    std::atomic<bool> running_{false};
    std::thread thread_{};

    // ALSA sequencer
    snd_seq_t* seq_{nullptr};
    int self_client_{-1};
    int in_port_{-1};

    // timing for printing
    std::chrono::steady_clock::time_point t0_{};
};

} // namespace pr::midi

