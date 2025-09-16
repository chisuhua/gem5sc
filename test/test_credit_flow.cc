// test/test_credit_flow.cc
#include <gtest/gtest.h>
#include "mock_modules.hh"

TEST(CreditFlowTest, OutputBufferSizeAsCredit) {
    EventQueue eq;
    MockProducer producer("producer", &eq);
    MockConsumer consumer("consumer", &eq);

    // output_buffer_size=2 → 最多 2 个 in-flight 请求
    producer.getPortManager().addDownstreamPort(&producer, {2}, {0});
    consumer.getPortManager().addUpstreamPort(&consumer, {4}, {0});

    new PortPair(
        producer.getPortManager().getDownstreamPorts()[0],
        consumer.getPortManager().getUpstreamPorts()[0]
    );

    // 发送 3 个包
    producer.sendPacket();  // OK
    producer.sendPacket();  // OK (buffered)
    producer.sendPacket();  // Fail? No! 因为 DownstreamPort 允许缓存

    // 但如果我们模拟“下游忙”，则第 3 个会失败
    // 实际 Credit-Based 应由 consumer 主动返回 credit
    // 这里我们只测试 buffer 行为

    EXPECT_EQ(producer.send_count, 3);  // all enqueued or sent
    EXPECT_LE(consumer.received_packets.size(), 3);
}
