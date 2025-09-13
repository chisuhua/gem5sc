// include/modules/arbiter.hh
#ifndef ARBITER_HH
#define ARBITER_HH

#include "../sim_object.hh"
#include "../packet.hh"
#include <queue>

class Arbiter : public SimObject {
private:
    std::queue<std::pair<Packet*, int>> req_queue;
    size_t next_src = 0;

public:
    Arbiter(const std::string& n, EventQueue* eq) : SimObject(n, eq) {}

    bool handleUpstreamRequest(Packet* pkt, int src_id) {
        req_queue.push({pkt, src_id});
        return true;
    }

    bool handleDownstreamResponse(Packet* pkt, int src_id) {
        delete pkt;
        return true;
    }

    void tick() override {
        if (!req_queue.empty()) {
            auto [pkt, src_id] = req_queue.front(); req_queue.pop();
            auto& pm = getPortManager();
            if (!pm.getDownstreamPorts().empty()) {
                MasterPort* out_port = pm.getDownstreamPorts()[0];
                if (out_port->sendReq(pkt)) {
                    DPRINTF(ARB, "[%s] Forwarded from in[%d]\n", name.c_str(), src_id);
                } else {
                    req_queue.push({pkt, src_id});
                }
            } else {
                delete pkt;
            }
        }
    }
};

#endif // ARBITER_HH
