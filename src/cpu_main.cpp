#include <iostream>
#include "event_queue.hh"
#include "modules/cpu_sim.hh"
#include "modules/cache_sim.hh"
#include "modules/stream_modules.hh"
#include "modules/credit_flow.hh"
#include "modules/router.hh"
#include "modules/memory_sim.hh"
/*
int sc_main(int,char*[])
{
  //SC_REPORT_INFO( "sc_api_version", "in sc_main" );
  return 0;
}
*/

extern "C" int sc_main(int argc, char* argv[]) {
    // 空函数，只是为了满足链接器需求
    return 0;
}
int main() {
    EventQueue eq;

    // --- Request/Response: CPU -> Cache -> Memory ---
    CPUSim cpu("cpu", &eq);
    MemorySim mem("mem", &eq);
    CacheSim cache("cache", &eq, &mem);  // 假设 CacheSim 已定义

    PortPair cpu_cache(&cpu, &cache);
    PortPair cache_mem(&cache, &mem);

    // --- 单向流 + Buffer 反压 ---
    StreamProducer sp("stream_producer", &eq);
    StreamConsumer sc("stream_consumer", &eq);
    PortPair stream_link(&sp, &sc);

    // --- Credit-based 流控 ---
    CreditManager credit_mgr(2);
    CreditPort credit_port(&credit_mgr, &sc);
    CreditStreamProducer credit_producer("credit_producer", &eq, &credit_port);

    // --- 启动 tick ---
    cpu.initiate_tick();
    cache.initiate_tick();
    mem.initiate_tick();
    sp.initiate_tick();
    sc.initiate_tick();
    credit_producer.initiate_tick();

    // 运行 200 周期
    eq.run(200);

    std::cout << "\n[INFO] Simulation finished.\n";
    return 0;
}
