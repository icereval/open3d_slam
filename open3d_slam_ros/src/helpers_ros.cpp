/*
 * helpers_ros.cpp
 *
 *  Created on: Oct 20, 2021
 *      Author: jelavice
 */

#include "open3d_slam_ros/helpers_ros.hpp"
#include "open3d_slam/SubmapCollection.hpp"
#include <random>
// ros stuff
#include "open3d_conversions/open3d_conversions.h"
#include <eigen_conversions/eigen_msg.h>
#include <ros/ros.h>
#include <tf2_ros/transform_broadcaster.h>
#include <tf2/convert.h>
#include <tf2_eigen/tf2_eigen.h>
#include <nav_msgs/Odometry.h>

namespace o3d_slam {

namespace {
template<typename T, typename Limits>
void clamp(T *val, Limits lo, Limits hi) {
	if (*val > hi) {
		*val = hi;
		return;
	}
	if (*val < lo) {
		*val = lo;
		return;
	}
}
} // namespace

Color::Color() :
		std_msgs::ColorRGBA() {
}
Color::Color(double red, double green, double blue) :
		Color(red, green, blue, 1.0) {
}
Color::Color(double red, double green, double blue, double alpha) :
		Color() {
	r = red;
	g = green;
	b = blue;
	a = alpha;
}

Color Color::operator*(double scalar) const {
	Color ret = *this;
	ret.r *= scalar;
	ret.g *= scalar;
	ret.b *= scalar;
	clamp(&ret.r, 0.0, 1.0);
	clamp(&ret.g, 0.0, 1.0);
	clamp(&ret.b, 0.0, 1.0);
	return ret;
}
Color operator*(double scalar, const Color &c) {
	return c * scalar;
}

void publishSubmapCoordinateAxes(const SubmapCollection &submaps, const std::string &frame_id,
		const ros::Time &timestamp, const ros::Publisher &pub) {
	visualization_msgs::MarkerArray msg;
	int id = 0;
	msg.markers.reserve(2 * submaps.getNumSubmaps());
	for (size_t j = 0; j < submaps.getNumSubmaps(); ++j) {
		const Submap &submap = submaps.getSubmap(j);
		visualization_msgs::Marker axes, text;
		drawAxes(submap.getMapToSubmapCenter(), Eigen::Quaterniond(submap.getMapToSubmapOrigin().rotation()), 0.8, 0.08,
				&axes);
		axes.ns = "submap_" + std::to_string(j);
		axes.header.frame_id = frame_id;
		axes.header.stamp = timestamp;
		axes.id = submap.getId();
		msg.markers.push_back(axes);
		text = axes;
		text.pose.position.x += 0.3;
		text.pose.position.y += 0.3;
		text.scale.x = text.scale.y = text.scale.z = 0.4;
		text.color.r=text.color.g=text.color.b=1.0;
		text.type = visualization_msgs::Marker::TEXT_VIEW_FACING;
		text.text = "(" + std::to_string(j) + ")";
		text.ns = "submap_id_" + std::to_string(j);
		text.id=id++;
		msg.markers.push_back(text);
	}
	pub.publish(msg);
}

void assembleColoredPointCloud(const SubmapCollection &submaps, open3d::geometry::PointCloud *cloud) {
	if (submaps.isEmpty()) {
		return;
	}
	std::mt19937 rndGen;
	rndGen.seed(time(NULL));
	const int nPoints = submaps.getTotalNumPoints();
	cloud->points_.reserve(nPoints);
	cloud->colors_.reserve(nPoints);
	std::uniform_int_distribution<int> rndInt(2, 12);
	for (size_t j = 0; j < submaps.getNumSubmaps(); ++j) {
		const Submap &submap = submaps.getSubmap(j);
		const auto color = Color::getColor(j % (Color::numColors_ - 2) + 2);
		for (size_t i = 0; i < submap.getMapPointCloud().points_.size(); ++i) {
			cloud->points_.push_back(submap.getMapPointCloud().points_.at(i));
			cloud->colors_.emplace_back(Eigen::Vector3d(color.r, color.g, color.b));
		}
	}
}

void publishCloud(const open3d::geometry::PointCloud &cloud, const std::string &frame_id, const ros::Time &timestamp,
		ros::Publisher &pub) {
	if (pub.getNumSubscribers() > 0) {
		sensor_msgs::PointCloud2 msg;
		const PointCloud copy = cloud;
		open3d_conversions::open3dToRos(copy, msg, frame_id);
		msg.header.stamp = timestamp;
		pub.publish(msg);
	}
}

void publishTfTransform(const Eigen::Matrix4d &Mat, const ros::Time &time, const std::string &frame,
		const std::string &childFrame, tf2_ros::TransformBroadcaster *broadcaster) {
	geometry_msgs::TransformStamped transformStamped = o3d_slam::toRos(Mat, time, frame, childFrame);
	broadcaster->sendTransform(transformStamped);
}

bool lookupTransform(const std::string &target_frame, const std::string &source_frame, const ros::Time &time,
		const tf2_ros::Buffer &tfBuffer, Eigen::Isometry3d *transform) {
	geometry_msgs::TransformStamped transformStamped;
	try {
		transformStamped = tfBuffer.lookupTransform(target_frame, source_frame, time);
	} catch (tf2::TransformException &ex) {
		ROS_WARN("caught exception while looking up the tf: %s", ex.what());
		*transform = Eigen::Isometry3d::Identity();
		return false;
	}
	*transform = tf2::transformToEigen(transformStamped);
	return true;
}

geometry_msgs::Pose getPose(const Eigen::MatrixXd &T) {
	geometry_msgs::Pose pose;

	// Fill pose
	Eigen::Affine3d eigenTr;
	eigenTr.matrix() = T;
	tf::poseEigenToMsg(eigenTr, pose);

	return pose;
}

geometry_msgs::TransformStamped toRos(const Eigen::Matrix4d &Mat, const ros::Time &time, const std::string &frame,
		const std::string &childFrame) {

	geometry_msgs::TransformStamped transformStamped;
	transformStamped.header.stamp = time;
	transformStamped.header.frame_id = frame;
	transformStamped.child_frame_id = childFrame;
	const auto pose = getPose(Mat);
	transformStamped.transform.translation.x = pose.position.x;
	transformStamped.transform.translation.y = pose.position.y;
	transformStamped.transform.translation.z = pose.position.z;
	transformStamped.transform.rotation = pose.orientation;
	return transformStamped;
}

geometry_msgs::Point createPoint(double x, double y, double z) {
	geometry_msgs::Point p;
	p.x = x;
	p.y = y;
	p.z = z;
	return p;
}

void drawAxes(const Eigen::Vector3d &p, const Eigen::Quaterniond &q, double scale, double line_width,
		visualization_msgs::Marker *marker) {
	marker->colors.resize(6);
	marker->points.resize(6);
	marker->points[0] = createPoint(0, 0, 0);
	marker->points[1] = createPoint(1 * scale, 0, 0);
	marker->points[2] = createPoint(0, 0, 0);
	marker->points[3] = createPoint(0, 1 * scale, 0);
	marker->points[4] = createPoint(0, 0, 0);
	marker->points[5] = createPoint(0, 0, 1 * scale);

	marker->color = Color::Black();
	marker->colors[0] = Color::Red();
	marker->colors[1] = Color::Red();
	marker->colors[2] = Color::Green();
	marker->colors[3] = Color::Green();
	marker->colors[4] = Color::Blue();
	marker->colors[5] = Color::Blue();

	marker->scale.x = line_width;  // rest is unused
	marker->type = visualization_msgs::Marker::LINE_LIST;
	marker->action = visualization_msgs::Marker::ADD;

	tf::pointEigenToMsg(p, marker->pose.position);
	tf::quaternionEigenToMsg(q, marker->pose.orientation);
}

const Color Color::getColor(int colorCode) {

	switch (colorCode) {
	case 0: {
		return White();
	}
	case 1: {
		return Black();
	}
	case 2: {
		return Gray();
	}
	case 3: {
		return Red();
	}
	case 4: {
		return Green();
	}
	case 5: {
		return Blue();
	}
	case 6: {
		return Yellow();
	}
	case 7: {
		return Orange();
	}
	case 8: {
		return Purple();
	}
	case 9: {
		return Chartreuse();
	}
	case 10: {
		return Teal();
	}
	case 11: {
		return Pink();
	}
	case 12: {
		return Magenta();
	}
	default:
		throw std::runtime_error("unknown color code");
	}

}

} /* namespace o3d_slam */