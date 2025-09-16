// test/mock_modules.hh
#ifndef MOCK_MODULES_HH
#define MOCK_MODULES_HH

#include "../include/sim_object.hh"
#include "../include/packet.hh"

// 简化版 Producer，用于发送请求
class MockProducer : public SimObject {
public:
    Packet* last_sent = nullptr;
    int send_count = 0;
    int fail_count = 0;

    explicit MockProducer(const std::string& n, EventQueue* eq) : SimObject(n, eq) {}

    bool handleDownstreamResponse(Packet* pkt, int src_id, uint64_t cycle) {
        delete pkt;
        return true;
    }

    void sendPacket(int vc_id = 0) {
        auto* trans = new tlm_generic_payload();
        trans->set_command(tlm::TLM_READ_COMMAND);
        trans->set_address(0x1000 + send_count * 4);
        trans->set_data_length(4);

        Packet* pkt = new Packet(trans, event_queue->getCurrentCycle(), PKT_REQ_READ);
        pkt->vc_id = vc_id;
        pkt->seq_num = send_count;

        if (getPortManager().getDownstreamPorts().empty()) {
            delete pkt;
            return;
        }

        MasterPort* port = getPortManager().getDownstreamPorts()[0];
        if (port->sendReq(pkt)) {
            last_sent = pkt;
            send_count++;
        } else {
            fail_count++;
            delete pkt;
        }
    }

    void tick() override {}
};

// 简化版 Consumer，用于接收请求
class MockConsumer : public SimObject {
public:
    std::vector<Packet*> received_packets;
    std::vector<int> received_vcs;

    explicit MockConsumer(const std::string& n, EventQueue* eq) : SimObject(n, eq) {}

    bool handleUpstreamRequest(Packet* pkt, int src_id, uint64_t current_cycle) {
        received_packets.push_back(pkt);
        received_vcs.push_back(pkt->vc_id);
        return true;
    }

    void tick() override {}
};

#endif // MOCK_MODULES_HH
