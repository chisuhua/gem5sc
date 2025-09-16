// include/utils/topology_dumper.hh
#ifndef TOPOLOGY_DUMPER_HH
#define TOPOLOGY_DUMPER_HH

#include "module_factory.hh"
#include "module_group.hh"
#include <fstream>

class TopologyDumper {
public:
    static void dumpToDot(const ModuleFactory& factory, const std::string& filename) {
        std::ofstream f(filename);
        if (!f.is_open()) {
            printf("[TopologyDumper] Cannot open file: %s\n", filename.c_str());
            return;
        }

        f << "digraph System {\n";
        f << "  rankdir=LR;\n";
        f << "  node [shape=box];\n\n";

        // 输出所有模块节点
        for (const auto& [name, obj] : factory.getAllInstances()) {
            f << "  \"" << name << "\" [label=\"" << name << "\\n("
              << getModuleType(obj) << ")\"];\n";
        }
        f << "\n";

        // 输出组（用子图）
        for (const std::string& group_name : ModuleGroup::getAllGroupNames()) {
            f << "  subgraph cluster_" << group_name << " {\n";
            f << "    label = \"" << group_name << "\";\n";
            f << "    style = dashed;\n";
            for (const std::string& member : ModuleGroup::getMembers(group_name)) {
                f << "    \"" << member << "\";\n";
            }
            f << "  }\n\n";
        }

        // 输出所有连接
        std::unordered_set<std::string> printed_edges;
        for (const auto& [src_name, src_obj] : factory.getAllInstances()) {
            const auto& src_pm = src_obj->getPortManager();

            for (auto* port_base : src_pm.getDownstreamPorts()) {
                auto* port = dynamic_cast<MasterPort*>(port_base);
                if (!port || !port->pair) continue;

                SlavePort* peer = (port->my_side == 0)
                    ? dynamic_cast<SlavePort*>(port->pair->side_b)
                    : dynamic_cast<SlavePort*>(port->pair->side_a);

                SimObject* dst_obj = findOwner(peer, factory.getAllInstances());
                if (!dst_obj) continue;

                std::string dst_name = dst_obj->getName();
                std::string edge_key = src_name + "->" + dst_name;
                if (printed_edges.count(edge_key)) continue;
                printed_edges.insert(edge_key);

                f << "  \"" << src_name << "\" -> \"" << dst_name
                  << "\" [label=\"delay:" << port->getDelay() << "\"];\n";
            }
        }

        f << "}\n";
        f.close();
        printf("[TopologyDumper] Topology saved to %s\n", filename.c_str());
    }

private:
    static std::string getModuleType(SimObject* obj) {
        return "Unknown";
    }

    static SimObject* findOwner(SlavePort* port, const std::unordered_map<std::string, SimObject*>& instances) {
        for (const auto& [name, obj] : instances) {
            const auto& pm = obj->getPortManager();
            for (auto* p : pm.getUpstreamPorts()) {
                if (p == port) return obj;
            }
        }
        return nullptr;
    }
};
