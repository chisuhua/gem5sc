// include/utils/dynamic_loader.hh
#ifndef DYNAMIC_LOADER_HH
#define DYNAMIC_LOADER_HH

#include <string>
#include <vector>
#include <unordered_set>

class DynamicLoader {
private:
    // 移除静态成员变量的定义，只保留声明
    static std::vector<void*> loaded_handles;
    static std::unordered_set<std::string> registered_plugins;

public:
    static bool loadPlugin(const std::string& plugin_path);
    static bool isPluginRegistered(const std::string& plugin_path);
};

#endif // DYNAMIC_LOADER_HH

