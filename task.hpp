#ifndef SIMULATOR_TASK_HPP_
#define SIMULATOR_TASK_HPP_

struct Task {
    Task(double weight = 1) : weight(weight) {}

    double weight;  // more -- slower
    int id;
};

#endif
