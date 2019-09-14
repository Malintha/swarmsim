#include "solver.h"
#include "ros/console.h"
#include <eigen_conversions/eigen_msg.h>
#include <mav_trajectory_generation/polynomial_optimization_nonlinear.h>
#include <mav_trajectory_generation_ros/ros_conversions.h>
#include <mav_trajectory_generation_ros/ros_visualization.h>
#include <mav_trajectory_generation/trajectory_sampling.h>
#include <tf/tf.h>

namespace mtg = mav_trajectory_generation;

Solver::Solver(int nDrones, double maxVel, double maxAcc,
               double frequency)
        : K(nDrones), maxVel(maxVel), maxAcc(maxAcc), nChecks(nChecks) {
    dt = (double) 1 / frequency;
}

vector<Trajectory> Solver::solve(vector<Trajectory> droneWpts) {
    vector<Trajectory> trajList;
    int dimension = 3;
    for (int k = 0; k < K; k++) {
        Trajectory t_k = droneWpts[k];
        // vector<double> tList = droneWpts[k].tList;

        mav_trajectory_generation::Vertex::Vector vertices;
        const int derivative_to_optimize = mav_trajectory_generation::derivative_order::SNAP;
        for (int i = 0; i < t_k.pos.size(); i++) {
            Eigen::Vector3d pos = t_k.pos[i];
            ROS_DEBUG_STREAM("wpt: "<<pos[0]<<" "<<pos[1]<<" "<<pos[2]);	    
	    Eigen::Vector3d rpy = t_k.rpy[i];
            mav_trajectory_generation::Vertex v(dimension);
            if (i == 0 || i == t_k.pos.size() - 1) {
                Eigen::Vector3d vel;
                vel << 0,0,0;
                v.makeStartOrEnd(pos, derivative_to_optimize);
                v.addConstraint(mtg::derivative_order::VELOCITY, vel);
            } else {
                v.addConstraint(mtg::derivative_order::POSITION, pos);
                // v.addConstraint(mtg::derivative_order::ORIENTATION, rpy[2]);
            }
            vertices.push_back(v);
        }
        //calculate segment times
        vector<double> segmentTimes = mav_trajectory_generation::estimateSegmentTimes(vertices, maxVel, maxAcc);
        mtg::NonlinearOptimizationParameters parameters;
        mav_trajectory_generation::PolynomialOptimizationNonLinear<10> opt(3, parameters);
        opt.setupFromVertices(vertices, segmentTimes, derivative_to_optimize);
        opt.addMaximumMagnitudeConstraint(mav_trajectory_generation::derivative_order::VELOCITY, maxVel);
        opt.addMaximumMagnitudeConstraint(mav_trajectory_generation::derivative_order::ACCELERATION, maxAcc);
        opt.optimize();
        mav_trajectory_generation::Trajectory trajectory;
        opt.getTrajectory(&trajectory);
        trajList.push_back(calculateTrajectoryWpts(trajectory));
    }
    return trajList;
}

Trajectory Solver::calculateTrajectoryWpts(mtg::Trajectory& traj) {



    mav_msgs::EigenTrajectoryPoint::Vector flat_states;
    mav_trajectory_generation::sampleWholeTrajectory(traj, dt, &flat_states);
    Trajectory tr;
    for (auto & flat_state : flat_states) {
        Eigen::Quaterniond att;
        att = flat_state.orientation_W_B;
        tf::Quaternion qt(att.x(), att.y(), att.z(), att.w());
        double roll, pitch, yaw;
        tf::Matrix3x3(qt).getRPY(roll, pitch, yaw);
        Eigen::Vector3d rpy;
        rpy << roll, pitch, yaw;

        tr.pos.push_back(flat_state.position_W);
        tr.rpy.push_back(rpy);

        // tr.rpy.push_back(flat_state.;
    }
    return tr;
}
