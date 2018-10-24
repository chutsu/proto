/**
 * @file
 * @defgroup kitti kitti
 * @ingroup dataset
 */
#ifndef PROTOTYPE_DATASET_KITTI_RAW_OXTS_HPP
#define PROTOTYPE_DATASET_KITTI_RAW_OXTS_HPP

#include <algorithm>
#include <vector>
#include <string>
#include <chrono>

#include "prototype/core.hpp"
#include "prototype/dataset/kitti/raw/parse.hpp"

namespace prototype {
/**
 * @addtogroup kitti
 * @{
 */

/**
 * OXTS entry
 */
struct OXTSEntry {
  EIGEN_MAKE_ALIGNED_OPERATOR_NEW
  Vec3 gps = zeros(3, 1);
  Vec3 rpy = zeros(3, 1);
  Vec3 v_G = zeros(3, 1);
  Vec3 v_B = zeros(3, 1);
  Vec3 a_G = zeros(3, 1);
  Vec3 a_B = zeros(3, 1);
  Vec3 w_G = zeros(3, 1);
  Vec3 w_B = zeros(3, 1);
  double pos_accuracy = 0.0;
  double vel_accuracy = 0.0;

  /**
   * Load OXTS entry
   *
   * @param file_path File path to a single OXTS data
   * @returns 0 for success, -1 for failure
   */
  int load(const std::string &file_path);
};

/**
 * OXTS
 */
struct OXTS {
  EIGEN_MAKE_ALIGNED_OPERATOR_NEW
  std::vector<long> timestamps;
  std::vector<double> time;
  std::vector<Vec3> gps;
  std::vector<Vec3> rpy;
  std::vector<Vec3> p_G;
  std::vector<Vec3> v_G;
  std::vector<Vec3> v_B;
  std::vector<Vec3> a_G;
  std::vector<Vec3> a_B;
  std::vector<Vec3> w_G;
  std::vector<Vec3> w_B;
  std::vector<double> pos_accuracy;
  std::vector<double> vel_accuracy;

  OXTS() {}

  /**
   * load OXTS
   *
   * @param oxts_dir Path to OXTS data
   * @returns 0 for success, -1 for failure
   */
  int loadOXTS(const std::string &oxts_dir);

  /**
   * Parse a single timestamp to seconds since epoch (Unix time)
   *
   * @param line Timestamp line in the form of "2011-09-26 13:04:32.349659964"
   * @param s Seconds
   * @returns 0 for success, -1 for failure
   */
  int parseSingleTimeStamp(const std::string &line, long *s);

  /**
   * Load timestamps
   *
   * @param oxts_dir Path to OXTS data
   * @returns 0 for success, -1 for failure
   */
  int loadTimeStamps(const std::string &oxts_dir);

  /**
   * Load OXTS
   *
   * @param oxts_dir Path to OXTS data
   * @returns 0 for success, -1 for failure
   */
  int load(const std::string &oxts_dir);
};

/** @} group kitti */
} //  namespace prototype
#endif // PROTOTYPE_DATASET_KITTI_RAW_OXTS_HPP
