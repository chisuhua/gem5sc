// include/utils/config_utils.hh
#ifndef CONFIG_UTILS_HH
#define CONFIG_UTILS_HH

#include <nlohmann/json.hpp>

using json = nlohmann::json;

// 获取目标模块指定端口的连接配置
inline json getConnectionConfig(const json& config, const std::string& node_port) {
    for (auto& conn : config["connections"]) {
        if ((conn.contains("src") && conn["src"] == node_port) ||
            (conn.contains("dst") && conn["dst"] == node_port)) {
            return conn;
        }
    }
    return {};
}

// 获取节点作为源的连接配置
inline json getSourceConnectionConfig(const json& config, const std::string& src) {
    for (auto& conn : config["connections"]) {
        if (conn.contains("src") && conn["src"] == src) {
            return conn;
        }
    }
    return {};
}

// 获取节点作为目标的连接配置
inline json getDestinationConnectionConfig(const json& config, const std::string& dst) {
    for (auto& conn : config["connections"]) {
        if (conn.contains("dst") && conn["dst"] == dst) {
            return conn;
        }
    }
    return {};
}

#endif // CONFIG_UTILS_HH
