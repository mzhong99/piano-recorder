#pragma once

#include <alsa/asoundlib.h>

#include <cstdint>
#include <iosfwd>
#include <string>
#include <string_view>
#include <vector>

#include <spdlog/spdlog.h>

namespace pr::midi {

struct MidiPortHandle {
    int client_id = -1;
    int port_id = -1;

    std::string client_name = "UNKNOWN";
    std::string port_name = "UNKNOWN";

    uint32_t capabilities = 0;
    uint32_t type = 0;

    bool is_kernel = false;

    MidiPortHandle(void) = default;
    MidiPortHandle(int client, int port) : client_id(client), port_id(port) {}

    void expand_from_seq(snd_seq_t *seq) {
        snd_seq_client_info_t *cinfo = nullptr;
        snd_seq_client_info_alloca(&cinfo);

        int rc = 0;
        if ((rc = snd_seq_get_any_client_info(seq, client_id, cinfo)) == 0) {
            client_name = snd_seq_client_info_get_name(cinfo);
            is_kernel = snd_seq_client_info_get_type(cinfo) == SND_SEQ_CLIENT_SYSTEM;
        } else {
            spdlog::error("snd_seq_get_client_info - {}", rc);
        }

        snd_seq_port_info_t *pinfo = nullptr;
        snd_seq_port_info_alloca(&pinfo);
        if ((rc = snd_seq_get_any_port_info(seq, client_id, port_id, pinfo)) == 0) {
            port_name = snd_seq_port_info_get_name(pinfo);
            capabilities = snd_seq_port_info_get_capability(pinfo);
            type = snd_seq_port_info_get_type(pinfo);
        } else {
            spdlog::error("snd_seq_get_port_info - {}, {}", rc, snd_strerror(rc));
        }
    }

    constexpr bool is_subscribable_source(void) {
        return (capabilities & SND_SEQ_PORT_CAP_SUBS_WRITE) != 0;
    }

    constexpr bool is_valid(void) const {
        return client_id > 0 && port_id > 0;
    }

    snd_seq_addr_t to_snd_addr(void) const {
        return snd_seq_addr_t{.client = (unsigned char)client_id, .port = (unsigned char)port_id};
    }

    static MidiPortHandle from_snd_addr(const snd_seq_addr_t &addr) {
        return MidiPortHandle{addr.client, addr.port};
    }

    constexpr bool operator==(const MidiPortHandle &other) const {
        return client_id == other.client_id && port_id == other.port_id;
    }
};

std::vector<MidiPortHandle> enumerate_midi_sources(void);
std::ostream &operator<<(std::ostream &os, const MidiPortHandle &h);
bool operator==(const snd_seq_addr_t &lhs, const snd_seq_addr_t &rhs);

} // namespace pr::midi
