#pragma once

#include <alsa/asoundlib.h>

#include <algorithm>
#include <atomic>
#include <chrono>
#include <filesystem>
#include <iosfwd>
#include <string>
#include <thread>
#include <vector>

#include <math.h>

#include <MidiFile.h>
#include <magic_enum/magic_enum.hpp>

#include "alsa_sequencer.hpp"
#include "midi_device.hpp"

// has to be in global namespace or else fmt::streamed() can't see it
std::ostream &operator<<(std::ostream &os, const snd_seq_event_t &ev);

static constexpr int kPpq = 960;
static constexpr int64_t kAutoSaveMs = 500;
static constexpr double kTempoBpm = 120.0;

namespace pr::midi {

struct TickClock {
    std::chrono::steady_clock::time_point t0 = std::chrono::steady_clock::now();
    int last_tick = 0;

    int now_tick() {
        using dsec = std::chrono::duration<double>;

        const auto now = std::chrono::steady_clock::now();
        const double secs = std::chrono::duration_cast<dsec>(now - t0).count();
        const double ticks_per_sec = kPpq * (kTempoBpm / 60.0);

        int tick = (secs <= 0.0) ? 0 : int(std::llround(secs * ticks_per_sec));

        tick = std::max(tick, last_tick);
        last_tick = tick;

        return tick;
    }
};

class MidiRecorder {
public:
    explicit MidiRecorder(MidiPortHandle src, const std::filesystem::path &out_path);
    ~MidiRecorder(void);

    MidiRecorder(const MidiRecorder &) = delete;
    MidiRecorder &operator=(const MidiRecorder &) = delete;

    void start(void);
    void stop(void);
    bool running(void) const noexcept {
        return running_.load();
    }

private:
    void record_loop_(void);
    void do_periodic_save_(void);
    void save_midi_(void);

private:
    smf::MidiFile midi_file_;
    MidiPortHandle src_;

    AlsaSequencer sequencer_{"piano-recorder", "Recorder In"};

    std::atomic<bool> running_{false};
    std::thread thread_{};
    std::filesystem::path out_path_;
    std::chrono::steady_clock::time_point time_last_saved_{std::chrono::steady_clock::now()};
};

} // namespace pr::midi
