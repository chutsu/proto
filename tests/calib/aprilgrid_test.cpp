#include "prototype/calib/aprilgrid.hpp"
#include "prototype/munit.hpp"
#include "prototype/vision/camera/pinhole.hpp"

namespace prototype {

#define TEST_OUTPUT "/tmp/aprilgrid.csv"
#define TEST_IMAGE "test_data/calib/aprilgrid/aprilgrid.png"
#define TEST_CONF "test_data/calib/aprilgrid/target.yaml"

static void visualize_grid(const cv::Mat &image,
                           const mat3_t &K,
                           const vec4_t &D,
                           const vec3s_t &landmarks,
                           const vec2s_t &keypoints,
                           const bool show) {
  // Undistort image
  cv::Mat image_undistorted;
  cv::Mat intrinsics = convert(K);
  cv::Mat distortions = convert(D);
  cv::undistort(image, image_undistorted, intrinsics, distortions);

  // Project landmarks in 3D to image plane
  for (size_t i = 0; i < landmarks.size(); i++) {
    // Project then scale to image plane
    vec3_t x = landmarks[i];
    x(0) = x(0) / x(2);
    x(1) = x(1) / x(2);
    x(2) = x(2) / x(2);
    x = K * x;

    // Draw corner, Set color to yellow on first corner (origin), else blue
    auto color = (i == 0) ? cv::Scalar(0, 255, 255) : cv::Scalar(0, 0, 255);
    cv::circle(image_undistorted, cv::Point(x(0), x(1)), 1.0, color, 2, 8);

    // Label corner
    cv::Point2f cxy(keypoints[i](0), keypoints[i](1));
    cv::Scalar text_color(0, 255, 0);
    std::string text = std::to_string(i);
    int font = cv::FONT_HERSHEY_PLAIN;
    double font_scale = 1.0;
    int thickness = 2;
    cv::putText(image_undistorted,
                text,
                cxy,
                font,
                font_scale,
                text_color,
                thickness);
  }

  if (show) {
    cv::imshow("Visualize grid", image_undistorted);
    cv::waitKey(0);
  }
}

int test_aprilgrid_constructor() {
  aprilgrid_t grid(0, 6, 6, 0.088, 0.3);

  MU_CHECK(grid.detected == false);
  MU_CHECK_EQ(0, grid.ids.size());
  MU_CHECK_EQ(0, grid.keypoints.size());

  MU_CHECK(grid.estimated == false);
  MU_CHECK_EQ(0, grid.points_CF.size());
  MU_CHECK((I(4) - grid.T_CF).norm() < 1e-5);

  return 0;
}

int test_aprilgrid_add() {
  aprilgrid_t grid(0, 6, 6, 0.088, 0.3);

  // Keypoints
  const vec2_t kp1{1.0, 2.0};
  const vec2_t kp2{3.0, 4.0};
  const vec2_t kp3{5.0, 6.0};
  const vec2_t kp4{7.0, 8.0};
  std::vector<cv::Point2f> keypoints;
  keypoints.emplace_back(kp1(0), kp1(1));
  keypoints.emplace_back(kp2(0), kp2(1));
  keypoints.emplace_back(kp3(0), kp3(1));
  keypoints.emplace_back(kp4(0), kp4(1));

  // Test save
  const int tag_id = 1;
  aprilgrid_add(grid, tag_id, keypoints);

  // Test load
  MU_CHECK_FLOAT(0.0, (grid.keypoints[0] - kp1).norm());
  MU_CHECK_FLOAT(0.0, (grid.keypoints[1] - kp2).norm());
  MU_CHECK_FLOAT(0.0, (grid.keypoints[2] - kp3).norm());
  MU_CHECK_FLOAT(0.0, (grid.keypoints[3] - kp4).norm());
  MU_CHECK_EQ(0, grid.points_CF.size());

  return 0;
}

int test_aprilgrid_remove() {
  aprilgrid_t grid(0, 6, 6, 0.088, 0.3);

  // Add keypoints and points_CF
  for (int i = 0; i < 5; i++) {
    std::vector<cv::Point2f> keypoints;
    keypoints.emplace_back(i, i);
    keypoints.emplace_back(i, i);
    keypoints.emplace_back(i, i);
    keypoints.emplace_back(i, i);

    vec3s_t points_CF;
    points_CF.emplace_back(i, i, i);
    points_CF.emplace_back(i, i, i);
    points_CF.emplace_back(i, i, i);
    points_CF.emplace_back(i, i, i);

    aprilgrid_add(grid, i, keypoints);
    extend(grid.points_CF, points_CF);
  }

  // Test remove
  aprilgrid_remove(grid, 2);
  aprilgrid_remove(grid, 4);
  aprilgrid_remove(grid, 5); // Remove non-existant id

  for (size_t i = 0; i < grid.ids.size(); i++) {
    const int id = grid.ids[i];

    MU_CHECK((vec2_t(id, id) - grid.keypoints[i * 4]).norm() < 1e-8);
    MU_CHECK((vec2_t(id, id) - grid.keypoints[i * 4 + 1]).norm() < 1e-8);
    MU_CHECK((vec2_t(id, id) - grid.keypoints[i * 4 + 2]).norm() < 1e-8);
    MU_CHECK((vec2_t(id, id) - grid.keypoints[i * 4 + 3]).norm() < 1e-8);

    MU_CHECK((vec3_t(id, id, id) - grid.points_CF[i * 4]).norm() < 1e-8);
    MU_CHECK((vec3_t(id, id, id) - grid.points_CF[i * 4 + 1]).norm() < 1e-8);
    MU_CHECK((vec3_t(id, id, id) - grid.points_CF[i * 4 + 2]).norm() < 1e-8);
    MU_CHECK((vec3_t(id, id, id) - grid.points_CF[i * 4 + 3]).norm() < 1e-8);
  }

  return 0;
}

int test_aprilgrid_get() {
  aprilgrid_t grid(0, 6, 6, 0.088, 0.3);

  for (int i = 0; i < 10; i++) {
    // Keypoints
    const vec2_t kp1(i, i);
    const vec2_t kp2(i, i);
    const vec2_t kp3(i, i);
    const vec2_t kp4(i, i);
    std::vector<cv::Point2f> keypoints;
    keypoints.emplace_back(kp1(0), kp1(1));
    keypoints.emplace_back(kp2(0), kp2(1));
    keypoints.emplace_back(kp3(0), kp3(1));
    keypoints.emplace_back(kp4(0), kp4(1));

    // Positions
    const vec3_t pos1(i, i, i);
    const vec3_t pos2(i, i, i);
    const vec3_t pos3(i, i, i);
    const vec3_t pos4(i, i, i);

    // Add measurment
    aprilgrid_add(grid, i, keypoints);
    grid.estimated = true;
    grid.points_CF.emplace_back(pos1);
    grid.points_CF.emplace_back(pos2);
    grid.points_CF.emplace_back(pos3);
    grid.points_CF.emplace_back(pos4);

    // Test get tag
    vec2s_t keypoints_result;
    vec3s_t positions_result;
    aprilgrid_get(grid, i, keypoints_result, positions_result);

    MU_CHECK((vec2_t(i, i) - keypoints_result[0]).norm() < 1e-4);
    MU_CHECK((vec2_t(i, i) - keypoints_result[1]).norm() < 1e-4);
    MU_CHECK((vec2_t(i, i) - keypoints_result[2]).norm() < 1e-4);
    MU_CHECK((vec2_t(i, i) - keypoints_result[3]).norm() < 1e-4);

    MU_CHECK((vec3_t(i, i, i) - positions_result[0]).norm() < 1e-4);
    MU_CHECK((vec3_t(i, i, i) - positions_result[1]).norm() < 1e-4);
    MU_CHECK((vec3_t(i, i, i) - positions_result[2]).norm() < 1e-4);
    MU_CHECK((vec3_t(i, i, i) - positions_result[3]).norm() < 1e-4);
  }

  return 0;
}

int test_aprilgrid_grid_index() {
  int i = 0;
  int j = 0;
  aprilgrid_t grid(0, 6, 6, 0.088, 0.3);

  MU_CHECK_EQ(0, aprilgrid_grid_index(grid, 0, i, j));
  MU_CHECK_EQ(0, i);
  MU_CHECK_EQ(0, j);

  MU_CHECK_EQ(0, aprilgrid_grid_index(grid, 1, i, j));
  MU_CHECK_EQ(0, i);
  MU_CHECK_EQ(1, j);

  MU_CHECK_EQ(0, aprilgrid_grid_index(grid, 5, i, j));
  MU_CHECK_EQ(0, i);
  MU_CHECK_EQ(5, j);

  MU_CHECK_EQ(0, aprilgrid_grid_index(grid, 7, i, j));
  MU_CHECK_EQ(1, i);
  MU_CHECK_EQ(1, j);

  MU_CHECK_EQ(0, aprilgrid_grid_index(grid, 17, i, j));
  MU_CHECK_EQ(2, i);
  MU_CHECK_EQ(5, j);

  return 0;
}

int test_aprilgrid_calc_relative_pose() {
  // Detect tags
  const cv::Mat image = cv::imread(TEST_IMAGE);
  const cv::Mat image_gray = rgb2gray(image);
  const auto detector = aprilgrid_detector_t();
  auto tags = detector.det.extractTags(image_gray);

  // Extract relative pose
  const mat3_t K = pinhole_K(458.654, 457.296, 367.215, 248.375);
  const vec4_t D{-0.28340811, 0.07395907, 0.00019359, 1.76187114e-05};
  const int tag_rows = 6;
  const int tag_cols = 6;
  const double tag_size = 0.088;
  const double tag_spacing = 0.3;
  aprilgrid_t grid(0, tag_rows, tag_cols, tag_size, tag_spacing);

  for (const auto &tag : tags) {
    // Image points (counter-clockwise, from bottom left)
    std::vector<cv::Point2f> img_pts;
    img_pts.emplace_back(tag.p[0].first, tag.p[0].second); // Bottom left
    img_pts.emplace_back(tag.p[1].first, tag.p[1].second); // Bottom right
    img_pts.emplace_back(tag.p[2].first, tag.p[2].second); // Top right
    img_pts.emplace_back(tag.p[3].first, tag.p[3].second); // Top left

    aprilgrid_add(grid, tag.id, img_pts);
  }
  aprilgrid_calc_relative_pose(grid, K, D);

  return 0;
}

int test_aprilgrid_save_and_load() {
  aprilgrid_t grid(0, 6, 6, 0.088, 0.3);

  const vec2_t kp1{1.0, 2.0};
  const vec2_t kp2{3.0, 4.0};
  const vec2_t kp3{5.0, 6.0};
  const vec2_t kp4{7.0, 8.0};
  const vec3_t pos1{1.0, 2.0, 3.0};
  const vec3_t pos2{4.0, 5.0, 6.0};
  const vec3_t pos3{7.0, 8.0, 9.0};
  const vec3_t pos4{10.0, 11.0, 12.0};


  // Test save
  grid.configured = true;
  grid.tag_rows = 6;
  grid.tag_cols = 6;
  grid.tag_size = 0.088;
  grid.tag_spacing = 0.3;

  grid.detected = true;
  grid.timestamp = 1403709381987836928;
  grid.ids.push_back(1);
  grid.keypoints = {kp1, kp2, kp3, kp4};

  grid.estimated = true;
  grid.points_CF = {pos1, pos2, pos3, pos4};
  grid.T_CF = I(4);
  MU_CHECK_EQ(0, aprilgrid_save(grid, TEST_OUTPUT));

  // // Test load
  // aprilgrid_t grid2(0, 6, 6, 0.088, 0.3);
  // MU_CHECK_EQ(0, aprilgrid_load(grid2, TEST_OUTPUT));
  // std::cout << grid2 << std::endl;
  //
  // MU_CHECK_EQ(1, grid2.ids.size());
  // MU_CHECK_EQ(4, grid2.keypoints.size());
  // MU_CHECK_EQ(4, grid2.points_CF.size());
  // MU_CHECK_FLOAT(0.0, (grid2.keypoints[0] - kp1).norm());
  // MU_CHECK_FLOAT(0.0, (grid2.keypoints[1] - kp2).norm());
  // MU_CHECK_FLOAT(0.0, (grid2.keypoints[2] - kp3).norm());
  // MU_CHECK_FLOAT(0.0, (grid2.keypoints[3] - kp4).norm());
  // MU_CHECK_FLOAT(0.0, (grid2.points_CF[0] - pos1).norm());
  // MU_CHECK_FLOAT(0.0, (grid2.points_CF[1] - pos2).norm());
  // MU_CHECK_FLOAT(0.0, (grid2.points_CF[2] - pos3).norm());
  // MU_CHECK_FLOAT(0.0, (grid2.points_CF[3] - pos4).norm());

  return 0;
}

int test_aprilgrid_print() {
  aprilgrid_t grid(0, 6, 6, 0.088, 0.3);

  const vec2_t kp1{1.0, 2.0};
  const vec2_t kp2{3.0, 4.0};
  const vec2_t kp3{5.0, 6.0};
  const vec2_t kp4{7.0, 8.0};
  const vec3_t pos1{1.0, 2.0, 3.0};
  const vec3_t pos2{4.0, 5.0, 6.0};
  const vec3_t pos3{7.0, 8.0, 9.0};
  const vec3_t pos4{10.0, 11.0, 12.0};

  grid.ids.push_back(1);
  grid.keypoints.push_back(kp1);
  grid.keypoints.push_back(kp2);
  grid.keypoints.push_back(kp3);
  grid.keypoints.push_back(kp4);
  grid.points_CF.push_back(pos1);
  grid.points_CF.push_back(pos2);
  grid.points_CF.push_back(pos3);
  grid.points_CF.push_back(pos4);

  std::cout << grid << std::endl;

  return 0;
}

int test_aprilgrid_detect() {
  aprilgrid_t grid;
  MU_CHECK_EQ(0, aprilgrid_configure(grid, TEST_CONF));

  const auto detector = aprilgrid_detector_t();
  const cv::Mat image = cv::imread(TEST_IMAGE);
  const mat3_t K = pinhole_K(458.654, 457.296, 367.215, 248.375);
  const vec4_t D{-0.28340811, 0.07395907, 0.00019359, 1.76187114e-05};
  const auto aprilgrid = aprilgrid_detect(grid, detector, image, K, D);

  for (const auto corner : grid.points_CF) {
    MU_CHECK(corner(0) < 1.0);
    MU_CHECK(corner(0) > -1.0);
    MU_CHECK(corner(1) < 1.0);
    MU_CHECK(corner(1) > -1.0);
    MU_CHECK(corner(2) < 2.0);
    MU_CHECK(corner(2) > 0.5);
  }

  return 0;
}

void test_suite() {
  MU_ADD_TEST(test_aprilgrid_constructor);
  MU_ADD_TEST(test_aprilgrid_add);
  MU_ADD_TEST(test_aprilgrid_remove);
  MU_ADD_TEST(test_aprilgrid_get);
  MU_ADD_TEST(test_aprilgrid_grid_index);
  MU_ADD_TEST(test_aprilgrid_calc_relative_pose);
  MU_ADD_TEST(test_aprilgrid_save_and_load);
  MU_ADD_TEST(test_aprilgrid_print);
  MU_ADD_TEST(test_aprilgrid_detect);
}

} // namespace prototype

MU_RUN_TESTS(prototype::test_suite);
