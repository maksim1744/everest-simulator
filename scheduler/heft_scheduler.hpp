#ifndef SIMULATOR_HEFT_SCHEDULER_HPP_
#define SIMULATOR_HEFT_SCHEDULER_HPP_

#include "scheduler.hpp"
#include "../action.hpp"
#include "../workflow.hpp"

#include <iostream>
#include <algorithm>

struct HeftScheduler: public Scheduler {
    std::vector<Action> assign_available() {
        std::vector<Action> plan;
        for (int res = 0; res < resources.size(); ++res) {
            for (int slot = 0; slot < resources[res].slots; ++slot) {
                if (resources[res].available_slots.count(slot) && !resource_schedules[res][slot].empty()) {
                    auto [start, task] = resource_schedules[res][slot].front();
                    if (dependencies_done(task)) {
                        resource_schedules[res][slot].pop_front();
                        resources[res].used_slots++;
                        resources[res].available_slots.erase(slot);
                        plan.push_back(Action(task, res));
                    }
                }
            }
        }
        return plan;
    }

    std::vector<Action> init(const Settings &settings) override {
        this->settings = settings;
        // task_location.resize(workflow.tasks.size());
        completed.assign(workflow.tasks.size(), false);
        // tasks_on_res.resize(resources.size());
        task_slot.resize(workflow.tasks.size());
        task_res.assign(workflow.tasks.size(), -1);
        resource_schedules.resize(resources.size());
        std::vector<std::vector<std::pair<int, int>>> inv_graph(workflow.tasks.size());
        for (int i = 0; i < workflow.tasks.size(); ++i) {
            for (auto [j, w] : workflow.dependency_graph[i]) {
                inv_graph[j].emplace_back(i, w);
            }
        }

        double avg_resource_time = 0;
        {
            int cnt = 0;
            for (auto res : resources) {
                cnt += res.slots;
                avg_resource_time += res.slots * 1. / res.speed;
            }
        }
        std::vector<double> rank(workflow.tasks.size(), -1);
        std::priority_queue<std::pair<double, int>> pq;

        std::function<void(int)> dfs = [&](int task) {
            rank[task] = 0;
            for (auto [succ, w] : inv_graph[task]) {
                if (rank[succ] == -1)
                    dfs(succ);
                rank[task] = std::max(rank[task], w / settings.net_speed + rank[succ]);
            }
            rank[task] += std::max(1e-3, workflow.tasks[task].weight) * avg_resource_time;
        };
        for (int i = 0; i < workflow.tasks.size(); ++i)
            if (rank[i] == -1)
                dfs(i);

        for (int i = 0; i < workflow.tasks.size(); ++i)
            pq.emplace(rank[i], i);

        std::vector<std::vector<std::set<std::pair<double, double>>>> resource_allocations(resources.size());
        for (size_t i = 0; i < resource_allocations.size(); ++i) {
            resource_allocations[i].assign(resources[i].slots, {{-1., 0.}});  // fake task at time=0 to avoid extra ifs
            resource_schedules[i].resize(resources[i].slots);
        }

        std::vector<double> eft(workflow.tasks.size(), 0);
        double estimated_finish = 0;
        while (!pq.empty()) {
            auto [rank, task] = pq.top();
            completed[task] = true;
            assert(dependencies_done(task));
            pq.pop();
            double ready_time = 0;
            for (auto [succ, w] : workflow.dependency_graph[task]) {
                ready_time = std::max(ready_time, eft[succ]);
            }
            double task_time = std::max(1e-3, workflow.tasks[task].weight);
            int best_res = -1;
            int best_slot = -1;
            double best_transfer_time;
            double best_est = -1;
            for (int res = 0; res < resource_allocations.size(); ++res) {
                double transfer_time = 0;
                for (auto [succ, w] : workflow.dependency_graph[task]) {
                    assert(task_res[succ] != -1);
                    if (settings.optimize_transfers && task_res[succ] == res) continue;
                    transfer_time = std::max(transfer_time, w / settings.net_speed);
                }
                for (int slot = 0; slot < resource_allocations[res].size(); ++slot) {
                    double res_task_time = transfer_time + task_time / resources[res].speed;
                    auto it = resource_allocations[res][slot].lower_bound({ready_time, -1.});
                    assert(it != resource_allocations[res][slot].begin());  // we have fake task
                    it = prev(it);
                    while (next(it) != resource_allocations[res][slot].end() && std::max(ready_time, it->second) + res_task_time >= next(it)->first)
                        ++it;
                    double start_time = std::max(ready_time, it->second);
                    if (best_est == -1 || best_est > start_time) {
                        best_est = start_time;
                        best_slot = slot;
                        best_res = res;
                        best_transfer_time = transfer_time;
                    }
                }
            }
            assert(best_res != -1);
            double start_time = best_est;
            double finish_time = start_time + best_transfer_time + task_time / resources[best_res].speed;
            estimated_finish = std::max(estimated_finish, finish_time);
            resource_allocations[best_res][best_slot].emplace(start_time, finish_time);
            eft[task] = finish_time;
            resource_schedules[best_res][best_slot].emplace_back(start_time, task);
            task_slot[task] = best_slot;
            task_res[task] = best_res;
        }
        for (int i = 0; i < resources.size(); ++i)
            for (int j = 0; j < resource_schedules[i].size(); ++j)
                std::sort(resource_schedules[i][j].begin(), resource_schedules[i][j].end());
        completed.assign(completed.size(), false);

        return assign_available();
    }

    std::vector<Action> notify(const Event &event) override {
        current_time = event.time;
        if (event.event_type == Event::EVENT_TASK_FINISHED) {
            completed[event.task_id] = true;
            resources[event.resource_id].used_slots--;
            resources[event.resource_id].return_slot(task_slot[event.task_id]);
            // tasks_on_res[event.resource_id].erase(event.task_id);
            // task_location[event.task_id] = event.resource_id;
        } else if (event.event_type == Event::EVENT_TASK_FAILED) {
            resources[event.resource_id].used_slots--;
            assert(false);
            // TODO: reschedule task
            // tasks_on_res[event.resource_id].erase(event.task_id);
        } else if (event.event_type == Event::EVENT_RESOURCE_DOWN) {
            resources[event.resource_id].is_up = false;
            // TODO: reschedule task
            assert(false);
            // tasks_on_res[event.resource_id].clear();
        } else if (event.event_type == Event::EVENT_RESOURCE_UP) {
            resources[event.resource_id].is_up = true;
            // TODO: reschedule task
            assert(false);
            resources[event.resource_id].used_slots = 0;
        }
        return assign_available();
    }

    ~HeftScheduler() {}

    Settings settings;
    std::vector<std::vector<std::deque<std::pair<double, int>>>> resource_schedules;
    std::vector<int> task_slot;
    std::vector<int> task_res;
    double current_time = 0;
};

#endif
