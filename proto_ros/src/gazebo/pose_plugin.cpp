#include <functional>
#include <gazebo/gazebo.hh>
#include <gazebo/common/common.hh>
#include <gazebo/physics/physics.hh>
#include <gazebo/gui/model/ModelEditorEvents.hh>
#include <ignition/math/Vector3.hh>

#include <ros/ros.h>
#include <geometry_msgs/Pose.h>
#include <geometry_msgs/PoseStamped.h>

#include <proto/proto.hpp>
#include "ros/ros_util.hpp"
#include "gazebo/gazebo_util.hpp"

namespace gazebo {

struct pose_plugin_t : ModelPlugin {
  physics::ModelPtr model_;
  sdf::ElementPtr sdf_;
  event::ConnectionPtr conn_;

  // ROS
  std::thread ros_th_;
  ros::NodeHandle *ros_nh_;
  ros::Publisher pose_pub_;
  ros::Subscriber pose_sub_;
  size_t ros_seq_ = 0;

  pose_plugin_t() {}
  ~pose_plugin_t() { delete ros_nh_; }

  void Load(physics::ModelPtr parent, sdf::ElementPtr sdf) {
    // Gazebo things
    model_ = parent;
    sdf_ = sdf;
    conn_ = event::Events::ConnectWorldUpdateBegin(
        std::bind(&pose_plugin_t::on_update, this));

    // Start ROS thread
    ros_th_ = std::thread(&pose_plugin_t::ros_thread, this);
  }

  void ros_thread() {
    // Initialize ros node
    if (ros::isInitialized() == false) {
      ROS_FATAL("ROS is not initialized!");
    }
    ros_nh_ = new ros::NodeHandle();

    // Register pose publisher
    {
      std::string topic_name = "pose";
      if (sdf_->HasElement("pose_topic")) {
        topic_name = sdf_->Get<std::string>("pose_topic");
      }
      pose_pub_ = ros_nh_->advertise<geometry_msgs::PoseStamped>(topic_name, 1);
    }

    // Register pose subscriber
    {
      std::string topic_name = "pose/set";
      int queue_size = 1;
      if (sdf_->HasElement("pose_set_topic")) {
        topic_name = sdf_->Get<std::string>("pose_set_topic");
      }
      pose_sub_ = ros_nh_->subscribe(topic_name,
                                     queue_size,
                                     &pose_plugin_t::pose_callback,
                                     this);
    }
  }

  void publish_pose() {
    const auto time = ros::Time(model_->GetWorld()->SimTime().Double());
    const proto::mat4_t T_WB = convert(model_->WorldPose());
    const auto msg = proto::msg_build(ros_seq_, time, "", T_WB);
    pose_pub_.publish(msg);
  }

  void pose_callback(const geometry_msgs::Pose::ConstPtr &msg) {
    const double x = msg->position.x;
    const double y = msg->position.y;
    const double z = msg->position.z;
    const double qw = msg->orientation.w;
    const double qx = msg->orientation.x;
    const double qy = msg->orientation.y;
    const double qz = msg->orientation.z;
    ignition::math::Pose3d pose(x, y, z, qw, qx, qy, qz);
    model_->SetWorldPose(pose);
  }

  void on_update() {
    const auto pose = model_->WorldPose();
    model_->SetWorldPose(pose);
    publish_pose();
    ros_seq_++;
  }
};

GZ_REGISTER_MODEL_PLUGIN(pose_plugin_t)
} // namespace gazebo
