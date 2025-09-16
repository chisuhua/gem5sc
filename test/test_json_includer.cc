// test/test_json_includer.cc
#include <gtest/gtest.h>
#include "../include/utils/json_includer.hh"
#include <fstream>

class JsonIncluderTest : public ::testing::Test {
protected:
    void SetUp() override {
        // 创建临时目录和文件
        system("mkdir -p tmp/configs");
        std::ofstream base("tmp/configs/base.json");
        base << R"({
            "include": "common.json",
            "extra": "value"
        })";
        base.close();

        std::ofstream common("tmp/configs/common.json");
        common << R"({
            "modules": [
                { "name": "cpu0", "type": "MockSim" },
                { "name": "l1", "type": "MockSim" }
            ],
            "connections": [
                { "src": "cpu0", "dst": "l1", "latency": 2 }
            ]
        })";
        common.close();
    }

    void TearDown() override {
        system("rm -rf tmp");
    }
};

TEST_F(JsonIncluderTest, IncludeMergesContent) {
    json config = JsonIncluder::loadAndInclude("tmp/configs/base.json");

    EXPECT_TRUE(config.contains("modules"));
    EXPECT_TRUE(config.contains("connections"));
    EXPECT_TRUE(config.contains("extra"));

    EXPECT_EQ(config["modules"].size(), 2);
    EXPECT_EQ(config["connections"][0]["src"], "cpu0");
    EXPECT_EQ(config["extra"], "value");
}

TEST_F(JsonIncluderTest, RelativePathResolution) {
    json config = JsonIncluder::loadAndInclude("tmp/configs/base.json");
    EXPECT_EQ(config["modules"][0]["name"], "cpu0");
}
