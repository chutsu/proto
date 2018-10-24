/**
 * @file
 * @ingroup feature2d
 */
#ifndef PROTOTYPE_VISION_FEATURE2D_KLT_TRACKER_HPP
#define PROTOTYPE_VISION_FEATURE2D_KLT_TRACKER_HPP

#include "prototype/calib/camera_property.hpp"
#include "prototype/feature2d/feature_tracker.hpp"
#include "prototype/feature2d/feature_container.hpp"
#include "prototype/feature2d/grid_fast.hpp"

namespace prototype {
/**
 * @addtogroup feature2d
 * @{
 */

/**
 * KLT based feature tracker
 */
class KLTTracker {
public:
  // Camera model
  CameraProperty camera_property;

  FrameID counter_frame_id = -1;
  FeatureContainer features;
  cv::Mat img_ref;
  Features fea_ref;

  std::vector<cv::Point2f> p_ref;
  std::vector<cv::Point2f> p_cur;
  std::vector<uchar> inlier_mask;

  bool use_grid_fast = false;
  int max_corners = 1000;
  double quality_level = 0.001;
  double min_distance = 5.0;
  bool show_matches = false;

  int image_width = 0;
  int image_height = 0;

  // Stats
  struct FeatureContainerStats stats;

  KLTTracker();

  KLTTracker(const CameraProperty &camera_property);

  KLTTracker(const CameraProperty &camera_property,
             const size_t min_track_length,
             const size_t max_track_length);

  KLTTracker(const CameraProperty &camera_property,
             const size_t min_track_length,
             const size_t max_track_length,
             const int max_corners,
             const double quality_level,
             const double min_distance);

  /**
   * Get lost feature tracks
   */
  std::vector<FeatureTrack> getLostTracks();

  /**
   * Initialize feature tracker
   *
   * @param img_cur Current image frame
   * @returns 0 for success, -1 for failure
   */
  int initialize(const cv::Mat &img_cur);

  /**
   * Detect features
   *
   * @param image Input image
   * @param features List of features
   * @returns 0 for success, -1 for failure
   */
  int detect(const cv::Mat &image, Features &features);

  /**
   * Process feature track
   *
   * @param status Status if tracked or not
   * @param fref Reference feature
   * @param fcur Current feature
   * @returns 0 for track removed, 1 for track added or updated
   */
  int processTrack(const uchar status, Feature &fref, Feature &fcur);

  /**
   * Track features
   *
   * The idea is that with the current features, we want to match it against
   * the current list of FeatureTrack.
   *
   * @param img_ref Reference image
   * @param img_cur Current image
   * @param fea_ref Reference features
   * @param tracked Tracked features
   * @returns 0 for success, -1 for failure
   */
  int track(const cv::Mat &img_ref,
            const cv::Mat &img_cur,
            const Features &fea_ref,
            Features &tracked);

  /**
   * Replenish features
   *
   * @param image Image
   * @param features Features
   * @returns 0 for success, -1 for failure
   */
  int replenishFeatures(const cv::Mat &image, Features &features);

  /**
   * Update feature tracker
   *
   * @param img_cur Current image frame
   * @returns 0 for success, -1 for failure
   */
  int update(const cv::Mat &img_cur);

  /**
   * Update feature tracker
   *
   * @param img_cur Current image frame
   * @param ts Current timestamp
   * @returns 0 for success, -1 for failure
   */
  int update2(const cv::Mat &img_cur, const long ts);
};

/** @} group feature2d */
} //  namespace prototype
#endif // PROTOTYPE_VISION_FEATURE2D_KLT_TRACKER_HPP
