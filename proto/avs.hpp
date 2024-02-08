#ifndef AVS_HPP
#define AVS_HPP

#include <map>
#include <vector>
#include <algorithm>
#include <ceres/ceres.h>

extern "C" {
#include "proto.h"
#include "euroc.h"
}

#include <opencv2/opencv.hpp>

/******************************************************************************
 * COMPUTER-VISION
 *****************************************************************************/

typedef std::map<int, std::vector<cv::KeyPoint>> TrackerKeypoints;

///////////
// UTILS //
///////////

/**
 * Convert cv::Mat type to string
 */
std::string type2str(const int type) {
  std::string r;

  uchar depth = type & CV_MAT_DEPTH_MASK;
  uchar chans = 1 + (type >> CV_CN_SHIFT);

  switch (depth) {
    case CV_8U:
      r = "8U";
      break;
    case CV_8S:
      r = "8S";
      break;
    case CV_16U:
      r = "16U";
      break;
    case CV_16S:
      r = "16S";
      break;
    case CV_32S:
      r = "32S";
      break;
    case CV_32F:
      r = "32F";
      break;
    case CV_64F:
      r = "64F";
      break;
    default:
      r = "User";
      break;
  }

  r += "C";
  r += (chans + '0');

  return r;
}

/**
 * Convert std::vector<cv::KeyPoint> to std::vector<cv::Point2f>
 */
std::vector<cv::Point2f> kps2pts(const std::vector<cv::KeyPoint> &kps) {
  std::vector<cv::Point2f> pts;
  for (const auto &kp : kps) {
    pts.push_back(kp.pt);
  }
  return pts;
}

/**
 * Print Keypoint
 */
void print_keypoint(const cv::KeyPoint &kp) {
  printf("angle: %f\n", kp.angle);
  printf("class_id: %d\n", kp.class_id);
  printf("octave: %d\n", kp.octave);
  printf("pt: [%.2f, %.2f]\n", kp.pt.x, kp.pt.y);
  printf("response: %f\n", kp.response);
  printf("size: %f\n", kp.size);
}

/**
 * Sort Keypoints
 */
void sort_keypoints(std::vector<cv::KeyPoint> &kps) {
  std::sort(kps.begin(), kps.end(), [](cv::KeyPoint a, cv::KeyPoint b) {
    return a.response > b.response;
  });
}

/**
 * Given a set of keypoints `kps` make sure they are atleast `min_dist` pixels
 * away from each other, if they are not remove them.
 */
std::vector<cv::KeyPoint> spread_keypoints(
    const cv::Mat &image,
    const std::vector<cv::KeyPoint> &kps,
    const int min_dist = 10,
    const std::vector<cv::KeyPoint> &kps_prev = std::vector<cv::KeyPoint>(),
    const bool debug = false) {
  // Pre-check
  std::vector<cv::KeyPoint> kps_filtered;
  if (kps.size() == 0) {
    return kps_filtered;
  }

  // Setup
  const int img_w = image.size().width;
  const int img_h = image.size().height;
  cv::Mat A = cv::Mat::zeros(img_h, img_w, CV_8UC1);

  // Loop through previous keypoints
  for (const auto kp : kps_prev) {
    // Fill the area of the matrix where the next keypoint cannot be around
    const int px = kp.pt.x;
    const int py = kp.pt.y;
    const int rs = std::max(py - min_dist, 0);
    const int re = std::min(py + min_dist + 1, img_h - 1);
    const int cs = std::max(px - min_dist, 0);
    const int ce = std::min(px + min_dist + 1, img_w - 1);

    for (size_t i = rs; i < re; i++) {
      for (size_t j = cs; j < ce; j++) {
        A.at<uint8_t>(i, j) = 255;
      }
    }
  }

  // Loop through keypoints
  for (const auto kp : kps) {
    // Check if point is ok to be added to results
    const int px = kp.pt.x;
    const int py = kp.pt.y;
    if (A.at<uint8_t>(py, px) > 0) {
      continue;
    }
    kps_filtered.push_back(kp);

    // Fill the area of the matrix where the next keypoint cannot be around
    const int rs = std::max(py - min_dist, 0);
    const int re = std::min(py + min_dist + 1, img_h - 1);
    const int cs = std::max(px - min_dist, 0);
    const int ce = std::min(px + min_dist + 1, img_w - 1);

    for (size_t i = rs; i < re; i++) {
      for (size_t j = cs; j < ce; j++) {
        A.at<uint8_t>(i, j) = 255;
      }
    }
  }

  // Debug
  if (debug) {
    cv::imshow("Debug", A);
    cv::waitKey(0);
  }

  return kps_filtered;
}

/**
 * Returns number of inliers, outliers and total from inlier vector.
 */
void inlier_stats(const std::vector<uchar> inliers,
                  size_t &num_inliers,
                  size_t &num_outliers,
                  size_t &num_total,
                  float &inlier_ratio) {
  num_inliers = 0;
  num_outliers = 0;
  num_total = inliers.size();
  for (size_t i = 0; i < inliers.size(); i++) {
    if (inliers[i]) {
      num_inliers++;
    } else {
      num_outliers++;
    }
  }

  inlier_ratio = (float) num_inliers / (float) num_total;
}

/**
 * Filter Outliers
 */
std::vector<cv::KeyPoint>
filter_outliers(const std::vector<cv::KeyPoint> &keypoints,
                const std::vector<uchar> inliers) {
  std::vector<cv::KeyPoint> kps_inliers;
  for (size_t i = 0; i < keypoints.size(); i++) {
    if (inliers[i]) {
      kps_inliers.push_back(keypoints[i]);
    }
  }
  return kps_inliers;
}

/**
 * Track keypoints `pts_i` from image `img_i` to image `img_j` using optical
 * flow. Returns a tuple of `(pts_i, pts_j, inliers)` points in image i, j and a
 * vector of inliers.
 */
void optflow_track(const cv::Mat &img_i,
                   const cv::Mat &img_j,
                   const std::vector<cv::KeyPoint> &kps_i,
                   std::vector<cv::KeyPoint> &kps_j,
                   std::vector<uchar> &inliers,
                   const int patch_size = 50,
                   const int max_iter = 50,
                   const int max_level = 3,
                   const real_t epsilon = 0.001,
                   const bool debug = false) {
  // Convert std::vector<cv::KeyPoint> to std::vector<cv::Point2f>
  std::vector<cv::Point2f> pts_i = kps2pts(kps_i);
  std::vector<cv::Point2f> pts_j = kps2pts(kps_j);

  // Track features from img i to img j
  const auto win_size = cv::Size(patch_size, patch_size);
  const auto crit_type = cv::TermCriteria::MAX_ITER | cv::TermCriteria::EPS;
  const auto crit = cv::TermCriteria(crit_type, max_iter, epsilon);
  const auto errs = cv::noArray();
  std::vector<uchar> optflow_inliers;
  cv::calcOpticalFlowPyrLK(img_i,
                           img_j,
                           pts_i,
                           pts_j,
                           optflow_inliers,
                           errs,
                           win_size,
                           max_level,
                           crit);

  // Make sure keypoints are within image dimensions
  std::vector<bool> bound_inliers;
  const int img_h = img_i.rows;
  const int img_w = img_i.cols;
  for (const auto &p : pts_j) {
    const auto x_ok = p.x >= 0 && p.x <= img_w;
    const auto y_ok = p.y >= 0 && p.y <= img_h;
    bound_inliers.push_back((x_ok && y_ok));
  }

  // Write out final inliers
  assert(pts_i.size() == optflow_inliers.size());
  assert(pts_i.size() == bound_inliers.size());

  kps_j.clear();
  inliers.clear();
  for (size_t i = 0; i < pts_i.size(); i++) {
    kps_j.emplace_back(pts_j[i],
                       kps_i[i].size,
                       kps_i[i].angle,
                       kps_i[i].response,
                       kps_i[i].octave,
                       kps_i[i].class_id);
    inliers.push_back(optflow_inliers[i] && bound_inliers[i]);
  }

  // Debug
  bool quit = false;
  while (debug && quit == false) {
    // Draw properties
    const int radius = 2;
    const cv::Scalar red{0, 0, 255};

    // Setup
    cv::Mat viz_i;
    cv::Mat viz_j;
    cv::cvtColor(img_i, viz_i, cv::COLOR_GRAY2RGB);
    cv::cvtColor(img_j, viz_j, cv::COLOR_GRAY2RGB);

    // Draw KeyPoints
    for (size_t n = 0; n < kps_i.size(); n++) {
      if (inliers[n] == 0) {
        continue;
      }

      auto pt_i = kps_i[n].pt;
      auto pt_j = kps_j[n].pt;
      cv::circle(viz_i, pt_i, radius, red, -1);
      cv::circle(viz_j, pt_j, radius, red, -1);
    }

    // Stitch images horizontally
    cv::Mat viz;
    cv::hconcat(viz_i, viz_j, viz);

    // Draw lines
    for (size_t n = 0; n < kps_i.size(); n++) {
      auto pt_i = kps_i[n].pt;
      auto pt_j = kps_j[n].pt;
      pt_j.x += viz_i.cols;
      const auto line = cv::LINE_AA;
      cv::line(viz, pt_i, pt_j, red, 1, line);
    }

    // Show
    cv::imshow("Optical-Flow Tracking", viz);
    if (cv::waitKey(0) == 'q') {
      quit = true;
    }
  }
}

/**
 * Perform RANSAC on keypoints `kps0` and `kps1` to reject outliers.
 */
void ransac(const std::vector<cv::KeyPoint> &kps0,
            const std::vector<cv::KeyPoint> &kps1,
            const undistort_func_t cam0_undist,
            const undistort_func_t cam1_undist,
            const real_t cam0_params[8],
            const real_t cam1_params[8],
            std::vector<uchar> &inliers,
            const double reproj_threshold = 0.75,
            const double confidence = 0.99) {
  assert(kps0.size() > 0 && kps1.size() > 0);
  assert(kps0.size() == kps1.size());
  assert(cam0_undist && cam1_undist);
  assert(cam0_params && cam1_params);

  // Undistort image points
  std::vector<cv::Point2f> pts0 = kps2pts(kps0);
  std::vector<cv::Point2f> pts1 = kps2pts(kps1);
  for (size_t i = 0; i < pts0.size(); i++) {
    const real_t z0_in[2] = {pts0[i].x, pts0[i].y};
    const real_t z1_in[2] = {pts1[i].x, pts1[i].y};

    real_t z0[2] = {0};
    real_t z1[2] = {0};
    cam0_undist(cam0_params, z0_in, z0);
    cam1_undist(cam1_params, z1_in, z1);

    pts0[i].x = z0[0];
    pts0[i].y = z0[1];

    pts1[i].x = z1[0];
    pts1[i].y = z1[1];
  }

  // Perform ransac
  const int method = cv::FM_RANSAC;
  cv::findFundamentalMat(pts0,
                         pts1,
                         method,
                         reproj_threshold,
                         confidence,
                         inliers);
}

/**
 * Filter features by triangulating them via a stereo-pair and see if the
 * reprojection error is reasonable.
 */
void reproj_filter(const project_func_t cam_i_proj_func,
                   const int cam_i_res[2],
                   const real_t *cam_i_params,
                   const real_t *cam_j_params,
                   const real_t T_C0Ci[4 * 4],
                   const real_t T_C0Cj[4 * 4],
                   const std::vector<cv::KeyPoint> &kps_i,
                   const std::vector<cv::KeyPoint> &kps_j,
                   std::vector<cv::Point3d> &points,
                   std::vector<bool> &inliers,
                   const real_t reproj_threshold = 0.5) {
  // Form camera i and camera j extrinsics T_CiCj
  TF_INV(T_C0Ci, T_CiC0);
  TF_CHAIN(T_CiCj, 2, T_CiC0, T_C0Cj);

  // Form Projection matrices P_i and P_j
  TF_IDENTITY(T_eye);
  real_t P_i[3 * 4] = {0};
  real_t P_j[3 * 4] = {0};
  pinhole_projection_matrix(cam_i_params, T_eye, P_i);
  pinhole_projection_matrix(cam_j_params, T_CiCj, P_j);

  // Check features
  for (size_t n = 0; n < kps_i.size(); n++) {
    // Triangulate feature
    const real_t z_i[2] = {kps_i[n].pt.x, kps_i[n].pt.y};
    const real_t z_j[2] = {kps_j[n].pt.x, kps_j[n].pt.y};

    real_t uz_i[2] = {0};
    real_t uz_j[2] = {0};
    pinhole_radtan4_undistort(cam_i_params, z_i, uz_i);
    pinhole_radtan4_undistort(cam_j_params, z_j, uz_j);

    real_t p_Ci[3] = {0};
    linear_triangulation(P_i, P_j, uz_i, uz_j, p_Ci);
    points.emplace_back(p_Ci[0], p_Ci[1], p_Ci[2]);

    // Check feature depth
    if (p_Ci[2] < 0.0) {
      inliers.push_back(false);
      continue;
    }

    // Reproject feature into camera i
    real_t z_i_hat[2] = {0};
    cam_i_proj_func(cam_i_params, p_Ci, z_i_hat);

    // Check image point bounds
    const bool x_ok = (z_i_hat[0] < cam_i_res[0] && z_i_hat[0] > 0);
    const bool y_ok = (z_i_hat[1] < cam_i_res[1] && z_i_hat[1] > 0);
    if (!x_ok || !y_ok) {
      inliers.push_back(false);
      continue;
    }

    // Check reprojection error
    const real_t r[2] = {z_i[0] - z_i_hat[0], z_i[1] - z_i_hat[1]};
    const real_t reproj_error = sqrt(r[0] * r[0] + r[1] * r[1]);
    if (reproj_error > reproj_threshold) {
      inliers.push_back(false);
      continue;
    }

    // Passed all tests
    inliers.push_back(true);
  }
}

//////////////////
// FEATURE GRID //
//////////////////

/**
 * Feature Grid
 *
 * The idea is to take all the feature positions and put them into grid cells
 * across the full image space. This is so that one could keep track of how
 * many feautures are being tracked in each individual grid cell and act
 * accordingly.
 *
 * o-----> x
 * | ---------------------
 * | |  0 |  1 |  2 |  3 |
 * V ---------------------
 * y |  4 |  5 |  6 |  7 |
 *   ---------------------
 *   |  8 |  9 | 10 | 11 |
 *   ---------------------
 *   | 12 | 13 | 14 | 15 |
 *   ---------------------
 *
 *   grid_x = ceil((max(1, pixel_x) / img_w) * grid_cols) - 1.0
 *   grid_y = ceil((max(1, pixel_y) / img_h) * grid_rows) - 1.0
 *   cell_id = int(grid_x + (grid_y * grid_cols))
 *
 */
struct FeatureGrid {
  int image_width;
  int image_height;
  int grid_rows;
  int grid_cols;
  std::vector<int> cells;
  std::vector<std::pair<int, int>> keypoints;

  FeatureGrid(const int image_width,
              const int image_height,
              const int grid_rows = 3,
              const int grid_cols = 4)
      : image_width{image_width},
        image_height{image_height}, grid_rows{grid_rows}, grid_cols{grid_cols} {
    for (int i = 0; i < (grid_rows * grid_cols); i++) {
      cells.push_back(0);
    }
  }

  virtual ~FeatureGrid() = default;

  /** Add keypoint */
  void add(const int pixel_x, const int pixel_y) {
    assert(pixel_x >= 0 && pixel_x <= image_width);
    assert(pixel_y >= 0 && pixel_y <= image_height);
    const int cell_idx = cellIndex(pixel_x, pixel_y);
    cells[cell_idx] += 1;
    keypoints.emplace_back(pixel_x, pixel_y);
  }

  /** Return cell index based on point `pt` */
  int cellIndex(const int pixel_x, const int pixel_y) const {
    const float img_w = image_width;
    const float img_h = image_height;
    float grid_x = ceil((std::max(1, pixel_x) / img_w) * grid_cols) - 1.0;
    float grid_y = ceil((std::max(1, pixel_y) / img_h) * grid_rows) - 1.0;
    const int cell_id = int(grid_x + (grid_y * grid_cols));
    return cell_id;
  }

  /** Return cell count */
  int count(const int cell_idx) const {
    return cells[cell_idx];
  }

  /** Debug */
  void debug(const bool imshow = true) const {
    const int w = image_width;
    const int h = image_height;
    cv::Mat viz = cv::Mat::zeros(h, w, CV_32F);

    for (const auto kp : keypoints) {
      const auto x = kp.first;
      const auto y = kp.second;

      const cv::Point p(x, y);
      const int radius = 2;
      const cv::Scalar color{255, 255, 255};
      cv::circle(viz, p, radius, color, -1);
    }

    if (imshow) {
      cv::imshow("Figure 1", viz);
      cv::waitKey(0);
    }
  }
};

///////////////////
// GRID DETECTOR //
///////////////////

/**
 * Grid detector
 */
struct GridDetector {
  // cv::Ptr<cv::Feature2D> detector = cv::ORB::create();
  cv::Ptr<cv::Feature2D> detector = cv::FastFeatureDetector::create();
  bool optflow_mode = true;
  int max_keypoints = 200;
  int grid_rows = 3;
  int grid_cols = 4;

  GridDetector() = default;
  virtual ~GridDetector() = default;

  /** Detect new keypoints **/
  void detect(
      const cv::Mat &image,
      std::vector<cv::KeyPoint> &kps_new,
      cv::Mat &des_new,
      const std::vector<cv::KeyPoint> &kps_prev = std::vector<cv::KeyPoint>(),
      bool debug = false) const {
    // Asserts
    assert(image.channels() == 1);

    // Calculate number of grid cells and max corners per cell
    const int img_w = image.size().width;
    const int img_h = image.size().height;
    const int dx = int(std::ceil(float(img_w) / float(grid_cols)));
    const int dy = int(std::ceil(float(img_h) / float(grid_rows)));
    const int num_cells = grid_rows * grid_cols;
    const int max_per_cell = floor(max_keypoints / num_cells);

    // Create feature grid of previous keypoints
    FeatureGrid grid{img_w, img_h};
    for (const auto &kp : kps_prev) {
      const int pixel_x = kp.pt.x;
      const int pixel_y = kp.pt.y;
      grid.add(pixel_x, pixel_y);
    }

    // Detect corners in each grid cell
    int cell_idx = 0;

    for (int y = 0; y < img_h; y += dy) {
      for (int x = 0; x < img_w; x += dx) {
        // Make sure roi width and height are not out of bounds
        const int w = (x + dx > img_w) ? img_w - x : dx;
        const int h = (y + dy > img_h) ? img_h - y : dy;

        // Detect corners in grid cell
        cv::Rect roi(x, y, w, h);
        std::vector<cv::KeyPoint> kps_roi;
        detector->detect(image(roi), kps_roi);
        sort_keypoints(kps_roi);
        kps_roi = spread_keypoints(image(roi), kps_roi, 10, kps_prev);

        // Extract feature descriptors
        cv::Mat des_roi;
        if (optflow_mode == false) {
          detector->compute(image(roi), kps_roi, des_roi);
        }

        // Offset keypoints
        const int vacancy = max_per_cell - grid.count(cell_idx);
        if (vacancy <= 0) {
          continue;
        }

        for (int i = 0; i < std::min((int) kps_roi.size(), vacancy); i++) {
          cv::KeyPoint kp = kps_roi[i];
          kp.pt.x += x;
          kp.pt.y += y;

          kps_new.push_back(kp);
          if (optflow_mode == false) {
            if (des_new.empty()) {
              des_new = des_roi.row(i);
            } else {
              cv::vconcat(des_roi.row(i), des_new, des_new);
            }
          }

          grid.add(kp.pt.x, kp.pt.y);
        }

        // Update cell_idx
        cell_idx += 1;
      }
    }

    // Debug
    if (debug) {
      visualize(image, grid, kps_new, kps_prev);
    }
  }

  void detect(
      const cv::Mat &image,
      std::vector<cv::KeyPoint> &kps_new,
      const std::vector<cv::KeyPoint> &kps_prev = std::vector<cv::KeyPoint>(),
      bool debug = false) const {
    assert(optflow_mode == true);
    cv::Mat des_new;
    detect(image, kps_new, des_new, kps_prev, debug);
  }

  /** Visualize **/
  void visualize(const cv::Mat &image,
                 const FeatureGrid &grid,
                 const std::vector<cv::KeyPoint> &kps_new,
                 const std::vector<cv::KeyPoint> &kps_prev) const {
    // Visualization properties
    const auto red = cv::Scalar{0, 0, 255};
    const auto yellow = cv::Scalar{0, 255, 255};
    const auto line = cv::LINE_AA;
    const auto font = cv::FONT_HERSHEY_SIMPLEX;

    // Setup
    const int img_w = image.size().width;
    const int img_h = image.size().height;
    const int dx = int(std::ceil(float(img_w) / float(grid_cols)));
    const int dy = int(std::ceil(float(img_h) / float(grid_rows)));
    cv::Mat viz;
    cv::cvtColor(image, viz, cv::COLOR_GRAY2RGB);

    // Draw horizontal lines
    for (int x = 0; x < img_w; x += dx) {
      cv::line(viz, cv::Point2f(x, 0), cv::Point2f(x, img_w), red, 1, line);
    }

    // Draw vertical lines
    for (int y = 0; y < img_h; y += dy) {
      cv::line(viz, cv::Point2f(0, y), cv::Point2f(img_w, y), red, 1, line);
    }

    // Draw bin numbers
    int cell_idx = 0;
    for (int y = 0; y < img_h; y += dy) {
      for (int x = 0; x < img_w; x += dx) {
        auto text = std::to_string(grid.count(cell_idx));
        auto origin = cv::Point2f{x + 10.0f, y + 20.0f};
        cv::putText(viz, text, origin, font, 0.5, red, 1, line);
        cell_idx += 1;
      }
    }

    // Draw keypoints
    cv::drawKeypoints(viz, kps_new, viz, red);
    cv::drawKeypoints(viz, kps_prev, viz, yellow);

    // Imshow
    cv::imshow("viz", viz);
    cv::waitKey(0);
  }
};

//////////////////
// FEATURE-INFO //
//////////////////

struct FeatureInfo {
  size_t feature_id = 0;
  std::vector<timestamp_t> timestamps;
  std::map<int, std::vector<cv::KeyPoint>> keypoints;

  FeatureInfo() = default;
  FeatureInfo(const size_t feature_id_,
              const timestamp_t ts_,
              const std::map<int, cv::KeyPoint> &keypoints_)
      : feature_id{feature_id_} {
    timestamps.push_back(ts_);
    for (const auto &[cam_idx, kp] : keypoints_) {
      keypoints[cam_idx].push_back(kp);
    }
  }
  virtual ~FeatureInfo() = default;

  /** Return Feature ID **/
  size_t featureId() const {
    return feature_id;
  }

  /** Return Timestamp **/
  timestamp_t timestamp(const size_t idx) const {
    return timestamps.at(idx);
  }

  /** Return First Timestamp **/
  timestamp_t first_timestamp() const {
    return timestamps.front();
  }

  /** Return Last Timestamp **/
  timestamp_t last_timestamp() const {
    return timestamps.back();
  }

  /** Return Camera Keypoints **/
  std::vector<cv::KeyPoint> get_keypoints(const int cam_idx) const {
    return keypoints.at(cam_idx);
  }

  /** Return Last Camera Keypoints **/
  cv::KeyPoint last_keypoint(const int cam_idx) const {
    return keypoints.at(cam_idx).back();
  }

  /** Update feature with new measurement **/
  void update(const timestamp_t ts, const int cam_idx, const cv::KeyPoint &kp) {
    timestamps.push_back(ts);
    keypoints[cam_idx].push_back(kp);
  }
};

/******************************************************************************
 * STATE-ESTIMATION
 *****************************************************************************/

// /** Estimate Relative Pose **/
// real_t estimate_relpose(const Mapper &mapper,
//                         CameraIntrinsics &cam_ints,
//                         CameraExtrinsics &cam_exts,
//                         TrackerData tracking_km1,
//                         TrackerData tracking_k,
//                         Features &features,
//                         Pose &pose_km1,
//                         Pose &pose_k,
//                         const bool fix_features = true,
//                         const bool fix_camera_intrinsics = true,
//                         const bool fix_camera_extrinsics = true) {
//   // Settings
//   bool verbose = true;
//   int max_iter = 10;
//   int max_num_threads = 2;

//   // Setup
//   ceres::Problem::Options prob_options;
//   ceres::Problem problem{prob_options};
//   PoseLocalParameterization *pose_param = new PoseLocalParameterization();
//   ceres::LossFunction *loss_fn = new ceres::CauchyLoss(1.0);

//   // Update feature positions from mapper
//   for (auto &[fid, feature] : features) {
//     if (mapper.features.count(fid)) {
//       feature = mapper.features.at(fid);
//     }
//   }

//   // Add camera intrinsics and extrinsics to problem
//   for (auto &[cam_id, cam_int] : cam_ints) {
//     problem.AddParameterBlock(cam_int.data, 8);
//     if (fix_camera_intrinsics) {
//       problem.SetParameterBlockConstant(cam_int.data);
//     }
//   }
//   for (auto &[cam_id, cam_ext] : cam_exts) {
//     problem.AddParameterBlock(cam_ext.data, 7);
//     problem.SetParameterization(cam_ext.data, pose_param);
//     if (fix_camera_extrinsics) {
//       problem.SetParameterBlockConstant(cam_ext.data);
//     }
//   }

//   // Add poses at km1 and k, fix pose at km1
//   problem.AddParameterBlock(pose_km1.data, 7);
//   problem.AddParameterBlock(pose_k.data, 7);
//   problem.SetParameterization(pose_km1.data, pose_param);
//   problem.SetParameterization(pose_k.data, pose_param);
//   problem.SetParameterBlockConstant(pose_km1.data);

//   // Add vision factors
//   std::set<size_t> feature_ids;
//   std::map<size_t, CameraFactor *> factors;
//   auto add_factors =
//       [&](const int ts_idx, TrackerData &tracking_data, Pose &pose) {
//         for (auto &[cam_id, cam_data] : tracking_data) {
//           for (const auto &[fid, kp] : cam_data) {
//             // Add feature to problem
//             if (feature_ids.count(fid) == 0) {
//               problem.AddParameterBlock(features.at(fid).data, 3);
//               if (fix_features) {
//                 problem.SetParameterBlockConstant(features.at(fid).data);
//               }
//               feature_ids.insert(fid);
//             }

//             // Form factor
//             auto &feature = features.at(fid);
//             const real_t z[2] = {kp.pt.x, kp.pt.y};
//             auto res_fn = new CameraFactor{ts_idx, cam_id, fid, z};
//             factors[fid] = res_fn;

//             // Add factor to problem
//             std::vector<double *> param_blocks;
//             param_blocks.push_back(pose.data);
//             param_blocks.push_back(cam_exts.at(cam_id).data);
//             param_blocks.push_back(feature.data);
//             param_blocks.push_back(cam_ints.at(cam_id).data);
//             problem.AddResidualBlock(res_fn, loss_fn, param_blocks);
//           }
//         }
//       };
//   add_factors(0, tracking_km1, pose_km1);
//   add_factors(1, tracking_k, pose_k);

//   // Solve
//   ceres::Solver::Options options;
//   options.minimizer_progress_to_stdout = verbose;
//   options.max_num_iterations = max_iter;
//   options.num_threads = max_num_threads;
//   ceres::Solver::Summary summary;
//   ceres::Solve(options, &problem, &summary);
//   if (verbose) {
//     std::cout << summary.FullReport() << std::endl << std::endl;
//   }

//   // Evaluate reprojection errors
//   std::vector<double> reproj_errors;
//   std::map<size_t, double> feature_errors;
//   auto eval_residuals = [&]() {
//     for (auto &[fid, factor] : factors) {
//       double r[2] = {0.0, 0.0};

//       std::vector<double *> params;
//       if (factor->ts == 0) {
//         params.push_back(pose_km1.data);
//       } else {
//         params.push_back(pose_k.data);
//       }
//       params.push_back(cam_exts.at(factor->cam_id).data);
//       params.push_back(features[factor->feature_id].data);
//       params.push_back(cam_ints.at(factor->cam_id).data);
//       factor->Evaluate(params.data(), r, nullptr);

//       const auto reproj_error = sqrt(r[0] * r[0] + r[1] * r[1]);
//       reproj_errors.push_back(reproj_error);
//       feature_errors[factor->feature_id] = reproj_error;
//     }
//   };
//   eval_residuals();

//   // printf("mean reproj error: %f\n", mean(reproj_errors));
//   // printf("median reproj error: %f\n", median(reproj_errors));
//   // printf("rmse reproj error: %f\n", rmse(reproj_errors));
//   // printf("var reproj error: %f\n", var(reproj_errors));
//   return rmse(reproj_errors);
// }

#endif // AVS_HPP

//////////////////////////////////////////////////////////////////////////////
//                              UNIT-TESTS                                  //
//////////////////////////////////////////////////////////////////////////////

#ifdef AVS_UNITTESTS

// UNITESTS GLOBAL VARIABLES
static int nb_tests = 0;
static int nb_passed = 0;
static int nb_failed = 0;

#define ENABLE_TERM_COLORS 0
#if ENABLE_TERM_COLORS == 1
#define TERM_RED "\x1B[1;31m"
#define TERM_GRN "\x1B[1;32m"
#define TERM_WHT "\x1B[1;37m"
#define TERM_NRM "\x1B[1;0m"
#else
#define TERM_RED
#define TERM_GRN
#define TERM_WHT
#define TERM_NRM
#endif

/**
 * Run unittests
 * @param[in] test_name Test name
 * @param[in] test_ptr Pointer to unittest
 */
void run_test(const char *test_name, int (*test_ptr)()) {
  if ((*test_ptr)() == 0) {
    printf("-> [%s] " TERM_GRN "OK!\n" TERM_NRM, test_name);
    fflush(stdout);
    nb_passed++;
  } else {
    printf(TERM_RED "FAILED!\n" TERM_NRM);
    fflush(stdout);
    nb_failed++;
  }
  nb_tests++;
}

/**
 * Add unittest
 * @param[in] TEST Test function
 */
#define TEST(TEST_FN) run_test(#TEST_FN, TEST_FN);

/**
 * Unit-test assert
 * @param[in] TEST Test condition
 */
#define TEST_ASSERT(TEST)                                                      \
  do {                                                                         \
    if ((TEST) == 0) {                                                         \
      printf(TERM_RED "ERROR!" TERM_NRM " [%s:%d] %s FAILED!\n",               \
             __func__,                                                         \
             __LINE__,                                                         \
             #TEST);                                                           \
      return -1;                                                               \
    }                                                                          \
  } while (0)

int test_feature_grid() {
  int image_width = 640;
  int image_height = 480;
  int grid_rows = 3;
  int grid_cols = 4;
  FeatureGrid grid{image_width, image_height, grid_rows, grid_cols};

  const float dx = (image_width / grid_cols);
  const float dy = (image_height / grid_rows);
  for (int i = 0; i < grid_rows; i++) {
    for (int j = 0; j < grid_cols; j++) {
      const int pixel_x = dx * j + dx / 2;
      const int pixel_y = dy * i + dy / 2;
      grid.add(pixel_x, pixel_y);
    }
  }
  grid.debug(true);

  return 0;
}

int test_spread_keypoints() {
  // Load image
  const auto img_path = "./test_data/frontend/cam0/1403715297312143104.png";
  const auto img = cv::imread(img_path, cv::IMREAD_GRAYSCALE);

  // Detect keypoints
  const cv::Ptr<cv::Feature2D> detector = cv::ORB::create();
  std::vector<cv::KeyPoint> kps;
  detector->detect(img, kps);

  // Spread keypoints
  const int min_dist = 5;
  const std::vector<cv::KeyPoint> kps_prev;
  const bool debug = false;
  const auto kps_new = spread_keypoints(img, kps, min_dist, kps_prev, debug);

  // Assert keypoints are a Euclidean distance away from each other
  for (const auto kp : kps_new) {
    for (const auto kp_ref : kps_new) {
      const int px = kp.pt.x;
      const int py = kp.pt.y;
      const int px_ref = kp_ref.pt.x;
      const int py_ref = kp_ref.pt.y;

      const int dx = std::abs(px_ref - px);
      const int dy = std::abs(py_ref - py);
      if (dx == 0 && dy == 0) {
        continue;
      }

      const int dist = sqrt(dx * dx + dy * dy);
      TEST_ASSERT(dist >= min_dist);
    }
  }

  return 0;
}

int test_grid_detect() {
  const auto img_path = "./test_data/frontend/cam0/1403715297312143104.jpg";
  const auto img = cv::imread(img_path, cv::IMREAD_GRAYSCALE);

  std::vector<cv::KeyPoint> kps_prev;
  std::vector<cv::KeyPoint> kps_new;
  cv::Mat des_new;
  GridDetector detector;
  detector.detect(img, kps_new, des_new, kps_prev, true);

  return 0;
}

int test_optflow_track() {
  const auto img_i_path = "./test_data/frontend/cam0/1403715297312143104.jpg";
  const auto img_j_path = "./test_data/frontend/cam1/1403715297312143104.jpg";
  const auto img_i = cv::imread(img_i_path, cv::IMREAD_GRAYSCALE);
  const auto img_j = cv::imread(img_j_path, cv::IMREAD_GRAYSCALE);

  // Detect keypoints
  std::vector<cv::KeyPoint> kps_new;
  GridDetector det;
  det.detect(img_i, kps_new);

  // Optical-flow track
  std::vector<cv::KeyPoint> kps_i = kps_new;
  std::vector<cv::KeyPoint> kps_j = kps_new;
  std::vector<uchar> inliers;
  optflow_track(img_i, img_j, kps_i, kps_j, inliers);

  return 0;
}

int test_reproj_filter() {
  const auto img_i_path = "./test_data/frontend/cam0/1403715297312143104.jpg";
  const auto img_j_path = "./test_data/frontend/cam1/1403715297312143104.jpg";
  auto img_i = cv::imread(img_i_path, cv::IMREAD_GRAYSCALE);
  auto img_j = cv::imread(img_j_path, cv::IMREAD_GRAYSCALE);

  // Detect keypoints
  std::vector<cv::KeyPoint> kps_new;
  GridDetector det;
  det.detect(img_i, kps_new);

  // Optical-flow track
  std::vector<cv::KeyPoint> kps_i = kps_new;
  std::vector<cv::KeyPoint> kps_j = kps_new;
  std::vector<uchar> optflow_inliers;
  optflow_track(img_i, img_j, kps_i, kps_j, optflow_inliers);

  // clang-format off
  const int cam_res[2] = {752, 480};
  real_t cam0_int[8] = {
    458.654, 457.296, 367.215, 248.375, -0.28340811,
    0.07395907, 0.00019359, 1.76187114e-05
  };
  real_t cam1_int[8] = {
    457.587, 456.134, 379.999, 255.238,
    -0.28368365, 0.07451284, -0.00010473, -3.55590700e-05
  };
  const real_t T_SC0[4 * 4] = {
    0.0148655429818, -0.999880929698, 0.00414029679422, -0.0216401454975,
    0.999557249008, 0.0149672133247, 0.025715529948, -0.064676986768,
    -0.0257744366974, 0.00375618835797, 0.999660727178, 0.00981073058949,
    0.0, 0.0, 0.0, 1.0
  };
  const real_t T_SC1[4 * 4] = {
    0.0125552670891, -0.999755099723, 0.0182237714554, -0.0198435579556,
    0.999598781151, 0.0130119051815, 0.0251588363115, 0.0453689425024,
    -0.0253898008918, 0.0179005838253, 0.999517347078, 0.00786212447038,
    0.0, 0.0, 0.0, 1.0
  };
  TF_INV(T_SC0, T_C0S);
  TF_IDENTITY(T_C0C0);
  TF_CHAIN(T_C0C1, 2, T_C0S, T_SC1);
  // clang-format on

  TIC(reproj_filter_time);
  std::vector<cv::Point3d> points;
  std::vector<bool> reproj_inliers;
  reproj_filter(pinhole_radtan4_project,
                cam_res,
                cam0_int,
                cam1_int,
                T_C0C0,
                T_C0C1,
                kps_i,
                kps_j,
                points,
                reproj_inliers);
  PRINT_TOC("Reproj Filter", reproj_filter_time);

  // Filter keypoints
  TrackerKeypoints keypoints;
  keypoints[0] = std::vector<cv::KeyPoint>();
  keypoints[1] = std::vector<cv::KeyPoint>();
  for (size_t n = 0; n < kps_i.size(); n++) {
    if (optflow_inliers[n] && reproj_inliers[n]) {
      keypoints[0].push_back(kps_i[n]);
      keypoints[1].push_back(kps_j[n]);
    }
  }

  // Visualize
  const auto red = cv::Scalar{0, 0, 255};
  cv::Mat img0_viz;
  cv::Mat img1_viz;
  cv::Mat viz;
  cv::cvtColor(img_i, img0_viz, cv::COLOR_GRAY2RGB);
  cv::cvtColor(img_j, img1_viz, cv::COLOR_GRAY2RGB);
  cv::drawKeypoints(img0_viz, keypoints[0], img0_viz, red);
  cv::drawKeypoints(img1_viz, keypoints[1], img1_viz, red);
  cv::hconcat(img0_viz, img1_viz, viz);
  cv::imshow("viz", viz);
  cv::waitKey(0);

  return 0;
}

class EuRoCParams {
public:
  camera_params_t cam0_params;
  camera_params_t cam1_params;
  extrinsic_t cam0_ext;
  extrinsic_t cam1_ext;

  EuRoCParams() {
    // clang-format off
    // camera_params_t cam0_params;
    // camera_params_t cam1_params;
    // extrinsic_t cam0_ext;
    // extrinsic_t cam1_ext;

    const int cam_res[2] = {752, 480};
    const char *proj_model = "pinhole";
    const char *dist_model = "radtan4";
    real_t cam0_data[8] = {
      458.654, 457.296, 367.215, 248.375,
      -0.28340811, 0.07395907, 0.00019359, 1.76187114e-05
    };
    real_t cam1_data[8] = {
      457.587, 456.134, 379.999, 255.238,
      -0.28368365, 0.07451284, -0.00010473, -3.55590700e-05
    };
    real_t cam0_ext_data[7] = {0.0, 0.0, 0.0, 1.0, 0.0, 0.0, 0.0};
    real_t cam1_ext_data[7] = {0.0, 0.0, 0.0, 1.0, 0.0, 0.0, 0.0};
    const real_t T_SC0[4 * 4] = {
      0.0148655429818, -0.999880929698, 0.00414029679422, -0.0216401454975,
      0.999557249008, 0.0149672133247, 0.025715529948, -0.064676986768,
      -0.0257744366974, 0.00375618835797, 0.999660727178, 0.00981073058949,
      0.0, 0.0, 0.0, 1.0
    };
    const real_t T_SC1[4 * 4] = {
      0.0125552670891, -0.999755099723, 0.0182237714554, -0.0198435579556,
      0.999598781151, 0.0130119051815, 0.0251588363115, 0.0453689425024,
      -0.0253898008918, 0.0179005838253, 0.999517347078, 0.00786212447038,
      0.0, 0.0, 0.0, 1.0
    };
    TF_INV(T_SC0, T_C0S);
    TF_CHAIN(T_C0C1, 2, T_C0S, T_SC1);
    tf_vector(T_C0C1, cam1_ext_data);

    camera_params_setup(&cam0_params, 0, cam_res, proj_model, dist_model, cam0_data);
    camera_params_setup(&cam1_params, 1, cam_res, proj_model, dist_model, cam1_data);
    extrinsic_setup(&cam0_ext, cam0_ext_data);
    extrinsic_setup(&cam1_ext, cam1_ext_data);
    // clang-format on
  }
};

// int test_front_end() {
//   // const auto img0_path = "./test_data/frontend/cam0/1403715297312143104.jpg";
//   // const auto img1_path = "./test_data/frontend/cam1/1403715297312143104.jpg";
//   // const auto img0 = cv::imread(img0_path, cv::IMREAD_GRAYSCALE);
//   // const auto img1 = cv::imread(img1_path, cv::IMREAD_GRAYSCALE);

//   // Feature Tracker
//   EuRoCParams euroc;
//   FeatureTracker ft;
//   ft.add_camera(euroc.cam0_params, euroc.cam0_ext);
//   ft.add_camera(euroc.cam1_params, euroc.cam1_ext);
//   ft.add_overlap({0, 1});

//   // Setup
//   const char *data_path = "/data/euroc/V1_01";
//   euroc_data_t *data = euroc_data_load(data_path);
//   euroc_timeline_t *timeline = data->timeline;

//   int imshow_wait = 1;
//   for (size_t k = 0; k < timeline->num_timestamps; k++) {
//     const timestamp_t ts = timeline->timestamps[k];
//     const euroc_event_t *event = &timeline->events[k];

//     if (event->has_cam0 && event->has_cam1) {
//       const cv::Mat img0 = cv::imread(event->cam0_image, cv::IMREAD_GRAYSCALE);
//       const cv::Mat img1 = cv::imread(event->cam1_image, cv::IMREAD_GRAYSCALE);
//       assert(img0.empty() == false);
//       assert(img1.empty() == false);
//       struct timespec t_start = tic();
//       // tracker.track({{0, img0}, {1, img1}}, true);
//       ft.track(ts, {{0, img0}, {1, img1}}, true);
//       printf("track elasped: %f [s]\n", toc(&t_start));

//       char key = cv::waitKey(imshow_wait);
//       if (key == 'q') {
//         k = timeline->num_timestamps;
//       } else if (key == 's') {
//         imshow_wait = 0;
//       } else if (key == ' ' && imshow_wait == 1) {
//         imshow_wait = 0;
//       } else if (key == ' ' && imshow_wait == 0) {
//         imshow_wait = 1;
//       }
//     }
//   }

//   // Clean up
//   euroc_data_free(data);

//   return 0;
// }

void run_unittests() {
  // TEST(test_feature_grid);
  // TEST(test_spread_keypoints);
  // TEST(test_grid_detect);
  // TEST(test_optflow_track);
  // TEST(test_reproj_filter);
  // TEST(test_front_end);
}

#endif // AVS_UNITTESTS
