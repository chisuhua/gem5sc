#ifndef PACKET_HH
#define PACKET_HH

#include "tlm.h"
#include <cstdint>
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
    uint64_t dst_cycle;           // 接收时间（用于延迟统计）
    PacketType type;

    Packet* original_req = nullptr;
    std::vector<Packet*> dependents;

    // 流控相关
    uint64_t stream_id = 0;
    uint64_t seq_num = 0;
    int credits = 0;

    // routing
    std::vector<std::string> route_path;
    int hop_count = 0;
    uint8_t priority = 0;
    uint64_t flow_id = 0;

    uint8_t vc_id = 0;

    Packet(tlm::tlm_generic_payload* p, uint64_t cycle, PacketType t)
        : payload(p), src_cycle(cycle), type(t) {}

    bool isRequest() const { return type == PKT_REQ_READ || type == PKT_REQ_WRITE; }
    bool isResponse() const { return type == PKT_RESP; }
    bool isStream() const { return type == PKT_STREAM_DATA; }
    bool isCredit() const { return type == PKT_CREDIT_RETURN; }

    ~Packet() {
        if (payload && !isCredit()) delete payload;
    }

    uint64_t getDelayCycles() const {
        return dst_cycle > src_cycle ? dst_cycle - src_cycle : 0;
    }

    uint64_t getEnd2EndCycles() const {
        return original_req ? dst_cycle - original_req->src_cycle : getDelayCycles();
    }
};

#endif // PACKET_HH
