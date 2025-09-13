// include/config_loader.hh
#ifndef CONFIG_LOADER_HH
#define CONFIG_LOADER_HH

#include <iostream>
#include <fstream>
#include "nlohmann/json.hpp"

using json = nlohmann::json;

/**
 * 从文件加载 JSON 配置
 * @param filename 配置文件路径
 * @return 解析后的 JSON 对象
 */
inline json loadConfig(const std::string& filename) {
    std::ifstream f(filename);
    if (!f.is_open()) {
        std::cerr << "[ERROR] Cannot open config file: " << filename << "\n";
        exit(1);
    }

    try {
        json config;
        f >> config;
        return config;
    } catch (const json::parse_error& e) {
        std::cerr << "[ERROR] JSON parse error in " << filename 
                  << " at byte " << e.byte << ": " << e.what() << "\n";
        exit(1);
    } catch (const std::exception& e) {
        std::cerr << "[ERROR] Failed to load config: " << e.what() << "\n";
        exit(1);
    }
}

#endif // CONFIG_LOADER_HH
