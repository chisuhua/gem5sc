// main.cpp
#include <iostream>
#include "sim_core.hh"
#include "event_queue.hh"
#include "module_factory.hh"
#include "config_loader.hh"  // 包含 loadConfig

extern "C" int sc_main(int argc, char* argv[]) {
    // 空函数，只是为了满足链接器需求
    return 0;
}

int main(int argc, char* argv[]) {
    if (argc < 2) {
        std::cerr << "Usage: " << argv[0] << " <config.json>\n";
        return 1;
    }

    EventQueue eq;
    ModuleFactory factory(&eq);

    // 加载配置文件
    json config = loadConfig(argv[1]);

    // 构建系统
    factory.instantiateAll(config);
    factory.startAllTicks();

    // 运行仿真
    eq.run(10000);

    std::cout << "\n[INFO] Simulation finished.\n";
    return 0;
}
