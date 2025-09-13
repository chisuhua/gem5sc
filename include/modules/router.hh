// include/modules/router.hh
#ifndef ROUTER_HH
#define ROUTER_HH

#include "../sim_object.hh"
#include "../packet.hh"

class Router : public SimObject {
private:
    int routeByAddress(uint64_t addr) {
        if (addr >= 0x80000000ULL) return 1;  // DMA
        if (addr >= 0xF0000000ULL) return 2;  // MMIO
        return 0;  // 默认内存
    }

public:
    Router(const std::string& n, EventQueue* eq) : SimObject(n, eq) {}

    bool handleUpstreamRequest(Packet* pkt, int src_id) {
        int dst = routeByAddress(pkt->payload->get_address());
        auto& pm = getPortManager();
        if (dst < (int)pm.getDownstreamPorts().size()) {
            pm.getDownstreamPorts()[dst]->sendReq(pkt);
            DPRINTF(ROUTER, "[%s] Route 0x%" PRIx64 " → out[%d]\n", name.c_str(), pkt->payload->get_address(), dst);
        } else {
            delete pkt;
        }
        return true;
    }

    bool handleDownstreamResponse(Packet* pkt, int src_id) {
        if (pkt->original_req && src_id < (int)getPortManager().getUpstreamPorts().size()) {
            getPortManager().getUpstreamPorts()[src_id]->sendResp(pkt);
        } else {
            delete pkt;
        }
        return true;
    }

    void tick() override {}
};

#endif // ROUTER_HH
