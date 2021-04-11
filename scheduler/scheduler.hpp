#ifndef SIMULATOR_SCHEDULER_HPP_
#define SIMULATOR_SCHEDULER_HPP_

#include "../action.hpp"
#include "../settings.hpp"
#include "../workflow.hpp"

struct Scheduler {
    bool dependencies_done(size_t task) {
        for (auto [j, w] : workflow.dependency_graph[task]) {
            if (!completed[j]) {
                return false;
            }
        }
        return true;
    }

    virtual std::vector<Action> init(const Settings &) { return {}; }

    virtual std::vector<Action> notify(const Event &) { return {}; }

    virtual ~Scheduler() {}

    Workflow workflow;
    std::vector<bool> completed;
    std::vector<Resource> resources;
};

#endif
