// test/test_module_registration.cc
#include <gtest/gtest.h>
#include "../include/module_factory.hh"

// 测试专用 Mock 模块
class TestModuleA : public SimObject {
public:
    explicit TestModuleA(const std::string& n, EventQueue* eq) : SimObject(n, eq) {}
    void tick() override {}
};

class TestModuleB : public SimObject {
public:
    explicit TestModuleB(const std::string& n, EventQueue* eq) : SimObject(n, eq) {}
    void tick() override {}
};

TEST(ModuleRegistrationTest, RegisterAndUnregisterSingleType) {
    // 注册
    ModuleFactory::registerType<TestModuleA>("TestModuleA");
    EXPECT_EQ(ModuleFactory::getRegisteredTypes().size(), 1);

    // 注销
    bool success = ModuleFactory::unregisterType("TestModuleA");
    EXPECT_TRUE(success);
    EXPECT_EQ(ModuleFactory::getRegisteredTypes().size(), 0);

    // 再次注销应返回 false
    success = ModuleFactory::unregisterType("TestModuleA");
    EXPECT_FALSE(success);
}

TEST(ModuleRegistrationTest, ClearAllTypes) {
    ModuleFactory::registerType<TestModuleA>("TestModuleA");
    ModuleFactory::registerType<TestModuleB>("TestModuleB");

    EXPECT_EQ(ModuleFactory::getRegisteredTypes().size(), 2);

    ModuleFactory::clearAllTypes();

    EXPECT_EQ(ModuleFactory::getRegisteredTypes().size(), 0);
}

TEST(ModuleRegistrationTest, InstantiateAfterRegistration) {
    EventQueue eq;
    ModuleFactory::registerType<TestModuleA>("TestModuleA");

    json config = R"({
        "modules": [
            { "name": "inst0", "type": "TestModuleA" }
        ],
        "connections": []
    })"_json;

    ModuleFactory factory(&eq);
    factory.instantiateAll(config);

    SimObject* obj = factory.getInstance("inst0");
    ASSERT_NE(obj, nullptr);
    EXPECT_EQ(obj->getName(), "inst0");

    // 清理
    ModuleFactory::clearAllTypes();
}

TEST(ModuleRegistrationTest, IsolationBetweenTests) {
    // 测试前：应为空
    EXPECT_EQ(ModuleFactory::getRegisteredTypes().size(), 0);

    ModuleFactory::registerType<TestModuleA>("TestModuleA");
    EXPECT_EQ(ModuleFactory::getRegisteredTypes().size(), 1);

    // 测试结束时自动清理（或手动）
    ModuleFactory::clearAllTypes();
}
