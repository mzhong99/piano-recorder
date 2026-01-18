#include "midi_device.hpp"
#include <alsa/asoundlib.h>
#include <vector>
#include <string>
#include <map>
#include <sstream>
#include <spdlog/spdlog.h>

namespace pr::midi {

std::vector<MidiPortHandle> enumerate_midi_sources(void) {
    std::vector<MidiPortHandle> out;

    snd_seq_t* seq = nullptr;
    if (snd_seq_open(&seq, "default", SND_SEQ_OPEN_DUPLEX, 0) < 0) {
        return out;
    }

    snd_seq_client_info_t* cinfo;
    snd_seq_port_info_t* pinfo;
    snd_seq_client_info_alloca(&cinfo);
    snd_seq_port_info_alloca(&pinfo);

    snd_seq_client_info_set_client(cinfo, -1);

    while (snd_seq_query_next_client(seq, cinfo) >= 0) {
        const int client = snd_seq_client_info_get_client(cinfo);
        const char* cname = snd_seq_client_info_get_name(cinfo);
        const bool is_kernel = snd_seq_client_info_get_type(cinfo) == SND_SEQ_CLIENT_SYSTEM;

        snd_seq_port_info_set_client(pinfo, client);
        snd_seq_port_info_set_port(pinfo, -1);

        while (snd_seq_query_next_port(seq, pinfo) >= 0) {
            const unsigned capabilities = snd_seq_port_info_get_capability(pinfo);

            // We only care about source ports (ports that can write events to us)
            if (!(capabilities & SND_SEQ_PORT_CAP_SUBS_WRITE)) {
                continue;
            }

            MidiPortHandle h = {
                .client_id = client,
                .port_id = snd_seq_port_info_get_port(pinfo),
                .client_name = cname ? cname : "",
                .port_name = snd_seq_port_info_get_name(pinfo),
                .capabilities = capabilities,
                .type = snd_seq_port_info_get_type(pinfo),
                .is_kernel = is_kernel,
            };

            out.push_back(std::move(h));
        }
    }

    snd_seq_close(seq);
    return out;
}

static std::string to_string_mapping(uint32_t bitfield, const std::map<uint32_t, std::string> &stringify) {
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

std::ostream& operator<<(std::ostream& os, const MidiPortHandle& h) {
    os << fmt::format("{}:{} [{}] / [{}] kernel={} capabilities={} type={}",
            h.client_id, h.port_id, h.client_name, h.port_name, h.is_kernel ? "yes" : "no",
            to_string_capabilities(h.capabilities), to_string_type(h.type));
    return os;
}

} // namespace pr::midi

