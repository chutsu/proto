/**
 * @file
 * @ingroup msckf
 */
#ifndef PROTOTYPE_VISION_MSCKF_IMU_STATE_HPP
#define PROTOTYPE_VISION_MSCKF_IMU_STATE_HPP

#include "prototype/core.hpp"
#include "prototype/core/quaternion/jpl.hpp"

namespace prototype {
/**
 * @addtogroup msckf
 * @{
 */

/**
 * IMU State Config
 */
struct IMUStateConfig {
  EIGEN_MAKE_ALIGNED_OPERATOR_NEW
  // Initial Estimate Covariances
  Vec3 q_init_var = zeros(3, 1);
  Vec3 bg_init_var = zeros(3, 1);
  Vec3 v_init_var = zeros(3, 1);
  Vec3 ba_init_var = zeros(3, 1);
  Vec3 p_init_var = zeros(3, 1);

  // Process Noises
  Vec3 w_var = zeros(3, 1);
  Vec3 dbg_var = zeros(3, 1);
  Vec3 a_var = zeros(3, 1);
  Vec3 dba_var = zeros(3, 1);

  // Constants
  Vec3 g_G = zeros(3, 1);
};

/**
 * IMU State
 */
class IMUState {
public:
  EIGEN_MAKE_ALIGNED_OPERATOR_NEW
  int size = 0;  ///< Size of error-state vector
  bool rk4 = false;  ///< Runge-Kutta 4th order integration

  // State
  Vec4 q_IG = Vec4{0.0, 0.0, 0.0, 1.0}; ///< JPL Quaternion in Global frame
  Vec3 b_g = zeros(3, 1);               ///< Bias of gyroscope
  Vec3 v_G = zeros(3, 1);               ///< Velocity in Global frame
  Vec3 b_a = zeros(3, 1);               ///< Bias of accelerometer
  Vec3 p_G = zeros(3, 1);               ///< Position in Global frame

  // Extrinsics
  // -- IMU-Camera extrinsics
  Vec3 p_IC = zeros(3, 1);
  Vec4 q_IC = zeros(4, 1);
  // -- Gimbal joint angle extrinsics
  Vec2 theta = zeros(2, 1);

  // Constants
  Vec3 g_G = Vec3{0.0, 0.0, -9.81}; ///< Gravitational acceleration

  // Misc
  MatX P = 1e-5 * I(15); ///< Covariance matrix
  MatX Q = 1e-2 * I(12); ///< Noise matrix
  MatX Phi = I(15); ///< Noise matrix

  IMUState();
  IMUState(const IMUStateConfig &config);

  /**
   * Transition F matrix
   *
   * @param w_hat Estimated angular velocity
   * @param q_hat Estimated quaternion (x, y, z, w)
   * @param a_hat Estimated acceleration
   * @returns Transition jacobian matrix F
   */
  MatX F(const Vec3 &w_hat, const Vec4 &q_hat, const Vec3 &a_hat);

  /**
   * Input G matrix
   *
   * A matrix that maps the input vector (IMU gaussian noise) to the state
   * vector (IMU error state vector), it tells us how the inputs affect the
   * state vector.
   *
   * @param q_hat Estimated quaternion (x, y, z, w)
   * @returns Input jacobian matrix G
   */
  MatX G(const Vec4 &q_hat);

  /**
   * Update
   *
   * @param a_m Measured acceleration
   * @param w_m Measured angular velocity
   * @param dt Time difference in seconds
   */
  void update(const Vec3 &a_m, const Vec3 &w_m, const double dt);

  /**
   * Correct the IMU state
   *
   * @param dx Correction state vector
   */
  void correct(const VecX &dx);
};

/** @} group msckf */
} //  namespace prototype
#endif // PROTOTYPE_VISION_MSCKF_IMU_STATE_HPP
