// include/modules/cpu_sim.hh
#ifndef CPU_SIM_HH
#define CPU_SIM_HH

#include "../sim_core.hh"
#include "../sim_object.hh"
#include "../simple_port.hh"
#include "../packet.hh"
#include <unordered_map>
#include <cstdlib>

class CPUSim : public SimObject {
private:
    struct OutPort : public SimplePort {
        CPUSim* owner;
        explicit OutPort(CPUSim* o) : owner(o) {}
        bool recv(Packet* pkt) override {
            return owner->handleResponse(pkt);
        }
    };

    OutPort* out_port;
    std::unordered_map<uint64_t, Packet*> inflight_reqs;
    uint64_t next_addr = 0x1000;

public:
    CPUSim(const std::string& n, EventQueue* eq)
        : SimObject(n, eq), out_port(new OutPort(this)) {}

    SimplePort* getOutPort() { return out_port; }

    void tick() override {
        if (inflight_reqs.size() < 4 && rand() % 20 == 0) {
            auto* trans = new tlm::tlm_generic_payload();
            trans->set_command(tlm::TLM_READ_COMMAND);
            trans->set_address(next_addr);
            trans->set_data_length(4);
            trans->set_response_status(tlm::TLM_INCOMPLETE_RESPONSE);

            Packet* pkt = new Packet(trans, event_queue->getCurrentCycle(), PKT_REQ_READ);

            if (out_port->send(pkt)) {
                inflight_reqs[next_addr] = pkt;
                DPRINTF(CPU, "Sent request to 0x%" PRIx64 "\n", next_addr);
                next_addr += 4;
            } else {
                DPRINTF(CPU, "Backpressure: request to 0x%" PRIx64 " rejected\n", next_addr);
                delete pkt;
            }
        }
    }

    bool handleResponse(Packet* pkt) {
        if (pkt->isResponse()) {
            uint64_t addr = pkt->original_req->payload->get_address();
            DPRINTF(CPU, "Received response for 0x%" PRIx64 "\n", addr);
            inflight_reqs.erase(addr);
            delete pkt;
            return true;
        }
        return false;
    }
};

#endif // CPU_SIM_HH
