/**
 * @file
 * @ingroup calib
 */
#ifndef PROTOTYPE_CALIB_RESIDUAL_HPP
#define PROTOTYPE_CALIB_RESIDUAL_HPP

#include <ceres/ceres.h>

#include "prototype/core.hpp"
#include "gimbal/gimbal_model.hpp"

namespace prototype {
/**
 * @addtogroup calib
 * @{
 */

/**
 * Gimbal calibration residual
 */
struct GimbalCalibResidual {
  // Observed 3d point in static camera
  double P_s[3] = {0};

  // Observed 3d point in dynamic camera
  double P_d[3] = {0};

  // Observed pixel in static camera
  double Q_s[2] = {0};

  // Observed pixel in dynamic camera
  double Q_d[2] = {0};

  // Theta1 and Theta2 offset
  double theta1_offset = 0.0;
  double theta2_offset = 0.0;

  // Static camera intrinsics
  double fx_s = 0.0;
  double fy_s = 0.0;
  double cx_s = 0.0;
  double cy_s = 0.0;

  // Dynamic camera intrinsics
  double fx_d = 0.0;
  double fy_d = 0.0;
  double cx_d = 0.0;
  double cy_d = 0.0;

  // Static camera distortion coefficients
  double k1_s = 0.0;
  double k2_s = 0.0;
  double k3_s = 0.0;
  double k4_s = 0.0;

  // Dynamic camera distortion coefficients
  double k1_d = 0.0;
  double k2_d = 0.0;
  double k3_d = 0.0;
  double k4_d = 0.0;

  GimbalCalibResidual();
  GimbalCalibResidual(const Vec3 &P_s,
                      const Vec3 &P_d,
                      const Vec2 &Q_s,
                      const Vec2 &Q_d,
                      const Mat3 &K_s,
                      const Mat3 &K_d,
                      const Vec4 &D_s,
                      const Vec4 &D_d,
                      double theta1_offset,
                      double theta2_offset);

  /// Form DH-Transform
  template <typename T>
  Eigen::Matrix<T, 4, 4>
  dhTransform(const T theta, const T alpha, const T a, const T d) const;

  /// Euler 3-2-1 to rotation matrix
  template <typename T>
  Eigen::Matrix<T, 3, 3> euler321ToRot(const T phi,
                                       const T theta,
                                       const T psi) const;

  /// Form camera intrinsics matrix K
  template <typename T>
  Eigen::Matrix<T, 3, 3>
  K(const T fx, const T fy, const T cx, const T cy) const;

  /// Form camera distortion vector D
  template <typename T>
  Eigen::Matrix<T, 4, 1>
  D(const T k1, const T k2, const T k3, const T k4) const;

  /// Form transform from static to dynamic camera
  template <typename T>
  Eigen::Matrix<T, 4, 4> T_ds(const T *const tau_s,
                              const T *const tau_d,
                              const T *const w1,
                              const T *const w2,
                              const T *const Lambda1,
                              const T *const Lambda2) const;

  /// Project 3D point using pinhole-equi
  template <typename T>
  Eigen::Matrix<T, 2, 1>
  project_pinhole_equi(const Eigen::Matrix<T, 3, 3> &K,
                       const Eigen::Matrix<T, 4, 1> &D,
                       const Eigen::Matrix<T, 3, 1> &X) const;

  /// Project 3D point using pinhole-radtan
  template <typename T>
  Eigen::Matrix<T, 2, 1>
  project_pinhole_radtan(const Eigen::Matrix<T, 3, 3> &K,
                         const Eigen::Matrix<T, 4, 1> &D,
                         const Eigen::Matrix<T, 3, 1> &X) const;

  /// Project 3D point using pinhole
  template <typename T>
  Eigen::Matrix<T, 2, 1> project_pinhole(const Eigen::Matrix<T, 3, 3> &K,
                                         const Eigen::Matrix<T, 3, 1> &X) const;

  /// Calculate residual
  template <typename T>
  bool operator()(const T *const tau_s,
                  const T *const tau_d,
                  const T *const w1,
                  const T *const w2,
                  const T *const Lambda1,
                  const T *const Lambda2,
                  T *residual) const;
};

/**
 * GimbalCalibResidual to string
 */
std::ostream &operator<<(std::ostream &os, const GimbalCalibResidual &residual);

/**
 * GimbalCalibResidual to string
 */
std::ostream &operator<<(std::ostream &os, const GimbalCalibResidual *residual);

/**
 * Gimbal calibration numerical residual
 */
struct GimbalCalibNumericalResidual {
  Vec3 P_s = zeros(3, 1); ///< Observed 3d point in static camera
  Vec3 P_d = zeros(3, 1); ///< Observed 3d point in dynamic camera
  Vec2 Q_s = zeros(2, 1); ///< Observed pixel in static camera
  Vec2 Q_d = zeros(2, 1); ///< Observed pixel in dynamic camera
  Mat3 K_s = zeros(3, 3); ///< Static camera intrinsics
  Mat3 K_d = zeros(3, 3); ///< Dynamic camera intrinsics

  GimbalCalibNumericalResidual();
  GimbalCalibNumericalResidual(const Vec3 &P_s,
                               const Vec3 &P_d,
                               const Vec2 &Q_s,
                               const Vec2 &Q_d,
                               const Mat3 &K_s,
                               const Mat3 &K_d)
      : P_s{P_s}, P_d{P_d}, Q_s{Q_s}, Q_d{Q_d}, K_s{K_s}, K_d{K_d} {}

  /// Calculate transform between static and dynamic camera
  Mat4 T_ds(const VecX &tau_s,
            const double Lambda1,
            const Vec3 &w1,
            const VecX &tau_d,
            const double Lambda2,
            const Vec3 &w2) const;

  /// Calculate residual
  bool operator()(const double *const p0,
                  const double *const p1,
                  const double *const p2,
                  const double *const p3,
                  const double *const p4,
                  const double *const p5,
                  double *residual) const;
};

/** @} group calibration */
} //  namespace prototype
#include "residual_impl.hpp"
#endif // PROTOTYPE_CALIB_RESIDUAL_HPP
