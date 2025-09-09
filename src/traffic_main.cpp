// main.cpp
#include "modules/traffic_gen.hh"
#include "modules/memory_sim.hh"

int main() {
    EventQueue eq;

    MemorySim mem("mem", &eq);
    TrafficGenerator tg("tg", &eq, &mem, GenMode::RANDOM, 0x1000, 0x2000, 20);

    tg.initiate_tick();
    mem.initiate_tick();

    eq.run(1000);
    return 0;
}
