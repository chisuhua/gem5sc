#include "modules/cpu_sim.hh"
#include "modules/cache_sim.hh"
#include "modules/memory_sim.hh"
#include "modules/crossbar.hh"
#include "modules/router.hh"
#include "modules/arbiter.hh"
#include "modules/traffic_gen.hh"

#define REGISTER_ALL \
    ModuleFactory::registerType<CPUSim>("CPUSim");
    ModuleFactory::registerType<CacheSim>("CacheSim");
    ModuleFactory::registerType<MemorySim>("MemorySim");
    ModuleFactory::registerType<MemorySim>("Crossbar");
    ModuleFactory::registerType<MemorySim>("Router");
    ModuleFactory::registerType<MemorySim>("Arbiter");
    ModuleFactory::registerType<TrafficGenerator>("TrafficGenerator");
