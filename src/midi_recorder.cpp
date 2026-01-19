// midi_recorder_print.cpp
#include "midi_recorder.hpp"

#include <spdlog/fmt/ostr.h>
#include <spdlog/spdlog.h>

#include <errno.h>
#include <iostream>
#include <map>
#include <poll.h>
#include <stdexcept>
#include <stdio.h>
#include <string.h>
#include <string>

namespace pr::midi {

static std::string midi_bytes_hex(const std::vector<unsigned char> &bytes) {
    std::string out;
    out.reserve(bytes.size() * 3);
    for (size_t i = 0; i < bytes.size(); ++i) {
        out += fmt::format("{}{:02X}", i ? " " : "", bytes[i]);
    }
    return out;
}

static void throw_sys(const char *what) {
    int e = errno;
    throw std::runtime_error(std::string(what) + ": " + std::strerror(e));
}

MidiRecorder::MidiRecorder(MidiPortHandle src, const std::filesystem::path &out_path) : src_(src), out_path_(out_path) {
    midi_file_.absoluteTicks();
    midi_file_.setTicksPerQuarterNote(kPpq);
    midi_file_.addTempo(0, 0, kTempoBpm);

    sequencer_.subscribe(src);
}

MidiRecorder::~MidiRecorder() {
    stop();
};

void MidiRecorder::start() {
    bool expected = false;
    if (!running_.compare_exchange_strong(expected, true)) {
        return;
    }

    thread_ = std::thread([this]() { record_loop_(); });
}

void MidiRecorder::stop() {
    if (!running()) {
        return;
    }

    if (thread_.joinable()) {
        thread_.join();
    }

    running_.store(false);
}

void MidiRecorder::record_loop_(void) {
    std::vector<pollfd> fds = sequencer_.get_poll_desc();

    spdlog::info("Subscribed :{}", fmt::streamed(src_));
    spdlog::info("Logging events...");

    TickClock tick_clock{};
    std::chrono::steady_clock::time_point time_last_saved = std::chrono::steady_clock::now();

    while (true) {
        int rc = ::poll(fds.data(), (nfds_t)fds.size(), 50); // timeout => stop latency
        if (rc < 0) {
            throw_sys("poll");
        }

        auto midi_values = sequencer_.get_midi_sequence();
        while (midi_values) {
            int now_tick = tick_clock.now_tick();
            spdlog::trace("[{}] {}", now_tick, midi_bytes_hex(*midi_values));
            midi_file_.addEvent(0, now_tick, *midi_values);
            midi_values = sequencer_.get_midi_sequence();
        }

        std::chrono::steady_clock::time_point now = std::chrono::steady_clock::now();
        int64_t ms_since_save =
            std::chrono::duration_cast<std::chrono::milliseconds>(now - time_last_saved).count();
        if (ms_since_save > kAutoSaveMs) {
            time_last_saved = now;
            save_midi_();
        }
    }
}

void MidiRecorder::save_midi_(void) {
    std::filesystem::path tmp_path{out_path_.string() + ".tmp"};

    // copy the midi file before writing it
    smf::MidiFile midi_file = midi_file_;
    midi_file.sortTracks();
    midi_file.deltaTicks();

    if (!midi_file.write(tmp_path.string())) {
        spdlog::warn("Could not write to {}", tmp_path.string());
        return;
    }

    int fd = open(tmp_path.c_str(), O_RDONLY | O_CLOEXEC);
    if (fd > 0) {
        (void)fdatasync(fd);
        close(fd);
    }

    rename(tmp_path.c_str(), out_path_.c_str());
    spdlog::info("Wrote to {}", out_path_.string());
}

} // namespace pr::midi
