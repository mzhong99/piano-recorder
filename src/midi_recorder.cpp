// midi_recorder.cpp
#include "midi_recorder.hpp"

#include <spdlog/fmt/ostr.h>
#include <spdlog/spdlog.h>

#include <errno.h>
#include <iostream>
#include <map>
#include <poll.h>
#include <stdexcept>
#include <algorithm>
#include <vector>
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
    : preferred_src_(src), out_path_(out_path) {
    midi_file_.absoluteTicks();
    midi_file_.setTicksPerQuarterNote(kPpq);
    midi_file_.addTempo(0, 0, kTempoBpm);

    do_resubscribe_();
}

MidiRecorder::~MidiRecorder() {
    stop();
    close(killswitch_fd_);
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

    const uint64_t dummy = 1;
    write(killswitch_fd_, &dummy, sizeof(dummy));
    stop_requested_.store(true, std::memory_order_relaxed);

    if (thread_.joinable()) {
        thread_.join();
    }

    stop_requested_.store(false, std::memory_order_relaxed);
    running_.store(false, std::memory_order_relaxed);

    save_midi_();
}

void MidiRecorder::record_loop_(void) {
    std::vector<pollfd> fds = {{.fd = killswitch_fd_, .events = POLLIN, .revents = 0}};
    std::vector<pollfd> sequencer_fds = sequencer_.get_poll_desc();
    fds.insert(fds.end(), sequencer_fds.begin(), sequencer_fds.end());

    spdlog::info("Logging events...");

    TickClock tick_clock{};

    while (!stop_requested_.load(std::memory_order_relaxed)) {
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
                    samples_last_saved_++;
                },
                [&](AnnounceMsg msg) {
                    sequencer_.expand_midi_port(msg.addr);
                    spdlog::info("{} - {}", magic_enum::enum_name(msg.type), fmt::streamed(msg.addr));
                    if (msg.type == AnnounceType::PORT_START) {
                        do_resubscribe_();
                    }
                },
            }, event.value());
            event = sequencer_.get_event();
        }

        do_periodic_save_();
    }
}

void MidiRecorder::do_resubscribe_(void) {
    spdlog::info("Preferred: {}", fmt::streamed(preferred_src_));
    if (preferred_src_.is_valid()) {
        spdlog::info("Preferred resolution: subscribe to {}", fmt::streamed(preferred_src_));
        sequencer_.subscribe(preferred_src_);
    } else {
        std::vector<MidiPortHandle> sources = enumerate_midi_sources();
        if (sources.empty()) {
            return;
        }

        std::sort(sources.begin(), sources.end());
        MidiPortHandle auto_resub = sources.back();

        spdlog::info("Auto resolution: subscribe to {}", fmt::streamed(auto_resub));
        sequencer_.subscribe(auto_resub);
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

    if (samples_last_saved_ > 0) {
        spdlog::info("{} - wrote {} samples", out_path_.string(), samples_last_saved_);
    }
    samples_last_saved_ = 0;
}

} // namespace pr::midi
