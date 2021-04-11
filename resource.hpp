#ifndef SIMULATOR_RESOURCE_HPP_
#define SIMULATOR_RESOURCE_HPP_

struct Resource {
    Resource(int slots = 1, double speed = 1) : slots(slots), speed(speed) {}

    int slots;
    int used_slots = 0;
    double speed;  // more -- faster
    bool is_up = true;
    int id;
};

#endif
