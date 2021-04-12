#ifndef SIMULATOR_SIMULATOR_HPP_
#define SIMULATOR_SIMULATOR_HPP_

#include <iomanip>
#include <iostream>
#include <numeric>
#include <queue>
#include <random>
#include <set>
#include <vector>
#include <memory>

#include "event.hpp"
#include "resource.hpp"
#include "scheduler/scheduler.hpp"
#include "settings.hpp"
#include "task.hpp"
#include "workflow.hpp"

struct Simulator {
    void add_resource(Resource resource) {
        resource.id = resources.size();
        resources.push_back(std::move(resource));
    }

    void inject_resource_failure(int resource, double start_time, double duration) {
        if (resource_failures.size() < resources.size())
            resource_failures.resize(resources.size());
        resource_failures[resource].emplace_back(start_time, start_time + duration);
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

        if (!resources[action.resource_id].is_up) {
            std::cerr << "wrong action: resource " << action.resource_id << " is down" << std::endl;
            exit(1);
        }

        if (resources[action.resource_id].used_slots >= resources[action.resource_id].slots) {
            std::cerr << "wrong action: no empty slots on resource " << action.resource_id << std::endl;
            exit(1);
        }

        for (auto [j, w] : workflow.dependency_graph[action.task_id]) {
            if (!completed[j]) {
                std::cerr << "wrong action: not all required tasks completed before task " << action.task_id << std::endl;
                exit(1);
            }
        }

        Event e;
        e.task_id = action.task_id;
        e.resource_id = action.resource_id;
        e.time = current_time + resources[e.resource_id].delay * ud(rnd);
        for (auto [pred, data] : workflow.dependency_graph[e.task_id]) {
            if (settings.optimize_transfers) {
                if (e.resource_id == task_location[pred]) {
                    continue;
                }
            }
            e.time = std::max(e.time, current_time + data);
        }
        e.event_type = Event::EVENT_TASK_STARTED;

        events.push(e);

        // fail task
        if (with_prob(fail_prob)) {
            // fail at random time from 0 to 1.1 * estimated task time
            e.time += workflow.tasks[action.task_id].weight / resources[action.resource_id].speed * ud(rnd);
            e.event_type = Event::EVENT_TASK_FAILED;

            events.push(e);
        } else {
            // normal distibution with mean = estimated time and standard deviation = 10%
            e.time += workflow.tasks[action.task_id].weight / resources[action.resource_id].speed * nd(rnd);
            e.event_type = Event::EVENT_TASK_FINISHED;

            events.push(e);
        }

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
        completion_time.resize(workflow.tasks.size());
        task_location.resize(workflow.tasks.size());
        std::vector<std::set<int>> resource_tasks(resources.size());
        resource_failures.resize(resources.size());

        make_scheduler_actions(scheduler->init(settings));

        for (size_t i = 0; i < resources.size(); ++i) {
            for (auto [start, end] : resource_failures[i]) {
                Event e;
                e.time = start;
                e.event_type = Event::EVENT_RESOURCE_DOWN;
                e.resource_id = i;
                events.push(e);

                e.event_type = Event::EVENT_RESOURCE_UP;
                e.time = end;
                events.push(e);
            }
        }

        if (logging) {
            // print edges for drawing purposes
            for (int i = 0; i < (int)workflow.tasks.size(); ++i) {
                for (auto [j, w] : workflow.dependency_graph[i]) {
                    std::cout << j << "-" << i << " ";
                }
            }
            std::cout << std::endl;
        }

        std::cout << std::fixed;
        std::cout.precision(2);

        while (!events.empty()) {
            Event e = events.top();
            events.pop();
            current_time = e.time;

            if (failed_ids.count(e.id)) {
                continue;
            }

            if (e.event_type == Event::EVENT_TASK_STARTED) {
                if (logging) {
                    std::cout << "time " << std::setw(6) << current_time << ": task " << e.task_id << " started on " << e.resource_id << std::endl;
                }
                resource_tasks[e.resource_id].insert(e.id);
            } else if (e.event_type == Event::EVENT_TASK_FINISHED) {
                if (logging) {
                    std::cout << "time " << std::setw(6) << current_time << ": task " << e.task_id << " finished on " << e.resource_id << std::endl;
                }
                resource_tasks[e.resource_id].erase(e.id);
                resources[e.resource_id].used_slots--;
                completed[e.task_id] = true;
                completion_time[e.task_id] = e.time;
                task_location[e.task_id] = e.resource_id;
                make_scheduler_actions(scheduler->notify(e));
            } else if (e.event_type == Event::EVENT_TASK_FAILED) {
                if (logging) {
                    std::cout << "time " << std::setw(6) << current_time << ": task " << e.task_id << " failed on " << e.resource_id << std::endl;
                }
                resource_tasks[e.resource_id].erase(e.id);
                resources[e.resource_id].used_slots--;
                make_scheduler_actions(scheduler->notify(e));
            } else if (e.event_type == Event::EVENT_RESOURCE_DOWN) {
                if (!resources[e.resource_id].is_up) {
                    std::cerr << "resource " << e.resource_id << " is already down" << std::endl;
                    exit(1);
                }
                if (logging) {
                    std::cout << "time " << std::setw(6) << current_time << ": resource " << e.resource_id << " down" << std::endl;
                }
                resources[e.resource_id].is_up = false;
                for (int id : resource_tasks[e.resource_id])
                    failed_ids.insert(id);
                make_scheduler_actions(scheduler->notify(e));
            } else if (e.event_type == Event::EVENT_RESOURCE_UP) {
                if (resources[e.resource_id].is_up) {
                    std::cerr << "resource " << e.resource_id << " is already up" << std::endl;
                    exit(1);
                }
                if (logging) {
                    std::cout << "time " << std::setw(6) << current_time << ": resource " << e.resource_id << " up" << std::endl;
                }
                resources[e.resource_id].is_up = true;
                resources[e.resource_id].used_slots = 0;
                make_scheduler_actions(scheduler->notify(e));
            }
        }

        if (logging) {
            std::cerr << "done!" << std::endl;
        }
        std::cerr << "time spent: " << current_time << std::endl;
        std::cerr << "tasks completed: " << std::accumulate(completed.begin(), completed.end(), 0) << " / " << workflow.tasks.size() << std::endl;
    }

    int with_prob(double d) {
        std::uniform_real_distribution<> ud{0.0, 1.0};
        return ud(rnd) < d;
    }

    std::vector<Resource> resources;
    Workflow workflow;
    std::shared_ptr<Scheduler> scheduler;
    bool logging = true;

    std::priority_queue<Event, std::vector<Event>, std::greater<Event>> events;
    std::vector<bool> completed;
    std::vector<double> completion_time;
    std::vector<int> task_location;
    double current_time = 0;

    std::mt19937 rnd{123};
    std::normal_distribution<> nd{1, 0.1};
    std::uniform_real_distribution<> ud{0.0, 1.1};
    double fail_prob = 0.5;

    std::set<int> failed_ids;
    std::vector<std::vector<std::pair<double, double>>> resource_failures;

    Settings settings{.optimize_transfers = true};
};

#endif
