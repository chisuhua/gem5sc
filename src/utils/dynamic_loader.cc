// src/utils/dynamic_loader.cc
#include "utils/dynamic_loader.hh"
#include <dlfcn.h>
#include <iostream>

// 在这里定义静态成员变量
std::vector<void*> DynamicLoader::loaded_handles;
std::unordered_set<std::string> DynamicLoader::registered_plugins;

bool DynamicLoader::loadPlugin(const std::string& plugin_path) {
    if (isPluginRegistered(plugin_path)) {
        std::cout << "[INFO] Plugin already loaded: " << plugin_path << std::endl;
        return true;
    }

    void* handle = dlopen(plugin_path.c_str(), RTLD_LAZY | RTLD_GLOBAL);
    if (!handle) {
        std::cerr << "[ERROR] Cannot load plugin " << plugin_path 
                  << ": " << dlerror() << std::endl;
        return false;
    }

    loaded_handles.push_back(handle);
    registered_plugins.insert(plugin_path);
    std::cout << "[INFO] Successfully loaded plugin: " << plugin_path << std::endl;
    return true;
}

bool DynamicLoader::isPluginRegistered(const std::string& plugin_path) {
    return registered_plugins.find(plugin_path) != registered_plugins.end();
}
