#ifndef SIMULATOR_WORKFLOW_HPP_
#define SIMULATOR_WORKFLOW_HPP_

#include <iostream>
#include <functional>

#include "task.hpp"

struct Workflow {
    void add_task(Task task) {
        task.id = tasks.size();
        tasks.push_back(std::move(task));
        dependency_graph.emplace_back();
    }
    void set_tasks(std::vector<Task> tasks) {
        swap(tasks, this->tasks);
        for (size_t i = 0; i < tasks.size(); ++i) {
            if (i != tasks[i].id) {
                std::cerr << "tasks should be in order (or use add_task)" << std:: endl;
                std::exit(1);
            }
        }
        dependency_graph.clear();
        dependency_graph.resize(tasks.size());
    }

    void add_dependency(int from, int to, int weight = 1) {
        dependency_graph.at(to).emplace_back(from, weight);
    }
    void set_dependency_graph(std::vector<std::vector<std::pair<int, int>>> g) {
        swap(g, dependency_graph);
    }

    void check_correctness() const {
        if (tasks.size() != dependency_graph.size()) {
            std::cerr << "size of tasks should be equal to size of dependency_graph" << std::endl;
            exit(1);
        }
        for (size_t i = 0; i < tasks.size(); ++i) {
            for (auto [j, w] : dependency_graph[i]) {
                if (j >= tasks.size()) {
                    std::cerr << "wrong id in dependency_graph" << std::endl;
                    exit(1);
                }
            }
        }

        // check that graph is acyclic
        {
            std::vector<bool> used(tasks.size(), false);
            std::vector<int> pos(tasks.size(), -1);
            int ind = 0;
            std::function<void(int)> dfs = [&](int v) {
                used[v] = true;
                for (auto [k, w] : dependency_graph[v]) {
                    if (!used[k]) {
                        dfs(k);
                    }
                }
                pos[v] = ind++;
            };
            for (size_t i = 0; i < tasks.size(); ++i) {
                if (!used[i]) {
                    dfs(i);
                }
            }
            for (size_t i = 0; i < tasks.size(); ++i) {
                for (auto [j, w] : dependency_graph[i]) {
                    if (pos[i] <= pos[j]) {
                        std::cerr << "there is a cycle in dependency_graph" << std::endl;
                        exit(1);
                    }
                }
            }
        }
    }

    std::vector<Task> tasks;
    // dependency_graph[i] contains [j, w] iff j must be completed before start of i, and there is w data from j to i
    std::vector<std::vector<std::pair<int, int>>> dependency_graph;
};

Workflow get_romboid_workflow() {
    Workflow workflow;
    workflow.add_task(2);
    workflow.add_task(4);
    workflow.add_task(5);
    workflow.add_task(3);
    workflow.add_dependency(0, 1);
    workflow.add_dependency(0, 2);
    workflow.add_dependency(1, 3);
    workflow.add_dependency(2, 3);
    return workflow;
}

Workflow get_heft_workflow() {
    Workflow workflow;

    workflow.add_task(14);
    workflow.add_task(13);
    workflow.add_task(11);
    workflow.add_task(13);
    workflow.add_task(12);
    workflow.add_task(13);
    workflow.add_task(7);
    workflow.add_task(5);
    workflow.add_task(18);
    workflow.add_task(21);

    workflow.add_dependency(0, 1, 18);
    workflow.add_dependency(0, 2, 12);
    workflow.add_dependency(0, 3, 9);
    workflow.add_dependency(0, 4, 11);
    workflow.add_dependency(0, 5, 14);
    workflow.add_dependency(1, 7, 19);
    workflow.add_dependency(1, 8, 16);
    workflow.add_dependency(2, 6, 23);
    workflow.add_dependency(3, 7, 27);
    workflow.add_dependency(3, 8, 23);
    workflow.add_dependency(4, 8, 13);
    workflow.add_dependency(5, 7, 15);
    workflow.add_dependency(6, 9, 17);
    workflow.add_dependency(7, 9, 11);
    workflow.add_dependency(8, 9, 13);

    return workflow;
}

#endif
