// test/test_end_to_end_delay.cc
#include <gtest/gtest.h>
#include "mock_modules.hh"

class DelayConsumer : public SimObject {
public:
    explicit DelayConsumer(const std::string& n, EventQueue* eq) : SimObject(n, eq) {}

    bool handleUpstreamRequest(Packet* pkt, int src_id, uint64_t current_cycle) {
        event_queue->schedule(new LambdaEvent([this, pkt, current_cycle]() {
            Packet* resp = new Packet(pkt->payload, current_cycle, PKT_RESP);
            resp->original_req = pkt;

            auto& pm = getPortManager();
            if (!pm.getUpstreamPorts().empty()) {
                pm.getUpstreamPorts()[0]->sendResp(resp);
            } else {
                delete resp;
            }
            delete pkt;
        }), 5);

        return true;
    }

    void tick() override {}
};

TEST(EndToEndDelayTest, FiveCycleProcessingPlusLinkLatency) {
    EventQueue eq;
    MockProducer producer("producer", &eq);
    DelayConsumer consumer("consumer", &eq);

    auto* src_port = producer.getPortManager().addDownstreamPort(&producer, {4}, {0});
    auto* dst_port = consumer.getPortManager().addUpstreamPort(&consumer, {4}, {0});

    // 注入 2-cycle 链路延迟
    static_cast<MasterPort*>(src_port)->setDelay(2);

    new PortPair(src_port, dst_port);

    uint64_t req_cycle = 100;
    eq.schedule(new LambdaEvent([&]() {
        producer.sendPacket();
    }), req_cycle);

    eq.run(120);

    auto& stats = producer.getPortManager().getDownstreamStats();
    EXPECT_EQ(stats.resp_count, 1);
    EXPECT_EQ(stats.total_delay_cycles, 7);  // 2 + 5
}
