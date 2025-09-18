#include "cpu_cluster.hh"

// 注册 CpuCluster 类型
static bool cpu_cluster_registered = []() {
    ModuleFactory::registerModule<CpuCluster>("CPUCluster");
    return true;
}();
