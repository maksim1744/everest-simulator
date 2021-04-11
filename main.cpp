#include "simulator.hpp"
#include "workflow.hpp"
#include "scheduler/greedy_scheduler.hpp"
#include "scheduler/heft_scheduler.hpp"

using namespace std;

void simple_test() {
    Simulator simulator;

    // simulator.scheduler = std::shared_ptr<Scheduler>(new GreedyScheduler{});
    simulator.scheduler = std::shared_ptr<Scheduler>(new HeftScheduler{});
    simulator.workflow = get_romboid_workflow();

    simulator.add_resource(Resource(1, 1));
    simulator.add_resource(Resource(1, 2));

    // simulator.inject_resource_failure(1, 3, 5);

    simulator.fail_prob = 0.0;

    simulator.run();
}

void heft_test() {
    Simulator simulator;

    // simulator.scheduler = std::shared_ptr<Scheduler>(new GreedyScheduler{});
    simulator.scheduler = std::shared_ptr<Scheduler>(new HeftScheduler{});
    simulator.workflow = get_heft_workflow();

    simulator.settings.optimize_transfers = true;

    simulator.add_resource(Resource(1, 1));
    simulator.add_resource(Resource(1, 2));
    simulator.add_resource(Resource(1, 1.5));

    simulator.inject_resource_failure(0, 35, 20);
    simulator.inject_resource_failure(1, 5, 4);

    simulator.fail_prob = 0.2;

    simulator.run();
}

int main() {
    heft_test();
    // simple_test();

    return 0;
}
