// midi_recorder.cpp
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

template<class... Ts> struct overloaded : Ts... { using Ts::operator()...; };
template<class... Ts> overloaded(Ts...) -> overloaded<Ts...>;

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

MidiRecorder::MidiRecorder(MidiPortHandle src, const std::filesystem::path &out_path)
    : src_(src), out_path_(out_path) {
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

    while (true) {
        if (poll(fds.data(), (nfds_t)fds.size(), 50) < 0) {
            throw_sys("poll");
        }

        std::optional<SequencerMsg> event = sequencer_.get_event();
        while (event.has_value()) {
            // Who in god's name thought this was clearer as a feature than the classic union with
            // a type switch at the start???
            std::visit(overloaded {
                [&](MidiMsg msg) {
                    int now_tick = tick_clock.now_tick();
                    spdlog::trace("[{}] {}", now_tick, midi_bytes_hex(msg.data));
                    midi_file_.addEvent(0, now_tick, msg.data);
                },
                [&](AnnounceMsg msg) {
                    spdlog::info("Got: {} - {}", magic_enum::enum_name(msg.type), fmt::streamed(msg.addr));
                    if (msg.type == AnnounceType::PORT_START && msg.addr == src_) {
                        spdlog::info("Resubscribe: {}", fmt::streamed(msg.addr));
                        sequencer_.subscribe(src_);
                    }
                },
            }, event.value());
            event = sequencer_.get_event();
        }

        do_periodic_save_();
    }
}

void MidiRecorder::do_periodic_save_(void) {
    std::chrono::steady_clock::time_point now = std::chrono::steady_clock::now();
    int64_t ms_since_save =
        std::chrono::duration_cast<std::chrono::milliseconds>(now - time_last_saved_).count();
    if (ms_since_save > kAutoSaveMs) {
        time_last_saved_ = now;
        save_midi_();
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
