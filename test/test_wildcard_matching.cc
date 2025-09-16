// test/test_wildcard_matching.cc
#include <gtest/gtest.h>
#include "../include/utils/wildcard.hh"

TEST(WildcardTest, BasicMatching) {
    EXPECT_TRUE(Wildcard::match("cpu*", "cpu0"));
    EXPECT_TRUE(Wildcard::match("cpu*", "cpu_core_1"));
    EXPECT_FALSE(Wildcard::match("cpu*", "gpu0"));

    EXPECT_TRUE(Wildcard::match("gpu?", "gpu0"));
    EXPECT_TRUE(Wildcard::match("gpu?", "gpu9"));
    EXPECT_FALSE(Wildcard::match("gpu?", "gpu10"));
    EXPECT_FALSE(Wildcard::match("gpu?", "gpa0"));

    EXPECT_TRUE(Wildcard::match("mem.ctrl", "mem.ctrl"));
    EXPECT_FALSE(Wildcard::match("mem.ctrl", "memxctrl"));
}
