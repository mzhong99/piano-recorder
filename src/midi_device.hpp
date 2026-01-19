#pragma once

#include <alsa/asoundlib.h>

#include <cstdint>
#include <iosfwd>
#include <string>
#include <string_view>
#include <vector>

namespace pr::midi {

struct MidiPortHandle {
    int client_id = -1;
    int port_id = -1;

    std::string client_name;
    std::string port_name;

    uint32_t capabilities;
    uint32_t type;

    bool is_kernel;

    constexpr bool is_valid(void) const {
        return client_id > 0 && port_id > 0;
    }

    snd_seq_addr_t to_snd_addr(void) const {
        return snd_seq_addr_t{.client = (unsigned char)client_id, .port = (unsigned char)port_id};
    }
};

std::vector<MidiPortHandle> enumerate_midi_sources(void);
std::ostream &operator<<(std::ostream &os, const MidiPortHandle &h);

} // namespace pr::midi
