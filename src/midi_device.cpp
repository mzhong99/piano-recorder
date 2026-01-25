#include "midi_device.hpp"
#include <alsa/asoundlib.h>
#include <map>
#include <spdlog/spdlog.h>
#include <spdlog/fmt/ostr.h>
#include <sstream>
#include <string>
#include <vector>

namespace pr::midi {

std::vector<MidiPortHandle> enumerate_midi_sources(void) {
    std::vector<MidiPortHandle> out;

    snd_seq_t *seq = nullptr;
    if (snd_seq_open(&seq, "default", SND_SEQ_OPEN_DUPLEX, 0) < 0) {
        return out;
    }

    snd_seq_client_info_t *cinfo;
    snd_seq_port_info_t *pinfo;
    snd_seq_client_info_alloca(&cinfo);
    snd_seq_port_info_alloca(&pinfo);

    snd_seq_client_info_set_client(cinfo, -1);

    while (snd_seq_query_next_client(seq, cinfo) >= 0) {
        const int client = snd_seq_client_info_get_client(cinfo);
        snd_seq_port_info_set_client(pinfo, client);
        snd_seq_port_info_set_port(pinfo, -1);

        while (snd_seq_query_next_port(seq, pinfo) >= 0) {
            int port_id = snd_seq_port_info_get_port(pinfo);

            MidiPortHandle handle = MidiPortHandle{client, port_id};
            handle.expand_from_seq(seq);
            if (handle.is_subscribable_source()) {
                out.push_back(std::move(handle));
            }
        }
    }

    snd_seq_close(seq);
    return out;
}

static std::string to_string_mapping(
    uint32_t bitfield, const std::map<uint32_t, std::string> &stringify) {
    std::string sep = "";
    std::stringstream ss;
    for (auto &[key, val] : stringify) {
        if ((key & bitfield) != 0) {
            ss << sep << val;
            sep = "|";
        }
    }

    return ss ? ss.str() : "NONE";
}

static std::string to_string_capabilities(uint32_t capabilities) {
    const std::map<uint32_t, std::string> STRINGIFY = {
        {SND_SEQ_PORT_CAP_READ, "READ"},
        {SND_SEQ_PORT_CAP_WRITE, "WRITE"},
        {SND_SEQ_PORT_CAP_SUBS_READ, "SUBS_READ"},
        {SND_SEQ_PORT_CAP_SUBS_WRITE, "SUBS_WRITE"},
    };

    return to_string_mapping(capabilities, STRINGIFY);
}

static std::string to_string_type(uint32_t type) {
    const std::map<uint32_t, std::string> STRINGIFY = {
        {SND_SEQ_PORT_TYPE_HARDWARE, "HARDWARE"},
        {SND_SEQ_PORT_TYPE_APPLICATION, "APPLICATION"},
        {SND_SEQ_PORT_TYPE_MIDI_GENERIC, "MIDI_GENERIC"},
        {SND_SEQ_PORT_TYPE_SYNTH, "SYNTH"},
    };

    return to_string_mapping(type, STRINGIFY);
}

std::ostream &operator<<(std::ostream &os, const MidiPortHandle &h) {
    os << fmt::format("{}:{} [{}] / [{}] kernel={} capabilities={} type={}", h.client_id, h.port_id,
        h.client_name, h.port_name, h.is_kernel ? "yes" : "no",
        to_string_capabilities(h.capabilities), to_string_type(h.type));
    return os;
}

bool operator==(const snd_seq_addr_t &lhs, const snd_seq_addr_t &rhs) {
    return lhs.client == rhs.client && lhs.port == rhs.port;
}

} // namespace pr::midi
