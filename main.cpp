#include "simulator.hpp"
#include "workflow.hpp"
#include "scheduler/greedy_scheduler.hpp"

using namespace std;

int main() {
    Simulator simulator;

    simulator.scheduler = new GreedyScheduler{};
    simulator.workflow = get_romboid_workflow();

    simulator.add_resource(Resource(1, 1));
    simulator.add_resource(Resource(1, 1));

    simulator.run();

    return 0;
}
