// include/modules/memory_sim.hh
#ifndef MEMORY_SIM_HH
#define MEMORY_SIM_HH

#include "../sim_object.hh"
#include "../simple_port.hh"
#include "../packet.hh"

class MemorySim : public SimObject, public SimplePort {
public:
    MemorySim(const std::string& n, EventQueue* eq) : SimObject(n, eq) {}

    void tick() override {
        // 内存通常被动响应
    }

    bool recv(Packet* pkt) override {
        DPRINTF(MEM, "Memory: receive request\n");
        if (pkt->isRequest()) {
            // 模拟 100 cycle 延迟响应
            pkt->payload->set_response_status(tlm::TLM_OK_RESPONSE);
            Packet* resp = new Packet(pkt->payload, event_queue->getCurrentCycle(), PKT_RESP);
            resp->original_req = pkt;

            event_queue->schedule(new LambdaEvent([this, resp]() {
                this->send(resp);
            }), 100);

            delete pkt;
            DPRINTF(MEM, "Memory: received request, will respond in 100 cycles\n");
        }
        return true;
    }
};

#endif // MEMORY_SIM_HH
