/*
 * frames.hpp
 *
 *  Created on: Sep 26, 2021
 *      Author: jelavice
 */

#pragma once
#include <string>

namespace o3d_slam {
namespace frames {
static const std::string odomFrame = "odom_open3d";
static const std::string rangeSensorFrame = "range_sensor_open3d";
static const std::string mapFrame = "map_open3d";
static const std::string imageFrame = "image_frame_open3d";

} /* namespace frames */
} /* namespace o3d_slam */
