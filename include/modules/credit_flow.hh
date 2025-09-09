// include/modules/credit_flow.hh
#ifndef CREDIT_FLOW_HH
#define CREDIT_FLOW_HH

#include "../sim_object.hh"
#include "../simple_port.hh"
#include "../packet.hh"

class CreditManager {
private:
    int available_credits;
    int total_credits;

public:
    CreditManager(int credits) : total_credits(credits), available_credits(credits) {}

    bool tryGetCredit() {
        if (available_credits > 0) {
            available_credits--;
            return true;
        }
        return false;
    }

    void returnCredit() {
        if (available_credits < total_credits) {
            available_credits++;
        }
    }

    int getCredits() const { return available_credits; }
};

class CreditPort : public SimplePort {
private:
    CreditManager* credit_mgr;
    SimplePort* downstream;

public:
    CreditPort(CreditManager* mgr, SimplePort* down) : credit_mgr(mgr), downstream(down) {}

    bool send(Packet* pkt) override {
        if (!credit_mgr->tryGetCredit()) {
            DPRINTF(CREDIT, "CreditPort: No credit! Send rejected.\n");
            return false;
        }

        bool sent = downstream->recv(pkt);
        if (!sent) {
            credit_mgr->returnCredit();  // 下游拒绝，归还 credit
            return false;
        }
        return true;
    }

    bool recv(Packet* pkt) override {
        if (pkt->isCredit()) {
            credit_mgr->returnCredit();
            DPRINTF(CREDIT, "Credit returned: now %d available\n", credit_mgr->getCredits());
            delete pkt;
            return true;
        }
        return downstream->recv(pkt);
    }

    void bind(PortPair* p, int side) override {
        // 不绑定 pair，只使用 downstream
        downstream->bind(p, side);
    }
};

class CreditStreamProducer : public SimObject {
private:
    CreditPort* out_port;
    uint64_t counter = 0;

public:
    CreditStreamProducer(const std::string& n, EventQueue* eq, CreditPort* port)
        : SimObject(n, eq), out_port(port) {}

    void tick() override {
        if (rand() % 10 == 0) {
            auto* trans = new tlm::tlm_generic_payload();
            trans->set_command(tlm::TLM_WRITE_COMMAND);
            uint32_t data = counter++;
            trans->set_data_ptr(reinterpret_cast<unsigned char*>(&data));
            trans->set_data_length(4);

            Packet* pkt = new Packet(trans, event_queue->getCurrentCycle(), PKT_STREAM_DATA);

            if (out_port->send(pkt)) {
                DPRINTF(CREDIT, "CreditProducer: Sent data with credit\n");
            } else {
                DPRINTF(CREDIT, "CreditProducer: Send failed (no credit)\n");
            }
        }
    }
};
/*
 流控机制：Credit-based Flow Control 

CreditManager 管理 credit 数量。
发送前必须获取 credit。
接收方处理完后通过 PKT_CREDIT_RETURN 或直接调用 returnCredit() 归还。
避免缓冲区无限增长，适合长延迟链路。
*/
#endif // CREDIT_FLOW_HH
