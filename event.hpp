#ifndef SIMULATOR_EVENT_HPP_
#define SIMULATOR_EVENT_HPP_

struct Event {
    const static int EVENT_TASK_STARTED = 0;
    const static int EVENT_TASK_FINISHED = 1;
    const static int EVENT_TASK_FAILED = 2;

    bool operator > (const Event &other) const {
        return time > other.time;
    }

    double time;
    int event_type;
    int task_id;
    int resource_id;
};


#endif
