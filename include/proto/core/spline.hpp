#ifndef PROTO_CORE_SPLINE_HPP
#define PROTO_CORE_SPLINE_HPP

#include <vector>
#include <fstream>

#include <Eigen/Dense>
#include <unsupported/Eigen/Splines>

#include "proto/core/log.hpp"
#include "proto/core/math.hpp"

namespace proto {

typedef Eigen::Spline<double, 1> Spline1D;
typedef Eigen::Spline<double, 2> Spline2D;
typedef Eigen::Spline<double, 3> Spline3D;

#define SPLINE1D(X, Y, DEG) \
  Eigen::SplineFitting<Spline1D>::Interpolate(X, DEG, Y)

#define SPLINE2D(X, Y, DEG) \
  Eigen::SplineFitting<Spline2D>::Interpolate(X, DEG, Y)

#define SPLINE3D(X, Y, DEG) \
  Eigen::SplineFitting<Spline3D>::Interpolate(X, DEG, Y)

/*****************************************************************************
 * Continuous trajectory generator
 *****************************************************************************/

/**
 * Continuous trajectory generator
 */
struct ctraj_t {
  const timestamps_t timestamps;
  const vec3s_t positions;
  const quats_t orientations;

  const double ts_s_start;
  const double ts_s_end;
  const double ts_s_gap;

  Spline3D pos_spline;
  Spline3D rvec_spline;

  ctraj_t(const timestamps_t &timestamps,
          const vec3s_t &positions,
          const quats_t &orientations);
};

/**
 * Container for multiple continuous trajectories
 */
typedef std::vector<ctraj_t> ctrajs_t;

/**
 * Initialize continuous trajectory.
 */
void ctraj_init(ctraj_t &ctraj);

/**
 * Calculate pose `T_WB` at timestamp `ts`.
 */
mat4_t ctraj_get_pose(const ctraj_t &ctraj, const timestamp_t ts);

/**
 * Calculate velocity `v_WB` at timestamp `ts`.
 */
vec3_t ctraj_get_velocity(const ctraj_t &ctraj, const timestamp_t ts);

/**
 * Calculate acceleration `a_WB` at timestamp `ts`.
 */
vec3_t ctraj_get_acceleration(const ctraj_t &ctraj, const timestamp_t ts);

/**
 * Calculate angular velocity `w_WB` at timestamp `ts`.
 */
vec3_t ctraj_get_angular_velocity(const ctraj_t &ctraj, const timestamp_t ts);

/**
 * Save trajectory to file
 */
int ctraj_save(const ctraj_t &ctraj, const std::string &save_path);

/*****************************************************************************
 * IMU measurements generator
 *****************************************************************************/

struct sim_imu_t {
  // IMU parameters
  double rate = 0.0;        // IMU rate [Hz]
  double tau_a = 0.0;       // Reversion time constant for accel [s]
  double tau_g = 0.0;       // Reversion time constant for gyro [s]
  double sigma_g_c = 0.0;   // Gyro noise density [rad/s/sqrt(Hz)]
  double sigma_a_c = 0.0;   // Accel noise density [m/s^s/sqrt(Hz)]
  double sigma_gw_c = 0.0;  // Gyro drift noise density [rad/s^s/sqrt(Hz)]
  double sigma_aw_c = 0.0;  // Accel drift noise density [m/s^2/sqrt(Hz)]
  double g = 0.0;           // Gravity vector [ms-2]

  // IMU flags and biases
  bool started = false;
  vec3_t b_g = zeros(3, 1);
  vec3_t b_a = zeros(3, 1);
  timestamp_t ts_prev = 0;
};

void sim_imu_reset(sim_imu_t &imu);

void sim_imu_measurement(
    sim_imu_t &imu,
    std::default_random_engine &rndeng,
    const timestamp_t &ts,
    const mat4_t &T_WS_W,
    const vec3_t &w_WS_W,
    const vec3_t &a_WS_W,
    vec3_t &a_WS_S,
    vec3_t &w_WS_S);

} //  namespace proto
#endif // PROTO_CORE_SPLINE_HPP