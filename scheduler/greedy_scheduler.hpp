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
                for (size_t j = 0; j < resources.size(); ++j) {
                    if (resources[j].used_slots < resources[j].slots && resources[j].is_up) {
                        ++resources[j].used_slots;
                        actions.emplace_back(i, j);
                        scheduled[i] = true;
                        tasks_on_res[j].insert(i);
                        break;
                    }
                }
            }
        }
        return actions;
    }

    std::vector<Action> init(const Settings &) override {
        completed.assign(workflow.tasks.size(), false);
        scheduled.assign(workflow.tasks.size(), false);
        tasks_on_res.resize(resources.size());
        return assign_available();
    }

    std::vector<Action> notify(const Event &event) override {
        if (event.event_type == Event::EVENT_TASK_FINISHED) {
            completed[event.task_id] = true;
            resources[event.resource_id].used_slots--;
            tasks_on_res[event.resource_id].erase(event.task_id);
        } else if (event.event_type == Event::EVENT_TASK_FAILED) {
            resources[event.resource_id].used_slots--;
            scheduled[event.task_id] = false;
            tasks_on_res[event.resource_id].erase(event.task_id);
        } else if (event.event_type == Event::EVENT_RESOURCE_DOWN) {
            resources[event.resource_id].is_up = false;
            for (int task : tasks_on_res[event.resource_id]) {
                scheduled[task] = false;
            }
            tasks_on_res[event.resource_id].clear();
        } else if (event.event_type == Event::EVENT_RESOURCE_UP) {
            resources[event.resource_id].is_up = true;
            resources[event.resource_id].used_slots = 0;
        }
        return assign_available();
    }

    ~GreedyScheduler() {}

    std::vector<bool> scheduled;
    std::vector<std::set<int>> tasks_on_res;
};

#endif
