#ifndef SIMULATOR_EVENT_HPP_
#define SIMULATOR_EVENT_HPP_

struct Event {
    const static int EVENT_TASK_STARTED = 0;
    const static int EVENT_TASK_FINISHED = 1;
    const static int EVENT_TASK_FAILED = 2;
    const static int EVENT_RESOURCE_DOWN = 3;
    const static int EVENT_RESOURCE_UP = 4;
    static int ID;

    Event() : id(ID++) {}

    bool operator > (const Event &other) const {
        return time > other.time;
    }

    double time;
    int event_type;
    int task_id;
    int resource_id;
    int id;
};

int Event::ID = 0;

#endif
