#include "alsa_sequencer.hpp"
#include <iostream>
#include <spdlog/fmt/ostr.h>
#include <spdlog/spdlog.h>

static void check_alsa(const char *what, int rc) {
    if (rc < 0) {
        throw std::runtime_error(std::string(what) + ": " + snd_strerror(rc));
    }
}

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

static inline bool is_midi_event(int type) {
    switch (type) {
        case SND_SEQ_EVENT_NOTEON:
        case SND_SEQ_EVENT_NOTEOFF:
        case SND_SEQ_EVENT_CONTROLLER:
        case SND_SEQ_EVENT_PITCHBEND:
        case SND_SEQ_EVENT_KEYPRESS:
        case SND_SEQ_EVENT_CHANPRESS:
            return true;
        default:
            return false;
    }
}

static inline bool is_announce_event(int type) {
    switch (type) {
        case SND_SEQ_EVENT_CLIENT_START:
        case SND_SEQ_EVENT_CLIENT_EXIT:
        case SND_SEQ_EVENT_PORT_START:
        case SND_SEQ_EVENT_PORT_EXIT:
        case SND_SEQ_EVENT_PORT_CHANGE:
            return true;
        default:
            return false;
    }
}

static AnnounceType to_announce_type(int type) {
    switch (type) {
        case SND_SEQ_EVENT_CLIENT_START:
            return AnnounceType::CLIENT_START;
        case SND_SEQ_EVENT_CLIENT_EXIT:
            return AnnounceType::CLIENT_EXIT;
        case SND_SEQ_EVENT_PORT_START:
            return AnnounceType::PORT_START;
        case SND_SEQ_EVENT_PORT_EXIT:
            return AnnounceType::PORT_EXIT;
        case SND_SEQ_EVENT_PORT_CHANGE:
            return AnnounceType::PORT_CHANGE;
        default:
            return AnnounceType::UNKNOWN;
    }
}


AlsaSequencer::AlsaSequencer(const std::string &client_name, const std::string &port_name) {
    int rc = snd_seq_open(&seq_, "default", SND_SEQ_OPEN_INPUT, 0);
    check_alsa("snd_seq_open", rc);

    snd_seq_nonblock(seq_, 1);
    snd_seq_set_client_name(seq_, client_name.c_str());
    input_.client_id = snd_seq_client_id(seq_);
    check_alsa("snd_seq_client_id", input_.client_id);

    input_.port_id = snd_seq_create_simple_port(seq_, port_name.c_str(),
        SND_SEQ_PORT_CAP_WRITE | SND_SEQ_PORT_CAP_SUBS_WRITE, SND_SEQ_PORT_TYPE_APPLICATION);
    check_alsa("snd_seq_create_simple_port", input_.port_id);

    subscribe_announcements_();
}

AlsaSequencer::~AlsaSequencer(void) {
    if (!seq_) {
        return;
    }

    snd_seq_close(seq_);
    seq_ = nullptr;
}

std::vector<struct pollfd> AlsaSequencer::get_poll_desc(void) {
    std::vector<struct pollfd> pollfds;
    const int ndesc = snd_seq_poll_descriptors_count(seq_, POLLIN);

    if (ndesc > 0) {
        pollfds.resize((size_t)ndesc);
        snd_seq_poll_descriptors(seq_, pollfds.data(), (unsigned int)ndesc, POLLIN);
    }

    return pollfds;
}

void AlsaSequencer::subscribe(const MidiPortHandle &new_src) {
    unsubscribe(src_);
    subscribe_naive_(new_src);
    src_ = new_src;
}

void AlsaSequencer::subscribe_naive_(const MidiPortHandle &src) {
    if (!src.is_valid()) {
        return;
    }

    snd_seq_addr_t src_addr = src.to_snd_addr();
    snd_seq_addr_t dst_addr = input_.to_snd_addr();

    snd_seq_port_subscribe_t *sub = nullptr;
    snd_seq_port_subscribe_alloca(&sub);
    snd_seq_port_subscribe_set_sender(sub, &src_addr);
    snd_seq_port_subscribe_set_dest(sub, &dst_addr);

    snd_seq_port_subscribe_set_time_update(sub, 1);
    snd_seq_port_subscribe_set_time_real(sub, 1);

    int rc = snd_seq_subscribe_port(seq_, sub);
    check_alsa("snd_seq_subscribe_port", rc);
}

void AlsaSequencer::subscribe_announcements_(void) {
    MidiPortHandle announce = MidiPortHandle{SND_SEQ_CLIENT_SYSTEM, SND_SEQ_PORT_SYSTEM_ANNOUNCE};
    subscribe_naive_(announce);
}

void AlsaSequencer::unsubscribe(const MidiPortHandle &src) {
    if (!src.is_valid()) {
        return;
    }

    snd_seq_addr_t src_addr = src_.to_snd_addr();
    snd_seq_addr_t dst_addr = input_.to_snd_addr();

    snd_seq_port_subscribe_t *sub = nullptr;
    snd_seq_port_subscribe_alloca(&sub);
    snd_seq_port_subscribe_set_sender(sub, &src_addr);
    snd_seq_port_subscribe_set_dest(sub, &dst_addr);

    (void)snd_seq_unsubscribe_port(seq_, sub);
}

std::optional<SequencerMsg> AlsaSequencer::get_event(void) {
    snd_seq_event_t *ev = nullptr;
    if (snd_seq_event_input(seq_, &ev) < 0 || ev == nullptr) {
        return std::nullopt;
    }

    spdlog::trace("Got: {}", fmt::streamed(*ev));

    if (is_midi_event(ev->type)) {
        MidiMsg msg;
        if (to_midi_bytes_(*ev, msg.data) && !msg.data.empty()) {
            return msg;
        }
    }

    if (is_announce_event(ev->type)) {
        AnnounceMsg msg{
            .type = to_announce_type(ev->type),
            .addr = MidiPortHandle::from_snd_addr(ev->data.addr),
        };
        if (AnnounceType::UNKNOWN != msg.type) {
            return msg;
        }
    }

    return std::nullopt;
}

bool AlsaSequencer::to_midi_bytes_(const snd_seq_event_t &ev, std::vector<uint8_t> &out) {
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

} // namespace pr::midi
