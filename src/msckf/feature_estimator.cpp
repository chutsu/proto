#include "prototype/msckf/feature_estimator.hpp"

namespace prototype {

vec3_t lls_triangulation(const vec3_t &u1,
                       const mat34_t &P1,
                       const vec3_t &u2,
                       const mat34_t &P2) {
  // Build matrix A for homogenous equation system Ax = 0, assume X = (x,y,z,1),
  // for Linear-LS method which turns it into a AX = B system, where:
  // - A is 4x3,
  // - X is 3x1
  // - B is 4x1

  // clang-format off
  matx_t A = zeros(4, 3);
  A << u1(0) * P1(2, 0) - P1(0, 0), u1(0) * P1(2, 1) - P1(0, 1), u1(0) * P1(2, 2) - P1(0, 2),
       u1(1) * P1(2, 0) - P1(1, 0), u1(1) * P1(2, 1) - P1(1, 1), u1(1) * P1(2, 2) - P1(1, 2),
       u2(0) * P2(2, 0) - P2(0, 0), u2(0) * P2(2, 1) - P2(0, 1), u2(0) * P2(2, 2) - P2(0, 2),
       u2(1) * P2(2, 0) - P2(1, 0), u2(1) * P2(2, 1) - P2(1, 1), u2(1) * P2(2, 2) - P2(1, 2);

  vec4_t B{-(u1(0) * P1(2, 3) - P1(0,3)),
         -(u1(1) * P1(2, 3) - P1(1,3)),
         -(u2(0) * P2(2, 3) - P2(0,3)),
         -(u2(1) * P2(2, 3) - P2(1,3))};
  // clang-format on

  // SVD
  const auto svd_options = Eigen::ComputeThinU | Eigen::ComputeThinV;
  const vec3_t X = A.jacobiSvd(svd_options).solve(B);

  return X;
}

vec3_t lls_triangulation(const vec2_t &z1,
                       const vec2_t &z2,
                       const mat3_t &C_C0C1,
                       const vec3_t &t_C0_C0C1) {
  // Triangulate
  // -- Matrix A
  matx_t A = zeros(3, 2);
  A.block(0, 0, 3, 1) = z1.homogeneous();
  A.block(0, 1, 3, 1) = -C_C0C1 * z2.homogeneous();

  // -- Vector b
  const vec3_t b{t_C0_C0C1};
  // -- Perform SVD
  const auto svd_options = Eigen::ComputeThinU | Eigen::ComputeThinV;
  const vecx_t x = A.jacobiSvd(svd_options).solve(b);
  // -- Calculate p_C0_f
  const vec3_t p_C0_f = x(0) * z1.homogeneous();

  return p_C0_f;
}

vec3_t lls_triangulation(const vec2_t &z1, const vec2_t &z2, const mat4_t T_C1_C0) {
  const mat3_t C_C1C0 = T_C1_C0.block(0, 0, 3, 3);
  const vec3_t t_C0_C1C0 = T_C1_C0.block(0, 3, 3, 1);
  const vec3_t m = C_C1C0 * z1.homogeneous();

  // Form A
  vec2_t A;
  A(0) = m(0) - z2(0) * m(2);
  A(1) = m(1) - z2(1) * m(2);

  // Form b
  vec2_t b;
  b(0) = z2(0) * t_C0_C1C0(2) - t_C0_C1C0(0);
  b(1) = z2(1) * t_C0_C1C0(2) - t_C0_C1C0(1);

  // Solve for depth
  const double depth = (A.transpose() * A).inverse() * A.transpose() * b;

  // Form initial feature position relative to camera 0
  vec3_t p_C0_f;
  p_C0_f(0) = z1(0) * depth;
  p_C0_f(1) = z1(1) * depth;
  p_C0_f(2) = depth;

  return p_C0_f;
}

void triangulate_mono_tracks(const mat4_t &T_cam1_cam0, FeatureTracks &tracks) {
  // Pre-check
  if (tracks.size() == 0) {
    return;
  }

  // Camera 0 - projection matrix P
  const mat3_t cam0_R = I(3);
  const vec3_t cam0_t = zeros(3, 1);
  const mat34_t cam0_P = pinhole_projection_matrix(I(3), cam0_R, cam0_t);
  const cv::Mat P0 = convert(cam0_P);

  // Camera 1 - projection matrix P
  const mat3_t cam1_R = T_cam1_cam0.block(0, 0, 3, 3);
  const vec3_t cam1_t = T_cam1_cam0.block(0, 3, 3, 1);
  mat34_t cam1_P;
  cam1_P.block(0, 0, 3, 3) = cam1_R;
  cam1_P.block(0, 3, 3, 1) = cam1_t;
  const cv::Mat P1 = convert(cam1_P);

  // Construct cam0 and cam1 points for triangulation
  cv::Mat cam0_pts(2, tracks.size(), CV_32FC1);
  cv::Mat cam1_pts(2, tracks.size(), CV_32FC1);
  int column = 0;
  for (auto track : tracks) {
    // We are using the last keypoint observed because all tracks have
    // different lengths, the only keypoints we can guarantee are observed on
    // the same stereo camera pose are the keypoints from the last frame
    const vec2_t z1 = track.track[0].getKeyPoint();
    const vec2_t z2 = track.track[1].getKeyPoint();
    cam0_pts.at<float>(0, column) = (float) z1(0);
    cam0_pts.at<float>(1, column) = (float) z1(1);
    cam1_pts.at<float>(0, column) = (float) z2(0);
    cam1_pts.at<float>(1, column) = (float) z2(1);
    column++;
  }

  // Triangulate points
  cv::Mat pts_3d(4, tracks.size(), CV_64F);
  cv::triangulatePoints(P0, P1, cam0_pts, cam1_pts, pts_3d);

  // Normalize homogeneous 3D points and set feature tracl
  for (int i = 0; i < pts_3d.cols; i++) {
    const cv::Mat pt = pts_3d.col(i);
    const float x = pt.at<float>(0, 0);
    const float y = pt.at<float>(1, 0);
    const float z = pt.at<float>(2, 0);
    const float h = pt.at<float>(3, 0);
    tracks[i].p_C0_f = vec3_t{x / h, y / h, z / h};
  }
}

void group_tracks(const FeatureTracks &tracks,
                  std::map<FrameID, FeatureTracks> &groups,
                  std::set<FrameID> &frame_starts) {
  for (const auto &track : tracks) {
    const FrameID frame_start = track.frame_start;
    groups[frame_start].push_back(track);
    frame_starts.insert(frame_start);
  }
}

FeatureTracks triangulate_stereo_tracks(const mat4_t &T_cam1_cam0,
                                        FeatureTracks &tracks) {
  assert(tracks.size() != 0);
  assert(tracks[0].type != -1 || tracks[0].type != MONO_TRACK);

  // Camera 0 - projection matrix P
  const mat3_t cam0_R = I(3);
  const vec3_t cam0_t = zeros(3, 1);
  const mat34_t cam0_P = pinhole_projection_matrix(I(3), cam0_R, cam0_t);
  const cv::Mat P0 = convert(cam0_P);

  // Camera 1 - projection matrix P
  const mat3_t cam1_R = T_cam1_cam0.block(0, 0, 3, 3);
  const vec3_t cam1_t = T_cam1_cam0.block(0, 3, 3, 1);
  mat34_t cam1_P;
  cam1_P.block(0, 0, 3, 3) = cam1_R;
  cam1_P.block(0, 3, 3, 1) = cam1_t;
  const cv::Mat P1 = convert(cam1_P);

  // Group tracks depending on when they were first captured
  std::map<FrameID, FeatureTracks> track_groups;
  std::set<FrameID> frame_starts;
  group_tracks(tracks, track_groups, frame_starts);

  FeatureTracks triangulated_tracks;
  for (const auto &frame_start : frame_starts) {
    auto &group = track_groups[frame_start];

    // Construct cam0 and cam1 points for triangulation
    cv::Mat cam0_pts(2, group.size(), CV_32FC1);
    cv::Mat cam1_pts(2, group.size(), CV_32FC1);
    int cols = 0;
    for (auto &track : group) {
      const vec2_t z1 = track.track0.front().getKeyPoint();
      const vec2_t z2 = track.track1.front().getKeyPoint();
      cam0_pts.at<float>(0, cols) = (float) z1(0);
      cam0_pts.at<float>(1, cols) = (float) z1(1);
      cam1_pts.at<float>(0, cols) = (float) z2(0);
      cam1_pts.at<float>(1, cols) = (float) z2(1);
      cols++;
    }

    // Triangulate points
    cv::Mat pts_3d(4, group.size(), CV_64F);
    cv::triangulatePoints(P0, P1, cam0_pts, cam1_pts, pts_3d);

    // Normalize homogeneous 3D points and set feature tracl
    for (int i = 0; i < pts_3d.cols; i++) {
      const cv::Mat pt = pts_3d.col(i);
      const float x = pt.at<float>(0, 0);
      const float y = pt.at<float>(1, 0);
      const float z = pt.at<float>(2, 0);
      const float h = pt.at<float>(3, 0);
      const vec3_t pt_3d{x / h, y / h, z / h};

      // Add triangulated feature position back to feature track
      auto &track = group[i];
      track.p_C0_f = pt_3d;
      triangulated_tracks.push_back(track);
    }
  }

  return triangulated_tracks;
}

FeatureEstimator::FeatureEstimator(const FeatureTrack &track,
                                   const CameraStates &track_cam_states)
    : track{track}, track_cam_states{track_cam_states} {}

FeatureEstimator::FeatureEstimator(const FeatureTrack &track,
                                   const CameraStates &track_cam_states,
                                   const GimbalModel &gimbal_model)
    : track{track}, track_cam_states{track_cam_states}, gimbal_model{gimbal_model} {}

int FeatureEstimator::triangulate(const vec2_t &z1,
                                  const vec2_t &z2,
                                  const mat3_t &C_C0C1,
                                  const vec3_t &t_C0_C0C1,
                                  vec3_t &p_C0_f) {
  // Convert points to homogenous coordinates and normalize
  vec3_t pt1{z1[0], z1[1], 1.0};
  vec3_t pt2{z2[0], z2[1], 1.0};
  pt1.normalize();
  pt2.normalize();

  // Form camera matrix P1
  const mat34_t P1 = I(3) * I(3, 4);

  // Form camera matrix P2
  mat34_t T2;
  T2.block(0, 0, 3, 3) = C_C0C1;
  T2.block(0, 3, 3, 1) = -C_C0C1 * t_C0_C0C1;
  const mat34_t P2 = I(3) * T2;

  // Perform linear least squares triangulation from 2 views
  p_C0_f = lls_triangulation(pt1, P1, pt2, P2);

  return 0;
}

int FeatureEstimator::initialEstimate(vec3_t &p_C0_f) {
  if (this->track.type == MONO_TRACK) {
    // -- Calculate rotation and translation of first and second camera states
    const CameraState cam0 = this->track_cam_states.front();
    const CameraState cam1 = this->track_cam_states.back();
    // -- Get rotation and translation of camera 0 and camera 1
    const mat3_t C_C0G = C(cam0.q_CG);
    const mat3_t C_C1G = C(cam1.q_CG);
    const vec3_t p_G_C0 = cam0.p_G;
    const vec3_t p_G_C1 = cam1.p_G;
    // -- Calculate rotation and translation from camera 0 to camera 1
    const mat3_t C_C0C1 = C_C0G * C_C1G.transpose();
    const vec3_t t_C0_C0C1 = C_C0G * (p_G_C1 - p_G_C0);
    // -- Get observed image points
    const vec2_t z1 = this->track.track.front().getKeyPoint();
    const vec2_t z2 = this->track.track.back().getKeyPoint();

    // Triangulate
    if ((z1 - z2).norm() > 1.0e-3) {
      FeatureEstimator::triangulate(z1, z2, C_C0C1, t_C0_C0C1, p_C0_f);
    } else {
      return -1;
    }

  } else if (this->track.type == STATIC_STEREO_TRACK) {
    // Triangulate feature point observed by stereo camera
    // -- Make sure the camera extrinsics are set
    const auto T_C1_C0 = this->track.T_cam1_cam0;
    assert(T_C1_C0.isApprox(zeros(4, 4)) == false);
    // -- Get observed image points
    const vec2_t z1 = this->track.track0.front().getKeyPoint();
    const vec2_t z2 = this->track.track1.front().getKeyPoint();

    // // Camera 0 - projection matrix P
    // const mat3_t cam0_R = I(3);
    // const vec3_t cam0_t = zeros(3, 1);
    // const mat34_t cam0_P = pinhole_projection_matrix(I(3), cam0_R, cam0_t);
    // const cv::Mat P0 = convert(cam0_P);
    //
    // // Camera 1 - projection matrix P
    // const mat3_t cam1_R = T_C1_C0.block(0, 0, 3, 3);
    // const vec3_t cam1_t = T_C1_C0.block(0, 3, 3, 1);
    // mat34_t cam1_P;
    // cam1_P.block(0, 0, 3, 3) = cam1_R;
    // cam1_P.block(0, 3, 3, 1) = cam1_t;
    // const cv::Mat P1 = convert(cam1_P);
    //
    // // Construct cam0 and cam1 points for triangulation
    // cv::Mat cam0_pts(2, 1, CV_32FC1);
    // cv::Mat cam1_pts(2, 1, CV_32FC1);
    // cam0_pts.at<float>(0, 0) = (float) z1(0);
    // cam0_pts.at<float>(1, 0) = (float) z1(1);
    // cam1_pts.at<float>(0, 0) = (float) z2(0);
    // cam1_pts.at<float>(1, 0) = (float) z2(1);
    //
    // // Triangulate points
    // cv::Mat pts_3d(4, 1, CV_64F);
    // cv::triangulatePoints(P0, P1, cam0_pts, cam1_pts, pts_3d);
    // const cv::Mat pt = pts_3d.col(0);
    // const float x = pt.at<float>(0, 0);
    // const float y = pt.at<float>(1, 0);
    // const float z = pt.at<float>(2, 0);
    // const float h = pt.at<float>(3, 0);
    // p_C0_f = vec3_t{x / h, y / h, z / h};

    // -- Triangulate
    p_C0_f = lls_triangulation(z1, z2, T_C1_C0);
    if (p_C0_f(2) < 0.0) {
      LOG_WARN("Bad initialization: [%.2f, %.2f, %.2f]",
               p_C0_f(0),
               p_C0_f(1),
               p_C0_f(2));
    }

    // std::cout << "p_C0_f: " << p_C0_f.transpose() << std::endl;
    // std::cout << "z1: " << z1.transpose() << std::endl;
    // std::cout << "z2: " << z2.transpose() << std::endl;
    // std::cout << "T_C1_C0: " << T_C1_C0 << std::endl;
    // std::cout << this->track.T_cam1_cam0.size() << std::endl;
    // exit(0);

  } else if (this->track.type == DYNAMIC_STEREO_TRACK) {
    // Triangulate feature point observed by stereo camera
    // -- Make sure the camera extrinsics are set
    const vec2_t theta = this->track.joint_angles.front();
    const auto T_C1_C0 = this->gimbal_model.T_ds(theta);
    assert(T_C1_C0.isApprox(zeros(4, 4)) == false);
    // -- Get observed image points
    const vec2_t z1 = this->track.track0.front().getKeyPoint();
    const vec2_t z2 = this->track.track1.front().getKeyPoint();

    // -- Triangulate
    p_C0_f = lls_triangulation(z1, z2, T_C1_C0);
    if (p_C0_f(2) < 0.0) {
      LOG_WARN("Bad initialization: [%.2f, %.2f, %.2f]",
               p_C0_f(0),
               p_C0_f(1),
               p_C0_f(2));
    }

    // std::cout << "ground_truth: " << this->track.track0.front().ground_truth.transpose() << std::endl;
    // std::cout << "p_C0_f: " << p_C0_f.transpose() << std::endl;
    // std::cout << "z1: " << z1.transpose() << std::endl;
    // std::cout << "z2: " << z2.transpose() << std::endl;
    // std::cout << "T_C1_C0: " << T_C1_C0 << std::endl;
    // exit(0);

  } else {
    FATAL("Invalid feature track type [%d]", track.type);
  }

  return 0;
}

int FeatureEstimator::checkEstimate(const vec3_t &p_G_f) {
  const int N = this->track_cam_states.size();

  // Pre-check
  if (std::isnan(p_G_f(0)) || std::isnan(p_G_f(1)) || std::isnan(p_G_f(2))) {
    return -1;
  }

  // Make sure feature is infront of camera all the way through
  for (int i = 0; i < N; i++) {
    // Transform feature from global frame to i-th camera frame
    const mat3_t C_CiG = C(this->track_cam_states[i].q_CG);
    const vec3_t p_Ci_f = C_CiG * (p_G_f - this->track_cam_states[i].p_G);

    // Check if feature is in-front of camera
    if (p_Ci_f(2) < 0.0 || p_Ci_f(2) > 100.0) {
      return -1;
    }
  }

  // const mat3_t C_C0G = C(this->track_cam_states[0].q_CG);
  // const vec3_t p_C0_f = C_C0G * (p_G_f - this->track_cam_states[0].p_G);
  // std::cout << "p_C0_f: " << p_C0_f.transpose() << std::endl;

  return 0;
}

void FeatureEstimator::transformEstimate(const double alpha,
                                         const double beta,
                                         const double rho,
                                         vec3_t &p_G_f) {
  // Transform feature position from camera to global frame
  const vec3_t X{alpha, beta, 1.0};
  const double z = 1 / rho;
  const mat3_t C_C0G = C(this->track_cam_states[0].q_CG);
  const vec3_t p_G_C0 = this->track_cam_states[0].p_G;
  p_G_f = z * C_C0G.transpose() * X + p_G_C0;
}

vec2_t FeatureEstimator::residual(const mat4_t &T_Ci_C0,
                                const vec2_t &z,
                                const vec3_t &x) {
  // Project estimated feature location to image plane
  // -- Inverse depth params
  const double alpha = x(0);
  const double beta = x(1);
  const double rho = x(2);
  // -- Setup vectors and matrices
  const vec3_t A{alpha, beta, 1.0};
  const mat3_t C_CiC0 = T_Ci_C0.block(0, 0, 3, 3);
  const vec3_t t_C0_CiC0 = T_Ci_C0.block(0, 3, 3, 1);
  // -- Project estimated feature
  const vec3_t h = C_CiC0 * A + rho * t_C0_CiC0;

  // Calculate reprojection error
  // -- Convert feature location to normalized coordinates
  const vec2_t z_hat{h(0) / h(2), h(1) / h(2)};
  // -- Reprojection error
  const vec2_t r = z - z_hat;

  return r;
}

matx_t FeatureEstimator::jacobian(const mat4_t &T_Ci_C0, const vecx_t &x) {
  double alpha = x(0);
  double beta = x(1);
  double rho = x(2);

  // Set camera 0 as origin, work out rotation and translation
  // of camera i relative to to camera 0
  const mat3_t C_CiC0 = T_Ci_C0.block(0, 0, 3, 3);
  const vec3_t t_C0_CiC0 = T_Ci_C0.block(0, 3, 3, 1);

  // Project estimated feature location to image plane
  const vec3_t A{alpha, beta, 1.0};
  const vec3_t h = C_CiC0 * A + rho * t_C0_CiC0;

  // Compute jacobian
  const double hx_div_hz2 = (h(0) / pow(h(2), 2));
  const double hy_div_hz2 = (h(1) / pow(h(2), 2));

  const vec2_t drdalpha{-C_CiC0(0, 0) / h(2) + hx_div_hz2 * C_CiC0(2, 0),
                      -C_CiC0(1, 0) / h(2) + hy_div_hz2 * C_CiC0(2, 0)};
  const vec2_t drdbeta{-C_CiC0(0, 1) / h(2) + hx_div_hz2 * C_CiC0(2, 1),
                     -C_CiC0(1, 1) / h(2) + hy_div_hz2 * C_CiC0(2, 1)};
  const vec2_t drdrho{-t_C0_CiC0(0) / h(2) + hx_div_hz2 * t_C0_CiC0(2),
                    -t_C0_CiC0(1) / h(2) + hy_div_hz2 * t_C0_CiC0(2)};

  // Fill in the jacobian
  matx_t J = zeros(2, 3);
  J.block(0, 0, 2, 1) = drdalpha;
  J.block(0, 1, 2, 1) = drdbeta;
  J.block(0, 2, 2, 1) = drdrho;

  return J;
}

int FeatureEstimator::estimate(vec3_t &p_G_f) {
  // Calculate initial estimate of 3D position
  vec3_t p_C0_f = this->track.p_C0_f;
  if (p_C0_f.isApprox(vec3_t::Zero()) && this->initialEstimate(p_C0_f) != 0) {
    return -1;
  }

  // Prepare data
  const mat3_t C_C0G = C(this->track_cam_states[0].q_CG);
  const vec3_t p_G_C0 = this->track_cam_states[0].p_G;
  const int N = this->track_cam_states.size();
  std::vector<vec2_t> measurements;
  std::vector<mat4_t> cam_poses;
  for (int i = 0; i < N; i++) {
    // Add the measurement
    if (this->track.type == MONO_TRACK) {
      measurements.push_back(this->track.track[i].getKeyPoint());
    } else if (this->track.type == STATIC_STEREO_TRACK) {
      measurements.push_back(this->track.track0[i].getKeyPoint());
    }

    // Get camera current rotation and translation
    const mat3_t C_CiG = C(this->track_cam_states[i].q_CG);
    const vec3_t p_G_Ci = this->track_cam_states[i].p_G;

    // Set camera 0 as origin, work out rotation and translation
    // of camera i relative to to camera 0
    const mat3_t C_CiC0 = C_CiG * C_C0G.transpose();
    const vec3_t t_C0_CiC0 = C_CiG * (p_G_C0 - p_G_Ci);

    // Add camera pose
    const mat4_t T_Ci_CiC0 = transformation_matrix(C_CiC0, t_C0_CiC0);
    cam_poses.push_back(T_Ci_CiC0);
  }

  // Create inverse depth params (these are to be optimized)
  const double alpha = p_C0_f(0) / p_C0_f(2);
  const double beta = p_C0_f(1) / p_C0_f(2);
  const double rho = 1.0 / p_C0_f(2);
  vec3_t x{alpha, beta, rho};

  // Apply Levenberg-Marquart method to solve for the 3d position.
  struct OptimizationConfig optimization_config;
  double lambda = optimization_config.initial_damping;
  int inner_loop_cntr = 0;
  int outer_loop_cntr = 0;
  bool is_cost_reduced = false;
  double delta_norm = 0;

  // Compute the initial cost.
  double total_cost = 0.0;
  for (size_t i = 0; i < cam_poses.size(); i++) {
    const vec2_t r = this->residual(cam_poses[i], measurements[i], x);
    total_cost += r.squaredNorm();
  }

  // Outer loop.
  do {
    mat3_t A = mat3_t::Zero();
    vec3_t b = vec3_t::Zero();

    for (size_t i = 0; i < cam_poses.size(); ++i) {
      // Calculate jacobian and residual
      const matx_t J = this->jacobian(cam_poses[i], x);
      const vec2_t r = this->residual(cam_poses[i], measurements[i], x);

      // Compute weight based on residual
      double e = r.norm();
      double w = 0.0;
      if (e <= optimization_config.huber_epsilon) {
        w = 1.0;
      } else {
        w = optimization_config.huber_epsilon / (2 * e);
      }

      // Apply weight
      if (w == 1) {
        A += J.transpose() * J;
        b += J.transpose() * r;
      } else {
        double w_square = w * w;
        A += w_square * J.transpose() * J;
        b += w_square * J.transpose() * r;
      }
    }

    // Inner loop.
    // Solve for the delta that can reduce the total cost.
    do {
      const mat3_t damper = lambda * mat3_t::Identity();
      const vec3_t delta = (A + damper).ldlt().solve(b);
      const vec3_t x_new = x - delta;
      delta_norm = delta.norm();

      double new_cost = 0.0;
      for (size_t i = 0; i < cam_poses.size(); ++i) {
        const vec2_t r = this->residual(cam_poses[i], measurements[i], x_new);
        new_cost += r.squaredNorm();
      }

      if (new_cost < total_cost) {
        is_cost_reduced = true;
        x = x_new;
        total_cost = new_cost;
        lambda = lambda / 10 > 1e-10 ? lambda / 10 : 1e-10;

      } else {
        is_cost_reduced = false;
        lambda = lambda * 10 < 1e12 ? lambda * 10 : 1e12;
      }
    } while (inner_loop_cntr++ < optimization_config.inner_loop_max_iteration &&
             !is_cost_reduced);

    inner_loop_cntr = 0;
  } while (outer_loop_cntr++ < optimization_config.outer_loop_max_iteration &&
           delta_norm > optimization_config.estimation_precision);

  // Transform feature position from camera to global frame
  this->transformEstimate(x(0), x(1), x(2), p_G_f);
  if (std::isnan(p_G_f(0)) || std::isnan(p_G_f(1)) || std::isnan(p_G_f(2))) {
    return -2;
  }

  return 0;
}

AutoDiffReprojectionError::AutoDiffReprojectionError(const mat3_t &C_CiC0,
                                                     const vec3_t &t_Ci_CiC0,
                                                     const vec2_t &kp) {
  // Camera extrinsics
  mat2array(C_CiC0, this->C_CiC0);
  vec2array(t_Ci_CiC0, this->t_Ci_CiC0);

  // Measurement
  this->u = kp(0);
  this->v = kp(1);
}

bool AnalyticalReprojectionError::Evaluate(double const *const *x,
                                           double *residuals,
                                           double **jacobians) const {
  // Inverse depth parameters
  const double alpha = x[0][0];
  const double beta = x[0][1];
  const double rho = x[0][2];

  // Project estimated feature location to image plane
  const vec3_t A{alpha, beta, 1.0};
  const vec3_t h = this->C_CiC0 * A + rho * this->t_Ci_CiC0;

  // Calculate reprojection error
  // -- Convert measurment to image coordinates
  const vec2_t z{this->keypoint};
  // -- Convert feature location to normalized coordinates
  const vec2_t z_hat{h(0) / h(2), h(1) / h(2)};

  // Calculate residual error
  residuals[0] = z(0) - z_hat(0);
  residuals[1] = z(1) - z_hat(1);

  // Compute the Jacobian if asked for.
  if (jacobians != NULL && jacobians[0] != NULL) {
    // Pre-compute common terms
    const double hx_div_hz2 = (h(0) / (h(2), h(2)));
    const double hy_div_hz2 = (h(1) / (h(2), h(2)));

    // **IMPORTANT** The ceres-solver documentation does not explain very well
    // how one goes about forming the jacobian. In a ceres analytical cost
    // function, the jacobian ceres needs is a local jacobian only, in this
    // problem we have:
    //
    // - 2 Residuals (Reprojection Error in x, y axis)
    // - 1 Parameter block of size 3 (Inverse depth, alpha, beta, rho)
    //
    // The resultant local jacobian should be of size 2x3 (2 residuals, 1st
    // parameter of size 3). The way we fill in the `jacobians` double array
    // variable is that since we are only calculating 1 local jacobian, we only
    // need to access the first index (i.e. 0), and then we fill in the
    // jacobian in ROW-MAJOR-ORDER, `jacobians[0][0...5]` for the 2x3
    // analytical jacobian, Ceres then in turn uses that information and forms
    // the global jacobian themselves.
    //
    // **IF** the problem had n parameter blocks, you would have filled in the
    // `jacobians[0..n][...]` local jacobians.
    //
    // For this problem our local jacobian has the form:
    //
    //   [drx / dalpha, drx / dbeta, drx / drho]
    //   [dry / dalpha, dry / dbeta, dry / drho]
    //
    // Or in row-major index form:
    //
    //   [0, 1, 2]
    //   [3, 4, 5]
    //

    // dr / dalpha
    jacobians[0][0] = -C_CiC0(0, 0) / h(2) + hx_div_hz2 * C_CiC0(2, 0);
    jacobians[0][3] = -C_CiC0(1, 0) / h(2) + hy_div_hz2 * C_CiC0(2, 0);

    // dr / dbeta
    jacobians[0][1] = -C_CiC0(0, 1) / h(2) + hx_div_hz2 * C_CiC0(2, 1);
    jacobians[0][4] = -C_CiC0(1, 1) / h(2) + hy_div_hz2 * C_CiC0(2, 1);

    // dr / drho
    jacobians[0][2] = -t_Ci_CiC0(0) / h(2) + hx_div_hz2 * t_Ci_CiC0(2);
    jacobians[0][5] = -t_Ci_CiC0(1) / h(2) + hy_div_hz2 * t_Ci_CiC0(2);
  }

  return true;
}

void CeresFeatureEstimator::addResidualBlock(const vec2_t &kp,
                                             const mat3_t &C_CiC0,
                                             const vec3_t &t_Ci_CiC0,
                                             double *x) {
  // Build residual
  if (this->method == "ANALYTICAL") {
    auto cost_func = new AnalyticalReprojectionError(C_CiC0, t_Ci_CiC0, kp);

    // Add residual block to problem
    this->problem.AddResidualBlock(cost_func, // Cost function
                                   NULL,      // Loss function
                                   x); // Optimization parameters

  } else if (this->method == "AUTODIFF") {
    // Build residual
    auto residual = new AutoDiffReprojectionError(C_CiC0, t_Ci_CiC0, kp);

    // Build cost and loss function
    auto cost_func = new ceres::AutoDiffCostFunction<
        AutoDiffReprojectionError, // Residual
        2,                         // Size of residual
        3                          // Size of 1st parameter - inverse depth
        >(residual);

    // Add residual block to problem
    this->problem.AddResidualBlock(cost_func, // Cost function
                                   nullptr,   // Loss function
                                   x);        // Optimization parameters

  } else {
    LOG_ERROR("Invalid feature estimator method [%s]!", this->method.c_str());
    exit(-1);
  }
}

int CeresFeatureEstimator::setupProblem() {
  // Calculate initial estimate of 3D position
  vec3_t p_C0_f = this->track.p_C0_f;
  if (p_C0_f.isApprox(vec3_t::Zero()) && this->initialEstimate(p_C0_f) != 0) {
    return -1;
  }
  /* std::cout << "p_C0_f: " << p_C0_f.transpose() << std::endl; */
  /* if (p_C0_f(2) > 10.0) { */
  /*   std::cout << this->track << std::endl; */
  /*   #<{(| exit(0); |)}># */
  /*   return -1; */
  /* } */


  // Create inverse depth params (these are to be optimized)
  this->x[0] = p_C0_f(0) / p_C0_f(2); // Alpha
  this->x[1] = p_C0_f(1) / p_C0_f(2); // Beta
  this->x[2] = 1.0 / p_C0_f(2);       // Rho

  // Add residual blocks
  const int N = this->track_cam_states.size();
  const mat3_t C_C0G = C(this->track_cam_states[0].q_CG);
  const vec3_t p_G_C0 = this->track_cam_states[0].p_G;

  for (int i = 0; i < N; i++) {
    // Get camera's current rotation and translation
    const mat3_t C_CiG = C(track_cam_states[i].q_CG);
    const vec3_t p_G_Ci = track_cam_states[i].p_G;

    // Set camera 0 as origin, work out rotation and translation
    // of camera i relative to to camera 0
    const mat3_t C_CiC0 = C_CiG * C_C0G.transpose();
    const vec3_t t_Ci_CiC0 = C_CiG * (p_G_C0 - p_G_Ci);

    // Add residual block
    if (this->track.type == MONO_TRACK) {
      this->addResidualBlock(this->track.track[i].getKeyPoint(),
                             C_CiC0,
                             t_Ci_CiC0,
                             this->x);
    } else if (this->track.type == STATIC_STEREO_TRACK
               || this->track.type == DYNAMIC_STEREO_TRACK) {
      this->addResidualBlock(this->track.track0[i].getKeyPoint(),
                             C_CiC0,
                             t_Ci_CiC0,
                             this->x);

    } else {
      FATAL("Invalid feature track type [%d]", this->track.type);
    }
  }

  return 0;
}

int CeresFeatureEstimator::estimate(vec3_t &p_G_f) {
  // Set options
  this->options.max_num_iterations = 1000;
  this->options.num_threads = 1;
  // this->options.num_linear_solver_threads = 1;
  this->options.minimizer_progress_to_stdout = false;

  /* // Cheat by using ground truth data */
  /* if (this->track.track0[0].ground_truth.isApprox(vec3_t::Zero()) == false) { */
  /*   p_G_f = this->track.track0[0].ground_truth; */
  /*   return 0; */
  /* } */

  // Setup problem
  if (this->setupProblem() != 0) {
    return -1;
  }

  // // Check if camera has actually moved?
  // const auto cam_state_first = this->track_cam_states.front();
  // const auto cam_state_last = this->track_cam_states.back();
  // const vec3_t pos_diff = cam_state_first.p_G - cam_state_last.p_G;

  // double pos_norm = 0.0;
  // for (size_t i = 1; i < this->track_cam_states.size(); i++) {
  //   const auto cam_state_prev = this->track_cam_states[i - 1];
  //   const auto cam_state_curr = this->track_cam_states[i];
  //   pos_norm += (cam_state_prev.p_G - cam_state_curr.p_G).norm();
  // }
  // pos_norm = pos_norm / this->track_cam_states.size();

  // const double pos_norm = pos_diff.norm();
  // if (pos_norm < 0.1) {
  //   const vec3_t X{this->x[0] / this->x[2], this->x[1] / this->x[2], 1.0 / this->x[2]};
  //   if (X(2) > 10.0) {
  //     LOG_WARN("Bad p_C0_f: [%.2f, %.2f, %.2f]", X(0), X(1), X(2));
  //     return -2;
  //   }
  //   this->transformEstimate(this->x[0], this->x[1], this->x[2], p_G_f);
  //   std::cout << "p_G_f: " << p_G_f.transpose() << std::endl;
  //   return 0;
  // }

  // Solve
  ceres::Solve(this->options, &this->problem, &this->summary);

  // Transform feature position from camera to global frame
  this->transformEstimate(this->x[0], this->x[1], this->x[2], p_G_f);

  // Check estimate
  if (this->checkEstimate(p_G_f) != 0) {
    LOG_WARN("Bad estimate p_G_f [%.2f, %.2f, %.2f]",
             p_G_f(0),
             p_G_f(1),
             p_G_f(2));
    LOG_WARN("track length: %d", (int) this->track.trackedLength());
    return -2;
  }

  // vec3_t gnd = this->track.track0[0].ground_truth;
  // std::cout << "gnd: " << gnd.transpose() << std::endl;
  // std::cout << "est: " << p_G_f.transpose() << std::endl;
  // std::cout << "diff: " << (gnd - p_G_f).norm() << std::endl;
  // std::cout << std::endl;

  return 0;
}

} //  namespace prototype
