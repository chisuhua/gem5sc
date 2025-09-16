// include/port_manager.hh
#ifndef PORT_MANAGER_HH
#define PORT_MANAGER_HH

#include "slave_port.hh"
#include "master_port.hh"
#include "virtual_channel.hh"
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
    std::vector<InputVC> input_vcs;

    explicit UpstreamPort(Owner* o, int i,
                          const std::vector<size_t>& in_sizes,
                          const std::vector<size_t>& priorities = {})
        : SlavePort("upstream[" + std::to_string(i) + "]"), owner(o), id(i) {
        for (size_t vid = 0; vid < in_sizes.size(); ++vid) {
            size_t pri = vid < priorities.size() ? priorities[vid] : vid;
            input_vcs.emplace_back(vid, in_sizes[vid], pri);
        }
    }

    bool recvReq(Packet* pkt) override {
        int vc_id = pkt->vc_id;
        if (vc_id < 0 || vc_id >= (int)input_vcs.size()) {
            DPRINTF(VC, "[%s] Invalid VC ID %d\n", owner->name.c_str(), vc_id);
            delete pkt;
            return false;
        }

        pkt->dst_cycle = getCurrentCycle();
        if (input_vcs[vc_id].enqueue(pkt)) {
            DPRINTF(VC, "[%s] Enqueued to VC%d\n", owner->name.c_str(), vc_id);
            return true;
        } else {
            DPRINTF(VC, "[%s] VC%d buffer full! Backpressure\n", owner->name.c_str(), vc_id);
            input_vcs[vc_id].stats.dropped++;
            delete pkt;
            return false;
        }
    }

    uint64_t getCurrentCycle() const override {
        return owner->getCurrentCycle();
    }

    const std::vector<InputVC>& getInputVCs() const { return input_vcs; }
};

template<typename Owner>
struct DownstreamPort : public MasterPort {
    Owner* owner;
    int id;
    std::vector<OutputVC> output_vcs;


    explicit DownstreamPort(Owner* o, int i,
                            const std::vector<size_t>& out_sizes,
                            const std::vector<size_t>& priorities = {}
                            )
        : MasterPort("downstream[" + std::to_string(i) + "]"), owner(o), id(i) {
        for (size_t vid = 0; vid < out_sizes.size(); ++vid) {
            size_t pri = vid < priorities.size() ? priorities[vid] : vid;
            output_vcs.emplace_back(vid, out_sizes[vid], pri);
        }
    }

    bool recvResp(Packet* pkt) override {
        return owner->handleDownstreamResponse(pkt, id);
    }

    uint64_t getCurrentCycle() const override {
        return owner->getCurrentCycle();
    }

    bool sendReq(Packet* pkt) override {
        int vc_id = pkt->vc_id;
        if (vc_id < 0 || vc_id >= (int)output_vcs.size()) {
            delete pkt;
            return false;
        }

        auto& vc = output_vcs[vc_id];
        if (!vc.empty()) {
            if (vc.enqueue(pkt)) {
                DPRINTF(VC, "[%s] VC%d not empty, enqueued %p\n", owner->name.c_str(), vc_id, pkt);
                return true;
            } else {
                DPRINTF(VC, "[%s] VC%d full, dropped\n", owner->name.c_str(), vc_id);
                vc.stats.dropped++;
                delete pkt;
                return false;
            }
        }
        
        if (MasterPort::sendReq(pkt)) {
            DPRINTF(VC, "[%s] VC%d empty, direct send\n", owner->name.c_str(), vc_id);
            return true;
        }

        if (vc.enqueue(pkt)) {
            DPRINTF(VC, "[%s] Direct send failed, enqueued\n", owner->name.c_str());
            return true;
        } else {
            vc.stats.dropped++;
            delete pkt;
            return false;
        }
    }

    void tick() {
        for (auto& vc : output_vcs) {
            vc.trySend(this);
        }
    }

    const std::vector<OutputVC>& getOutputVCs() const { return output_vcs; }
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
    SlavePort* addUpstreamPort(Owner* owner, 
                               const std::vector<size_t>& in_sizes,
                               const std::vector<size_t>& priorities = {},
                               const std::string& label = "") {
        static_assert(std::is_base_of_v<SimObject, Owner>,
                      "Owner must derive from SimObject");
        int id = upstream_ports.size();
        std::string name = label.empty() ? "upstream[" + std::to_string(id) + "]" : label;

        auto* port = new UpstreamPort<Owner>(owner, id, in_sizes, priorities);
        upstream_ports.push_back(port);
        if (!label.empty()) {
            upstream_map[label] = port;
        }
        return port;
    }

    template<typename Owner>
    MasterPort* addDownstreamPort(Owner* owner, 
                               const std::vector<size_t>& out_sizes,
                               const std::vector<size_t>& priorities = {},
                                  const std::string& label = "") {
        static_assert(std::is_base_of_v<SimObject, Owner>,
                      "Owner must derive from SimObject");
        int id = downstream_ports.size();
        std::string name = label.empty() ? "downstream[" + std::to_string(id) + "]" : label;

        auto* port = new DownstreamPort<Owner>(owner, id, out_sizes, priorities);
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
