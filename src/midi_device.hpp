#pragma once

#include <cstdint>
#include <iosfwd>
#include <string>
#include <string_view>
#include <vector>

namespace pr::midi {

struct MidiPortHandle {
    int client_id;
    int port_id;

    std::string client_name;
    std::string port_name;

    uint32_t capabilities;
    uint32_t type;

    bool is_kernel;
};

std::vector<MidiPortHandle> enumerate_midi_sources(void);
std::ostream &operator<<(std::ostream &os, const MidiPortHandle &h);

} // namespace pr::midi
