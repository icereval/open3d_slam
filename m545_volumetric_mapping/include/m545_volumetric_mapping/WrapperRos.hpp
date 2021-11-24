/*
 * WrapperRos.hpp
 *
 *  Created on: Nov 23, 2021
 *      Author: jelavice
 */

#pragma once

#include <ros/ros.h>
#include <ros/package.h>

#include <sensor_msgs/PointCloud2.h>

#include <tf2_ros/transform_broadcaster.h>
#include <nav_msgs/Odometry.h>

#include <thread>
#include <Eigen/Dense>
#include "m545_volumetric_mapping/Parameters.hpp"
#include "m545_volumetric_mapping/typedefs.hpp"
#include "m545_volumetric_mapping/croppers.hpp"
#include "m545_volumetric_mapping/TransformInterpolationBuffer.hpp"
#include "m545_volumetric_mapping/CircularBuffer.hpp"

namespace m545_mapping {

class LidarOdometry;
class Mapper;
class Mesher;
class SubmapCollection;
class OptimizationProblem;


class WrapperRos {
	struct TimestampedPointCloud {
		Time time_;
		PointCloud cloud_;
	};

public:
	WrapperRos(ros::NodeHandlePtr nh);
	EIGEN_MAKE_ALIGNED_OPERATOR_NEW

	~WrapperRos();

	void addRangeScan(const open3d::geometry::PointCloud cloud, const Time timestamp);
	void initialize();
	void start();

private:

	void odometryWorker();
	void mappingWorker();
	void featureComputationWorker();
	void loopClosureWorker();
	void mesherWorker();
	void globalOptimizationWorker();
	void publishMaps(const Time &time);
	void publishMapToOdomTf(const Time &time);

	ros::NodeHandlePtr nh_;
	std::shared_ptr<tf2_ros::TransformBroadcaster> tfBroadcaster_;
	ros::Publisher odometryInputPub_,submapOriginsPub_, assembledMapPub_, denseMapPub_, submapsPub_, meshPub_;


	// non ros types
	CircularBuffer<TimestampedPointCloud> odometryBuffer_, mappingBuffer_;
	CircularBuffer<Time> mesherBufffer_;
	m545_mapping::MapperParameters mapperParams_;
	m545_mapping::LocalMapParameters localMapParams_;
	m545_mapping::MesherParameters mesherParams_;
	PointCloud rawCloudPrev_;
	std::shared_ptr<Mesher> mesher_;
	std::shared_ptr<LidarOdometry> odometry_;
	std::shared_ptr<Mapper> mapper_;
	std::shared_ptr<SubmapCollection> submaps_;
	std::shared_ptr<OptimizationProblem> optimizationProblem_;
	std::string folderPath_;
	std::thread odometryWorker_, mappingWorker_;
	Timer mappingStatisticsTimer_,odometryStatisticsTimer_, visualizationUpdateTimer_;
	bool isVisualizationFirstTime_ = true;

};

} // namespace m545_mapping
