// include/sim_module.hh
class SimModule : public SimObject {
private:
    std::unique_ptr<ModuleFactory> internal_factory;
    json config;

public:
    explicit SimModule(const std::string& n, EventQueue* eq)
        : SimObject(n, eq), internal_factory(std::make_unique<ModuleFactory>(eq)) {}

    // 外部传入配置并构建
    void configure(const json& cfg) {
        config = cfg;
        internal_factory->instantiateAll(config);
        exposeUnconnectedPorts();
        dumpTopology();  // 生成 .dot
    }

    // 统一 tick 接口
    void tick() override {
        internal_factory->startAllTicks();  // 触发所有内部模块
    }

    // 暴露端口（用于连接）
    void exposeUnconnectedPorts();

    // 输出拓扑图
    void dumpTopology() {
        std::string dot_file = getName() + ".dot";
        TopologyDumper::dumpToDot(*internal_factory, config, dot_file);
    }

    // 调试：获取内部实例
    SimObject* getInternalInstance(const std::string& name) {
        return internal_factory->getInstance(name);
    }
};
