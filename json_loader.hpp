#ifndef SIMULATOR_JSON_LOADER_HPP_
#define SIMULATOR_JSON_LOADER_HPP_

#include "nlohmann/json.hpp"
#include "simulator.hpp"
#include "scheduler/scheduler.hpp"
#include "scheduler/greedy_scheduler.hpp"
#include "scheduler/heft_scheduler.hpp"

#include <fstream>
#include <iostream>
#include <string>
#include <memory>

namespace json_loader {

using json = nlohmann::json;

Simulator load(const std::string &filename) {
    std::ifstream i(filename);
    json data;
    i >> data;

    Simulator simulator;

    auto error = [](const std::string &msg) {
        std::cerr << msg << std::endl;
        exit(1);
    };

    if (!data.contains("scheduler")) {
        error("need to specify scheduler");
    }
    if (!data.contains("settings")) {
        error("need to specify settings");
    }
    if (!data.contains("workflow")) {
        error("need to specify workflow");
    }
    if (!data.contains("resources")) {
        error("need to specify resources");
    }

    if (data["scheduler"].get<std::string>() == "greedy") {
        simulator.scheduler = std::shared_ptr<Scheduler>(new GreedyScheduler{});
    } else if (data["scheduler"].get<std::string>() == "heft") {
        simulator.scheduler = std::shared_ptr<Scheduler>(new HeftScheduler{});
    } else {
        error("wrong scheduler");
    }

    auto settings = data["settings"];
    if (!settings.contains("net_speed")) {
        error("need to specify settings/net_speed");
    }
    if (!settings.contains("optimize_transfers")) {
        error("need to specify settings/optimize_transfers");
    }
    if (!settings.contains("task_fail_prob")) {
        error("need to specify settings/task_fail_prob");
    }

    simulator.settings.net_speed = settings["net_speed"].get<double>();
    simulator.settings.optimize_transfers = settings["optimize_transfers"].get<bool>();
    simulator.fail_prob = settings["task_fail_prob"].get<double>();

    auto workflow_data = data["workflow"];
    if (!workflow_data.contains("tasks")) {
        error("need to specify workflow/tasks");
    }
    if (!workflow_data.contains("edges")) {
        error("need to specify workflow/edges");
    }

    Workflow workflow;

    auto tasks = workflow_data["tasks"].get<std::vector<double>>();
    for (auto task : tasks)
        workflow.add_task(task);

    auto edges = workflow_data["edges"];
    for (auto edge : edges) {
        if (!edge.contains("from")) {
            error("need to specify workflow/edges[i]/from");
        }
        if (!edge.contains("to")) {
            error("need to specify workflow/edges[i]/to");
        }
        if (!edge.contains("weight")) {
            error("need to specify workflow/edges[i]/weight");
        }
        workflow.add_dependency(
            edge["from"].get<int>(),
            edge["to"].get<int>(),
            edge["weight"].get<double>()
        );
    }

    simulator.workflow = workflow;

    auto resources = data["resources"];
    for (auto resource : resources) {
        if (!resource.contains("slots")) {
            error("need to specify resources[i]/slots");
        }
        if (!resource.contains("speed")) {
            error("need to specify resources[i]/speed");
        }
        if (!resource.contains("delay")) {
            error("need to specify resources[i]/delay");
        }
        simulator.add_resource(Resource(
            resource["slots"].get<int>(),
            resource["speed"].get<double>(),
            resource["delay"].get<double>()
        ));
    }

    if (data.contains("resource_failures")) {
        for (auto failure : data["resource_failures"]) {
            if (!failure.contains("resource")) {
                error("need to specify resource_failures[i]/resource");
            }
            if (!failure.contains("start")) {
                error("need to specify resource_failures[i]/start");
            }
            if (!failure.contains("duration")) {
                error("need to specify resource_failures[i]/duration");
            }
            simulator.inject_resource_failure(
                failure["resource"].get<int>(),
                failure["start"].get<double>(),
                failure["duration"].get<double>()
            );
        }
    }

    return simulator;
}

}

#endif
