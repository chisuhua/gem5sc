// include/port_manager.hh
#ifndef PORT_MANAGER_HH
#define PORT_MANAGER_HH

#include "slave_port.hh"
#include "master_port.hh"
#include "port_stats.hh"
#include <vector>
#include <unordered_map>
#include <string>

// ========================
// 模板化端口定义
// ========================

template<typename Owner>
struct UpstreamPort : public SlavePort {
    Owner* owner;
    int id;
    explicit UpstreamPort(Owner* o, int i)
        : SlavePort("upstream[" + std::to_string(i) + "]"), owner(o), id(i) {}

    bool recvReq(Packet* pkt) override {
        return owner->handleUpstreamRequest(pkt, id);
    }

    uint64_t getCurrentCycle() const override {
        return owner->getCurrentCycle();
    }
};

template<typename Owner>
struct DownstreamPort : public MasterPort {
    Owner* owner;
    int id;
    explicit DownstreamPort(Owner* o, int i)
        : MasterPort("downstream[" + std::to_string(i) + "]"), owner(o), id(i) {}

    bool recvResp(Packet* pkt) override {
        return owner->handleDownstreamResponse(pkt, id);
    }

    uint64_t getCurrentCycle() const override {
        return owner->getCurrentCycle();
    }
};

// ========================
// PortManager
// ========================
class PortManager {
private:
    std::vector<SlavePort*> upstream_ports;
    std::vector<MasterPort*> downstream_ports;
    std::unordered_map<std::string, SlavePort*> upstream_map;
    std::unordered_map<std::string, MasterPort*> downstream_map;

public:
    // 泛型添加端口
    template<typename Owner>
    SlavePort* addUpstreamPort(Owner* owner, const std::string& label = "") {
        static_assert(std::is_base_of_v<SimObject, Owner>,
                      "Owner must derive from SimObject");
        int id = upstream_ports.size();
        std::string name = label.empty() ? "upstream[" + std::to_string(id) + "]" : label;

        auto* port = new UpstreamPort<Owner>(owner, id);
        upstream_ports.push_back(port);
        if (!label.empty()) {
            upstream_map[label] = port;
        }
        return port;
    }

    template<typename Owner>
    MasterPort* addDownstreamPort(Owner* owner, const std::string& label = "") {
        static_assert(std::is_base_of_v<SimObject, Owner>,
                      "Owner must derive from SimObject");
        int id = downstream_ports.size();
        std::string name = label.empty() ? "downstream[" + std::to_string(id) + "]" : label;

        auto* port = new DownstreamPort<Owner>(owner, id);
        downstream_ports.push_back(port);
        if (!label.empty()) {
            downstream_map[label] = port;
        }
        return port;
    }

    // 按索引访问
    SlavePort* getUpstreamPort(size_t id) const {
        return id < upstream_ports.size() ? upstream_ports[id] : nullptr;
    }

    MasterPort* getDownstreamPort(size_t id) const {
        return id < downstream_ports.size() ? downstream_ports[id] : nullptr;
    }

    // 按标签访问
    SlavePort* getUpstreamPort(const std::string& label) const {
        auto it = upstream_map.find(label);
        return it != upstream_map.end() ? it->second : nullptr;
    }

    MasterPort* getDownstreamPort(const std::string& label) const {
        auto it = downstream_map.find(label);
        return it != downstream_map.end() ? it->second : nullptr;
    }

    // 获取向量
    const std::vector<SlavePort*>& getUpstreamPorts() const { return upstream_ports; }
    const std::vector<MasterPort*>& getDownstreamPorts() const { return downstream_ports; }

    // 统计
    PortStats getUpstreamStats() const {
        PortStats total;
        for (auto* p : upstream_ports) {
            total.merge(p->getStats());
        }
        return total;
    }

    PortStats getDownstreamStats() const {
        PortStats total;
        for (auto* p : downstream_ports) {
            total.merge(p->getStats());
        }
        return total;
    }

    void resetAllStats() {
        for (auto* p : upstream_ports) p->resetStats();
        for (auto* p : downstream_ports) p->resetStats();
    }
};

#endif // PORT_MANAGER_HH
