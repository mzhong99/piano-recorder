#pragma once

#include <alsa/asoundlib.h>
#include <poll.h>

#include <iosfwd>
#include <optional>
#include <string>
#include <vector>

#include "midi_device.hpp"

std::ostream &operator<<(std::ostream &os, const snd_seq_event_t &ev);

namespace pr::midi {

class AlsaSequencer {
public:
    AlsaSequencer(const std::string &client_name, const std::string &port_name);
    AlsaSequencer(const AlsaSequencer &) = delete;
    AlsaSequencer &operator=(const AlsaSequencer &) = delete;
    ~AlsaSequencer(void);

    void subscribe(const MidiPortHandle &new_src);
    void unsubscribe(const MidiPortHandle &src);

    std::vector<struct pollfd> get_poll_desc(void);
    std::optional<std::vector<uint8_t>> get_midi_sequence(void);

private:
    bool to_midi_bytes_(const snd_seq_event_t &ev, std::vector<uint8_t> &out);

private:
    snd_seq_t *seq_{nullptr};
    MidiPortHandle src_;
    MidiPortHandle input_;
};

} // namespace pr::midi
