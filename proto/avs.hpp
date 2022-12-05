#ifndef AVS_HPP
#define AVS_HPP

#include <vector>
#include <map>

extern "C" {
#include "proto.h"
#include "euroc.h"
} // extern C

#include <opencv2/opencv.hpp>

///////////
// UTILS //
///////////

/**
 * Convert cv::Mat type to string
 */
std::string cvtype2str(const int type);

//////////////////
// FEATURE GRID //
//////////////////

/**
 * Feature Grid
 *
 * The idea is to take all the feature positions and put them into grid cells
 * across the full image space. This is so that one could keep track of how many
 * feautures are being tracked in each individual grid cell and act accordingly.
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
 */
class FeatureGrid {
public:
  int image_width_;
  int image_height_;
  int grid_rows_;
  int grid_cols_;
  std::vector<int> cells_;
  std::vector<std::pair<int, int>> keypoints_;

  FeatureGrid(const int image_width,
              const int image_height,
              const int grid_rows = 3,
              const int grid_cols = 4);

  /** Add keypoint */
  void add(const int pixel_x, const int pixel_y);

  /** Return cell index based on point `pt` */
  int cellIndex(const int pixel_x, const int pixel_y) const;

  /** Return cell count */
  int count(const int cell_idx) const;

  /** Debug */
  void debug(const bool imshow = true) const;
};

///////////////////
// GRID DETECTOR //
///////////////////

/**
 * Sort Keypoints
 */
void sort_keypoints(std::vector<cv::KeyPoint> &kps);

/**
 * Given a set of keypoints `kps` make sure they are atleast `min_dist` pixels
 * away from each other, if they are not remove them.
 */
std::vector<cv::KeyPoint> spread_keypoints(
    const cv::Mat &image,
    const std::vector<cv::KeyPoint> &kps,
    const int min_dist = 10,
    const std::vector<cv::KeyPoint> &kps_prev = std::vector<cv::KeyPoint>(),
    const bool debug = false);

/**
 * Grid detector
 */
class GridDetector {
public:
  cv::Ptr<cv::Feature2D> detector_ = cv::ORB::create();
  int max_keypoints_ = 200;
  int grid_rows_ = 3;
  int grid_cols_ = 4;

  /** Detect new keypoints **/
  void detect(const cv::Mat &image,
              const std::vector<cv::KeyPoint> &prev_kps,
              std::vector<cv::KeyPoint> &kps,
              cv::Mat &des,
              bool debug = false);
};

///////////////
// FRONT-END //
///////////////

class FrontEnd {
public:
  GridDetector detector_;
  cv::Ptr<cv::DescriptorMatcher> matcher_ =
      cv::DescriptorMatcher::create("BruteForce-Hamming");

  std::vector<cv::KeyPoint> kps0_;
  std::vector<cv::KeyPoint> kps1_;
  cv::Mat des0_;
  cv::Mat des1_;

  FrontEnd() = default;
  ~FrontEnd() = default;

  void detect(const cv::Mat &img0,
              const cv::Mat &img1,
              const bool debug = false);
};

#endif // AVS_HPP
