#include "calibration/chessboard.hpp"
#include "prototype/munit.hpp"

namespace prototype {

#define TEST_CONFIG "test_configs/gimbal/calibration/chessboard.yaml"
#define TEST_IMAGE "test_data/calibration2/img_0.jpg"

int test_chessboard_constructor() {
  chessboard_t cb;

  MU_CHECK(cb.nb_rows == 10);
  MU_CHECK(cb.nb_cols == 10);
  MU_CHECK_FLOAT(cb.square_size, 0.1);

  return 0;
}

int test_chessboard_load() {
  chessboard_t cb;

  cb.load(TEST_CONFIG);
  MU_CHECK(cb.nb_rows == 6);
  MU_CHECK(cb.nb_cols == 7);
  MU_CHECK_FLOAT(cb.square_size, 0.0285);

  return 0;
}

int test_chessboard_detect() {
  chessboard_t cb;

  cb.load(TEST_CONFIG);
  const cv::Mat image = cv::imread(TEST_IMAGE);
  std::vector<cv::Point2f> corners;
  int retval = cb.detect(image, corners);
  MU_CHECK_EQ(retval, 0);
  MU_CHECK_EQ((int) corners.size(), cb.nb_rows * cb.nb_cols);

  // cv::imshow("Image", image);
  // cv::waitKey();

  return 0;
}

int test_chessboard_drawCorners() {
  chessboard_t cb;

  cb.load(TEST_CONFIG);
  cv::Mat image = cv::imread(TEST_IMAGE);
  const int retval = cb.drawCorners(image);
  MU_CHECK_EQ(retval, 0);

  // cv::imshow("Image", image);
  // cv::waitKey();

  return 0;
}

int test_chessboard_solvePnP() {
  chessboard_t cb;

  cb.load(TEST_CONFIG);
  const cv::Mat image = cv::imread(TEST_IMAGE);
  std::vector<cv::Point2f> corners;
  int retval = cb.detect(image, corners);

  // Solve PnP
  // -- Form camera intrinsics K
  cv::Mat K(3, 3, cv::DataType<double>::type);
  K.at<double>(0, 0) = 359.8796;
  K.at<double>(0, 1) = 0.0;
  K.at<double>(0, 2) = 341.8768;
  K.at<double>(1, 0) = 0.0;
  K.at<double>(1, 1) = 361.4580;
  K.at<double>(1, 2) = 255.9160;
  K.at<double>(2, 0) = 0.0;
  K.at<double>(2, 1) = 0.0;
  K.at<double>(2, 2) = 1.0;
  // -- Solve
  mat4_t T_c_t;
  retval = cb.solvePnP(corners, K, T_c_t);
  std::cout << T_c_t << std::endl;

  MU_CHECK_EQ(retval, 0);

  return 0;
}

int test_chessboard_calcCornerPositions() {
  chessboard_t cb;

  cb.load(TEST_CONFIG);
  cv::Mat image = cv::imread(TEST_IMAGE);
  std::vector<cv::Point2f> corners;
  int retval = cb.detect(image, corners);

  // Calculate corner positions
  // -- Form camera intrinsics K
  cv::Mat K(3, 3, cv::DataType<double>::type);
  K.at<double>(0, 0) = 359.8796;
  K.at<double>(0, 1) = 0.0;
  K.at<double>(0, 2) = 341.8768;
  K.at<double>(1, 0) = 0.0;
  K.at<double>(1, 1) = 361.4580;
  K.at<double>(1, 2) = 255.9160;
  K.at<double>(2, 0) = 0.0;
  K.at<double>(2, 1) = 0.0;
  K.at<double>(2, 2) = 1.0;
  // -- Calculate
  matx_t X;
  retval = cb.calcCornerPositions(corners, K, X);
  std::cout << X.transpose() << std::endl;

  cb.project3DPoints(X, K, image);

  MU_CHECK_EQ(retval, 0);

  // cv::imshow("Image", image);
  // cv::waitKey();

  return 0;
}

void test_suite() {
  MU_ADD_TEST(test_chessboard_constructor);
  MU_ADD_TEST(test_chessboard_load);
  MU_ADD_TEST(test_chessboard_detect);
  MU_ADD_TEST(test_chessboard_drawCorners);
  MU_ADD_TEST(test_chessboard_solvePnP);
  MU_ADD_TEST(test_chessboard_calcCornerPositions);
}

} // namespace prototype

MU_RUN_TESTS(prototype::test_suite);
