#ifndef SIMULATOR_GRREDY_SCHEDULER_HPP_
#define SIMULATOR_GRREDY_SCHEDULER_HPP_

#include "scheduler.hpp"
#include "../action.hpp"
#include "../workflow.hpp"

#include <iostream>

struct GreedyScheduler: public Scheduler {
    std::vector<Action> assign_available() {
        std::vector<Action> actions;
        for (size_t i = 0; i < workflow.tasks.size(); ++i) {
            if (!completed[i] && !scheduled[i] && dependencies_done(i)) {
                int res = -1;
                for (size_t j = 0; j < resources.size(); ++j) {
                    if (resources[j].used_slots < resources[j].slots) {
                        ++resources[j].used_slots;
                        actions.emplace_back(i, j);
                        scheduled[i] = true;
                        break;
                    }
                }
            }
        }
        return actions;
    }

    std::vector<Action> init() override {
        completed.assign(workflow.tasks.size(), false);
        scheduled.assign(workflow.tasks.size(), false);
        return assign_available();
    }

    std::vector<Action> notify(const Event &event) override {
        if (event.event_type == Event::EVENT_TASK_FINISHED) {
            completed[event.task_id] = true;
            resources[event.resource_id].used_slots--;
        }
        return assign_available();
    }

    std::vector<bool> scheduled;
};

#endif
