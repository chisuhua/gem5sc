// include/module_factory.hh
#ifndef MODULE_FACTORY_HH
#define MODULE_FACTORY_HH

#include "sim_object.hh"
#include "utils/config_utils.hh"
#include "utils/json_includer.hh"
#include "utils/wildcard.hh"
#include "utils/regex_matcher.hh"
#include "utils/module_group.hh"

#include <unordered_map>
#include <nlohmann/json.hpp>
#include <functional>
#include <vector>

using json = nlohmann::json;

class EventQueue;
class ModuleFactory {
private:
    EventQueue* event_queue;
    std::unordered_map<std::string, SimObject*> instances;

    using CreatorFunc = std::function<SimObject*(const std::string&, EventQueue*)>;
    static std::unordered_map<std::string, CreatorFunc>& getTypeRegistry() {
        static std::unordered_map<std::string, CreatorFunc> registry;
        return registry;
    }

public:
    explicit ModuleFactory(EventQueue* eq) : event_queue(eq) {}

    template<typename T>
    static void registerType(const std::string& name) {
        auto& registry = getTypeRegistry();
        if (registry.find(name) != registry.end()) {
            DPRINTF(MODULE, "[ModuleFactory] Warning: Type '%s' already registered.\n", name.c_str());
        }
        registry[name] = [](const std::string& n, EventQueue* eq) -> SimObject* {
            return new T(n, eq);
        };
    }

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

    static void clearAllTypes() {
        getTypeRegistry().clear();
        DPRINTF(MODULE, "[ModuleFactory] Cleared %zu registered types.\n", registry.size());
    }

    static std::vector<std::string> getRegisteredTypes() {
        std::vector<std::string> names;
        for (const auto& kv : getTypeRegistry()) {
            names.push_back(kv.first);
        }
        return names;
    }

    void instantiateAll(const json& config);
    void startAllTicks();

    SimObject* getInstance(const std::string& name) const {
        auto it = instances.find(name);
        return it != instances.end() ? it->second : nullptr;
    }

    const std::unordered_map<std::string, SimObject*>& getAllInstances() const {
        return instances;
    }

    static void listRegisteredTypes() {
        printf("[ModuleFactory] Registered types:\n");
        for (const auto& kv : getTypeRegistry()) {
            printf("  - %s\n", kv.first.c_str());
        }
    }
};

void ModuleFactory::instantiateAll(const json& config) {
    json final_config = JsonIncluder::loadAndIncludeFromJson(config);


    // 创建所有模块
    for (auto& mod : final_config["modules"]) {
        if (!mod.contains("name") || !mod.contains("type")) continue;
        std::string name = mod["name"];
        std::string type = mod["type"];

        auto& registry = ModuleFactory::getTypeRegistry();

        if (mod.contains("plugin")) {
            std::string plugin_path = mod["plugin"];
            if (!DynamicLoader::loadPlugin(plugin_path)) {
                printf("[ERROR] Failed to load plugin for %s\n", name.c_str());
                continue;
            }

            auto it = registry.find(type);
            if (it == registry.end()) {
                printf("[ERROR] Type '%s' not registered by plugin\n", type.c_str());
                continue;
            }

            instances[name] = it->second(name, event_queue);
            if (mod.contains("config")) {
                if (auto* sim_mod = dynamic_cast<SimModule*>(instances[name])) {
                    std::string config_file = mod["config"];
                    std::ifstream f(config_file);
                    if (f.is_open()) {
                        json internal_cfg = json::parse(f);
                        sim_mod->configure(internal_cfg);
                    } else {
                        printf("[ERROR] Cannot open config: %s\n", config_file.c_str());
                    }
                }
            }

            if (mod.contains("layout")) {
                auto& l = mod["layout"];
                double x = l.value("x", -1);
                double y = l.value("y", -1);
                if (x >= 0 && y >= 0) {
                    instances[name]->setLayout(x, y);
                }
            }
        }
        // 普通模块
        else {
            auto it = registry.find(type);
            if (it != registry.end()) {
                instances[name] = it->second(name, event_queue);
            } else {
                printf("[ERROR] Unknown or unregistered module type: %s\n", type.c_str());
            }
        }

    }

    // 2. 解析 groups
    if (final_config.contains("groups")) {
        for (auto& [group_name, members] : final_config["groups"].items()) {
            std::vector<std::string> member_list;
            for (auto& m : members) {
                member_list.push_back(m.get<std::string>());
            }
            ModuleGroup::define(group_name, member_list);
        }
    }

    // 动态创建端口
    std::unordered_map<std::string, std::vector<std::string>> in_edges;
    std::unordered_map<std::string, std::vector<std::string>> out_edges;

    for (auto& conn : final_config["connections"]) {
        if (!conn.contains("src") || !conn.contains("dst")) continue;
        std::string src = conn["src"];
        std::string dst = conn["dst"];

        if (!RegexMatcher::isRegexPattern(src) && !ModuleGroup::isGroupReference(src) &&
            !RegexMatcher::isRegexPattern(dst) && !ModuleGroup::isGroupReference(dst)) {
            in_edges[dst].push_back(src);
            out_edges[src].push_back(dst);
        }
    }

    for (auto& [name, obj] : instances) {
        if (!obj->hasPortManager()) continue;

        auto& pm = obj->getPortManager();
        size_t in_degree  = in_edges[name].size();
        size_t out_degree = out_edges[name].size();

        std::vector<size_t> default_in_sizes  = {4};
        std::vector<size_t> default_out_sizes = {4};
        std::vector<size_t> default_priorities = {};

        for (size_t i = 0; i < in_degree; ++i) {
            json conn_cfg = getInputConnectionConfig(final_config, name, i);
            std::string label = "from_" + in_edges[name][i];

            pm.addUpstreamPort(
                obj,
                conn_cfg.value("input_buffer_sizes", default_in_sizes),
                conn_cfg.value("vc_priorities", default_priorities),
                label
            );
        }
        for (size_t i = 0; i < out_degree; ++i) {
            json conn_cfg = getOutputConnectionConfig(final_config, name, i);
            std::string label = "to_" + out_edges[name][i];

            pm.addDownstreamPort(
                obj,
                conn_cfg.value("output_buffer_sizes", default_out_sizes),
                conn_cfg.value("vc_priorities", default_priorities),
                label
            );
        }
    }

    // 处理 wildcard/group 连接
    std::unordered_map<std::string, size_t> src_indices;
    std::unordered_map<std::string, size_t> dst_indices;

    for (auto& conn : final_config["connections"]) {
        if (!conn.contains("src") || !conn.contains("dst")) continue;

        std::string src_spec = conn["src"];
        std::string dst_spec = conn["dst"];
        int latency = conn.value("latency", 0);
        json exclude_list = conn.value("exclude", json::array());

        std::vector<std::string> src_names, dst_names;

        if (ModuleGroup::isGroupReference(src_spec)) {
            src_names = ModuleGroup::resolve(src_spec);
        } else {
            for (auto& [name, obj] : instances) {
                if (RegexMatcher::match(src_spec, name)) {
                    src_names.push_back(name);
                }
            }
        }

        if (ModuleGroup::isGroupReference(dst_spec)) {
            dst_names = ModuleGroup::resolve(dst_spec);
        } else {
            for (auto& [name, obj] : instances) {
                if (RegexMatcher::match(dst_spec, name)) {
                    dst_names.push_back(name);
                }
            }
        }

        src_names = filterExcluded(src_names, exclude_list);
        dst_names = filterExcluded(dst_names, exclude_list);

        for (const std::string& src_name : src_names) {
            auto src_obj = instances.find(src_name);
            if (src_obj == instances.end()) continue;
            auto& src_pm = src_obj->second->getPortManager();
            size_t& src_idx = src_indices[src_name];

            for (const std::string& dst_name : dst_names) {
                auto dst_obj = instances.find(dst_name);
                if (dst_obj == instances.end()) continue;
                auto& dst_pm = dst_obj->second->getPortManager();
                size_t& dst_idx = dst_indices[dst_name];

                MasterPort* src_port = nullptr;
                SlavePort* dst_port = nullptr;

                if (!src_pm.getDownstreamPorts().empty()) {
                    src_port = src_pm.getDownstreamPorts()[src_idx % src_pm.getDownstreamPorts().size()];
                    src_idx++;
                }

                if (!dst_pm.getUpstreamPorts().empty()) {
                    dst_port = dst_pm.getUpstreamPorts()[dst_idx % dst_pm.getUpstreamPorts().size()];
                    dst_idx++;
                }

                if (src_port && dst_port) {
                    new PortPair(src_port, dst_port);
                    src_port->setDelay(latency);
                    DPRINTF(CONN, "[CONN] Connected %s -> %s (latency=%d)\n",
                            src_name.c_str(), dst_name.c_str(), latency);
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
