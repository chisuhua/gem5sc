// include/utils/wildcard.hh
#ifndef WILDCARD_HH
#define WILDCARD_HH

#include <string>
#include <regex>

class Wildcard {
public:
    static bool match(const std::string& pattern, const std::string& str) {
        std::string regex_str = "^" + pattern;
        for (size_t i = 0; i < regex_str.size(); ++i) {
            if (regex_str[i] == '*') {
                regex_str.replace(i, 1, ".*");
                i += 2;
            } else if (regex_str[i] == '?') {
                regex_str.replace(i, 1, ".");
            }
        }
        regex_str += "$";

        try {
            std::regex re(regex_str);
            return std::regex_match(str, re);
        } catch (...) {
            return pattern == str;
        }
    }
};

#endif // WILDCARD_HH
