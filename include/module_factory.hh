// include/module_factory.hh
#ifndef MODULE_FACTORY_HH
#define MODULE_FACTORY_HH

#include "sim_object.hh"
#include "simple_port.hh"
#include "event_queue.hh"
#include <unordered_map>
#include <functional>
#include <nlohmann/json.hpp>

using json = nlohmann::json;

class ModuleFactory {
private:
    EventQueue* event_queue;
    std::unordered_map<std::string, SimObject*> instances;
    //std::unordered_map<std::string, SimplePort*> ports;

public:
    ModuleFactory(EventQueue* eq) : event_queue(eq) {}
    SimplePort* getPort(SimObject* obj) ;

    SimObject* createInstance(const std::string& type, const std::string& name, const json& params);
    void connect(const std::string& src, const std::string& dst);
    void instantiateAll(const json& config);
    void startAllTicks();
};

#endif
