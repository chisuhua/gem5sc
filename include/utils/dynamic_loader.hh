// include/utils/dynamic_loader.hh
#ifndef DYNAMIC_LOADER_HH
#define DYNAMIC_LOADER_HH

#include <dlfcn.h>
#include <unordered_map>
#include <string>

class DynamicLoader {
private:
    // 记录已加载的 .so 文件
    static std::unordered_map<std::string, void*> loaded_handles;

    // 记录每个 .so 是否已注册类型
    static std::unordered_set<std::string> registered_plugins;

public:
    // 加载并初始化插件
    static bool loadPlugin(const std::string& path) {
        // 1. 已加载则跳过
        if (loaded_handles.find(path) != loaded_handles.end()) {
            return true;
        }

        // 2. dlopen
        void* handle = dlopen(path.c_str(), RTLD_NOW | RTLD_GLOBAL);
        if (!handle) {
            printf("[DynamicLoader] dlopen failed: %s\n", dlerror());
            return false;
        }

        loaded_handles[path] = handle;
        DPRINTF(PLUGIN, "[DynamicLoader] Loaded: %s\n", path.c_str());

        // 3. 查找并调用 register_types 函数
        using RegisterFunc = void(*)();
        RegisterFunc reg_func = (RegisterFunc)dlsym(handle, "register_plugin_types");

        if (!reg_func) {
            printf("[DynamicLoader] No register_plugin_types() in %s\n", path.c_str());
            dlclose(handle);
            loaded_handles.erase(path);
            return false;
        }

        reg_func();  // 注册所有类型
        registered_plugins.insert(path);
        return true;
    }

    static void unloadAll() {
        for (auto& [path, handle] : loaded_handles) {
            dlclose(handle);
        }
        loaded_handles.clear();
        registered_plugins.clear();
    }

    static bool isLoaded(const std::string& path) {
        return loaded_handles.find(path) != loaded_handles.end();
    }
};

std::unordered_map<std::string, void*> DynamicLoader::loaded_handles;
std::unordered_set<std::string> DynamicLoader::registered_plugins;
