#ifndef SIMULATOR_ADAPTIVE_SCHEDULER_HPP_
#define SIMULATOR_ADAPTIVE_SCHEDULER_HPP_

#include "scheduler.hpp"
#include "../action.hpp"
#include "../workflow.hpp"

#include <iostream>
#include <algorithm>
#include <map>

struct AdaptiveScheduler: public Scheduler {
    double get_task_res_time(int task, int resource) const {
        double transfer_time = 0;
        for (auto [succ, w] : workflow.dependency_graph[task]) {
            assert(task_res[succ] != -1);
            if (settings.optimize_transfers && task_res[succ] == resource) continue;
            transfer_time = std::max(transfer_time, w / settings.net_speed);
        }
        double task_time = std::max(1e-3, workflow.tasks[task].weight);
        return transfer_time + task_time / resources[resource].speed + resources[resource].delay * 0.55;
    }

    std::vector<Action> assign_available() {
        std::vector<Action> plan;
        for (int res = 0; res < resources.size(); ++res) {
            if (!resources[res].is_up) continue;
            for (int slot = 0; slot < resources[res].slots; ++slot) {
                if (resources[res].available_slots.count(slot) && !resource_schedules[res][slot].empty()) {
                    auto [start, task] = *resource_schedules[res][slot].begin();
                    if (dependencies_done(task)) {
                        resource_schedules[res][slot].erase(resource_schedules[res][slot].begin());
                        resources[res].used_slots++;
                        resources[res].available_slots.erase(slot);
                        plan.push_back(Action(task, res));
                        tasks_on_res[res].insert(task);
                        task_eft[task] = start + get_task_res_time(task, res);
                        slot_free_time[res][slot] = task_eft[task];
                        scheduled[task] = true;
                    }
                }
            }
        }
        return plan;
    }

    void find_equivalency_classes() {
        std::vector<std::vector<int>> g(workflow.tasks.size());
        for (int i = 0; i < workflow.tasks.size(); ++i) {
            for (auto [j, w] : workflow.dependency_graph[i]) {
                g[j].emplace_back(i);
            }
        }
        std::vector<int> order;

        std::vector<bool> u(workflow.tasks.size(), false);
        std::function<void(int)> dfs = [&](int task) {
            u[task] = true;
            for (auto succ : g[task]) {
                if (!u[succ])
                    dfs(succ);
            }
            order.push_back(task);
        };
        for (int i = 0; i < workflow.tasks.size(); ++i)
            if (!u[i])
                dfs(i);

        std::reverse(order.begin(), order.end());

        std::vector<int> forward_class(workflow.tasks.size());
        std::map<std::vector<int>, int> class_by_pred;
        for (int task : order) {
            std::vector<int> cur;
            for (auto [j, w] : workflow.dependency_graph[task])
                cur.push_back(forward_class[j]);
            sort(cur.begin(), cur.end());
            if (class_by_pred.count(cur)) {
                forward_class[task] = class_by_pred[cur];
            } else {
                forward_class[task] = class_by_pred.size();
                class_by_pred[cur] = forward_class[task];
            }
        }

        std::reverse(order.begin(), order.end());
        std::vector<int> backward_class(workflow.tasks.size());
        std::map<std::vector<std::pair<int, int>>, int> class_by_succ;
        for (int task : order) {
            std::vector<std::pair<int, int>> cur;
            for (auto j : g[task])
                cur.emplace_back(backward_class[j], forward_class[j]);
            sort(cur.begin(), cur.end());
            if (class_by_succ.count(cur)) {
                backward_class[task] = class_by_succ[cur];
            } else {
                backward_class[task] = class_by_succ.size();
                class_by_succ[cur] = backward_class[task];
            }
        }

        std::map<std::pair<int, int>, std::vector<int>> by_class;
        for (int i = 0; i < workflow.tasks.size(); ++i)
            by_class[{forward_class[i], backward_class[i]}].push_back(i);

        task_class.resize(workflow.tasks.size());
        int last_class = 0;
        for (auto [a, v] : by_class) {
            for (int k : v) {
                task_class[k] = last_class;
            }
            ++last_class;
        }

        avg_time_s.assign(last_class, 0);
        avg_time_c.assign(last_class, 0);

        // for (auto [a, b] : by_class) {
        //     std::cerr << "(" << a.first << ", " << a.second << "): ";
        //     for (int k : b)
        //         std::cerr << k << ' ';
        //     std::cerr << std::endl;
        // }
    }

    void remove_profile() {
        for (int i = 0; i < workflow.tasks.size(); ++i) {
            for (auto &[j, w] : workflow.dependency_graph[i])
                w = 0;
            workflow.tasks[i].weight = 1;
        }
        for (int i = 0; i < resources.size(); ++i) {
            resources[i].speed = 1;
            resources[i].delay = 0;
        }

        find_equivalency_classes();
    }

    void assign_heuristic_weights() {
        for (int i = 0; i < workflow.tasks.size(); ++i) {
            if (avg_time_c[task_class[i]] == 0)
                workflow.tasks[i].weight = total_avg_time_s / total_avg_time_c * 1.;
            else
                workflow.tasks[i].weight = avg_time_s[task_class[i]] / avg_time_c[task_class[i]] * 1.;
        }
    }

    void init_ranks() {
        rank.assign(workflow.tasks.size(), -1);

        double avg_resource_time = 0;
        {
            int cnt = 0;
            for (auto res : resources) {
                cnt += res.slots;
                avg_resource_time += res.slots * 1. / res.speed;
            }
        }

        std::vector<std::vector<std::pair<int, int>>> inv_graph(workflow.tasks.size());
        for (int i = 0; i < workflow.tasks.size(); ++i) {
            for (auto [j, w] : workflow.dependency_graph[i]) {
                inv_graph[j].emplace_back(i, w);
            }
        }

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
    }

    void init_heft() {
        tasks_on_res.resize(resources.size());
        task_slot.resize(workflow.tasks.size());
        task_res.resize(workflow.tasks.size());
        task_est.resize(workflow.tasks.size());
        task_eft.resize(workflow.tasks.size());
        scheduled.assign(workflow.tasks.size(), false);

        if (!profile)
            remove_profile();

        slot_free_time.resize(resources.size());
        for (size_t i = 0; i < slot_free_time.size(); ++i)
            slot_free_time[i].assign(resources[i].slots, 0);

        init_ranks();
    }

    void run_heft() {
        std::priority_queue<std::pair<double, int>> pq;
        for (int i = 0; i < workflow.tasks.size(); ++i)
            pq.emplace(rank[i], i);

        resource_schedules.clear();
        resource_schedules.resize(resources.size());

        std::vector<std::vector<std::set<std::pair<double, double>>>> resource_allocations(resources.size());
        for (size_t i = 0; i < resource_allocations.size(); ++i) {
            resource_allocations[i].resize(resources[i].slots);  // fake task at time=0 to avoid extra ifs
            for (size_t j = 0; j < resources[i].slots; ++j) {
                resource_allocations[i][j].emplace(-1, slot_free_time[i][j]);
            }
            resource_schedules[i].resize(resources[i].slots);
        }

        std::vector<double> eft(workflow.tasks.size(), 0);
        double estimated_finish = 0;
        while (!pq.empty()) {
            auto [rank, task] = pq.top();
            pq.pop();
            if (scheduled[task] || completed[task]) continue;
            double ready_time = current_time;
            for (auto [succ, w] : workflow.dependency_graph[task]) {
                ready_time = std::max(ready_time, task_eft[succ]);
            }
            int best_res = -1;
            int best_slot = -1;
            double best_est = -1;
            for (int res = 0; res < resource_allocations.size(); ++res) {
                if (!resources[res].is_up) continue;
                for (int slot = 0; slot < resource_allocations[res].size(); ++slot) {
                    double res_task_time = get_task_res_time(task, res);
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
                    }
                }
            }
            assert(best_res != -1);
            double start_time = best_est;
            double finish_time = start_time + get_task_res_time(task, best_res);
            estimated_finish = std::max(estimated_finish, finish_time);
            resource_allocations[best_res][best_slot].emplace(start_time, finish_time);
            resource_schedules[best_res][best_slot].emplace(start_time, task);
            task_slot[task] = best_slot;
            task_res[task] = best_res;
            task_est[task] = start_time;
            task_eft[task] = finish_time;
        }
        // std::cerr << "estimated finish: " << estimated_finish << std::endl;
    }

    std::vector<Action> init(const Settings &settings) override {
        this->settings = settings;
        completed.assign(workflow.tasks.size(), false);

        init_heft();
        run_heft();

        return assign_available();
    }

    std::vector<Action> notify(const Event &event) override {
        current_time = event.time;
        if (event.event_type == Event::EVENT_TASK_FINISHED) {
            completed[event.task_id] = true;
            resources[event.resource_id].used_slots--;
            resources[event.resource_id].return_slot(task_slot[event.task_id]);
            tasks_on_res[event.resource_id].erase(event.task_id);
            task_eft[event.task_id] = current_time;
            int task = event.task_id;
            slot_free_time[task_res[task]][task_slot[task]] = current_time;

            if (!profile) {
                total_avg_time_s += task_eft[event.task_id] - task_est[event.task_id];
                total_avg_time_c += 1;
                avg_time_s[task_class[event.task_id]] += task_eft[event.task_id] - task_est[event.task_id];
                avg_time_c[task_class[event.task_id]] += 1;
                assign_heuristic_weights();
                init_ranks();
                if (std::accumulate(completed.begin(), completed.end(), 0) == completed.size()) {
                    for (int i = 0; i < workflow.tasks.size(); ++i)
                        std::cerr << i << ": " << avg_time_s[task_class[i]] / avg_time_c[task_class[i]] << ' ' << task_eft[i] - task_est[i] << std::endl;
                }
            }
        } else if (event.event_type == Event::EVENT_TASK_FAILED) {
            resources[event.resource_id].used_slots--;
            int task = event.task_id;
            resource_schedules[task_res[task]][task_slot[task]].emplace(task_est[task], task);
            resources[event.resource_id].return_slot(task_slot[task]);
            tasks_on_res[event.resource_id].erase(task);
            scheduled[task] = false;
            slot_free_time[task_res[task]][task_slot[task]] = current_time;
        } else if (event.event_type == Event::EVENT_RESOURCE_DOWN) {
            resources[event.resource_id].is_up = false;
            for (int task : tasks_on_res[event.resource_id]) {
                resource_schedules[task_res[task]][task_slot[task]].emplace(task_est[task], task);
                scheduled[task] = false;
            }
            tasks_on_res[event.resource_id].clear();
        } else if (event.event_type == Event::EVENT_RESOURCE_UP) {
            resources[event.resource_id].is_up = true;
            resources[event.resource_id].used_slots = 0;
            resources[event.resource_id].fill_slots();
            slot_free_time[event.resource_id].assign(resources[event.resource_id].slots, current_time);
        }
        run_heft();
        return assign_available();
    }

    ~AdaptiveScheduler() {}

    Settings settings;
    std::vector<double> rank;
    std::vector<std::vector<double>> slot_free_time;
    std::vector<std::vector<std::set<std::pair<double, int>>>> resource_schedules;
    std::vector<int> task_slot;
    std::vector<int> task_res;
    std::vector<double> task_est;
    std::vector<double> task_eft;
    std::vector<std::set<int>> tasks_on_res;
    std::vector<bool> scheduled;
    double current_time = 0;

    bool profile = true;
    std::vector<double> avg_time_s;
    std::vector<double> avg_time_c;
    double total_avg_time_s = 0;
    double total_avg_time_c = 0;
    std::vector<int> task_class;
};

#endif
