#include "PlanningPhase.h"
#include<ros/console.h>

PlanningPhase::PlanningPhase() {}

PlanningPhase::PlanningPhase(int nDrones, double frequency) : nDrones(nDrones), frequency(frequency) {
    maxVelocity = 4;
    maxAcceleration = 4;
    doneInitPlanning = false;
}

vector<Trajectory> PlanningPhase::computeSmoothTrajectories(bool initialQP, bool lastQP, std::vector<Trajectory> prevPlan) {
    Solver* solver = new Solver(nDrones, maxVelocity, maxAcceleration, frequency);
    vector<Trajectory> results = solver->solve(discreteWpts, initialQP, lastQP, prevPlan);
    delete solver;
    return results;
}

vector<Trajectory> PlanningPhase::getPlanningResults() {
    //wait for the initial planning to finish
    do {
        ROS_DEBUG_ONCE("Waiting for initial planning to finish");
    }
    while(!doneInitPlanning); 
    planning_t->join();
    vector<Trajectory> results;
    try {
        results = fut.get();
    }
    catch(future_error& e) {
        ROS_ERROR_STREAM("Caught a future_error while getting the future\"" << e.what());
    }
    return results;
}

void PlanningPhase::doPlanning(int horizonId, std::vector<Trajectory> prevPlan) {}
vector<Trajectory> PlanningPhase::getDiscretePlan(int horizonId) {}
