#ifndef SIMULATOR_RESOURCE_HPP_
#define SIMULATOR_RESOURCE_HPP_

#include <set>

struct Resource {
    Resource(int slots = 1, double speed = 1, double delay = 0) : slots(slots), speed(speed), delay(delay) {
        fill_slots();
    }

    void fill_slots() {
        for (int i = 0; i < slots; ++i)
            available_slots.insert(i);
    }

    int get_slot() {
        int x = *available_slots.begin();
        available_slots.erase(available_slots.begin());
        return x;
    }

    void return_slot(int x) {
        available_slots.insert(x);
    }

    int slots;
    int used_slots = 0;
    double speed;  // more -- faster
    bool is_up = true;
    int id;
    double delay = 0;
    std::set<int> available_slots;
};

#endif
