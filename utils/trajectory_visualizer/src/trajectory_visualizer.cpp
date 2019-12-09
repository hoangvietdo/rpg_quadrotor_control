#include "trajectory_visualizer/trajectory_visualizer.h"

#include <quadrotor_common/parameter_helper.h>
#include <visualization_msgs/MarkerArray.h>
#include <nav_msgs/Odometry.h>

namespace trajectory_visualizer
{

TrajectoryVisualizer::TrajectoryVisualizer()
{
  if (!loadParameters())
  {
    ROS_ERROR("[%s] Failed to load all parameters",
              ros::this_node::getName().c_str());
    ros::shutdown();
  }

  marker_pub_ref_ = nh_.advertise<visualization_msgs::MarkerArray>(
      "autopilot_feedback_trajectory_ref", 1);
  marker_pub_se_ = nh_.advertise<visualization_msgs::MarkerArray>(
      "autopilot_feedback_trajectory_se", 1);
  odometry_ref_pub_ = nh_.advertise<nav_msgs::Odometry>(
      "reference_odometry", 1);

  autopilot_feedback_sub_ = nh_.subscribe(
      "autopilot/feedback", 1, &TrajectoryVisualizer::autopilotFeedbackCallback,
      this);
}

TrajectoryVisualizer::~TrajectoryVisualizer()
{
}

void TrajectoryVisualizer::autopilotFeedbackCallback(
    const quadrotor_msgs::AutopilotFeedback::ConstPtr &msg)
{
  // store message in buffer
  feedback_queue_.push_front(*msg);
  while (feedback_queue_.size() > n_points_to_visualize_)
  {
    feedback_queue_.pop_back();
  }

  if (feedback_queue_.size() < 2)
  {
    return;
  }

  // visualize full pose of reference...
  nav_msgs::Odometry reference_odometry;
  reference_odometry.header.stamp = msg->header.stamp;
  reference_odometry.header.frame_id = "world";
  reference_odometry.pose.pose.orientation = msg->reference_state.pose.orientation;
  reference_odometry.pose.pose.position = msg->reference_state.pose.position;

  odometry_ref_pub_.publish(reference_odometry);

  visualization_msgs::MarkerArray marker_msg_ref;
  visualization_msgs::MarkerArray marker_msg_se;

  // General marker
  visualization_msgs::Marker marker;
  marker.header.frame_id = "world";
  marker.header.stamp = ros::Time::now();
  marker.ns = "";
  marker.action = visualization_msgs::Marker::MODIFY;
  marker.lifetime = ros::Duration(0);
  marker.type = visualization_msgs::Marker::LINE_STRIP;
  marker.pose.position.x = 0.0;
  marker.pose.position.y = 0.0;
  marker.pose.position.z = 0.0;
  marker.pose.orientation.w = 1.0;
  marker.pose.orientation.x = 0.0;
  marker.pose.orientation.y = 0.0;
  marker.pose.orientation.z = 0.0;

  // Reference trajectory
  marker.id = 0;
  marker.scale.x = 0.015;
  marker.color.r = 1.0;
  marker.color.g = 0.0;
  marker.color.b = 0.0;
  marker.color.a = 1.0;

  for (auto it = feedback_queue_.begin(); it != feedback_queue_.end(); it++)
  {
    geometry_msgs::Point point;
    point.x = it->reference_state.pose.position.x;
    point.y = it->reference_state.pose.position.y;
    point.z = it->reference_state.pose.position.z;

    marker.points.push_back(point);
  }

  marker_msg_ref.markers.push_back(marker);
  marker_pub_ref_.publish(marker_msg_ref);

  // Estimated trajectory
  marker.id = 1;
  marker.scale.x = 0.025;
  marker.color.r = 0.0;
  marker.color.g = 1.0;
  marker.color.b = 0.0;
  marker.color.a = 1.0;

  marker.points.clear();
  for (auto it = feedback_queue_.begin(); it != feedback_queue_.end(); it++)
  {
    geometry_msgs::Point point;
    point.x = it->state_estimate.pose.pose.position.x;
    point.y = it->state_estimate.pose.pose.position.y;
    point.z = it->state_estimate.pose.pose.position.z;

    marker.points.push_back(point);
  }

  marker_msg_se.markers.push_back(marker);

  marker_pub_se_.publish(marker_msg_se);
}

bool TrajectoryVisualizer::loadParameters()
{
  if (!quadrotor_common::getParam("n_points_to_visualize",
                                  n_points_to_visualize_, 50.0))
  {
    return false;
  }

  return true;
}

} // namespace trajectory_visualizer

int main(int argc, char **argv)
{
  ros::init(argc, argv, "trajectory_visualizer");
  trajectory_visualizer::TrajectoryVisualizer trajectory_visualizer;

  ros::spin();

  return 0;
}
