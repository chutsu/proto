/**
 * @file
 * @ingroup calibration
 */
#ifndef PROTOTYPE_VISION_CALIBRATION_CHESSBOARD_HPP
#define PROTOTYPE_VISION_CALIBRATION_CHESSBOARD_HPP

#include <opencv2/calib3d/calib3d.hpp>

#include "prototype/core.hpp"

namespace prototype {
/**
 * @addtogroup calibration
 * @{
 */

/**
 * Chessboard
 */
struct Chessboard {
  int nb_rows = 10;
  int nb_cols = 10;
  double square_size = 0.1;
  std::vector<cv::Point3f> object_points;

  Chessboard();
  virtual ~Chessboard();

  /**
   * Load config file
   *
   * @param config_file Path to config file
   * @returns 0 for success, -1 for failure
   */
  int load(const std::string &config_file);

  /**
   * Create 2D grid points
   * @returns Vector of gridpoints
   */
  std::vector<vec2_t> createGridPoints2d();

  /**
   * Create object points
   * @returns Vector of object points
   */
  std::vector<cv::Point3f> createObjectPoints();

  /**
   * Detect chessboard corners
   *
   * @param image Input image
   * @param corners Detected chessboard corners
   * @returns 0 for success, -1 for failure
   */
  int detect(const cv::Mat &image, std::vector<cv::Point2f> &corners);

  /**
   * Draw chessboard corners
   * @param image Image where chessboard corners are drawn
   * @returns 0 for success, -1 for failure
   */
  int drawCorners(cv::Mat &image);

  /**
   * Solve PnP between camera and chessboard
   *
   * @param corners Detected chessboard corners
   * @param K Camera intrinsics matrix K
   * @param T_c_t Transform from camera to calibration target
   *
   * @returns 0 for success, -1 for failure
   */
  int solvePnP(const std::vector<cv::Point2f> corners,
               const cv::Mat &K,
               mat4_t &T_c_t);

  /**
   * Calculate corner positions
   *
   * @param corners Detected chessboard corners
   * @param K Camera intrinsics matrix K
   * @param X Corner positions relative to camera in ideal coordinates
   *
   * @returns 0 for success, -1 for failure
   */
  int calcCornerPositions(const std::vector<cv::Point2f> corners,
                          const cv::Mat &K,
                          matx_t &X);

  /**
   * Calculate corner positions
   *
   * @param corners Detected chessboard corners
   * @param K Camera intrinsics matrix K
   * @param X Corner positions relative to camera in ideal coordinates
   *
   * @returns 0 for success, -1 for failure
   */
  int calcCornerPositions(const std::vector<cv::Point2f> corners,
                          const mat3_t &K,
                          matx_t &X);

  /**
   * Project 3D points to image
   *
   * @param X Corner positions relative to camera in ideal coordinates
   * @param K Camera intrinsics matrix K
   * @param image Target image
   */
  void project3DPoints(const matx_t &X, const mat3_t &K, cv::Mat &image);

  /**
   * Project 3D points to image
   *
   * @param X Corner positions relative to camera in ideal coordinates
   * @param K Camera intrinsics matrix K
   * @param image Target image
   */
  void project3DPoints(const matx_t &X, const cv::Mat &K, cv::Mat &image);
};

/** @} group calibration */
} //  namespace prototype
#endif // PROTOTYPE_VISION_CALIBRATION_CHESSBOARD_HPP
