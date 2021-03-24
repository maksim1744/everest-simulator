#ifndef SIMULATOR_SIMULATOR_HPP_
#define SIMULATOR_SIMULATOR_HPP_

#include <iomanip>
#include <iostream>
#include <numeric>
#include <queue>
#include <vector>

#include "resource.hpp"
#include "task.hpp"
#include "event.hpp"
#include "scheduler/scheduler.hpp"
#include "workflow.hpp"

struct Simulator {
    void add_resource(Resource resource) {
        resource.id = resources.size();
        resources.push_back(std::move(resource));
    }

    void make_action(const Action &action) {
        if (action.resource_id < 0 || action.resource_id >= (int)resources.size()) {
            std::cerr << "wrong action resource_id" << std::endl;
            exit(1);
        }
        if (action.task_id < 0 || action.task_id >= (int)workflow.tasks.size()) {
            std::cerr << "wrong action task_id" << std::endl;
            exit(1);
        }

        if (completed[action.task_id]) {
            std::cerr << "wrong action: task " << action.task_id << " already completed" << std::endl;
            exit(1);
        }

        if (resources[action.resource_id].used_slots >= resources[action.resource_id].slots) {
            std::cerr << "wrong action: no empty slots on resource " << action.resource_id << std::endl;
            exit(1);
        }

        for (int j : workflow.dependency_graph[action.task_id]) {
            if (!completed[j]) {
                std::cerr << "wrong action: not all required tasks completed before task " << action.task_id << std::endl;
                exit(1);
            }
        }

        Event e;
        e.task_id = action.task_id;
        e.resource_id = action.resource_id;
        e.time = current_time;  // TODO: add time for data transfer
        e.event_type = Event::EVENT_TASK_STARTED;

        events.push(e);

        e.time += workflow.tasks[action.task_id].weight / resources[action.resource_id].speed;  // TODO: add random
        e.event_type = Event::EVENT_TASK_FINISHED;

        events.push(e);

        resources[action.resource_id].used_slots++;
    }

    void make_scheduler_actions(const std::vector<Action> &actions) {
        for (const auto &action : actions) {
            make_action(action);
        }
    }

    void run() {
        workflow.check_correctness();
        scheduler->workflow = workflow;
        scheduler->resources = resources;
        completed.assign(workflow.tasks.size(), false);
        make_scheduler_actions(scheduler->init());

        while (!events.empty()) {
            Event e = events.top();
            events.pop();
            current_time = e.time;

            if (e.event_type == Event::EVENT_TASK_STARTED) {
                if (logging) {
                    std::cout << "time " << std::setprecision(2) << current_time << ": task " << e.task_id << " started on " << e.resource_id << std::endl;
                }
            } else if (e.event_type == Event::EVENT_TASK_FINISHED) {
                if (logging) {
                    std::cout << "time " << std::setprecision(2) << current_time << ": task " << e.task_id << " finished on " << e.resource_id << std::endl;
                }
                resources[e.resource_id].used_slots--;
                completed[e.task_id] = true;
                make_scheduler_actions(scheduler->notify(e));
            }
        }

        if (logging) {
            std::cerr << "done!" << std::endl;
        }
        std::cerr << "time spent: " << current_time << std::endl;
        std::cerr << "tasks completed: " << std::accumulate(completed.begin(), completed.end(), 0) << " / " << workflow.tasks.size() << std::endl;
    }

    std::vector<Resource> resources;
    Workflow workflow;
    Scheduler *scheduler;
    bool logging = true;

    std::priority_queue<Event, std::vector<Event>, std::greater<Event>> events;
    std::vector<bool> completed;
    double current_time = 0;
};

#endif
