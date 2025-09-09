#ifndef PACKET_HH
#define PACKET_HH

#include "tlm.h"
#include <unordered_map>

enum PacketType {
    PKT_REQ_READ,
    PKT_REQ_WRITE,
    PKT_RESP,
    PKT_STREAM_DATA,
    PKT_CREDIT_RETURN
};

class Packet {
public:
    tlm::tlm_generic_payload* payload;
    uint64_t src_cycle;
    PacketType type;
    Packet* original_req;
    uint64_t stream_id;
    int credits;

    Packet(tlm::tlm_generic_payload* p, uint64_t cycle, PacketType t)
        : payload(p), src_cycle(cycle), type(t), original_req(nullptr), stream_id(0), credits(0) {}

    bool isRequest() const { return type == PKT_REQ_READ || type == PKT_REQ_WRITE; }
    bool isResponse() const { return type == PKT_RESP; }
    bool isStream() const { return type == PKT_STREAM_DATA; }
    bool isCredit() const { return type == PKT_CREDIT_RETURN; }

    ~Packet() {
        if (payload && !isCredit()) delete payload;
    }
};

#endif // PACKET_HH
