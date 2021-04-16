#ifndef SIMULATOR_EVENT_HPP_
#define SIMULATOR_EVENT_HPP_

struct Event {
    const static int EVENT_TASK_ARRIVED  = 0;
    const static int EVENT_TASK_LOADED   = 1;
    const static int EVENT_TASK_STARTED  = 2;
    const static int EVENT_TASK_FINISHED = 3;
    const static int EVENT_TASK_FAILED   = 4;
    const static int EVENT_RESOURCE_DOWN = 5;
    const static int EVENT_RESOURCE_UP   = 6;
    static int ID;

    Event() : id(ID++) {}

    bool operator > (const Event &other) const {
        if (time != other.time)
            return time > other.time;
        return event_type > other.event_type;
    }

    double time;
    int event_type;
    int task_id;
    int resource_id;
    int slot;
    int id;
};

int Event::ID = 0;

#endif
