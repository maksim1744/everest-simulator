#ifndef SIMULATOR_ACTION_HPP_
#define SIMULATOR_ACTION_HPP_

// submit task on a resource
struct Action {
    Action() {}
    Action(int task_id, int resource_id) : task_id(task_id), resource_id(resource_id) {}

    int task_id = -1;
    int resource_id = -1;
};

#endif
