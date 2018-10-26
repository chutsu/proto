#include "prototype/munit.hpp"
#include "prototype/dataset/euroc/imu_data.hpp"

namespace prototype {

#define TEST_DATA "test_data/euroc/imu0"

int test_imu_data_constructor() {
  imu_data_t imu_data;

  // Data
  MU_CHECK_EQ(0, imu_data.timestamps.size());
  MU_CHECK_EQ(0, imu_data.time.size());
  MU_CHECK_EQ(0, imu_data.w_B.size());
  MU_CHECK_EQ(0, imu_data.a_B.size());

  // Sensor properties
  MU_CHECK_EQ("", imu_data.sensor_type);
  MU_CHECK_EQ("", imu_data.comment);
  MU_CHECK(imu_data.T_BS.isApprox(I(4)));
  MU_CHECK_FLOAT(0.0, imu_data.rate_hz);
  MU_CHECK_FLOAT(0.0, imu_data.gyro_noise_density);
  MU_CHECK_FLOAT(0.0, imu_data.gyro_random_walk);
  MU_CHECK_FLOAT(0.0, imu_data.accel_noise_density);
  MU_CHECK_FLOAT(0.0, imu_data.accel_random_walk);

  return 0;
}

int test_imu_data_load() {
  imu_data_t imu_data{TEST_DATA};

  int retval = imu_data_load(imu_data);
  MU_CHECK_EQ(0, retval);

  MU_CHECK_EQ(91, imu_data.timestamps.size());
  MU_CHECK_EQ(91, imu_data.time.size());
  MU_CHECK_EQ(91, imu_data.w_B.size());
  MU_CHECK_EQ(91, imu_data.a_B.size());

  MU_CHECK_EQ("imu", imu_data.sensor_type);
  MU_CHECK_EQ("VI-Sensor IMU (ADIS16448)", imu_data.comment);
  MU_CHECK_FLOAT(1.6968e-04, imu_data.gyro_noise_density);
  MU_CHECK_FLOAT(1.9393e-05, imu_data.gyro_random_walk);
  MU_CHECK_FLOAT(2.0000e-3, imu_data.accel_noise_density);
  MU_CHECK_FLOAT(3.0000e-3, imu_data.accel_random_walk);

  return 0;
}

void test_suite() {
  MU_ADD_TEST(test_imu_data_constructor);
  MU_ADD_TEST(test_imu_data_load);
}

}

MU_RUN_TESTS(prototype::test_suite);
