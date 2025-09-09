#ifndef SIMPLE_PORT_HH
#define SIMPLE_PORT_HH

class PortPair;

class SimplePort {
protected:
    PortPair* pair = nullptr;
    int my_side;

public:
    virtual void bind(PortPair* p, int side) {
        pair = p;
        my_side = side;
    }

    virtual bool send(class Packet* pkt) ;

    virtual bool recv(class Packet* pkt) = 0;

    virtual ~SimplePort() = default;
};

class PortPair {
public:
    SimplePort* side_a;
    SimplePort* side_b;
    PortPair(SimplePort* a, SimplePort* b) : side_a(a), side_b(b) {
        side_a->bind(this, 0);
        side_b->bind(this, 1);
    }
};

inline bool SimplePort::send(class Packet* pkt) {
    if (pair) {
        SimplePort* other = (my_side == 0) ? pair->side_b : pair->side_a;
        return other->recv(pkt);
    }
    return false;
}

#endif // SIMPLE_PORT_HH
