// test/test_regex_matching.cc
#include <gtest/gtest.h>
#include "../include/utils/regex_matcher.hh"

TEST(RegexMatcherTest, RegexPatternDetection) {
    EXPECT_TRUE(RegexMatcher::isRegexPattern("regex:cpu[0-3]"));
    EXPECT_FALSE(RegexMatcher::isRegexPattern("cpu*"));
}

TEST(RegexMatcherTest, RegexMatching) {
    EXPECT_TRUE(RegexMatcher::match("regex:cpu[0-3]", "cpu0"));
    EXPECT_TRUE(RegexMatcher::match("regex:cpu[0-3]", "cpu3"));
    EXPECT_FALSE(RegexMatcher::match("regex:cpu[0-3]", "cpu4"));
    EXPECT_FALSE(RegexMatcher::match("regex:cpu[0-3]", "gpu0"));

    EXPECT_TRUE(RegexMatcher::match("regex:node\\d+", "node1"));
    EXPECT_TRUE(RegexMatcher::match("regex:node\\d+", "node123"));
    EXPECT_FALSE(RegexMatcher::match("regex:node\\d+", "node"));
}

TEST(RegexMatcherTest, FallbackToWildcard) {
    EXPECT_TRUE(RegexMatcher::match("cpu*", "cpu0"));
    EXPECT_FALSE(RegexMatcher::match("invalid[", "cpu0"));  // invalid regex → fallback to wildcard
}
