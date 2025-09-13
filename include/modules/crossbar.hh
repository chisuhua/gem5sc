// include/modules/crossbar.hh
#ifndef CROSSBAR_HH
#define CROSSBAR_HH

#include "../sim_object.hh"
#include "../packet.hh"
#include <queue>

class Crossbar : public SimObject {
private:
    std::queue<std::pair<Packet*, int>> req_queue;  // (pkt, src_id)
    size_t next_output = 0;

    int route(Packet* pkt) {
        auto& pm = getPortManager();
        return pm.getDownstreamPorts().empty() ? 0 :
               pkt->payload->get_address() % pm.getDownstreamPorts().size();
    }

public:
    Crossbar(const std::string& n, EventQueue* eq) : SimObject(n, eq) {}

    // 回调函数（由 UpstreamPort<Owner> 调用）
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
            int dst = route(pkt);
            auto& pm = getPortManager();
            if (dst < (int)pm.getDownstreamPorts().size()) {
                MasterPort* out_port = pm.getDownstreamPorts()[dst];
                if (out_port->sendReq(pkt)) {
                    DPRINTF(XBAR, "[%s] Forwarded from in[%d] to out[%d]\n", name.c_str(), src_id, dst);
                } else {
                    req_queue.push({pkt, src_id});  // 重试
                }
            } else {
                delete pkt;
            }
        }
    }
};

#endif // CROSSBAR_HH
