// include/modules/memory_sim.hh
#ifndef MEMORY_SIM_HH
#define MEMORY_SIM_HH

#include "../sim_object.hh"
#include "../packet.hh"

class MemorySim : public SimObject {
public:
    MemorySim(const std::string& n, EventQueue* eq) : SimObject(n, eq) {}

    bool handleUpstreamRequest(Packet* pkt, int src_id, const std::string& src_label) override {
        pkt->payload->set_response_status(tlm::TLM_OK_RESPONSE);
        Packet* resp = new Packet(pkt->payload, event_queue->getCurrentCycle(), PKT_RESP);
        resp->original_req = pkt;
        resp->vc_id = pkt->vc_id; // 保持相同的VC

        event_queue->schedule(new LambdaEvent([this, resp, src_id]() {
            getPortManager().getUpstreamPorts()[src_id]->sendResp(resp);
        }), 100);

        delete pkt;
        DPRINTF(MEM, "Received request, responding in 100 cycles\n");
        return true;
    }

    void tick() override {}
};

#endif // MEMORY_SIM_HH
