int main() {
    EventQueue eq;
    ModuleFactory::registerType<CPUSim>("CPUSim");
    ModuleFactory::registerType<CacheSim>("CacheSim");

    json config = R"({
        "include": "base.json",
        "modules": [
            { "name": "cpu0", "type": "CPUSim", "layout": { "x": 100, "y": 200 } }
        ],
        "groups": { "cpus": ["cpu0"] },
        "connections": [
            { "src": "group:cpus", "dst": "l1", "latency": 2 }
        ]
    })"_json;

    ModuleFactory factory(&eq);
    factory.instantiateAll(config);
    factory.startAllTicks();

    TopologyDumper::dumpToDot(factory, "topo.dot");
    system("dot -Tpng -o chip_layout.png chip_layout.dot");
    system("xdg-open chip_layout.png");

    eq.run(1000);
}
