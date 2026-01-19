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

std::ostream &operator<<(std::ostream &os, const snd_seq_event_t &ev) {
    std::map<int, std::string> EVENT_TYPE_NAME = {
        {SND_SEQ_EVENT_NOTEON, "NOTEON"},
        {SND_SEQ_EVENT_NOTEOFF, "NOTEOFF"},
        {SND_SEQ_EVENT_CONTROLLER, "CC"},
        {SND_SEQ_EVENT_PGMCHANGE, "PGM"},
        {SND_SEQ_EVENT_CHANPRESS, "CHANPRESS"},
        {SND_SEQ_EVENT_PITCHBEND, "PITCHBEND"},
        {SND_SEQ_EVENT_SYSEX, "SYSEX"},
    };

    // clang-format off
    switch (ev.type) {
        case SND_SEQ_EVENT_NOTEON:
        case SND_SEQ_EVENT_NOTEOFF:
            os << fmt::format("{} ch={} note={} vel={}", EVENT_TYPE_NAME[ev.type],
                ev.data.note.channel, ev.data.note.note, ev.data.note.velocity);
            break;

        case SND_SEQ_EVENT_CONTROLLER:
            os << fmt::format("{} ch={} cc={} val={}", EVENT_TYPE_NAME[ev.type],
                ev.data.control.channel, ev.data.control.param, ev.data.control.value);
            break;

        case SND_SEQ_EVENT_PGMCHANGE:
            os << fmt::format("{} ch={} program={}", EVENT_TYPE_NAME[ev.type],
                ev.data.control.channel, ev.data.control.value);
            break;

        case SND_SEQ_EVENT_CHANPRESS:
            os << fmt::format("{} ch={} pressure={}", EVENT_TYPE_NAME[ev.type],
                ev.data.control.channel, ev.data.control.value);
            break;

        case SND_SEQ_EVENT_PITCHBEND:
            os << fmt::format("{} ch={} value={}", EVENT_TYPE_NAME[ev.type],
                ev.data.control.channel, ev.data.control.value);
            break;

        case SND_SEQ_EVENT_SYSEX:
            os << fmt::format("{} len=", EVENT_TYPE_NAME[ev.type], ev.data.ext.len);
            break;

        default:
            break;
    }
    // clang-format on

    return os;
}

namespace pr::midi {

static std::string midi_bytes_hex(const std::vector<unsigned char> &bytes) {
    std::string out;
    out.reserve(bytes.size() * 3);
    for (size_t i = 0; i < bytes.size(); ++i) {
        if (i) {
            out.push_back(' ');
        }
        out += fmt::format("{:02X}", bytes[i]);
    }
    return out;
}

static void throw_alsa(const char *what, int rc) {
    throw std::runtime_error(std::string(what) + ": " + snd_strerror(rc));
}

static void throw_sys(const char *what) {
    int e = errno;
    throw std::runtime_error(std::string(what) + ": " + std::strerror(e));
}

MidiRecorder::MidiRecorder(MidiPortHandle src, const std::filesystem::path &out_path) : src_(src), out_path_(out_path) {
    midi_file_.absoluteTicks();
    midi_file_.setTicksPerQuarterNote(kPpq);
    midi_file_.addTempo(0, 0, kTempoBpm);
}

MidiRecorder::~MidiRecorder() {
    stop();
};

void MidiRecorder::start() {
    bool expected = false;
    if (!running_.compare_exchange_strong(expected, true))
        return;

    alsa_open_();
    alsa_create_input_port_();
    alsa_subscribe_();

    t0_ = std::chrono::steady_clock::now();

    thread_ = std::thread([this]() { record_loop_(); });
}

void MidiRecorder::stop() {
    if (!running_.load())
        return;

    if (thread_.joinable()) {
        thread_.join();
    }

    alsa_unsubscribe_best_effort_();
    alsa_close_();
    running_.store(false);
}

void MidiRecorder::alsa_open_() {
    int rc = snd_seq_open(&seq_, "default", SND_SEQ_OPEN_INPUT, 0);
    if (rc < 0) {
        throw_alsa("snd_seq_open", rc);
    }

    snd_seq_set_client_name(seq_, "piano-recorder");
    self_client_ = snd_seq_client_id(seq_);
    if (self_client_ < 0) {
        throw std::runtime_error("snd_seq_client_id failed");
    }
}

void MidiRecorder::alsa_close_() noexcept {
    if (seq_) {
        snd_seq_close(seq_);
        seq_ = nullptr;
    }
    self_client_ = -1;
    in_port_ = -1;
}

void MidiRecorder::alsa_create_input_port_() {
    in_port_ = snd_seq_create_simple_port(seq_, "Recorder In",
        SND_SEQ_PORT_CAP_WRITE | SND_SEQ_PORT_CAP_SUBS_WRITE, SND_SEQ_PORT_TYPE_APPLICATION);
    snd_seq_nonblock(seq_, 1);

    if (in_port_ < 0) {
        int rc = in_port_;
        throw_alsa("snd_seq_create_simple_port", rc);
    }
}

void MidiRecorder::alsa_subscribe_() {
    snd_seq_addr_t src_addr{.client = (uint8_t)src_.client_id, .port = (uint8_t)src_.port_id};
    snd_seq_addr_t dst_addr{.client = (uint8_t)self_client_, .port = (uint8_t)in_port_};

    snd_seq_port_subscribe_t *sub = nullptr;
    snd_seq_port_subscribe_alloca(&sub);
    snd_seq_port_subscribe_set_sender(sub, &src_addr);
    snd_seq_port_subscribe_set_dest(sub, &dst_addr);

    int rc = snd_seq_subscribe_port(seq_, sub);
    if (rc < 0)
        throw_alsa("snd_seq_subscribe_port", rc);
}

void MidiRecorder::alsa_unsubscribe_best_effort_() noexcept {
    if (!seq_ || in_port_ < 0 || self_client_ < 0)
        return;

    snd_seq_addr_t src_addr{.client = (uint8_t)src_.client_id, .port = (uint8_t)src_.port_id};
    snd_seq_addr_t dst_addr{.client = (uint8_t)self_client_, .port = (uint8_t)in_port_};

    snd_seq_port_subscribe_t *sub = nullptr;
    snd_seq_port_subscribe_alloca(&sub);
    snd_seq_port_subscribe_set_sender(sub, &src_addr);
    snd_seq_port_subscribe_set_dest(sub, &dst_addr);

    (void)snd_seq_unsubscribe_port(seq_, sub);
}

bool MidiRecorder::alsa_to_midi_bytes_(const snd_seq_event_t &ev, std::vector<uint8_t> &out) {
    out.clear();
    auto ch = [](int c) -> uint8_t { return static_cast<uint8_t>(c & 0x0F); };

    switch (ev.type) {
    case SND_SEQ_EVENT_NOTEON:
        out = {static_cast<uint8_t>(0x90 | ch(ev.data.note.channel)),
            static_cast<uint8_t>(ev.data.note.note & 0x7F),
            static_cast<uint8_t>(ev.data.note.velocity & 0x7F)};
        return true;
    case SND_SEQ_EVENT_NOTEOFF:
        out = {static_cast<uint8_t>(0x80 | ch(ev.data.note.channel)),
            static_cast<uint8_t>(ev.data.note.note & 0x7F),
            static_cast<uint8_t>(ev.data.note.velocity & 0x7F)};
        return true;
    case SND_SEQ_EVENT_CONTROLLER:
        out = {static_cast<uint8_t>(0xB0 | ch(ev.data.control.channel)),
            static_cast<uint8_t>(ev.data.control.param & 0x7F),
            static_cast<uint8_t>(ev.data.control.value & 0x7F)};
        return true;
    case SND_SEQ_EVENT_PGMCHANGE:
        out = {static_cast<uint8_t>(0xC0 | ch(ev.data.control.channel)),
            static_cast<uint8_t>(ev.data.control.value & 0x7F)};
        return true;
    case SND_SEQ_EVENT_CHANPRESS:
        out = {static_cast<uint8_t>(0xD0 | ch(ev.data.control.channel)),
            static_cast<uint8_t>(ev.data.control.value & 0x7F)};
        return true;
    case SND_SEQ_EVENT_PITCHBEND: {
        int v = ev.data.control.value;
        if (v < -8192)
            v = -8192;
        if (v > 8191)
            v = 8191;
        int pb = v + 8192; // 0..16383
        out = {static_cast<uint8_t>(0xE0 | ch(ev.data.control.channel)),
            static_cast<uint8_t>(pb & 0x7F), static_cast<uint8_t>((pb >> 7) & 0x7F)};
        return true;
    }
    case SND_SEQ_EVENT_SYSEX:
        if (!ev.data.ext.ptr || ev.data.ext.len <= 0)
            return false;
        out.resize(static_cast<size_t>(ev.data.ext.len));
        std::memcpy(out.data(), ev.data.ext.ptr, out.size());
        return true;
    default:
        return false;
    }
}

void MidiRecorder::record_loop_(void) {
    const unsigned int ndesc = (unsigned int)snd_seq_poll_descriptors_count(seq_, POLLIN);
    if (ndesc <= 0)
        return;

    std::vector<pollfd> fds(static_cast<size_t>(ndesc));
    snd_seq_poll_descriptors(seq_, fds.data(), ndesc, POLLIN);

    spdlog::info("Subscribed {}:{} -> {}:{}. Printing events...", src_.client_id, src_.port_id,
        self_client_, in_port_);

    TickClock tick_clock{};

    std::chrono::steady_clock::time_point time_last_saved = std::chrono::steady_clock::now();

    int rc = 0;
    while (true) {
        rc = ::poll(fds.data(), (nfds_t)fds.size(), 50); // timeout => stop latency
        if (rc < 0) {
            throw_sys("poll");
        }

        snd_seq_event_t *ev = nullptr;
        while ((rc = snd_seq_event_input(seq_, &ev)) != -EAGAIN && ev != nullptr) {
            int now_tick = tick_clock.now_tick();

            std::vector<uint8_t> bytes;
            if (alsa_to_midi_bytes_(*ev, bytes) && !bytes.empty()) {
                spdlog::info("[{}] {} | {}", now_tick, fmt::streamed(*ev), midi_bytes_hex(bytes));
                midi_file_.addEvent(0, now_tick, bytes);
            }

            snd_seq_free_event(ev);
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

    int fd = ::open(tmp_path.c_str(), O_RDONLY | O_CLOEXEC);
    if (fd > 0) {
        (void)::fdatasync(fd);
        ::close(fd);
    }

    rename(tmp_path.c_str(), out_path_.c_str());
    spdlog::info("Wrote to {}", out_path_.string());
}

} // namespace pr::midi
