/*
 * typedefs.hpp
 *
 *  Created on: Nov 8, 2021
 *      Author: jelavice
 */
#pragma once

#include <cstdint>
#include <Eigen/Core>
#include <open3d/geometry/PointCloud.h>

namespace m545_mapping {


using int8 = int8_t;
using int16 = int16_t;
using int32 = int32_t;
using int64 = int64_t;
using uint8 = uint8_t;
using uint16 = uint16_t;
using uint32 = uint32_t;
using uint64 = uint64_t;
using Matrix6d = Eigen::Matrix<double, 6, 6>;
using PointCloud = open3d::geometry::PointCloud;


} //namespace m545_mapping
