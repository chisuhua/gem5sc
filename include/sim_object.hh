#ifndef SIM_OBJECT_HH
#define SIM_OBJECT_HH

#include "event_queue.hh"
#include <string>

class SimObject {
protected:
    EventQueue* event_queue;
    std::string name;

public:
    SimObject(const std::string& n, EventQueue* eq) : name(n), event_queue(eq) {}

    virtual void tick() = 0;

    void initiate_tick() {
        event_queue->schedule(new TickEvent(this), 1);
    }

    virtual ~SimObject() = default;
};

inline void TickEvent::process() {
    obj->tick();
    obj->initiate_tick();
}

#endif // SIM_OBJECT_HH
