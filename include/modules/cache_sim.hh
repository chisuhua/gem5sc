// include/modules/cache_sim.hh
#ifndef CACHE_SIM_HH
#define CACHE_SIM_HH

#include "../sim_core.hh"
#include "../sim_object.hh"
#include "../simple_port.hh"
#include "../packet.hh"
#include <queue>

class CacheSim : public SimObject, public SimplePort {
private:
    SimplePort* downstream;
    std::queue<Packet*> req_buffer;
    static const int MAX_BUFFER = 4;

    bool tryForward() {
        if (req_buffer.empty()) return true;
        Packet* pkt = req_buffer.front();
        if (downstream && downstream->send(pkt)) {
            req_buffer.pop();
            DPRINTF(CACHE, "Forwarded request to downstream\n");
            return true;
        }
        DPRINTF(CACHE, "Downstream busy, hold packet\n");
        return false;
    }

public:
    CacheSim(const std::string& n, EventQueue* eq, SimplePort* mem_port)
        : SimObject(n, eq), downstream(mem_port) {}

    bool recv(Packet* pkt) override {
        if (pkt->isRequest()) {
            if (req_buffer.size() >= MAX_BUFFER) {
                DPRINTF(CACHE, "Buffer full! Backpressure.\n");
                return false;
            }

            uint64_t addr = pkt->payload->get_address();
            bool hit = (addr & 0x7) == 0;

            if (hit) {
                pkt->payload->set_response_status(tlm::TLM_OK_RESPONSE);
                Packet* resp = new Packet(pkt->payload, event_queue->getCurrentCycle(), PKT_RESP);
                resp->original_req = pkt;

                event_queue->schedule(new LambdaEvent([this, resp]() {
                    this->send(resp);
                }), 1);

                delete pkt;
            } else {
                req_buffer.push(pkt);
            }
        }
        else if (pkt->isResponse()) {
            // 处理来自 Memory 的响应！✅
            DPRINTF(CACHE, "Received response from memory\n");

            // 找到对应的原始请求
            Packet* orig_req = pkt->original_req;
            uint64_t addr = orig_req->payload->get_address();

            // 构造响应返回给 CPU
            Packet* resp_to_cpu = new Packet(orig_req->payload, event_queue->getCurrentCycle(), PKT_RESP);
            resp_to_cpu->original_req = orig_req;

            // 返回给 CPU
            event_queue->schedule(new LambdaEvent([this, resp_to_cpu]() {
                this->send(resp_to_cpu);
            }), 1);

            delete pkt;
            delete orig_req;
        }
        return true;
    }

    void tick() override {
        tryForward();
    }

    void setDownstream(SimplePort* port) {
        downstream = port;
    }
};

#endif // CACHE_SIM_HH
