#include "Visualize.h"
#include <ros/console.h>
#include <yaml.h>
#include <regex>

Visualize::Visualize(ros::NodeHandle nh, string worldframe, int ndrones, string obstacleConfigFilePath)
            : nh(nh), worldframe(worldframe), ndrones(ndrones), 
            obstacleConfigFilePath(obstacleConfigFilePath) {
    obstacles = this->readObstacleConfig();
    this->initMarkers();
    this->markerPub_traj = nh.advertise<visualization_msgs::Marker>("visualization_marker/traj", 10);
    this->markerPub_obs = nh.advertise<visualization_msgs::Marker>("visualization_marker/obs", 10);

}

void Visualize::initMarkers() {
    //marker for the robot trajectories
    for(int i=0;i<this->ndrones;i++)  {
        visualization_msgs::Marker m;
        m.header.stamp = ros::Time::now();
        m.type = visualization_msgs::Marker::LINE_STRIP;
        m.action = visualization_msgs::Marker::ADD;
        m.header.frame_id=this->worldframe;
        m.id = i;
        m.color.r=0; 
        m.color.g=0; 
        m.color.b=1;
        m.color.a = 1;
        m.scale.x = 0.05;
        m.pose.orientation.w = 1;
        this->marker_traj.push_back(m);
    }

    //makers for the obstacles
    for(int i=0;i<obstacles.size();i++) {
        visualization_msgs::Marker m;
        m.header.stamp = ros::Time::now();
        m.type = visualization_msgs::Marker::CUBE;
        m.action = visualization_msgs::Marker::ADD;
        m.header.frame_id=this->worldframe;
        m.id = i;
        m.color.r=0.7; 
        m.color.g=0.7; 
        m.color.b=0.7;
        m.color.a = 1;

        m.pose.position.x = obstacles[i].center[0];
        m.pose.position.y = obstacles[i].center[1];
        m.pose.position.z = obstacles[i].center[2];
        ROS_ERROR_STREAM("obs: "<<obstacles[i].center[0] <<" "<<obstacles[i].center[1]<<" "<<obstacles[i].center[2]);

        m.scale.x = obstacles[i].length;
        m.scale.y = obstacles[i].width;
        m.scale.z = obstacles[i].height;
        
        m.pose.orientation.w = 1;
        this->marker_obs.push_back(m);
    }
}

void Visualize::addToPaths(vector<Trajectory> trajs) {
    for(int i=0; i < trajs.size(); i++) {
        Trajectory traj = trajs[i];
        ROS_DEBUG_STREAM("position list size: "<<traj.pos.size() << " " << traj.pos[0][0] << " " << traj.pos[0][1] << " " << traj.pos[0][2]);

        for (int p = 0; p < traj.pos.size(); p++) {
            geometry_msgs::Point pt;
            pt.x = traj.pos[p][0];
            pt.y = traj.pos[p][1];
            pt.z = traj.pos[p][2];
            this->marker_traj[i].points.push_back(pt);
        }
    }
}

void Visualize::draw() {
    for(int i=0;i<this->ndrones;i++) {
        markerPub_traj.publish(this->marker_traj[i]);
    }
    for(int i = 0; i<obstacles.size(); i++) {
        markerPub_obs.publish(marker_obs[i]);
    }
}

std::vector<Obstacle> Visualize::readObstacleConfig() {
    ROS_DEBUG_STREAM("YAML file path: " << obstacleConfigFilePath);
    char cstr[obstacleConfigFilePath.size() + 1];
    copy(obstacleConfigFilePath.begin(), obstacleConfigFilePath.end(), cstr);
    cstr[obstacleConfigFilePath.size()] = '\0';
    const std::regex obs_regex("obstacle(\\d)");
    std::smatch pieces_match;
    std::vector<Obstacle> obsList;

    try {
        FILE *fh = fopen(cstr, "r");
        yaml_parser_t parser;
        yaml_token_t token;

        if (!yaml_parser_initialize(&parser)) {
            ROS_ERROR_STREAM("Failed to initialize parser!");
        }
        if (fh == NULL) {
            ROS_ERROR_STREAM("Failed to open file!\n" );
        }
        yaml_parser_set_input_file(&parser, fh);
        bool blockMapping, keyToken, valueToken;
        string currentKey;
        Obstacle ob;
        int valueLength;
        do {
            yaml_parser_scan(&parser, &token);
            switch (token.type)
            {
            case YAML_STREAM_START_TOKEN:
                puts("YAMLObstacle: STREAM START");
                break;
            case YAML_STREAM_END_TOKEN:
                puts("YAMLObstacle: STREAM END");
                break;
            case YAML_KEY_TOKEN:
                keyToken = true;
                valueToken = false;
                valueLength = 0;
                break;
            case YAML_VALUE_TOKEN:
                keyToken = false;
                valueToken = true;
                break;
            case YAML_BLOCK_SEQUENCE_START_TOKEN:
                break;
            case YAML_BLOCK_ENTRY_TOKEN:
                break;
            case YAML_BLOCK_END_TOKEN:
                blockMapping = false;
                break;
            case YAML_BLOCK_MAPPING_START_TOKEN:
                blockMapping = true;
                break;
            case YAML_SCALAR_TOKEN:
                char *c = (char *) token.data.scalar.value;
                string s = std::string(c);

                if (keyToken) {
                    currentKey = s;
                }
                if (valueToken) {
                    if(currentKey.compare("center") == 0) {
                        ob.center[valueLength] = std::stod(s);
                    }
                    else if(currentKey.compare("height") == 0) {
                        ob.height = std::stod(s);
                    }
                    else if(currentKey.compare("width") == 0) {
                        ob.width = std::stod(s);
                    }
                    else if(currentKey.compare("length") == 0) {
                        ob.length = std::stod(s);
                        obsList.push_back(ob);
                        printf("YAMLObstacle: Obstacle pushed \n");
                    }
                valueLength++;
                }
                break;

                
            }
            if(token.type != YAML_STREAM_END_TOKEN)
            yaml_token_delete(&token);
        }
        while (token.type != YAML_STREAM_END_TOKEN);
        yaml_token_delete(&token);
    }
    
    catch(range_error &e) {
        ROS_ERROR_STREAM(e.what());
    }
    ROS_DEBUG_STREAM("YAMLObstacle: Length: "<< obsList.size());
    return obsList;
}