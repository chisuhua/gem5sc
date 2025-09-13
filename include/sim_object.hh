// include/sim_object.hh
#ifndef SIM_OBJECT_HH
#define SIM_OBJECT_HH

#include "event_queue.hh"
#include "port_manager.hh"
#include <memory>

class SimObject {
protected:
    EventQueue* event_queue;
    std::string name;
    std::unique_ptr<PortManager> port_manager;

public:
    SimObject(const std::string& n, EventQueue* eq) : name(n), event_queue(eq) {}

    virtual void tick() = 0;
    virtual ~SimObject() = default;

    void initiate_tick() {
        event_queue->schedule(new TickEvent(this), 1);
    }

    PortManager& getPortManager() {
        if (!port_manager) port_manager = std::make_unique<PortManager>();
        return *port_manager;
    }

    bool hasPortManager() const { return port_manager != nullptr; }

    uint64_t getCurrentCycle() const {
        return event_queue->getCurrentCycle();
    }
};

inline void TickEvent::process() {
    obj->tick();
    obj->initiate_tick();
}

#endif // SIM_OBJECT_HH
