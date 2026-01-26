#pragma once

#include <alsa/asoundlib.h>
#include <poll.h>

#include <iosfwd>
#include <optional>
#include <chrono>
#include <string>
#include <vector>
#include <variant>

#include "midi_device.hpp"

std::ostream &operator<<(std::ostream &os, const snd_seq_event_t &ev);

namespace pr::midi {

enum class AnnounceType {UNKNOWN, CLIENT_START, CLIENT_EXIT, PORT_START, PORT_EXIT, PORT_CHANGE};

struct MidiMsg {
    std::vector<uint8_t> data;
};

struct AnnounceMsg {
    AnnounceType type;
    MidiPortHandle addr;
};

using SequencerMsg = std::variant<MidiMsg, AnnounceMsg>;

class AlsaSequencer {
public:
    AlsaSequencer(const std::string &client_name, const std::string &port_name);
    AlsaSequencer(const AlsaSequencer &) = delete;
    AlsaSequencer &operator=(const AlsaSequencer &) = delete;
    ~AlsaSequencer(void);

    bool subscribe(const MidiPortHandle &new_src);
    void unsubscribe(const MidiPortHandle &src);

    std::vector<struct pollfd> get_poll_desc(void);
    std::optional<SequencerMsg> get_event(void);

    void expand_midi_port(MidiPortHandle &handle) {
        handle.expand_from_seq(seq_);
    }

private:
    static bool to_midi_bytes_(const snd_seq_event_t &ev, std::vector<uint8_t> &out);
    bool subscribe_naive_(const MidiPortHandle &src);
    void subscribe_announcements_(void);

private:
    snd_seq_t *seq_{nullptr};
    MidiPortHandle src_;
    MidiPortHandle input_;
};

} // namespace pr::midi
