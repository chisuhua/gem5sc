// test/test_connection_resolution.cc
#include <gtest/gtest.h>
#include "../include/module_factory.hh"
#include "mock_modules.hh"

// 注册 MockSim 类型
namespace {
auto _register = []() {
    ModuleFactory::registerType<MockSim>("MockSim");
    return 0;
}();
}

TEST(ConnectionResolutionTest, WildcardConnection) {
    EventQueue eq;

    json config = R"({
        "modules": [
            { "name": "cpu0", "type": "MockSim" },
            { "name": "cpu1", "type": "MockSim" },
            { "name": "l1", "type": "MockSim" }
        ],
        "connections": [
            { "src": "cpu*", "dst": "l1", "latency": 2 }
        ]
    })"_json;

    ModuleFactory factory(&eq);
    factory.instantiateAll(config);

    auto* l1 = factory.getInstance("l1");
    ASSERT_NE(l1, nullptr);

    const auto& upstream_ports = l1->getPortManager().getUpstreamPorts();
    EXPECT_EQ(upstream_ports.size(), 2);  // cpu0 + cpu1
}

TEST(ConnectionResolutionTest, RegexConnection) {
    EventQueue eq;

    json config = R"({
        "modules": [
            { "name": "cpu0", "type": "MockSim" },
            { "name": "cpu1", "type": "MockSim" },
            { "name": "cpu2", "type": "MockSim" },
            { "name": "l1", "type": "MockSim" }
        ],
        "connections": [
            { "src": "regex:cpu[0-1]", "dst": "l1", "latency": 2 }
        ]
    })"_json;

    ModuleFactory factory(&eq);
    factory.instantiateAll(config);

    auto* l1 = factory.getInstance("l1");
    const auto& upstream_ports = l1->getPortManager().getUpstreamPorts();
    EXPECT_EQ(upstream_ports.size(), 2);  // cpu0 + cpu1
}

TEST(ConnectionResolutionTest, ModuleGroupConnection) {
    EventQueue eq;

    json config = R"({
        "modules": [
            { "name": "cpu0", "type": "MockSim" },
            { "name": "cpu1", "type": "MockSim" },
            { "name": "l1", "type": "MockSim" }
        ],
        "groups": {
            "cpus": ["cpu0", "cpu1"]
        },
        "connections": [
            { "src": "group:cpus", "dst": "l1", "latency": 2 }
        ]
    })"_json;

    ModuleFactory factory(&eq);
    factory.instantiateAll(config);

    auto* l1 = factory.getInstance("l1");
    const auto& upstream_ports = l1->getPortManager().getUpstreamPorts();
    EXPECT_EQ(upstream_ports.size(), 2);
}

TEST(ConnectionResolutionTest, ExcludeList) {
    EventQueue eq;

    json config = R"({
        "modules": [
            { "name": "cpu0", "type": "MockSim" },
            { "name": "cpu1", "type": "MockSim" },
            { "name": "l1", "type": "MockSim" }
        ],
        "connections": [
            {
                "src": "cpu*",
                "dst": "l1",
                "latency": 2,
                "exclude": ["cpu1"]
            }
        ]
    })"_json;

    ModuleFactory factory(&eq);
    factory.instantiateAll(config);

    auto* l1 = factory.getInstance("l1");
    const auto& upstream_ports = l1->getPortManager().getUpstreamPorts();
    EXPECT_EQ(upstream_ports.size(), 1);  // 只有 cpu0
}
