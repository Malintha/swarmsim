#include <iostream>
#include <ros/console.h>
#include "Swarm.h"
#include "state.h"
#include "utils.h"
#include <thread>
#include <chrono>

using namespace std;

Swarm::Swarm(const ros::NodeHandle &n, double frequency, int n_drones, bool fileLoad)
    : frequency(frequency), n_drones(n_drones), nh(n) {
  state = States::Idle;
  phase = Phases::Planning;
  // double maxVel = 4;
  // double maxAcc = 5;
  // droneTrajSolver = new Solver(n_drones, maxVel, maxAcc, 2, frequency);
  
  yaml_fpath = "/home/malintha/drone_demo/install/share/swarmsim/launch/traj_data/goals.yaml";
  planningInitialized = false;
  optimizingInitialized = false;
  for (int i=0;i<n_drones;i++) {
    Drone* drone = new Drone(i, nh);
    dronesList.push_back(drone); 
  }

  //loading the full trajectories from files 
  if(fileLoad) {
    vector<Trajectory> trajectories = simutils::loadTrajectoriesFromFile(n_drones, nh, true);
    for(int i=0;i<n_drones;i++) {
      Trajectory traj = trajectories[i];
      dronesList[i]->pushTrajectory(traj);
    }
  }

  //performing online trajectory optimization
  else {
    thread planning_th(simutils::processYamlFile, ref(planningProm), yaml_fpath, 1);
    planning_th.join();
    future<vector<Trajectory> > f = planningProm.get_future();
    vector<Trajectory> trl = f.get();
    cout<<"Retrieved the future: size: "<<trl[0].pos.size()<<endl;
    trl = droneTrajSolver->solve(trl);
    horizonLen = trl[0].pos.size();
    for(int i=0;i<n_drones;i++) {
      dronesList[i]->pushTrajectory(trl[i]);
    }
  }
}

void Swarm::iteration(const ros::TimerEvent &e) {
  switch (state)
  {
  case States::Idle:
    checkSwarmForStates(States::Ready);
    break;

  case States::Ready:
    armDrones(true);
    checkSwarmForStates(States::Armed);
    break;

  case States::Armed:
    TOLService(true);
    checkSwarmForStates(States::Autonomous);
    break;

  case States::Autonomous:
    performPhaseTasks();
    sendPositionSetPoints();
    checkSwarmForStates(States::Reached);
    break;

  case States::Reached:
    TOLService(false);
    break;

  default:
    break;
  }
}

void Swarm::run(float frequency) {
  this->frequency = frequency;
  ros::Timer timer = nh.createTimer(ros::Duration(1 / frequency),
                                      &Swarm::iteration, this);
  ros::spin();
}

void Swarm::setState(int state) {
  this->state = state;
  ROS_DEBUG_STREAM("Set swarm state: "<<state);
}

void Swarm::checkSwarmForStates(int state) {
  bool swarmInState = true;
  for(int i = 0; i < n_drones; i++) {
    bool swarmInStateTemp;
    dronesList[i]->getState() == state ? swarmInStateTemp = true : swarmInStateTemp = false;
    swarmInState = swarmInState && swarmInStateTemp;
}

  if(swarmInState) {
    setState(state);
  }
}

void Swarm::armDrones(bool arm) {
  for(int i=0;i<n_drones;i++) {
    this->dronesList[i]->arm(arm);
  }
}

void Swarm::TOLService(bool takeoff) {
    for(int i=0;i<n_drones;i++) {
    this->dronesList[i]->TOLService(takeoff);
  }
}

void Swarm::sendPositionSetPoints() {
  int execPointer;
  for(int i=0;i<n_drones;i++) {
    execPointer = this->dronesList[i]->executeTrajectory();
  }
  setSwarmPhase(execPointer);
}

/**
 * todo: change these ratios if one wants to use receding horizon planning.
 * eg: plan again when progress is 0.5 if the execution horizon = 0.5*planning horizon
*/
void Swarm::setSwarmPhase(int execPointer) {
  double progress = execPointer/horizonLen;
  if(progress < 0.8) {
    if(progress == 0) {
      planningInitialized = false;
      // optimizingInitialized = false;
      executionInitialized = false;
    }
    phase = Phases::Planning;
  }
  else {
    phase = Phases::Execution;
  }
}

void Swarm::performPhaseTasks() {
  if(phase == Phases::Planning && !planningInitialized) {
    //initialize the external opertaions such as slam or task assignment
    //in this case we only load the waypoints from the yaml file
    planningTh = new thread(simutils::processYamlFile, ref(planningProm), yaml_fpath, 1);
    planningInitialized = true;
  }
  else if(phase == Phases::Execution && !executionInitialized) {
    //get the optimized trajectories from planning future and push them to the drones
    planningTh->join();
    future<vector<Trajectory> > f = planningProm.get_future();
    vector<Trajectory> tr_l = f.get();
    cout<<"Retrieved the future: size: "<<tr_l[0].pos.size();
    //get next wpts from the planning future and attach them to the swarm
    //initialize the trajectory optimization
    executionInitialized = true;
  }
}

// std::vector<Trajectory> Swarm::getTrajectories(int trajecoryId) {
//     wpts = simutils::getTrajectoryList(yaml_fpath, trajecoryId);
//     return droneTrajSolver->solve(wpts);
// }

void Swarm::setWaypoints(vector<Trajectory> wpts, vector<double> tList) {
  if (phase == Phases::Planning) {
    this->wpts = wpts;
    this->tList = tList;
  }
  else {
    ROS_DEBUG_STREAM("Swarm is not in the planning phase. Waypoints rejected");
  }
}
