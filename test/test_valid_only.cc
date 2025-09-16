// test/test_valid_only.cc
#include <gtest/gtest.h>
#include "mock_modules.hh"

TEST(ValidOnlyTest, LargeInputBuffer_NoBackpressure) {
    EventQueue eq;
    MockProducer producer("producer", &eq);
    MockConsumer consumer("consumer", &eq);

    // input_buffer_size=1024 → 几乎永不反压
    producer.getPortManager().addDownstreamPort(&producer, {4}, {0});
    consumer.getPortManager().addUpstreamPort(&consumer, {1024}, {0});

    new PortPair(
        producer.getPortManager().getDownstreamPorts()[0],
        consumer.getPortManager().getUpstreamPorts()[0]
    );

    // 发送 100 个包，应全部成功
    for (int i = 0; i < 100; ++i) {
        producer.sendPacket();
    }

    EXPECT_EQ(producer.send_count, 100);
    EXPECT_EQ(producer.fail_count, 0);
    EXPECT_EQ(consumer.received_packets.size(), 100);
}
