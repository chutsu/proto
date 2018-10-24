/**
 * @file
 * @ingroup camera
 */
#ifndef PROTOTYPE_VISION_CAMERA_DISTORTION_HPP
#define PROTOTYPE_VISION_CAMERA_DISTORTION_HPP

#include <opencv2/calib3d/calib3d.hpp>

#include "prototype/core.hpp"

namespace prototype {
/**
 * @addtogroup camera
 * @{
 */

/**
 * Distort 3D points with the radial-tangential distortion model
 *
 * Source:
 * http://docs.opencv.org/2.4/modules/calib3d/doc/camera_calibration_and_3d_reconstruction.html
 */
MatX radtan_distort(const double k1,
                    const double k2,
                    const double k3,
                    const double p1,
                    const double p2,
                    const MatX &points);

/**
 * Distort 3D points with the radial-tangential distortion model
 *
 * Source:
 * http://docs.opencv.org/2.4/modules/calib3d/doc/camera_calibration_and_3d_reconstruction.html
 */
Vec2 radtan_distort(const double k1,
                    const double k2,
                    const double k3,
                    const double p1,
                    const double p2,
                    const Vec3 &point);

/**
 * Distort 3D points with the equi-distant distortion model
 */
MatX equi_distort(const double k1,
                  const double k2,
                  const double k3,
                  const double k4,
                  const MatX &points);

/**
 * Distort a single 3D point with the equi-distant distortion model
 */
Vec2 equi_distort(const double k1,
                  const double k2,
                  const double k3,
                  const double k4,
                  const Vec3 &point);

/**
 * Un-distort a 2D point with the equi-distant distortion model
 */
void equi_undistort(const double k1,
                    const double k2,
                    const double k3,
                    const double k4,
                    Vec2 &p);

/**
 * Pinhole Equidistant undistort image
 */
cv::Mat pinhole_equi_undistort_image(const Mat3 &K,
                                     const VecX &D,
                                     const cv::Mat &image,
                                     const double balance,
                                     cv::Mat &K_new);

/**
 * Pinhole Equidistant undistort image
 */
cv::Mat pinhole_equi_undistort_image(const Mat3 &K,
                                     const VecX &D,
                                     const cv::Mat &image,
                                     cv::Mat &K_new);

/**
 * Project pinhole radial-tangential
 */
Vec2 project_pinhole_radtan(const Mat3 &K, const VecX &D, const Vec3 &X);

/**
 * Project pinhole equidistant
 */
Vec2 project_pinhole_equi(const Mat3 &K, const Vec4 &D, const Vec3 &X);

/** @} group camera */
} //  namespace prototype
#endif // PROTOTYPE_VISION_CAMERA_DISTORTION_HPP
