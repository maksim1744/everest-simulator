#ifndef SIMULATOR_WORKFLOW_HPP_
#define SIMULATOR_WORKFLOW_HPP_

#include <iostream>
#include <functional>

struct Workflow {
    void add_task(Task task) {
        task.id = tasks.size();
        tasks.push_back(std::move(task));
        dependency_graph.emplace_back();
    }
    void set_task(std::vector<Task> tasks) {
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

    void add_dependency(int v, int succ) {
        dependency_graph.at(v).push_back(succ);
    }
    void set_dependency_graph(std::vector<std::vector<int>> g) {
        swap(g, dependency_graph);
    }

    void check_correctness() const {
        if (tasks.size() != dependency_graph.size()) {
            std::cerr << "size of tasks should be equal to size of dependency_graph" << std::endl;
            exit(1);
        }
        for (size_t i = 0; i < tasks.size(); ++i) {
            for (size_t j : dependency_graph[i]) {
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
                for (int k : dependency_graph[v]) {
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
                for (size_t j : dependency_graph[i]) {
                    if (pos[i] < pos[j]) {
                        std::cerr << "there is a cycle in dependency_graph" << std::endl;
                        exit(1);
                    }
                }
            }
        }
    }

    std::vector<Task> tasks;
    std::vector<std::vector<int>> dependency_graph;  // dependency_graph[i] contains j iff j must be completed before start of i
};

Workflow get_romboid_workflow() {
    Workflow workflow;
    workflow.add_task(2);
    workflow.add_task(4);
    workflow.add_task(5);
    workflow.add_task(3);
    workflow.add_dependency(1, 0);
    workflow.add_dependency(2, 0);
    workflow.add_dependency(3, 1);
    workflow.add_dependency(3, 2);
    return workflow;
}

#endif
