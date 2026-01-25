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

    static MidiPortHandle from_snd_addr(const snd_seq_addr_t &addr) {
        return MidiPortHandle{.client_id = addr.client, .port_id = addr.port};
    }

    bool operator==(const MidiPortHandle &other) const {
        return client_id == other.client_id && port_id == other.port_id;
    }

    constexpr bool is_null_device(void) const {
        return client_id == -1 && port_id == -1;
    }
};

std::vector<MidiPortHandle> enumerate_midi_sources(void);
std::ostream &operator<<(std::ostream &os, const MidiPortHandle &h);
bool operator==(const snd_seq_addr_t &lhs, const snd_seq_addr_t &rhs);

} // namespace pr::midi
