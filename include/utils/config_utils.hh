// include/config_utils.hh
#ifndef CONFIG_UTILS_HH
#define CONFIG_UTILS_HH

#include <nlohmann/json.hpp>

using json = nlohmann::json;

// 获取目标模块第 idx 个输入连接的配置（用于 upstream port）
inline json getInputConnectionConfig(const json& config, const std::string& dst, size_t idx) {
    size_t counter = 0;
    for (auto& conn : config["connections"]) {
        if (conn.contains("dst") && conn["dst"] == dst) {
            if (counter++ == idx) {
                return conn;
            }
        }
    }
    return {};
}

// 获取源模块第 idx 个输出连接的配置（用于 downstream port）
inline json getOutputConnectionConfig(const json& config, const std::string& src, size_t idx) {
    size_t counter = 0;
    for (auto& conn : config["connections"]) {
        if (conn.contains("src") && conn["src"] == src) {
            if (counter++ == idx) {
                return conn;
            }
        }
    }
    return {};
}

// 获取节点（作为 src 或 dst）第 idx 次出现的连接配置（用于 vc_priorities 等）
inline json getConnectionConfigByNode(const json& config, const std::string& node, size_t idx) {
    size_t counter = 0;
    for (auto& conn : config["connections"]) {
        if ((conn.contains("src") && conn["src"] == node) ||
            (conn.contains("dst") && conn["dst"] == node)) {
            if (counter++ == idx) {
                return conn;
            }
        }
    }
    return {};
}

#endif // CONFIG_UTILS_HH
