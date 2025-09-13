// include/simple_port.hh
#ifndef SIMPLE_PORT_HH
#define SIMPLE_PORT_HH

#include "packet.hh"
#include <string>

class PortPair;

// 基础端口类：支持 send/recv
class SimplePort {
protected:
    PortPair* pair = nullptr;
    int my_side;

public:
    virtual void bind(PortPair* p, int side) {
        pair = p;
        my_side = side;
    }

    // 发送数据包
    virtual bool send(Packet* pkt);

    // 接收数据包（纯虚，由子类实现）
    virtual bool recv(Packet* pkt) = 0;

    virtual ~SimplePort() = default;
};

// 双向连接对
class PortPair {
public:
    SimplePort* side_a;
    SimplePort* side_b;

    PortPair(SimplePort* a, SimplePort* b) : side_a(a), side_b(b) {
        side_a->bind(this, 0);
        side_b->bind(this, 1);
    }
};

bool SimplePort::send(Packet* pkt) {
    if (pair) {
        SimplePort* other = (my_side == 0) ? pair->side_b : pair->side_a;
        return other->recv(pkt);
    }
    return false;
}


#endif // SIMPLE_PORT_HH
