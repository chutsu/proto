/**
 * @file
 * @ingroup calib
 */
#ifndef PROTOTYPE_CALIB_STEREO_CALIB_HPP
#define PROTOTYPE_CALIB_STEREO_CALIB_HPP

#include <string>

#include <opencv2/calib3d/calib3d.hpp>

#include "prototype/calib/camchain.hpp"
#include "prototype/calib/chessboard.hpp"
#include "prototype/core.hpp"

namespace prototype {
/**
 * @addtogroup calib
 * @{
 */

class StereoCalib {
public:
  Camchain camchain;
  Chessboard chessboard;

  std::vector<std::vector<cv::Point3f>> object_points;
  std::vector<std::vector<cv::Point2f>> imgpts0;
  std::vector<std::vector<cv::Point2f>> imgpts1;
  cv::Size image_size;

  StereoCalib();
  virtual ~StereoCalib();

  /**
   * Preprocess data
   *
   * @param data_path Path to data
   * @returns 0 for success, -1 for failure
   */
  int preprocessData(const std::string &data_path);

  // int calibrateIntrinsics();

  /**
   * Calibrate stereo extrinsics
   *
   * @returns 0 for success, -1 for failure
   */
  int calibrateExtrinsics();
};

/** @} group calib */
} //  namespace prototype
#endif // PROTOTYPE_CALIB_STEREO_CALIB_HPP
