// test/test_config_loader.cc
#include <gtest/gtest.h>
#include "../include/module_factory.hh"
#include "../include/sim_object.hh"
#include "../include/packet.hh"

// Mock 模块用于测试
class MockSim : public SimObject {
public:
    explicit MockSim(const std::string& n, EventQueue* eq) : SimObject(n, eq) {}

    bool handleUpstreamRequest(Packet* pkt, int src_id) {
        return true;
    }

    bool handleDownstreamResponse(Packet* pkt, int src_id) {
        return true;
    }

    void tick() override {}
};

// 为测试注册类型
namespace {
auto register_mock = []() {
    ModuleFactory::registerType<MockSim>("MockSim");
    return 0;
}();
}

TEST(ConfigLoaderTest, ParseAndInstantiateWithVCAndLatency) {
    EventQueue eq;

    // JSON 配置：包含 VC、优先级、标签、延迟
    json config = R"({
        "modules": [
            { "name": "src", "type": "MockSim" },
            { "name": "dst", "type": "MockSim" }
        ],
        "connections": [
            {
                "src": "src",
                "dst": "dst",
                "input_buffer_sizes": [8, 4],
                "output_buffer_sizes": [8, 4],
                "vc_priorities": [0, 2],
                "latency": 3,
                "label": "high_speed_link"
            }
        ]
    })"_json;

    ModuleFactory factory(&eq);
    factory.instantiateAll(config);

    SimObject* src_obj = factory.getInstance("src");
    SimObject* dst_obj = factory.getInstance("dst");

    ASSERT_NE(src_obj, nullptr);
    ASSERT_NE(dst_obj, nullptr);

    auto& src_pm = src_obj->getPortManager();
    auto& dst_pm = dst_obj->getPortManager();

    // 验证端口数量
    EXPECT_EQ(src_pm.getDownstreamPorts().size(), 1);
    EXPECT_EQ(dst_pm.getUpstreamPorts().size(), 1);

    MasterPort* out_port = src_pm.getDownstreamPorts()[0];
    SlavePort* in_port = dst_pm.getUpstreamPorts()[0];

    // 验证标签
    EXPECT_EQ(out_port->getName(), "downstream[0]");
    // 标签用于连接标识，不直接暴露

    // 验证 DownstreamPort 的 VC 配置
    auto* down_port = dynamic_cast<DownstreamPort<MockSim>*>(out_port);
    ASSERT_NE(down_port, nullptr);
    EXPECT_EQ(down_port->getOutputVCs().size(), 2);  // 2 VCs

    const auto& out_vc0 = down_port->getOutputVCs()[0];
    const auto& out_vc1 = down_port->getOutputVCs()[1];

    EXPECT_EQ(out_vc0.capacity, 8);
    EXPECT_EQ(out_vc1.capacity, 4);
    EXPECT_EQ(out_vc0.priority, 0);
    EXPECT_EQ(out_vc1.priority, 2);

    // 验证 UpstreamPort 的 VC 配置
    auto* up_port = dynamic_cast<UpstreamPort<MockSim>*>(in_port);
    ASSERT_NE(up_port, nullptr);
    EXPECT_EQ(up_port->getInputVCs().size(), 2);

    const auto& in_vc0 = up_port->getInputVCs()[0];
    const auto& in_vc1 = up_port->getInputVCs()[1];

    EXPECT_EQ(in_vc0.capacity, 8);
    EXPECT_EQ(in_vc1.capacity, 4);
    EXPECT_EQ(in_vc0.priority, 0);
    EXPECT_EQ(in_vc1.priority, 2);

    // 验证延迟注入（在 PortPair 之后）
    EXPECT_EQ(out_port->getDelay(), 3);
    EXPECT_EQ(in_port->getDelay(), 0);  // 只有发送端注入

    // 验证连接可工作
    Packet* pkt = new Packet(nullptr, 0, PKT_REQ_READ);
    pkt->vc_id = 0;
    bool sent = out_port->sendReq(pkt);
    EXPECT_TRUE(sent);  // 应能入队或直发
    delete pkt;
}

TEST(ConfigLoaderTest, MultipleConnections_PerConnectionVCConfig) {
    EventQueue eq;

    json config = R"({
        "modules": [
            { "name": "cpu", "type": "MockSim" },
            { "name": "l1", "type": "MockSim" }
        ],
        "connections": [
            {
                "src": "cpu",
                "dst": "l1",
                "input_buffer_sizes": [4],
                "output_buffer_sizes": [4],
                "latency": 1
            },
            {
                "src": "l1",
                "dst": "cpu",
                "input_buffer_sizes": [2],
                "output_buffer_sizes": [2],
                "latency": 1
            }
        ]
    })"_json;

    ModuleFactory factory(&eq);
    factory.instantiateAll(config);

    auto* cpu = factory.getInstance("cpu");
    auto* l1 = factory.getInstance("l1");

    ASSERT_NE(cpu, nullptr);
    ASSERT_NE(l1, nullptr);

    // cpu -> l1 (request path)
    auto* req_out = cpu->getPortManager().getDownstreamPorts()[0];
    auto* req_in  = l1->getPortManager().getUpstreamPorts()[0];
    EXPECT_EQ(dynamic_cast<DownstreamPort<MockSim>*>(req_out)->getOutputVCs()[0].capacity, 4);
    EXPECT_EQ(req_out->getDelay(), 1);

    // l1 -> cpu (response path)
    auto* resp_out = l1->getPortManager().getDownstreamPorts()[0];
    auto* resp_in  = cpu->getPortManager().getUpstreamPorts()[0];
    EXPECT_EQ(dynamic_cast<DownstreamPort<MockSim>*>(resp_out)->getOutputVCs()[0].capacity, 2);
    EXPECT_EQ(resp_out->getDelay(), 1);
}
