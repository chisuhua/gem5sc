// test/test_module_group.cc
#include <gtest/gtest.h>
#include "../include/utils/module_group.hh"

TEST(ModuleGroupTest, DefineAndResolve) {
    std::vector<std::string> members = {"cpu0", "cpu1", "cpu2"};
    ModuleGroup::define("cpus", members);

    auto resolved = ModuleGroup::getMembers("cpus");
    EXPECT_EQ(resolved.size(), 3);
    EXPECT_EQ(resolved[0], "cpu0");

    auto empty = ModuleGroup::getMembers("not_exist");
    EXPECT_TRUE(empty.empty());
}

TEST(ModuleGroupTest, GroupReferenceParsing) {
    EXPECT_TRUE(ModuleGroup::isGroupReference("group:cpus"));
    EXPECT_FALSE(ModuleGroup::isGroupReference("cpus"));

    EXPECT_EQ(ModuleGroup::extractGroupName("group:cpus"), "cpus");
    EXPECT_EQ(ModuleGroup::extractGroupName("group:"), "");
}
