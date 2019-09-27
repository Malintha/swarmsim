#include "swarmsim/Swarm.h"
#include "ros/ros.h"
#include "tf/tf.h"
#include <iostream>
#include <mavros_msgs/CommandBool.h>
#include <mavros_msgs/CommandTOL.h>
#include <mavros_msgs/SetMode.h>
#include <ros/console.h>

using namespace std;

int main(int argc, char **argv) {
    float frequency = 0.1;
    ros::init(argc, argv, "swarmsim_example");
    static ros::NodeHandle n("~");
    int nDrones;
    bool predefinedTrajectories;
    string yamlFileName;
    string trajDir;

    n.param("/swarmsim_example/nDrones", nDrones, 1);
    n.param("/swarmsim_example/predefined", predefinedTrajectories, false);
    n.getParam("/swarmsim_example/trajDir", trajDir);
    n.getParam("/swarmsim_example/yamlFileName", yamlFileName);

    ROS_DEBUG_STREAM("Number of drones: " << nDrones);
    ROS_DEBUG_STREAM("Use predefined trajectories: " << predefinedTrajectories);
    ROS_DEBUG_STREAM("Trajectory dir: " << trajDir);

    Swarm* sim;

    if(!predefinedTrajectories) {
        ROS_DEBUG_STREAM("YAML file name: " << yamlFileName);
        sim = new Swarm(n, frequency, nDrones, trajDir, yamlFileName);
    }
    else {
        sim = new Swarm(n, frequency, nDrones, trajDir);
    }
    sim->run(frequency);
    return 0;
}
