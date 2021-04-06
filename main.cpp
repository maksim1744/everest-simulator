#include "simulator.hpp"
#include "workflow.hpp"
#include "scheduler/greedy_scheduler.hpp"
#include "scheduler/heft_scheduler.hpp"

using namespace std;

void simple_test() {
    Simulator simulator;

    simulator.scheduler = new GreedyScheduler{};
    simulator.workflow = get_romboid_workflow();

    simulator.add_resource(Resource(1, 1));
    simulator.add_resource(Resource(1, 1));

    simulator.fail_prob = 0.5;

    simulator.run();
}

void heft_test() {
    Simulator simulator;

    // simulator.scheduler = new GreedyScheduler{};
    simulator.scheduler = new HeftScheduler{};
    simulator.workflow = get_heft_workflow();

    simulator.add_resource(Resource(1, 1));
    simulator.add_resource(Resource(1, 2));
    simulator.add_resource(Resource(1, 1.5));

    simulator.fail_prob = 0.2;

    simulator.run();
}

int main() {
    heft_test();
    // simple_test();

    return 0;
}
