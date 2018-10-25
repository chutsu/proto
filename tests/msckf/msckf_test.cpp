#include "prototype/munit.hpp"
#include "prototype/core.hpp"
#include "dataset/kitti/kitti.hpp"
#include "dataset/euroc/mav_dataset.hpp"
#include "msckf/msckf.hpp"
#include "msckf/blackbox.hpp"
#include "feature2d/klt_tracker.hpp"
#include "feature2d/orb_tracker.hpp"
#include "sim/world.hpp"

#include <opencv2/core/eigen.hpp>

namespace prototype {

// clang-format off
static const std::string KITTI_RAW_DATASET = "/data/kitti/raw";
static const std::string MSCKF_CONFIG = "test_configs/msckf/msckf_kitti_raw.yaml";
// clang-format on

int test_MSCKF_constructor() {
  MSCKF msckf;

  MU_CHECK_EQ(CameraState::size, msckf.P_cam.rows());
  MU_CHECK_EQ(CameraState::size, msckf.P_cam.cols());
  MU_CHECK_EQ(msckf.imu_state.size, msckf.P_imu_cam.rows());
  MU_CHECK_EQ(CameraState::size, msckf.P_imu_cam.cols());

  MU_CHECK_EQ(0, msckf.counter_frame_id);
  MU_CHECK(zeros(3, 1).isApprox(msckf.ext_p_IC));
  MU_CHECK(vec4_t(0.0, 0.0, 0.0, 1.0).isApprox(msckf.ext_q_CI));

  MU_CHECK(msckf.enable_ns_trick);
  MU_CHECK(msckf.enable_qr_trick);

  return 0;
}

int test_MSCKF_configure() {
  MSCKF msckf;

  int retval = msckf.configure(MSCKF_CONFIG);

  std::cout << msckf.imu_state.Q << std::endl;
  std::cout << msckf.imu_state.P << std::endl;

  MU_CHECK_EQ(0, retval);
  // MU_CHECK_EQ(CameraState::size, msckf.P_cam.rows());
  // MU_CHECK_EQ(CameraState::size, msckf.P_cam.cols());
  // MU_CHECK_EQ(msckf.imu_state.size, msckf.P_imu_cam.rows());
  // MU_CHECK_EQ(CameraState::size, msckf.P_imu_cam.cols());
  //
  // MU_CHECK_EQ(0, msckf.counter_frame_id);
  // MU_CHECK(zeros(3, 1).isApprox(msckf.ext_p_IC));
  // MU_CHECK(vec4_t(0.0, 0.0, 0.0, 1.0).isApprox(msckf.ext_q_CI));
  //
  // MU_CHECK(msckf.enable_ns_trick);
  // MU_CHECK(msckf.enable_qr_trick);

  return 0;
}

int test_MSCKF_initialize() {
  MSCKF msckf;

  msckf.initialize(1e9);
  MU_CHECK_EQ(1e9, msckf.last_updated);

  return 0;
}

int test_MSCKF_P() {
  // Setup
  MSCKF msckf;

  msckf.augmentState();
  msckf.augmentState();
  msckf.augmentState();

  msckf.imu_state.P.fill(1.0);
  msckf.P_cam.fill(2.0);
  msckf.P_imu_cam.fill(3.0);

  // Test
  const matx_t P = msckf.P();

  // Assert
  const int imu_sz = msckf.imu_state.size;
  const int cam_sz = CameraState::size * msckf.N();

  matx_t P_imu_expected = zeros(imu_sz);
  P_imu_expected.fill(1.0);

  matx_t P_cam_expected = zeros(cam_sz);
  P_cam_expected.fill(2.0);

  matx_t P_imu_cam_expected = zeros(imu_sz, cam_sz);
  P_imu_cam_expected.fill(3.0);

  MU_CHECK_EQ(cam_sz, msckf.P_cam.rows());
  MU_CHECK_EQ(cam_sz, msckf.P_cam.cols());
  MU_CHECK(P.block(0, 0, imu_sz, imu_sz).isApprox(P_imu_expected));

  MU_CHECK_EQ(imu_sz, msckf.P_imu_cam.rows());
  MU_CHECK_EQ(cam_sz, msckf.P_imu_cam.cols());
  MU_CHECK(P.block(0, imu_sz, imu_sz, cam_sz).isApprox(P_imu_cam_expected));

  MU_CHECK_EQ(imu_sz + cam_sz, P.cols());
  MU_CHECK_EQ(imu_sz + cam_sz, P.rows());
  MU_CHECK(P.block(imu_sz, imu_sz, cam_sz, cam_sz).isApprox(P_cam_expected));

  // mat2csv("/tmp/P.dat", P);
  // PYTHON_SCRIPT("scripts/plot_matrix.py /tmp/P.dat");

  return 0;
}

int test_MSCKF_J() {
  MSCKF msckf;

  const vec4_t cam_q_CI = vec4_t{0.5, -0.5, 0.5, -0.5};
  const vec3_t cam_p_IC = vec3_t{0.0, 0.0, 0.0};
  const vec4_t q_hat_IG = vec4_t{0.0, 0.0, 0.0, 1.0};
  const int N = 1;

  const matx_t J = msckf.J(cam_q_CI, cam_p_IC, q_hat_IG, N);
  std::cout << J << std::endl;

  return 0;
}

int test_MSCKF_N() {
  MSCKF msckf;
  MU_CHECK_EQ(0, msckf.N());

  msckf.augmentState();
  MU_CHECK_EQ(1, msckf.N());

  return 0;
}

int test_MSCKF_H() {
  MSCKF msckf;

  // Setup feature track
  const TrackID track_id = 0;
  const FrameID frame_id = 3;
  const auto data0 = Feature(vec2_t{0.0, 0.0});
  const auto data1 = Feature(vec2_t{0.0, 0.0});
  const auto track = FeatureTrack(track_id, frame_id, data0, data1);

  // Setup track cam states
  msckf.ext_p_IC = vec3_t{0.0, 0.0, 0.0};
  msckf.ext_q_CI = vec4_t{0.5, -0.5, 0.5, -0.5};
  msckf.augmentState();
  msckf.imu_state.p_G = vec3_t{0.1, 0.0, 0.0};
  msckf.augmentState();
  msckf.imu_state.p_G = vec3_t{0.2, 0.0, 0.0};
  msckf.augmentState();
  msckf.imu_state.p_G = vec3_t{0.3, 0.0, 0.0};
  msckf.augmentState();
  CameraStates track_cam_states = msckf.getTrackCameraStates(track);

  // Test
  const vec3_t p_G_f{1.0, 2.0, 3.0};
  matx_t H_f_j;
  matx_t H_x_j;
  msckf.H(track, track_cam_states, p_G_f, H_f_j, H_x_j);

  // Assert
  MU_CHECK_EQ(H_f_j.rows(), 4);
  MU_CHECK_EQ(H_f_j.cols(), 3);
  MU_CHECK_EQ(H_x_j.rows(), 4);
  MU_CHECK_EQ(H_x_j.cols(), 39);

  mat2csv("/tmp/H_f_j.dat", H_f_j);
  mat2csv("/tmp/H_x_j.dat", H_x_j);
  PYTHON_SCRIPT("scripts/plot_matrix.py /tmp/H_f_j.dat");
  PYTHON_SCRIPT("scripts/plot_matrix.py /tmp/H_x_j.dat");

  return 0;
}

int test_MSCKF_augmentState() {
  MSCKF msckf;

  // Augment state 1
  msckf.augmentState();
  MU_CHECK_EQ(6, msckf.P_cam.rows());
  MU_CHECK_EQ(6, msckf.P_cam.cols());
  MU_CHECK_EQ(15, msckf.P_imu_cam.rows());
  MU_CHECK_EQ(6, msckf.P_imu_cam.cols());
  MU_CHECK_EQ(1, msckf.N());
  MU_CHECK_EQ(1, msckf.counter_frame_id);

  mat2csv("/tmp/P.dat", msckf.P());
  PYTHON_SCRIPT("scripts/plot_matrix.py /tmp/P.dat");

  // Augment state 2
  msckf.augmentState();
  MU_CHECK_EQ(12, msckf.P_cam.rows());
  MU_CHECK_EQ(12, msckf.P_cam.cols());
  MU_CHECK_EQ(15, msckf.P_imu_cam.rows());
  MU_CHECK_EQ(12, msckf.P_imu_cam.cols());
  MU_CHECK_EQ(2, msckf.N());
  MU_CHECK_EQ(2, msckf.counter_frame_id);

  mat2csv("/tmp/P.dat", msckf.P());
  PYTHON_SCRIPT("scripts/plot_matrix.py /tmp/P.dat");

  // Augment state 3
  msckf.augmentState();
  MU_CHECK_EQ(18, msckf.P_cam.rows());
  MU_CHECK_EQ(18, msckf.P_cam.cols());
  MU_CHECK_EQ(15, msckf.P_imu_cam.rows());
  MU_CHECK_EQ(18, msckf.P_imu_cam.cols());
  MU_CHECK_EQ(3, msckf.N());
  MU_CHECK_EQ(3, msckf.counter_frame_id);

  mat2csv("/tmp/P.dat", msckf.P());
  PYTHON_SCRIPT("scripts/plot_matrix.py /tmp/P.dat");

  return 0;
}

int test_MSCKF_getTrackCameraStates() {
  MSCKF msckf;
  msckf.augmentState();
  msckf.augmentState();

  Feature f1{vec2_t{0.0, 0.0}};
  Feature f2{vec2_t{1.0, 1.0}};
  FeatureTrack track{0, 1, f1, f2};

  CameraStates track_cam_states = msckf.getTrackCameraStates(track);

  MU_CHECK_EQ(2, msckf.cam_states.size());
  MU_CHECK_EQ(2, track_cam_states.size());
  MU_CHECK_EQ(0, track_cam_states[0].frame_id);
  MU_CHECK_EQ(1, track_cam_states[1].frame_id);

  return 0;
}

int test_MSCKF_predictionUpdate() {
  // Load raw dataset
  RawDataset raw_dataset(KITTI_RAW_DATASET, "2011_09_26", "0005", "sync");
  if (raw_dataset.load() != 0) {
    LOG_ERROR("Failed to load KITTI raw dataset [%s]!",
              KITTI_RAW_DATASET.c_str());
    return -1;
  }

  // Prep blackbox
  BlackBox blackbox;
  if (blackbox.configure("/tmp", "test_msckf_predictionUpdate") != 0) {
    LOG_ERROR("Failed to configure MSCKF blackbox!");
  }

  // Setup MSCKF
  MSCKF msckf;
  msckf.initialize(raw_dataset.oxts.timestamps[0],
                   euler2quat(raw_dataset.oxts.rpy[0]),
                   raw_dataset.oxts.v_G[0],
                   vec3_t{0.0, 0.0, 0.0});

  // Record initial conditions
  blackbox.recordTimeStep(raw_dataset.oxts.time[0],
                          msckf,
                          raw_dataset.oxts.a_B[0],
                          raw_dataset.oxts.w_B[0],
                          raw_dataset.oxts.p_G[0],
                          raw_dataset.oxts.v_G[0],
                          raw_dataset.oxts.rpy[0]);

  // Loop through data and do prediction update
  for (int i = 1; i < (int) raw_dataset.oxts.time.size() - 1; i++) {
    const vec3_t a_B = raw_dataset.oxts.a_B[i];
    const vec3_t w_B = raw_dataset.oxts.w_B[i];
    const long ts = raw_dataset.oxts.timestamps[i];

    msckf.predictionUpdate(a_B, w_B, ts);
    std::cout << "position: " << msckf.imu_state.p_G.transpose() << std::endl;
    std::cout << "velocity: " << msckf.imu_state.v_G.transpose() << std::endl;
    std::cout << std::endl;
    blackbox.recordTimeStep(raw_dataset.oxts.time[i],
                            msckf,
                            a_B,
                            w_B,
                            raw_dataset.oxts.p_G[i],
                            raw_dataset.oxts.v_G[i],
                            raw_dataset.oxts.rpy[i]);

    // const std::string img_path = raw_dataset.cam0[i];
    // const cv::Mat image = cv::imread(img_path);
    // cv::imshow("Image", image);
    // cv::waitKey(0);
  }

  PYTHON_SCRIPT("scripts/plot_msckf.py /tmp/test_msckf_predictionUpdate");

  return 0;
}

int test_MSCKF_residualizeTrack() {
  // Camera model
  const int image_width = 640;
  const int image_height = 480;
  const double fov = 60.0;
  const double fx = pinhole_focal_length(image_width, fov);
  const double fy = pinhole_focal_length(image_height, fov);
  const double cx = image_width / 2.0;
  const double cy = image_height / 2.0;
  CameraProperty camera_property{0, fx, fy, cx, cy, image_width, image_height};

  // Setup MSCKF
  MSCKF msckf;
  // msckf.enable_ns_trick = false;
  // -- Modify default settings for test
  // -- Add first camera state
  msckf.initialize(0);
  // -- Add second camera state
  msckf.imu_state.p_G = vec3_t{1.0, 1.0, 0.0};
  msckf.augmentState();

  // Prepare features and feature track
  // -- Create 2 features
  const vec3_t p_G_f{0.0, 0.0, 10.0};
  vec2_t pt0 = pinhole_project(camera_property.K(),
                             C(msckf.cam_states[0].q_CG),
                             msckf.cam_states[0].p_G,
                             p_G_f);
  vec2_t pt1 = pinhole_project(camera_property.K(),
                             C(msckf.cam_states[1].q_CG),
                             msckf.cam_states[1].p_G,
                             p_G_f);
  pt0 = pinhole_pixel2ideal(camera_property.K(), pt0);
  pt1 = pinhole_pixel2ideal(camera_property.K(), pt1);
  Feature f0{pt0};
  Feature f1{pt1};
  // -- Create a feature track based on two features
  FeatureTrack track{0, 1, f0, f1};

  // Calculate track residual
  matx_t H_j;
  vecx_t r_j;
  int retval = msckf.residualizeTrack(track, H_j, r_j);

  // Assert
  MU_CHECK_EQ(0, retval);
  MU_CHECK_EQ(1, H_j.rows());
  MU_CHECK_EQ(27, H_j.cols());
  MU_CHECK_EQ(1, r_j.rows());
  MU_CHECK_EQ(1, r_j.cols());

  MU_CHECK_EQ(0, retval);
  for (int i = 0; i < r_j.rows(); i++) {
    MU_CHECK_NEAR(r_j(i), 0.0, 1e-5);
  }

  // mat2csv("/tmp/H_j.dat", H_j);
  // mat2csv("/tmp/r_j.dat", r_j);
  // PYTHON_SCRIPT("scripts/plot_matrix.py /tmp/H_j.dat");
  // PYTHON_SCRIPT("scripts/plot_matrix.py /tmp/r_j.dat");

  return 0;
}

int test_MSCKF_calcResiduals() {
  // Camera model
  const int image_width = 640;
  const int image_height = 480;
  const double fov = 60.0;
  const double fx = pinhole_focal_length(image_width, fov);
  const double fy = pinhole_focal_length(image_height, fov);
  const double cx = image_width / 2.0;
  const double cy = image_height / 2.0;
  const mat3_t K = pinhole_K({fx, fy, cx, cy});
  PinholeModel pinhole_model{image_width, image_height, fx, fy, cx, cy};

  // Setup MSCKF
  MSCKF msckf;
  // -- Modify default settings for test
  // -- Add first camera state
  msckf.initialize(0);
  // -- Add second camera state
  msckf.imu_state.p_G = vec3_t{1.0, 1.0, 0.0};
  msckf.augmentState();

  // Prepare features and feature track
  // -- Create a feature track1
  const vec3_t p_G_f0{0.0, 0.0, 10.0};
  vec2_t pt0 = pinhole_model.project(p_G_f0,
                                   C(msckf.cam_states[0].q_CG),
                                   msckf.cam_states[0].p_G);
  vec2_t pt1 = pinhole_model.project(p_G_f0,
                                   C(msckf.cam_states[1].q_CG),
                                   msckf.cam_states[1].p_G);
  pt0 = pinhole_pixel2ideal(K, pt0);
  pt1 = pinhole_pixel2ideal(K, pt1);
  Feature f0{pt0};
  Feature f1{pt1};
  FeatureTrack track1{0, 1, f0, f1};
  // -- Create a feature track2
  const vec3_t p_G_f1{1.0, 1.0, 10.0};
  vec2_t pt2 = pinhole_model.project(p_G_f1,
                                   C(msckf.cam_states[0].q_CG),
                                   msckf.cam_states[0].p_G);
  vec2_t pt3 = pinhole_model.project(p_G_f1,
                                   C(msckf.cam_states[1].q_CG),
                                   msckf.cam_states[1].p_G);
  pt2 = pinhole_pixel2ideal(K, pt0);
  pt3 = pinhole_pixel2ideal(K, pt1);
  Feature f2{pt2};
  Feature f3{pt3};
  FeatureTrack track2{1, 1, f2, f3};
  // // -- Create feature tracks
  FeatureTracks tracks{track1, track2};
  // FeatureTracks tracks{track1};

  // Calculate residuals
  matx_t T_H;
  vecx_t r_n;
  int retval = msckf.calcResiduals(tracks, T_H, r_n);
  print_shape("T_H", T_H);
  print_shape("r_n", r_n);

  // Assert
  MU_CHECK_EQ(0, retval);

  return 0;
}

int test_MSCKF_correctIMUState() {
  // Setup MSCKF
  MSCKF msckf;
  msckf.initialize(0);

  // Form correction vector
  const vec3_t dtheta_IG{0.0, 0.0, 0.0};
  const vec3_t db_g{1.0, 1.0, 1.0};
  const vec3_t dv_G{2.0, 2.0, 2.0};
  const vec3_t db_a{3.0, 3.0, 3.0};
  const vec3_t dp_G{4.0, 4.0, 4.0};

  vecx_t dx;
  dx.resize(15, 1);
  dx << dtheta_IG, db_g, dv_G, db_a, dp_G;

  // Correct IMU state
  msckf.correctIMUState(dx);

  // Assert
  MU_CHECK(msckf.imu_state.v_G.isApprox(dv_G));
  MU_CHECK(msckf.imu_state.p_G.isApprox(dp_G));

  return 0;
}

int test_MSCKF_correctCameraStates() {
  // Setup MSCKF
  MSCKF msckf;
  msckf.initialize(0);

  // Form correction vector
  const vec3_t dtheta_IG{0.0, 0.0, 0.0};
  const vec3_t db_g{0.0, 0.0, 0.0};
  const vec3_t dv_G{0.0, 0.0, 0.0};
  const vec3_t db_a{0.0, 0.0, 0.0};
  const vec3_t dp_G_I{0.0, 0.0, 0.0};

  const vec3_t dtheta_CG{0.0, 0.0, 0.0};
  const vec3_t dp_G_C{1.0, 2.0, 3.0};

  vecx_t dx;
  dx.resize(21, 1);
  dx << dtheta_IG, db_g, dv_G, db_a, dp_G_I, dtheta_CG, dp_G_C;

  // Correct camera states
  msckf.correctCameraStates(dx);

  // Assert
  MU_CHECK(msckf.cam_states[0].p_G.isApprox(dp_G_C));

  return 0;
}

int test_MSCKF_pruneCameraStates() {
  MSCKF msckf;

  msckf.augmentState();
  msckf.augmentState();
  msckf.augmentState();
  msckf.augmentState();
  msckf.augmentState();

  msckf.P_cam.block(0, 0, 6, 6) = 1.0 * ones(6, 6);

  msckf.P_cam.block(0, 6, 6, 6) = 2.0 * ones(6, 6);
  msckf.P_cam.block(6, 6, 6, 6) = 2.0 * ones(6, 6);
  msckf.P_cam.block(6, 0, 6, 6) = 2.0 * ones(6, 6);

  msckf.P_cam.block(0, 12, 12, 6) = 3.0 * ones(12, 6);
  msckf.P_cam.block(12, 12, 6, 6) = 3.0 * ones(6, 6);
  msckf.P_cam.block(12, 0, 6, 12) = 3.0 * ones(6, 12);

  msckf.P_cam.block(0, 18, 18, 6) = 4.0 * ones(18, 6);
  msckf.P_cam.block(18, 18, 6, 6) = 4.0 * ones(6, 6);
  msckf.P_cam.block(18, 0, 6, 18) = 4.0 * ones(6, 18);

  msckf.P_cam.block(0, 24, 24, 6) = 5.0 * ones(24, 6);
  msckf.P_cam.block(24, 24, 6, 6) = 5.0 * ones(6, 6);
  msckf.P_cam.block(24, 0, 6, 24) = 5.0 * ones(6, 24);


  msckf.P_imu_cam.block(0, 0, 15, 6) = 1.0 * ones(15, 6);
  msckf.P_imu_cam.block(0, 6, 15, 6) = 2.0 * ones(15, 6);
  msckf.P_imu_cam.block(0, 12, 15, 6) = 3.0 * ones(15, 6);
  msckf.P_imu_cam.block(0, 18, 15, 6) = 4.0 * ones(15, 6);
  msckf.P_imu_cam.block(0, 24, 15, 6) = 5.0 * ones(15, 6);

  // mat2csv("/tmp/P_cam.dat", msckf.P_cam);
  // PYTHON_SCRIPT("scripts/plot_matrix.py /tmp/P_cam.dat");

  msckf.max_window_size = 3;
  msckf.pruneCameraState();

  MU_CHECK_EQ(3, msckf.cam_states.size());
  MU_CHECK_EQ(0, msckf.cam_states[0].frame_id);
  MU_CHECK_EQ(3, msckf.cam_states[1].frame_id);
  MU_CHECK_EQ(4, msckf.cam_states[2].frame_id);
  MU_CHECK_EQ(CameraState::size * 3, msckf.P_cam.rows());
  MU_CHECK_EQ(CameraState::size * 3, msckf.P_cam.cols());
  MU_CHECK_EQ(msckf.imu_state.size, msckf.P_imu_cam.rows());
  MU_CHECK_EQ(CameraState::size * 3, msckf.P_imu_cam.cols());

  // std::cout << msckf.P_cam.rows() << " " << msckf.P_cam.cols() << std::endl;
  // std::cout << msckf.P_imu_cam.rows() << " " << msckf.P_imu_cam.cols() << std::endl;

  // mat2csv("/tmp/P_cam.dat", msckf.P_imu_cam);
  // PYTHON_SCRIPT("scripts/plot_matrix.py /tmp/P_cam.dat");

  return 0;
}

int test_MSCKF_measurementUpdate() {
  // Load raw dataset
  RawDataset raw_dataset(KITTI_RAW_DATASET, "2011_09_26", "0005");
  if (raw_dataset.load() != 0) {
    LOG_ERROR("Failed to load KITTI raw dataset [%s]!",
              KITTI_RAW_DATASET.c_str());
    return -1;
  }

  // Setup blackbox
  BlackBox blackbox;
  if (blackbox.configure("/tmp", "test_msckf_measurementUpdate") != 0) {
    LOG_ERROR("Failed to configure MSCKF blackbox!");
  }

  // Load first image
  cv::Mat img_ref = cv::imread(raw_dataset.cam0[0]);

  // Setup camera model
  const int image_width = img_ref.cols;
  const int image_height = img_ref.rows;
  const double fx = raw_dataset.calib_cam_to_cam.K[0](0, 0);
  const double fy = raw_dataset.calib_cam_to_cam.K[0](1, 1);
  const double cx = raw_dataset.calib_cam_to_cam.K[0](0, 2);
  const double cy = raw_dataset.calib_cam_to_cam.K[0](1, 2);
  CameraProperty camera_property{0, fx, fy, cx, cy, image_width, image_height};

  // Setup feature tracker
  KLTTracker tracker{camera_property};
  tracker.initialize(img_ref);

  // Setup MSCKF
  MSCKF msckf;
  msckf.configure(MSCKF_CONFIG);
  msckf.initialize(raw_dataset.oxts.timestamps[0],
                   euler2quat(raw_dataset.oxts.rpy[0]),
                   raw_dataset.oxts.v_G[0],
                   vec3_t{0.0, 0.0, 0.0});

  // Record initial conditions
  blackbox.recordTimeStep(raw_dataset.oxts.time[0],
                          msckf,
                          raw_dataset.oxts.a_B[0],
                          raw_dataset.oxts.w_B[0],
                          raw_dataset.oxts.p_G[0],
                          raw_dataset.oxts.v_G[0],
                          raw_dataset.oxts.rpy[0]);

  // Loop through data and do prediction update
  struct timespec msckf_start = tic();
  for (size_t i = 1; i < raw_dataset.oxts.time.size() - 1; i++) {
    // for (int i = 1; i < 50; i++) {
    // for (int i = 1; i < 200; i++) {
    // Feature tracker
    const std::string img_path = raw_dataset.cam0[i];
    const cv::Mat img_cur = cv::imread(img_path);
    tracker.update(img_cur);
    FeatureTracks tracks = tracker.getLostTracks();
    // tracker.show_matches = true;
    // cv::waitKey(1);

    // MSCKF
    const vec3_t a_B = raw_dataset.oxts.a_B[i];
    const vec3_t w_B = raw_dataset.oxts.w_B[i];
    const long ts = raw_dataset.oxts.timestamps[i];
    msckf.predictionUpdate(a_B, w_B, ts);

    // // Cheat by loading the robot ground truth to MSCKF's imu state
    // msckf.imu_state.p_G = raw_dataset.oxts.p_G[i];
    // msckf.imu_state.q_IG = euler2quat(raw_dataset.oxts.rpy[i]);
    msckf.measurementUpdate(tracks);

    // Record
    blackbox.recordTimeStep(raw_dataset.oxts.time[i],
                            msckf,
                            a_B,
                            w_B,
                            raw_dataset.oxts.p_G[i],
                            raw_dataset.oxts.v_G[i],
                            raw_dataset.oxts.rpy[i]);

    printf("frame: %zu, nb_tracks: %ld\n", i, tracks.size());
  }
  printf("-- total elasped: %fs --\n", toc(&msckf_start));
  blackbox.recordCameraStates(msckf);
  PYTHON_SCRIPT("scripts/plot_msckf.py /tmp/test_msckf_measurementUpdate");

  return 0;
}

int test_MSCKF_measurementUpdate2() {
  // Load raw dataset
  const std::string dataset_path = "/data/euroc_mav/raw/mav0";
  MAVDataset dataset(dataset_path);
  if (dataset.load() != 0) {
    LOG_ERROR("Failed to load mav raw dataset [%s]!", dataset_path.c_str());
    return -1;
  }

  // Setup blackbox
  BlackBox blackbox;
  if (blackbox.configure("/tmp", "test_msckf_measurementUpdate") != 0) {
    LOG_ERROR("Failed to configure MSCKF blackbox!");
  }

  // Load first image
  // cv::Mat img_ref = cv::imread(dataset.cam0[0]);

  // // Setup camera model
  // const int image_width = img_ref.cols;
  // const int image_height = img_ref.rows;
  // const double fx = raw_dataset.calib_cam_to_cam.K[0](0, 0);
  // const double fy = raw_dataset.calib_cam_to_cam.K[0](1, 1);
  // const double cx = raw_dataset.calib_cam_to_cam.K[0](0, 2);
  // const double cy = raw_dataset.calib_cam_to_cam.K[0](1, 2);
  // PinholeModel pinhole_model{image_width, image_height, fx, fy, cx, cy};

  // // Setup feature tracker
  // KLTTracker tracker{&pinhole_model};
  // tracker.initialize(img_ref);

  // Setup MSCKF
  MSCKF msckf;
  msckf.configure(MSCKF_CONFIG);
  msckf.initialize(dataset.timestamps[0]);
  // msckf.initialize(dataset.timestamps[0],
  //                  dataset.ground_truth.q_RS[0],
  //                  dataset.ground_truth.v_RS_R[0],
  //                  dataset.ground_truth.p_RS_R[0]);

  // std::cout << "init v: " << dataset.ground_truth.v_RS_R[0] << std::endl;

  // Record initial conditions
  blackbox.recordTimeStep(dataset.time[0],
                          msckf,
                          dataset.imu_data.a_B[0],
                          dataset.imu_data.w_B[0],
                          dataset.ground_truth.p_RS_R[0],
                          dataset.ground_truth.v_RS_R[0],
                          quat2euler(dataset.ground_truth.q_RS[0]));

  // Loop through data and do prediction update
  struct timespec msckf_start = tic();

  dataset.get_state = std::bind(&MSCKF::getState, &msckf);

  dataset.imu_cb = std::bind(&MSCKF::predictionUpdate,
                             &msckf,
                             std::placeholders::_1,
                             std::placeholders::_2,
                             std::placeholders::_3);

  dataset.record_cb = std::bind(&BlackBox::recordEstimate,
                                &blackbox,
                                std::placeholders::_1,
                                std::placeholders::_2,
                                std::placeholders::_3,
                                std::placeholders::_4);

  dataset.run();

  printf("-- total elasped: %fs --\n", toc(&msckf_start));
  // blackbox.recordCameraStates(msckf);
  PYTHON_SCRIPT("scripts/plot_msckf.py /tmp/test_msckf_measurementUpdate");

  return 0;
}

// int test_MSCKF_measurementUpdate3() {
//   std::srand(23);
//
//   // Setup world
//   SimWorld world;
//   const double dt = 0.1;
//   world.nb_features = 10000;
//   world.configure(dt);
//
//   // Setup blackbox
//   BlackBox blackbox;
//   if (blackbox.configure("/tmp", "test_msckf_measurementUpdate") != 0) {
//     LOG_ERROR("Failed to configure MSCKF blackbox!");
//   }
//
//   // Setup MSCKF
//   MSCKF msckf;
//   // Initialize MSCKF
//   msckf.configure(MSCKF_CONFIG);
//   msckf.initialize(0,
//                    euler2quat(world.robot.rpy_G),
//                    euler321ToRot(world.robot.rpy_G) * world.robot.v_B,
//                    vec3_t::Zero());
//
//   // Record initial conditions
//   blackbox.recordTimeStep(world.t,
//                           msckf,
//                           world.robot.a_B,
//                           world.robot.w_B,
//                           world.robot.p_G,
//                           world.robot.v_G,
//                           world.robot.rpy_G);
//
//   // Simulate
//   for (double t = dt; t < 10.0; t += dt) {
//     // for (double t = 0.0; t < 1.4; t += dt) {
//     // Step simulation
//     world.step();
//     FeatureTracks tracks = world.removeLostTracks();
//
//     // MSCKF
//     const vec3_t a_m = world.robot.a_B + vec3_t{0.0, 0.0, 9.81};
//     const vec3_t w_m = world.robot.w_B;
//     const long ts = t * 1e9;
//     msckf.predictionUpdate(a_m, w_m, ts);
//
//     // // Cheat by loading the robot ground truth to MSCKF's imu state
//     // msckf.imu_state.p_G = world.robot.p_G;
//     // msckf.imu_state.q_IG = euler2quat(world.robot.rpy_G);
//
//     msckf.measurementUpdate(tracks);
//
//     // Record
//     blackbox.recordTimeStep(t,
//                             msckf,
//                             world.robot.a_B,
//                             world.robot.w_B,
//                             world.robot.p_G,
//                             world.robot.v_G,
//                             world.robot.rpy_G);
//
//     printf("Time index: %lu, nb_tracks: %lu\n",
//            world.time_index,
//            tracks.size());
//   }
//   blackbox.recordCameraStates(msckf);
//   PYTHON_SCRIPT("scripts/plot_msckf.py /tmp/test_msckf_measurementUpdate");
//
//   // mat2csv("/tmp/features.dat", world.features3d);
//   // PYTHON_SCRIPT("scripts/plot_world.py /tmp/features.dat");
//
//   return 0;
// }

void test_suite() {
  // MU_ADD_TEST(test_MSCKF_constructor);
  // MU_ADD_TEST(test_MSCKF_configure);
  // MU_ADD_TEST(test_MSCKF_initialize);
  // MU_ADD_TEST(test_MSCKF_P);
  // MU_ADD_TEST(test_MSCKF_J);
  // MU_ADD_TEST(test_MSCKF_N);
  // MU_ADD_TEST(test_MSCKF_H);
  // MU_ADD_TEST(test_MSCKF_augmentState);
  // MU_ADD_TEST(test_MSCKF_getTrackCameraStates);
  // MU_ADD_TEST(test_MSCKF_predictionUpdate);
  // MU_ADD_TEST(test_MSCKF_residualizeTrack);
  // MU_ADD_TEST(test_MSCKF_calcResiduals);
  // MU_ADD_TEST(test_MSCKF_correctIMUState);
  // MU_ADD_TEST(test_MSCKF_correctCameraStates);
  MU_ADD_TEST(test_MSCKF_pruneCameraStates);
  // MU_ADD_TEST(test_MSCKF_measurementUpdate);
  // MU_ADD_TEST(test_MSCKF_measurementUpdate2);
  // MU_ADD_TEST(test_MSCKF_measurementUpdate3);
}

} // namespace prototype

MU_RUN_TESTS(prototype::test_suite);
