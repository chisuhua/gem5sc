// main.cpp
#include "event_queue.hh"
#include "module_factory.hh"
#include "utils/dynamic_loader.hh"
#include "sim_module.hh"

int main() {
    EventQueue eq;

    // 1. 加载插件（只做一次）
    if (!DynamicLoader::load("libsim_module.so")) {
        printf("[ERROR] Failed to load libsim_module.so\n");
        return -1;
    }

    // 2. 加载顶层配置
    json top_config = JsonIncluder::loadAndInclude("configs/system.json");

    // 3. 创建顶层 SimModule
    ModuleFactory factory(&eq);
    factory.instantiateAll(top_config);

    // 4. 为每个 SimModule 加载其内部配置
    for (auto& [name, obj] : factory.getAllInstances()) {
        if (auto* sim_mod = dynamic_cast<SimModule*>(obj)) {
            if (top_config.contains("config") && top_config["config"].is_string()) {
                std::string config_file = top_config["config"];
                std::ifstream f(config_file);
                if (f.is_open()) {
                    json internal_cfg = json::parse(f);
                    sim_mod->configure(internal_cfg);
                } else {
                    printf("[ERROR] Cannot open config: %s\n", config_file.c_str());
                }
            }
        }
    }

    // 5. 启动仿真
    factory.startAllTicks();
    eq.run(1000);

    // 6. 清理
    DynamicLoader::unloadAll();
    return 0;
}
