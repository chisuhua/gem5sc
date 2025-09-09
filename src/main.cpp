// main.cpp
#include <iostream>
#include <fstream>
#include "sim_core.hh"
#include "event_queue.hh"
#include "module_factory.hh"

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

    // 读取配置文件
    std::ifstream f(argv[1]);
    if (!f.is_open()) {
        std::cerr << "Cannot open config file: " << argv[1] << "\n";
        return 1;
    }

    json config;
    try {
        f >> config;
    } catch (json::parse_error& e) {
        std::cerr << "JSON parse error: " << e.what() << "\n";
        return 1;
    }

    // 创建工厂并构建系统
    ModuleFactory factory(&eq);
    factory.instantiateAll(config);
    factory.startAllTicks();

    // 运行仿真
    eq.run(2000);

    std::cout << "\n[INFO] Simulation finished.\n";
    return 0;
}
