#include "modules/cpu_sim.hh"
#include "modules/traffic_gen.hh"
#include "modules/cache_sim.hh"
#include "modules/memory_sim.hh"
#include "module_factory.hh"

SimObject* ModuleFactory::createInstance(const std::string& type, const std::string& name, const json& params) {
    if (type == "CPUSim") {
        auto* obj = new CPUSim(name, event_queue);
        instances[name] = obj;
        //ports[name] = obj;
        return obj;
    }
    else if (type == "TrafficGenerator") {
        GenMode mode = GenMode::RANDOM;
        if (params.contains("mode")) {
            std::string m = params["mode"];
            if (m == "SEQUENTIAL") mode = GenMode::SEQUENTIAL;
            else if (m == "STREAM_COPY") mode = GenMode::STREAM_COPY;
            else if (m == "TRACE") mode = GenMode::TRACE;
        }
        uint64_t start = params.value("start_addr", 0x1000);
        uint64_t end = params.value("end_addr", 0x2000);
        int num = params.value("num_requests", 10);

        auto* obj = new TrafficGenerator(name, event_queue, nullptr, mode, start, end, num);
        instances[name] = obj;
        //ports[name] = obj;
        return obj;
    }
    else if (type == "CacheSim") {
        // 临时存 nullptr，后面连接时再设置 downstream
        auto* obj = new CacheSim(name, event_queue, nullptr);
        instances[name] = obj;
        //ports[name] = obj;
        return obj;
    }
    else if (type == "MemorySim") {
        auto* obj = new MemorySim(name, event_queue);
        instances[name] = obj;
        //ports[name] = obj;
        return obj;
    }
    else {
        printf("[ERROR] Unknown module type: %s\n", type.c_str());
        return nullptr;
    }
}

void ModuleFactory::connect(const std::string& src, const std::string& dst) {
    /*
    auto src_port = ports.find(src);
    auto dst_port = ports.find(dst);
    if (src_port == ports.end() || dst_port == ports.end()) {
        printf("[ERROR] Port not found: %s -> %s\n", src.c_str(), dst.c_str());
        return;
    }
    */

    auto src_obj = instances.find(src);
    auto dst_obj = instances.find(dst);
    if (src_obj == instances.end() || dst_obj == instances.end()) return;

    SimplePort* src_port = getPort(src_obj->second);
    SimplePort* dst_port = getPort(dst_obj->second);

    if (!src_port || !dst_port) {
        DPRINTF(MODULE, "Failed to get port for %s -> %s\n", src.c_str(), dst.c_str());
        return;
    }

    new PortPair(src_port, dst_port);

    if (auto* cache = dynamic_cast<CacheSim*>(src_obj->second)) {
        cache->setDownstream(dst_port);
    }
}

SimplePort* ModuleFactory::getPort(SimObject* obj) {
    if (auto* cpu = dynamic_cast<CPUSim*>(obj)) {
        return cpu->getOutPort();
    }
    if (auto* cache = dynamic_cast<CacheSim*>(obj)) {
        return cache;
    }
    if (auto* mem = dynamic_cast<MemorySim*>(obj)) {
        return mem;
    }
    return dynamic_cast<SimplePort*>(obj);
}

void ModuleFactory::instantiateAll(const json& config) {
    // 1. 创建所有模块
    for (auto& mod : config["modules"]) {
        std::string name = mod["name"];
        std::string type = mod["type"];
        json params = mod.value("params", json::object());
        createInstance(type, name, params);
    }

    // 2. 建立连接
    for (auto& conn : config["connections"]) {
        std::string src = conn[0];
        std::string dst = conn[1];
        connect(src, dst);
    }
}

void ModuleFactory::startAllTicks() {
    for (auto& [name, obj] : instances) {
        obj->initiate_tick();
    }
}
