#include "prototype/munit.hpp"
#include "msckf/imu_state.hpp"
#include "control/carrot_controller.hpp"
#include "quadrotor/quadrotor_model.hpp"

namespace prototype {

#define TEST_MISSION "test_configs/missions/mission_local.yaml"

int test_QuadrotorModel_constructor() {
  QuadrotorModel quad;
  return 0;
}

int setup_output_files(std::ofstream &gnd_file,
                       std::ofstream &mea_file,
                       std::ofstream &est_file) {
  // Ground truth file
  const std::string gnd_file_path = "/tmp/quadrotor_gnd.dat";
  gnd_file.open(gnd_file_path);
  if (gnd_file.good() == false) {
    LOG_ERROR("Failed to open ground truth file for recording [%s]",
              gnd_file_path.c_str());
    return -1;
  }

  // Measurement file
  const std::string mea_file_path = "/tmp/quadrotor_mea.dat";
  mea_file.open(mea_file_path);
  if (mea_file.good() == false) {
    LOG_ERROR("Failed to open measurement file for recording [%s]",
              mea_file_path.c_str());
    return -1;
  }

  // Estimate file
  const std::string est_file_path = "/tmp/quadrotor_est.dat";
  est_file.open(est_file_path);
  if (est_file.good() == false) {
    LOG_ERROR("Failed to open estimate file for recording [%s]",
              est_file_path.c_str());
    return -1;
  }

  // Write header
  const std::string gnd_header = "t,x,y,z,vx,vy,vz,roll,pitch,yaw";
  gnd_file << gnd_header << std::endl;
  const std::string mea_header = "t,ax_B,ay_B,az_B,wx_B,wy_B,wz_B";
  mea_file << mea_header << std::endl;
  const std::string est_header = "t,x,y,z,vx,vy,vz,roll,pitch,yaw";
  est_file << est_header << std::endl;

  return 0;
}

void record_timestep(const double t,
                     const QuadrotorModel &quad,
                     const IMUState &imu,
                     std::ofstream &gnd_file,
                     std::ofstream &mea_file,
                     std::ofstream &est_file) {
  // Record quadrotor ground truth
  // -- Time
  gnd_file << t << ",";
  // -- Position
  gnd_file << quad.p_G(0) << ",";
  gnd_file << quad.p_G(1) << ",";
  gnd_file << quad.p_G(2) << ",";
  // -- Velocity
  gnd_file << quad.v_G(0) << ",";
  gnd_file << quad.v_G(1) << ",";
  gnd_file << quad.v_G(2) << ",";
  // -- Attitude
  gnd_file << quad.rpy_G(0) << ",";
  gnd_file << quad.rpy_G(1) << ",";
  gnd_file << quad.rpy_G(2) << std::endl;

  // Record quadrotor body acceleration and angular velocity
  // -- Time
  mea_file << t << ",";
  // -- Body acceleration
  mea_file << quad.a_B(0) << ",";
  mea_file << quad.a_B(1) << ",";
  mea_file << quad.a_B(2) << ",";
  // -- Body angular velocity
  mea_file << quad.w_B(0) << ",";
  mea_file << quad.w_B(1) << ",";
  mea_file << quad.w_B(2) << std::endl;

  // Record estimate
  // -- Time
  est_file << t << ",";
  // -- Position
  est_file << imu.p_G(0) << ",";
  est_file << imu.p_G(1) << ",";
  est_file << imu.p_G(2) << ",";
  // -- Velocity
  est_file << imu.v_G(0) << ",";
  est_file << imu.v_G(1) << ",";
  est_file << imu.v_G(2) << ",";
  // -- Attitude
  const vec3_t rpy = quat2euler(imu.q_IG);
  est_file << rpy(0) << ",";
  est_file << rpy(1) << ",";
  est_file << rpy(2) << std::endl;
}

int test_QuadrotorModel_update() {
  // Setup output files
  std::ofstream gnd_file;
  std::ofstream mea_file;
  std::ofstream est_file;

  int retval = setup_output_files(gnd_file, mea_file, est_file);
  if (retval != 0) {
    LOG_ERROR("Failed to setup output files!");
    return -1;
  }

  // Setup quadrotor model
  QuadrotorModel quad(vec3_t{0.0, 0.0, 0.0}, vec3_t{0.0, 0.0, 5.0});

  // Setup IMU state
  IMUState imu;
  imu.g_G = vec3_t{0.0, 0.0, -quad.g};
  imu.p_G = quad.p_G;
  imu.q_IG = euler2quat(quad.rpy_G);

  // Record initial quadrotor state
  record_timestep(0.0, quad, imu, gnd_file, mea_file, est_file);

  // Set quadrotor desired position
  const vec3_t pos_desired{1.0, 0.0, 5.0};
  quad.setPosition(pos_desired);

  // Simulate
  const double dt = 0.01;
  // for (double t = 0.0; t <= 30.0; t += dt) {
  for (double t = 0.0; t <= 30.0; t += dt) {
    // Update quadrotor model
    quad.update(dt);

    // Update imu
    const vec3_t a_m = quad.a_B;
    const vec3_t w_m = quad.w_B;
    imu.update(a_m, w_m, dt);

    // Record
    record_timestep(t, quad, imu, gnd_file, mea_file, est_file);
  }
  gnd_file.close();
  mea_file.close();
  est_file.close();

  // Plot quadrotor trajectory
  // PYTHON_SCRIPT("scripts/plot_quadrotor.py "
  //               "/tmp/quadrotor_gnd.dat "
  //               "/tmp/quadrotor_mea.dat "
  //               "/tmp/quadrotor_est.dat");

  return 0;
}

void test_suite() {
  MU_ADD_TEST(test_QuadrotorModel_constructor);
  MU_ADD_TEST(test_QuadrotorModel_update);
}

} // namespace prototype

MU_RUN_TESTS(prototype::test_suite);
