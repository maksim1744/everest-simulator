#ifndef SIMULATOR_HEFT_SCHEDULER_HPP_
#define SIMULATOR_HEFT_SCHEDULER_HPP_

#include "scheduler.hpp"
#include "../action.hpp"
#include "../workflow.hpp"

#include <iostream>

struct HeftScheduler: public Scheduler {
    std::vector<Action> assign_available() {
        std::vector<Action> actions;
        while (!pq.empty()) {
            auto [rk, task] = pq.top();
            bool can_schedule = true;
            for (auto [j, w] : workflow.dependency_graph[task]) {
                if (!completed[j]) {
                    can_schedule = false;
                    break;
                }
            }
            if (!can_schedule) break;
            bool scheduled = false;
            int best_resource = -1;
            double best_eft;
            for (auto &res : resources) {
                if (res.used_slots < res.slots && res.is_up) {
                    double eft = 0;
                    for (auto [j, w] : workflow.dependency_graph[task]) {
                        if (res.id == task_location[j]) continue;
                        eft = std::max(eft, (double)w / settings.net_speed);
                    }
                    eft += workflow.tasks[task].weight / res.speed;
                    if (best_resource == -1 || eft < best_eft) {
                        best_eft = eft;
                        best_resource = res.id;
                        scheduled = true;
                    }
                }
            }
            if (!scheduled) break;
            ++resources[best_resource].used_slots;
            actions.push_back(Action{task, best_resource});
            tasks_on_res[best_resource].insert(task);
            pq.pop();
        }
        return actions;
    }

    std::vector<Action> init(const Settings &settings) override {
        this->settings = settings;
        task_location.resize(workflow.tasks.size());
        completed.assign(workflow.tasks.size(), false);
        tasks_on_res.resize(resources.size());
        std::vector<std::vector<std::pair<int, int>>> inv_graph(workflow.tasks.size());
        for (int i = 0; i < workflow.tasks.size(); ++i) {
            for (auto [j, w] : workflow.dependency_graph[i]) {
                inv_graph[j].emplace_back(i, w);
            }
        }
        rank.assign(workflow.tasks.size(), -1);
        std::function<void(int)> dfs = [&](int task) {
            rank[task] = 0;
            for (auto [succ, w] : inv_graph[task]) {
                if (rank[succ] == -1)
                    dfs(succ);
                rank[task] = std::max(rank[task], w / settings.net_speed + rank[succ]);
            }
            double avg_time = 0;
            for (auto res : resources) {
                avg_time += workflow.tasks[task].weight / res.speed;
            }
            avg_time /= resources.size();
            rank[task] += avg_time;
        };
        for (int i = 0; i < workflow.tasks.size(); ++i)
            if (rank[i] == -1)
                dfs(i);

        for (int i = 0; i < workflow.tasks.size(); ++i)
            pq.emplace(rank[i], i);

        return assign_available();
    }

    std::vector<Action> notify(const Event &event) override {
        if (event.event_type == Event::EVENT_TASK_FINISHED) {
            completed[event.task_id] = true;
            resources[event.resource_id].used_slots--;
            tasks_on_res[event.resource_id].erase(event.task_id);
            task_location[event.task_id] = event.resource_id;
        } else if (event.event_type == Event::EVENT_TASK_FAILED) {
            resources[event.resource_id].used_slots--;
            pq.emplace(rank[event.task_id], event.task_id);
            tasks_on_res[event.resource_id].erase(event.task_id);
        } else if (event.event_type == Event::EVENT_RESOURCE_DOWN) {
            resources[event.resource_id].is_up = false;
            for (int task : tasks_on_res[event.resource_id]) {
                pq.emplace(rank[task], task);
            }
            tasks_on_res[event.resource_id].clear();
        } else if (event.event_type == Event::EVENT_RESOURCE_UP) {
            resources[event.resource_id].is_up = true;
            resources[event.resource_id].used_slots = 0;
        }
        return assign_available();
    }

    ~HeftScheduler() {}

    std::vector<bool> scheduled;
    std::vector<double> rank;
    std::priority_queue<std::pair<double, int>> pq;
    std::vector<std::set<int>> tasks_on_res;
    std::vector<int> task_location;

    Settings settings;
};

#endif
