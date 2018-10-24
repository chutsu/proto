/**
 * @file
 * @ingroup calibration
 */
#ifndef PROTOTYPE_CALIB_CALIB_VALIDATOR_HPP
#define PROTOTYPE_CALIB_CALIB_VALIDATOR_HPP

#include <ios>

#include "prototype/core.hpp"
#include "camera/distortion.hpp"
#include "gimbal/gimbal_model.hpp"
#include "calibration/camchain.hpp"
#include "calibration/chessboard.hpp"

namespace prototype {
/**
 * @addtogroup calibration
 * @{
 */

/**
 * Calibration validator
 */
class CalibValidator {
public:
  Camchain camchain;
  Chessboard chessboard;
  GimbalModel gimbal_model;

  CalibValidator();
  virtual ~CalibValidator();

  /**
   * Load
   *
   * @param nb_cameras Number of cameras
   * @param camchain_file Path to camchain file
   * @param target_file Path to target file
   *
   * @returns 0 for success, -1 for failure
   */
  int load(const int nb_cameras,
           const std::string &camchain_file,
           const std::string &target_file);

  /**
   * Load
   *
   * @param nb_cameras Number of cameras
   * @param camchain_file Path to camchain file
   *
   * @returns 0 for success, -1 for failure
   */
  int load(const int nb_cameras, const std::string &camchain_file);

  /**
   * Detect chessboard corners
   *
   * @param camera_index Camera index
   * @param image Image
   * @param X 3D position of chessboard corners
   *
   * @returns
   * - 0 for no chessboard detected
   * - 1 for chessboard detected
   * - -1 for failure
   */
  int detect(const int camera_index, const cv::Mat &image, MatX &X);

  /**
   * Draw detected
   *
   * @param image Image
   * @param color Color to visualize chessboard corners
   *
   * @returns Image with chessboard corners visualized
   */
  cv::Mat drawDetected(const cv::Mat &image, const cv::Scalar &color);

  /**
   * Calculate reprojection error
   *
   * @param image Image
   * @param image_points Image points
   * @returns Reprojection error
   */
  double reprojectionError(const cv::Mat &image,
                           const std::vector<cv::Point2f> &image_points);

  /**
   * Project 3D points to image plane and draw chessboard corners
   *
   * @param camera_index Camera index
   * @param image Input image
   * @param X 3D position of chessboard corners
   * @param color Color to visualize chessboard corners
   *
   * @returns Image with chessboard corners visualized
   */
  cv::Mat project(const int camera_index,
                  const cv::Mat &image,
                  const MatX &X,
                  const cv::Scalar &color = cv::Scalar(0, 0, 255));

  /**
   * Validate calibration
   *
   * @param camera_index Camera index
   * @param image Input image
   * @returns Validation image for visual inspection
   */
  cv::Mat validate(const int camera_index, cv::Mat &image);

  /**
   * Validate stereo calibration
   *
   * @param img0 Input image from cam0
   * @param img1 Input image from cam1
   * @returns Validation image for visual inspection
   */
  cv::Mat validateStereo(const cv::Mat &img0, const cv::Mat &img1);

  /**
   * Validate stereo + gimbal calibration
   *
   * @param img0 Input image from cam0
   * @param img1 Input image from cam1
   * @param img2 Input image from cam2
   * @param joint_roll Joint roll angle (radians)
   * @param joint_pitch Joint pitch angle (radians)
   * @returns Validation image for visual inspection
   */
  cv::Mat validateTriclops(const cv::Mat &img0,
                           const cv::Mat &img1,
                           const cv::Mat &img2,
                           const double joint_roll,
                           const double joint_pitch);

  /**
   * Validate gimbal calibration
   *
   * @param img0 Input image from cam0
   * @param img1 Input image from cam1
   * @param img2 Input image from cam2
   * @param joint_roll Joint roll angle (radians)
   * @param joint_pitch Joint pitch angle (radians)
   * @returns Reprojection error
   */
  double validateGimbal(const Vec3 &P_s,
                        const Vec2 &Q_d,
                        const Mat3 &K,
                        const double joint_roll,
                        const double joint_pitch);
};

/**
 * CalibValidator to string
 */
std::ostream &operator<<(std::ostream &os, const CalibValidator &validator);

/** @} group calibration */
} //  namespace prototype
#endif // PROTOTYPE_CALIB_CALIB_VALIDATOR_HPP
