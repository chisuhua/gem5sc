// include/module_factory.hh
#ifndef MODULE_FACTORY_HH
#define MODULE_FACTORY_HH

#include "sim_object.hh"
#include "modules/cpu_sim.hh"
#include "modules/cache_sim.hh"
#include "modules/memory_sim.hh"
#include "modules/crossbar.hh"
#include "modules/router.hh"
#include "modules/arbiter.hh"
#include "modules/traffic_gen.hh"
//#include "modules/stream_traffic_gen.hh"
//#include "modules/stream_producer.hh"
//#include "modules/stream_consumer.hh"
#include <unordered_map>
#include <nlohmann/json.hpp>

using json = nlohmann::json;

class ModuleFactory {
private:
    EventQueue* event_queue;
    std::unordered_map<std::string, SimObject*> instances;

public:
    ModuleFactory(EventQueue* eq) : event_queue(eq) {}

    void instantiateAll(const json& config);
    void startAllTicks();
};

void ModuleFactory::instantiateAll(const json& config) {
    // 1. 分析连接拓扑
    std::unordered_map<std::string, std::vector<std::string>> in_edges;
    std::unordered_map<std::string, std::vector<std::string>> out_edges;

    for (auto& conn : config["connections"]) {
        std::string src = conn["src"];
        std::string dst = conn["dst"];
        in_edges[dst].push_back(src);
        out_edges[src].push_back(dst);
    }

    // 2. 创建所有模块
    for (auto& mod : config["modules"]) {
        std::string name = mod["name"];
        std::string type = mod["type"];

        if (type == "CPUSim") {
            instances[name] = new CPUSim(name, event_queue);
        }
        else if (type == "CacheSim") {
            instances[name] = new CacheSim(name, event_queue);
        }
        else if (type == "MemorySim") {
            instances[name] = new MemorySim(name, event_queue);
        }
        else if (type == "Crossbar") {
            instances[name] = new Crossbar(name, event_queue);
        }
        else if (type == "Router") {
            instances[name] = new Router(name, event_queue);
        }
        else if (type == "Arbiter") {
            instances[name] = new Arbiter(name, event_queue);
        }
        else if (type == "TrafficGenerator") {
            instances[name] = new TrafficGenerator(name, event_queue);
        /*
        }
        else if (type == "StreamProducer") {
            instances[name] = new StreamProducer(name, event_queue);
        }
        else if (type == "StreamConsumer") {
            instances[name] = new StreamConsumer(name, event_queue);
        }
        else if (type == "StreamTrafficGenerator") {
            instances[name] = new StreamTrafficGenerator(name, event_queue);
            auto* tg = dynamic_cast<StreamTrafficGenerator*>(instances["tg"]);
            tg->configure(TrafficMode::BURST, 20, 3);
            */
        } else {
            printf("[ERROR] Unknown module type: %s\n", type.c_str());
        }
    }

    // 3. 动态创建端口
    for (auto& [name, obj] : instances) {
        if (!obj->hasPortManager()) continue;

        auto& pm = obj->getPortManager();
        size_t in_degree  = in_edges[name].size();
        size_t out_degree = out_edges[name].size();

        if (auto* cpu = dynamic_cast<CPUSim*>(obj)) {
            for (size_t i = 0; i < out_degree; ++i) {
                pm.addDownstreamPort(cpu);
            }
        } else if (auto* cache = dynamic_cast<CacheSim*>(obj)) {
            for (size_t i = 0; i < in_degree; ++i) {
                pm.addUpstreamPort(cache);
            }
            for (size_t i = 0; i < out_degree; ++i) {
                pm.addDownstreamPort(cache);
            }
        } else if (auto* mem = dynamic_cast<MemorySim*>(obj)) {
            for (size_t i = 0; i < in_degree; ++i) {
                pm.addUpstreamPort(mem);
            }
        }
        // 自动创建上游端口（接收请求）
        /*
        for (size_t i = 0; i < in_degree; ++i) {
            pm.addUpstreamPort(obj, "");
        }

        // 自动创建下游端口（发送请求）
        for (size_t i = 0; i < out_degree; ++i) {
            pm.addDownstreamPort(obj, "");
        }
             */
    }

    // 4. 建立连接
    std::unordered_map<std::string, size_t> src_indices;
    std::unordered_map<std::string, size_t> dst_indices;

    for (auto& conn : config["connections"]) {
        std::string src = conn["src"];
        std::string dst = conn["dst"];

        auto src_obj = instances.find(src);
        auto dst_obj = instances.find(dst);
        if (src_obj == instances.end() || dst_obj == instances.end()) continue;

        auto& src_pm = src_obj->second->getPortManager();
        auto& dst_pm = dst_obj->second->getPortManager();

        size_t& src_idx = src_indices[src];
        size_t& dst_idx = dst_indices[dst];

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
        }
    }
}

void ModuleFactory::startAllTicks() {
    for (const auto& pair : instances) {
        pair.second->initiate_tick();
    }
}

#endif // MODULE_FACTORY_HH
