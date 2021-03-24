#ifndef SIMULATOR_SCHEDULER_HPP_
#define SIMULATOR_SCHEDULER_HPP_

#include "../action.hpp"
#include "../workflow.hpp"

struct Scheduler {
    bool dependencies_done(size_t task) {
        for (int j : workflow.dependency_graph[task]) {
            if (!completed[j]) {
                return false;
            }
        }
        return true;
    }

    virtual std::vector<Action> init();

    virtual std::vector<Action> notify(const Event &event);

    Workflow workflow;
    std::vector<bool> completed;
    std::vector<Resource> resources;
};

#endif
