// midi_recorder_print.cpp
#include "midi_recorder.hpp"

#include <errno.h>
#include <poll.h>
#include <cstdio>
#include <cstring>
#include <stdexcept>

namespace pr::midi {

static void throw_alsa(const char* what, int rc) {
    throw std::runtime_error(std::string(what) + ": " + snd_strerror(rc));
}

static void throw_sys(const char* what) {
    int e = errno;
    throw std::runtime_error(std::string(what) + ": " + std::strerror(e));
}

MidiRecorder::MidiRecorder(MidiPortHandle src) : src_(src) {}
MidiRecorder::~MidiRecorder() { stop(); }

void MidiRecorder::start() {
    bool expected = false;
    if (!running_.compare_exchange_strong(expected, true)) return;

    alsa_open_();
    alsa_create_input_port_();
    alsa_subscribe_();

    t0_ = std::chrono::steady_clock::now();

    thread_ = std::thread([this]() { record_loop_(); });
}

void MidiRecorder::stop() {
    if (!running_.load()) return;

    if (thread_.joinable()) {
        thread_.join();
    }

    alsa_unsubscribe_best_effort_();
    alsa_close_();
    running_.store(false);
}

void MidiRecorder::alsa_open_() {
    int rc = snd_seq_open(&seq_, "default", SND_SEQ_OPEN_INPUT, 0);
    if (rc < 0) throw_alsa("snd_seq_open", rc);

    snd_seq_set_client_name(seq_, "piano-recorder");
    self_client_ = snd_seq_client_id(seq_);
    if (self_client_ < 0) throw std::runtime_error("snd_seq_client_id failed");
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
    in_port_ = snd_seq_create_simple_port(
        seq_,
        "Recorder In",
        SND_SEQ_PORT_CAP_WRITE | SND_SEQ_PORT_CAP_SUBS_WRITE,
        SND_SEQ_PORT_TYPE_APPLICATION);

    if (in_port_ < 0) {
        int rc = in_port_;
        throw_alsa("snd_seq_create_simple_port", rc);
    }
}

void MidiRecorder::alsa_subscribe_() {
    snd_seq_addr_t src_addr{ .client = (unsigned char)src_.client_id, .port = (unsigned char)src_.port_id };
    snd_seq_addr_t dst_addr{ .client = (unsigned char)self_client_, .port = (unsigned char)in_port_ };

    snd_seq_port_subscribe_t* sub = nullptr;
    snd_seq_port_subscribe_alloca(&sub);
    snd_seq_port_subscribe_set_sender(sub, &src_addr);
    snd_seq_port_subscribe_set_dest(sub, &dst_addr);

    int rc = snd_seq_subscribe_port(seq_, sub);
    if (rc < 0) throw_alsa("snd_seq_subscribe_port", rc);
}

void MidiRecorder::alsa_unsubscribe_best_effort_() noexcept {
    if (!seq_ || in_port_ < 0 || self_client_ < 0) return;

    snd_seq_addr_t src_addr{ .client = (unsigned char)src_.client_id, .port = (unsigned char)src_.port_id };
    snd_seq_addr_t dst_addr{ .client = (unsigned char)self_client_, .port = (unsigned char)in_port_ };

    snd_seq_port_subscribe_t* sub = nullptr;
    snd_seq_port_subscribe_alloca(&sub);
    snd_seq_port_subscribe_set_sender(sub, &src_addr);
    snd_seq_port_subscribe_set_dest(sub, &dst_addr);

    (void)snd_seq_unsubscribe_port(seq_, sub);
}

const char* MidiRecorder::event_type_name_(int type) noexcept {
    switch (type) {
        case SND_SEQ_EVENT_NOTEON: return "NOTEON";
        case SND_SEQ_EVENT_NOTEOFF: return "NOTEOFF";
        case SND_SEQ_EVENT_CONTROLLER: return "CC";
        case SND_SEQ_EVENT_PGMCHANGE: return "PGM";
        case SND_SEQ_EVENT_CHANPRESS: return "CHANPRESS";
        case SND_SEQ_EVENT_PITCHBEND: return "PITCHBEND";
        case SND_SEQ_EVENT_SYSEX: return "SYSEX";
        default: return "OTHER";
    }
}

void MidiRecorder::print_hex_(const unsigned char* p, size_t n) noexcept {
    for (size_t i = 0; i < n; ++i) {
        std::printf("%02X%s", p[i], (i + 1 == n) ? "" : " ");
    }
}

bool MidiRecorder::alsa_to_midi_bytes_(const snd_seq_event_t& ev, std::vector<unsigned char>& out) {
    out.clear();
    auto ch = [](int c) -> unsigned char { return static_cast<unsigned char>(c & 0x0F); };

    switch (ev.type) {
    case SND_SEQ_EVENT_NOTEON:
        out = { static_cast<unsigned char>(0x90 | ch(ev.data.note.channel)),
                static_cast<unsigned char>(ev.data.note.note & 0x7F),
                static_cast<unsigned char>(ev.data.note.velocity & 0x7F) };
        return true;
    case SND_SEQ_EVENT_NOTEOFF:
        out = { static_cast<unsigned char>(0x80 | ch(ev.data.note.channel)),
                static_cast<unsigned char>(ev.data.note.note & 0x7F),
                static_cast<unsigned char>(ev.data.note.velocity & 0x7F) };
        return true;
    case SND_SEQ_EVENT_CONTROLLER:
        out = { static_cast<unsigned char>(0xB0 | ch(ev.data.control.channel)),
                static_cast<unsigned char>(ev.data.control.param & 0x7F),
                static_cast<unsigned char>(ev.data.control.value & 0x7F) };
        return true;
    case SND_SEQ_EVENT_PGMCHANGE:
        out = { static_cast<unsigned char>(0xC0 | ch(ev.data.control.channel)),
                static_cast<unsigned char>(ev.data.control.value & 0x7F) };
        return true;
    case SND_SEQ_EVENT_CHANPRESS:
        out = { static_cast<unsigned char>(0xD0 | ch(ev.data.control.channel)),
                static_cast<unsigned char>(ev.data.control.value & 0x7F) };
        return true;
    case SND_SEQ_EVENT_PITCHBEND: {
        int v = ev.data.control.value;
        if (v < -8192) v = -8192;
        if (v >  8191) v =  8191;
        int pb = v + 8192; // 0..16383
        out = { static_cast<unsigned char>(0xE0 | ch(ev.data.control.channel)),
                static_cast<unsigned char>(pb & 0x7F),
                static_cast<unsigned char>((pb >> 7) & 0x7F) };
        return true;
    }
    case SND_SEQ_EVENT_SYSEX:
        if (!ev.data.ext.ptr || ev.data.ext.len <= 0) return false;
        out.resize(static_cast<size_t>(ev.data.ext.len));
        std::memcpy(out.data(), ev.data.ext.ptr, out.size());
        return true;
    default:
        return false;
    }
}

void MidiRecorder::record_loop_(void) {
    const unsigned int ndesc = (unsigned int)snd_seq_poll_descriptors_count(seq_, POLLIN);
    if (ndesc <= 0) return;

    std::vector<pollfd> fds(static_cast<size_t>(ndesc));
    snd_seq_poll_descriptors(seq_, fds.data(), ndesc, POLLIN);

    std::printf("Subscribed %d:%d -> %d:%d. Printing events...\n",
                src_.client_id, src_.port_id, self_client_, in_port_);
    std::fflush(stdout);

    while (true) {
        int rc = ::poll(fds.data(), (nfds_t)fds.size(), 50); // timeout => stop latency
        if (rc < 0) {
            if (errno == EINTR) continue;
            throw_sys("poll");
        }
        if (rc == 0) continue;

        while (snd_seq_event_input_pending(seq_, 0) > 0) {
            snd_seq_event_t* ev = nullptr;
            int r = snd_seq_event_input(seq_, &ev);
            if (r < 0 || !ev) break;

            const auto now = std::chrono::steady_clock::now();
            const double ms = std::chrono::duration<double, std::milli>(now - t0_).count();

            std::printf("[+%9.3f ms] %-9s", ms, event_type_name_(ev->type));

            if (ev->type == SND_SEQ_EVENT_NOTEON || ev->type == SND_SEQ_EVENT_NOTEOFF) {
                std::printf(" ch=%d note=%d vel=%d",
                            ev->data.note.channel, ev->data.note.note, ev->data.note.velocity);
            } else if (ev->type == SND_SEQ_EVENT_CONTROLLER) {
                std::printf(" ch=%d cc=%d val=%d",
                            ev->data.control.channel, ev->data.control.param, ev->data.control.value);
            } else if (ev->type == SND_SEQ_EVENT_PGMCHANGE) {
                std::printf(" ch=%d program=%d",
                            ev->data.control.channel, ev->data.control.value);
            } else if (ev->type == SND_SEQ_EVENT_PITCHBEND) {
                std::printf(" ch=%d value=%d",
                            ev->data.control.channel, ev->data.control.value);
            } else if (ev->type == SND_SEQ_EVENT_SYSEX) {
                std::printf(" len=%d", ev->data.ext.len);
            }

            std::vector<unsigned char> bytes;
            if (alsa_to_midi_bytes_(*ev, bytes) && !bytes.empty()) {
                std::printf(" | ");
                print_hex_(bytes.data(), bytes.size());
            }

            std::printf("\n");
            std::fflush(stdout);

            snd_seq_free_event(ev);
        }
    }
}

} // namespace pr::midi

