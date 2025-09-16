// include/module_factory.hh
#ifndef MODULE_FACTORY_HH
#define MODULE_FACTORY_HH

#include "sim_object.hh"
#include "config_utils.hh"
#include <unordered_map>
#include <nlohmann/json.hpp>
#include <functional>
#include <vector>

using json = nlohmann::json;

class ModuleFactory {
private:
    EventQueue* event_queue;
    std::unordered_map<std::string, SimObject*> instances;

    // 类型注册表
    using CreatorFunc = std::function<SimObject*(const std::string&, EventQueue*)>;
    static std::unordered_map<std::string, CreatorFunc>& getTypeRegistry() {
        static std::unordered_map<std::string, CreatorFunc> registry;
        return registry;
    }

public:
    explicit ModuleFactory(EventQueue* eq) : event_queue(eq) {}

    // 注册模块类型
    template<typename T>
    static void registerType(const std::string& name) {
        auto& registry = getTypeRegistry();
        if (registry.find(name) != registry.end()) {
            DPRINTF(MODULE, "[ModuleFactory] Warning: Type '%s' already registered. Overwriting.\n", name.c_str());
        }
        registry[name] = [](const std::string& n, EventQueue* eq) -> SimObject* {
            return new T(n, eq);
        };
    }

    // 注销单个类型
    static bool unregisterType(const std::string& name) {
        auto& registry = getTypeRegistry();
        auto it = registry.find(name);
        if (it != registry.end()) {
            registry.erase(it);
            DPRINTF(MODULE, "[ModuleFactory] Unregistered type: %s\n", name.c_str());
            return true;
        }
        DPRINTF(MODULE, "[ModuleFactory] Attempted to unregister unknown type: %s\n", name.c_str());
        return false;
    }

    // 清空所有注册类型（用于测试重置）
    static void clearAllTypes() {
        auto& registry = getTypeRegistry();
        DPRINTF(MODULE, "[ModuleFactory] Cleared %zu registered types.\n", registry.size());
        registry.clear();
    }

    // 获取当前注册的类型列表（调试用）
    static std::vector<std::string> getRegisteredTypes() {
        std::vector<std::string> names;
        for (const auto& kv : getTypeRegistry()) {
            names.push_back(kv.first);
        }
        return names;
    }

    // 创建所有模块实例
    void instantiateAll(const json& config);
    void startAllTicks();

    SimObject* getInstance(const std::string& name) const {
        auto it = instances.find(name);
        return it != instances.end() ? it->second : nullptr;
    }

    // 调试：打印已注册类型
    static void listRegisteredTypes() {
        printf("[ModuleFactory] Registered types:\n");
        for (const auto& kv : getTypeRegistry()) {
            printf("  - %s\n", kv.first.c_str());
        }
    }
};

// module_factory.cc 或继续放在 .hh 中
void ModuleFactory::instantiateAll(const json& config) {
    // 1. 分析连接拓扑
    std::unordered_map<std::string, std::vector<std::string>> in_edges;
    std::unordered_map<std::string, std::vector<std::string>> out_edges;

    for (auto& conn : config["connections"]) {
        if (!conn.contains("src") || !conn.contains("dst")) {
            printf("[ERROR] Invalid connection: missing 'src' or 'dst'\n");
            continue;
        }
        std::string src = conn["src"];
        std::string dst = conn["dst"];
        in_edges[dst].push_back(src);
        out_edges[src].push_back(dst);
    }

    // 2. 创建所有模块（使用注册表）
    for (auto& mod : config["modules"]) {
        if (!mod.contains("name") || !mod.contains("type")) {
            printf("[ERROR] Module missing 'name' or 'type'\n");
            continue;
        }
        std::string name = mod["name"];
        std::string type = mod["type"];

        auto& registry = ModuleFactory::getTypeRegistry();
        auto it = registry.find(type);
        if (it != registry.end()) {
            instances[name] = it->second(name, event_queue);
            DPRINTF(MODULE, "[ModuleFactory] Created %s as %s\n", name.c_str(), type.c_str());
        } else {
            printf("[ERROR] Unknown or unregistered module type: %s\n", type.c_str());
        }
    }

    // 3. 动态创建端口（保持不变）
    for (auto& [name, obj] : instances) {
        if (!obj->hasPortManager()) continue;

        auto& pm = obj->getPortManager();
        size_t in_degree  = in_edges[name].size();
        size_t out_degree = out_edges[name].size();

        std::vector<size_t> default_in_sizes  = {4};
        std::vector<size_t> default_out_sizes = {4};
        std::vector<size_t> default_priorities = {};

        for (size_t i = 0; i < in_degree; ++i) {
            json conn_cfg = getInputConnectionConfig(config, name, i);
            std::string label = "from_" + in_edges[name][i];

            pm.addUpstreamPort(
                obj,
                conn_cfg.value("input_buffer_sizes", default_in_sizes),
                conn_cfg.value("vc_priorities", default_priorities),
                label
            );
        }
        for (size_t i = 0; i < out_degree; ++i) {
            json conn_cfg = getOutputConnectionConfig(config, name, i);
            std::string label = "to_" + out_edges[name][i];

            pm.addDownstreamPort(
                obj,
                conn_cfg.value("output_buffer_sizes", default_out_sizes),
                conn_cfg.value("vc_priorities", default_priorities),
                label
            );
        }
    }

    // 4. 建立连接并注入延迟（保持不变）
    std::unordered_map<std::string, size_t> src_indices;
    std::unordered_map<std::string, size_t> dst_indices;

    for (auto& conn : config["connections"]) {
        if (!conn.contains("src") || !conn.contains("dst")) continue;

        std::string src = conn["src"];
        std::string dst = conn["dst"];
        int latency = conn.value("latency", 0);

        // 获取所有匹配的 src 模块
        std::vector<SimObject*> src_objs;
        std::vector<std::string> src_names;
        for (auto& [name, obj] : instances) {
            if (Wildcard::match(src_pattern, name)) {
                src_objs.push_back(obj);
                src_names.push_back(name);
            }
        }

        // 获取所有匹配的 dst 模块
        std::vector<SimObject*> dst_objs;
        std::vector<std::string> dst_names;
        for (auto& [name, obj] : instances) {
            if (Wildcard::match(dst_pattern, name)) {
                dst_objs.push_back(obj);
                dst_names.push_back(name);
            }
        }

        // 全连接：每个 src 连接到每个 dst
        for (size_t i = 0; i < src_objs.size(); ++i) {
            auto& src_pm = src_objs[i]->getPortManager();
            size_t& src_idx = src_indices[src_names[i]];

            for (size_t j = 0; j < dst_objs.size(); ++j) {
                auto& dst_pm = dst_objs[j]->getPortManager();
                size_t& dst_idx = dst_indices[dst_names[j]];

                MasterPort* src_port = nullptr;
                SlavePort* dst_port = nullptr;

                if (!src_pm.getDownstreamPorts().empty()) {
                    src_port = src_pm.getDownstreamPorts()[src_idx++ % src_pm.getDownstreamPorts().size()];
                    src_idx++;
                }
                if (!dst_pm.getUpstreamPorts().empty()) {
                    dst_port = dst_pm.getUpstreamPorts()[dst_idx++ % dst_pm.getUpstreamPorts().size()];
                    dst_idx++;
                }

                if (src_port && dst_port) {
                    new PortPair(src_port, dst_port);
                    src_port->setDelay(latency);
                    DPRINTF(CONN, "[CONN] Connected %s -> %s (latency=%d)\n",
                        src_names[i].c_str(), dst_names[j].c_str(), latency);
                }
            }
        }
    }
}

void ModuleFactory::startAllTicks() {
    for (const auto& pair : instances) {
        pair.second->initiate_tick();
    }
}

#endif // MODULE_FACTORY_HH
