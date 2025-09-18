// include/modules/cache_sim.hh
#ifndef CACHE_SIM_HH
#define CACHE_SIM_HH

#include "../sim_object.hh"
#include "../packet.hh"
#include <queue>

class CacheSim : public SimObject {
private:
    std::queue<Packet*> req_buffer;
    static const int MAX_BUFFER = 4;

    MasterPort* routeToDownstream(Packet* pkt) {
        auto& pm = getPortManager();
        if (pm.getDownstreamPorts().empty()) return nullptr;
        size_t port_idx = (pkt->payload->get_address() >> 8) % pm.getDownstreamPorts().size();
        return dynamic_cast<MasterPort*>(pm.getDownstreamPorts()[port_idx]);
    }

public:
    CacheSim(const std::string& n, EventQueue* eq) : SimObject(n, eq) {}

    bool handleUpstreamRequest(Packet* pkt, int src_id, const std::string& src_label) override {
        if (req_buffer.size() >= MAX_BUFFER) {
            delete pkt;
            return false;
        }

        uint64_t addr = pkt->payload->get_address();
        bool hit = (addr & 0x7) == 0;

        if (hit) {
            pkt->payload->set_response_status(tlm::TLM_OK_RESPONSE);
            Packet* resp = new Packet(pkt->payload, event_queue->getCurrentCycle(), PKT_RESP);
            resp->original_req = pkt;
            resp->vc_id = pkt->vc_id; // 保持相同的VC

            event_queue->schedule(new LambdaEvent([this, resp, src_id]() {
                getPortManager().getUpstreamPorts()[src_id]->sendResp(resp);
            }), 1);

            delete pkt;
        } else {
            req_buffer.push(pkt);
            scheduleForward(1);
        }
        return true;
    }

    bool handleDownstreamResponse(Packet* pkt, int src_id, const std::string& src_label) override {
        DPRINTF(CACHE, "[%s] Received response from %s\n", name.c_str(), src_label.c_str());
        Packet* orig_req = pkt->original_req;
        Packet* resp_to_upstream = new Packet(orig_req->payload, event_queue->getCurrentCycle(), PKT_RESP);
        resp_to_upstream->original_req = orig_req;
        resp_to_upstream->vc_id = pkt->vc_id; // 保持相同的VC

        int upstream_id = src_id % getPortManager().getUpstreamPorts().size();
        event_queue->schedule(new LambdaEvent([this, resp_to_upstream, upstream_id]() {
            getPortManager().getUpstreamPorts()[upstream_id]->sendResp(resp_to_upstream);
        }), 1);

        delete pkt;
        delete orig_req;
        return true;
    }

    void tick() override {
        tryForward();
    }

private:
    bool tryForward() {
        if (req_buffer.empty()) return true;
        Packet* pkt = req_buffer.front();
        MasterPort* dst = routeToDownstream(pkt);
        if (dst && dst->sendReq(pkt)) {
            req_buffer.pop();
            return true;
        }
        return false;
    }

    void scheduleForward(int delay) {
        event_queue->schedule(new LambdaEvent([this]() { tryForward(); }), delay);
    }
};

#endif // CACHE_SIM_HH
