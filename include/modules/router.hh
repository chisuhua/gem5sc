// include/modules/router.hh
#ifndef ROUTER_HH
#define ROUTER_HH

#include "../sim_object.hh"
#include "../simple_port.hh"
#include "../packet.hh"
#include <vector>
#include <queue>

class Router : public SimObject {
private:
    std::vector<SimplePort*> input_ports;
    std::vector<SimplePort*> output_ports;
    std::queue<std::pair<int, Packet*>> arb_queue; // (input_idx, packet)

    int route(Packet* pkt) {
        uint64_t addr = pkt->payload->get_address();
        return addr % output_ports.size(); // 简单哈希路由
    }

public:
    Router(const std::string& n, EventQueue* eq) : SimObject(n, eq) {}

    void addInputPort(SimplePort* p) { input_ports.push_back(p); }
    void addOutputPort(SimplePort* p) { output_ports.push_back(p); }

    // 输入端口调用此函数
    bool recvFromInput(int input_idx, Packet* pkt) {
        int out_idx = route(pkt);
        if (output_ports[out_idx]->send(pkt)) {
            DPRINTF(ROUTER, "Router: Forwarded packet to output %d\n", out_idx);
            return true;
        } else {
            DPRINTF(ROUTER, "Router: Output %d busy! Backpressure.\n", out_idx);
            return false;
        }
    }

    void tick() override {
        // 可加入仲裁逻辑（如轮询输入端口）
        for (int i = 0; i < input_ports.size(); i++) {
            // 模拟输入端口有数据
        }
    }
};

// 包装 Router 的端口适配器
class RouterInputPort : public SimplePort {
private:
    Router* router;
    int input_idx;

public:
    RouterInputPort(Router* r, int idx) : router(r), input_idx(idx) {}

    bool send(Packet* pkt) override {
        if (pair) {
            SimplePort* other = (my_side == 0) ? pair->side_b : pair->side_a;
            return other->recv(pkt);
        }
        return false;
    }

    bool recv(Packet* pkt) override {
        return router->recvFromInput(input_idx, pkt);
    }
};
/*
通信模式：多路复用（Multiplexing） 

多个输入 → 单个路由器 → 多个输出。
根据地址路由。
输出端若拒绝接收，触发反压。
可扩展支持优先级仲裁、QoS。
*/
#endif // ROUTER_HH
