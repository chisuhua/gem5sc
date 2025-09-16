// test/test_valid_ready.cc
#include <gtest/gtest.h>
#include "mock_modules.hh"

TEST(ValidReadyTest, NoBuffer_NoBypass) {
    EventQueue eq;
    MockProducer producer("producer", &eq);
    MockConsumer consumer("consumer", &eq);

    // input_buffer_size=0 → 必须立即处理，否则反压
    producer.getPortManager().addDownstreamPort(&producer, {4}, {0});
    consumer.getPortManager().addUpstreamPort(&consumer, {0}, {0});  // no buffer

    new PortPair(
        producer.getPortManager().getDownstreamPorts()[0],
        consumer.getPortManager().getUpstreamPorts()[0]
    );

    // 第一次发送：成功（consumer 可处理）
    producer.sendPacket();
    EXPECT_EQ(producer.send_count, 1);
    EXPECT_EQ(consumer.received_packets.size(), 1);

    // 即使 consumer 不处理新包，producer 也不能绕过
    producer.sendPacket();
    EXPECT_EQ(producer.send_count, 2);  // still succeeds (buffered in output)
    EXPECT_EQ(consumer.received_packets.size(), 1);  // only one processed

    // 手动清空接收队列
    for (auto* pkt : consumer.received_packets) delete pkt;
    consumer.received_packets.clear();
}
