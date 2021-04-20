#ifndef SIMULATOR_JSON_LOADER_HPP_
#define SIMULATOR_JSON_LOADER_HPP_

#include "nlohmann/json.hpp"
#include "simulator.hpp"
#include "scheduler/scheduler.hpp"
#include "scheduler/greedy_scheduler.hpp"
#include "scheduler/heft_scheduler.hpp"
#include "scheduler/adaptive_scheduler.hpp"

#include <fstream>
#include <iostream>
#include <string>
#include <memory>

namespace json_loader {

using json = nlohmann::json;

Simulator load(const std::string &scheduler,
               const std::string &workflow_file,
               const std::string &resources_file,
               const std::string &settings_file,
               const std::string &failures_file) {
    Simulator simulator;

    auto error = [](const std::string &msg) {
        std::cerr << msg << std::endl;
        exit(1);
    };

    if (scheduler == "greedy") {
        simulator.scheduler = std::shared_ptr<Scheduler>(new GreedyScheduler{});
    } else if (scheduler == "heft") {
        simulator.scheduler = std::shared_ptr<Scheduler>(new HeftScheduler{});
    } else if (scheduler == "adaptive") {
        simulator.scheduler = std::shared_ptr<Scheduler>(new AdaptiveScheduler{});
    } else {
        error("wrong scheduler");
    }

    // settings
    {
        std::ifstream i(settings_file);
        json settings;
        i >> settings;

        if (!settings.contains("net_speed")) {
            error("need to specify settings/net_speed");
        }
        if (!settings.contains("optimize_transfers")) {
            error("need to specify settings/optimize_transfers");
        }
        if (!settings.contains("task_fail_prob")) {
            error("need to specify settings/task_fail_prob");
        }
        if (!settings.contains("logging")) {
            error("need to specify settings/logging");
        }
        simulator.settings.net_speed = settings["net_speed"].get<double>();
        simulator.settings.optimize_transfers = settings["optimize_transfers"].get<bool>();
        simulator.fail_prob = settings["task_fail_prob"].get<double>();
        simulator.logging = settings["logging"].get<bool>();
    }

    {
        std::ifstream i(workflow_file);
        json workflow_data;
        i >> workflow_data;

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
    }

    {
        std::ifstream i(resources_file);
        json resources;
        i >> resources;

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
    }

    if (failures_file != "") {
        std::ifstream i(failures_file);
        json failures;
        i >> failures;

        for (auto failure : failures) {
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
