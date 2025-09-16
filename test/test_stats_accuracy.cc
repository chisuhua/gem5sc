// test/test_stats_accuracy.cc
#include <gtest/gtest.h>
#include "mock_modules.hh"

TEST(StatsAccuracyTest, CountAndBytes) {
    EventQueue eq;
    MockProducer producer("producer", &eq);
    MockConsumer consumer("consumer", &eq);

    producer.getPortManager().addDownstreamPort(&producer, {4}, {0});
    consumer.getPortManager().addUpstreamPort(&consumer, {4}, {0});

    new PortPair(
        producer.getPortManager().getDownstreamPorts()[0],
        consumer.getPortManager().getUpstreamPorts()[0]
    );

    for (int i = 0; i < 3; ++i) {
        producer.sendPacket();
    }

    eq.run(5);

    auto& out_stats = producer.getPortManager().getDownstreamStats();
    auto& in_stats = consumer.getPortManager().getUpstreamStats();

    EXPECT_EQ(out_stats.req_count, 3);
    EXPECT_EQ(in_stats.req_count, 3);
    EXPECT_EQ(out_stats.byte_count, 12);
    EXPECT_EQ(in_stats.byte_count, 12);
}

TEST(StatsTest, EndToEndDelayAndBytes) {
    EventQueue eq;
    MockProducer producer("producer", &eq);
    MockConsumer consumer("consumer", &eq);

    producer.getPortManager().addDownstreamPort(&producer, {4}, {0});
    consumer.getPortManager().addUpstreamPort(&consumer, {4}, {0});

    new PortPair(
        producer.getPortManager().getDownstreamPorts()[0],
        consumer.getPortManager().getUpstreamPorts()[0]
    );

    eq.schedule(new LambdaEvent([&]() {
        producer.sendPacket();
    }), 100);

    eq.run(110);

    auto& in_stats = consumer.getPortManager().getUpstreamStats();
    auto& out_stats = producer.getPortManager().getDownstreamStats();

    EXPECT_EQ(in_stats.req_count, 1);
    EXPECT_EQ(out_stats.resp_count, 0);  // 尚未返回响应

    // 手动发送响应并验证延迟
    Packet* resp = new Packet(nullptr, 105, PKT_RESP);
    resp->original_req = consumer.received_packets[0];
    consumer.getPortManager().getUpstreamPorts()[0]->sendResp(resp);

    eq.run(120);

    EXPECT_EQ(out_stats.resp_count, 1);
    EXPECT_EQ(out_stats.total_delay_cycles, 5);  // 105 - 100
}
