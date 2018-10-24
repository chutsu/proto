#include "prototype/vision/feature2d/feature_tracker.hpp"

namespace prototype {

FeatureTracker::FeatureTracker() {}

FeatureTracker::FeatureTracker(const CameraProperty &camera_property)
    : camera_property{camera_property} {}

FeatureTracker::FeatureTracker(const CameraProperty &camera_property,
                               const size_t min_track_length,
                               const size_t max_track_length)
    : camera_property{camera_property},
      features{min_track_length, max_track_length} {}

FeatureTracker::~FeatureTracker() {}

void FeatureTracker::getKeyPointsAndDescriptors(
    const Features &features,
    std::vector<cv::KeyPoint> &keypoints,
    cv::Mat &descriptors) {
  // Pre-check
  if (features.size() == 0) {
    return;
  }

  // Fill in keypoints and descriptors
  descriptors = cv::Mat(features.size(), features[0].desc.cols, CV_8UC1);
  for (size_t i = 0; i < features.size(); i++) {
    keypoints.push_back(features[i].kp);
    features[i].desc.copyTo(descriptors.row(i));
  }
}

void FeatureTracker::getFeatures(const std::vector<cv::KeyPoint> &keypoints,
                                 const cv::Mat &descriptors,
                                 Features &features) {
  for (size_t i = 0; i < keypoints.size(); i++) {
    features.emplace_back(keypoints[i], descriptors.row(i));
  }
}

std::vector<FeatureTrack> FeatureTracker::getLostTracks() {
  // Get lost tracks
  std::vector<FeatureTrack> tracks;
  this->features.removeLostTracks(tracks);
  if (this->camera_property.distortion_model.empty()) {
    return tracks;
  }

  // Transform keypoints
  for (auto &track : tracks) {
    for (auto &feature : track.track) {
      // Convert pixel coordinates to image coordinates
      cv::Point2f pt_ud = this->camera_property.undistortPoint(feature.kp.pt);
      feature.kp.pt.x = pt_ud.x;
      feature.kp.pt.y = pt_ud.y;
    }
  }

  return tracks;
}

int FeatureTracker::match(const Features &f1,
                          std::vector<cv::DMatch> &matches) {
  // Stack previously unmatched features with tracked features
  Features f0;
  f0.reserve(this->fea_ref.size() + this->unmatched.size());
  f0.insert(f0.end(), this->fea_ref.begin(), this->fea_ref.end());
  f0.insert(f0.end(), this->unmatched.begin(), this->unmatched.end());

  // Match features
  // -- Convert list of features to list of cv::KeyPoint and cv::descriptors
  std::vector<cv::KeyPoint> k0, k1;
  cv::Mat d0, d1;
  this->getKeyPointsAndDescriptors(f0, k0, d0);
  this->getKeyPointsAndDescriptors(f1, k1, d1);

  // -- Perform matching
  // Note: arguments to the brute-force matcher is (query descriptors,
  // train descriptors), here we use d1 as the query descriptors because
  // d1 represents the latest descriptors from the latest image frame
  this->matcher.match(k0, d0, k1, d1, this->img_size, matches);

  // Show matches
  if (this->show_matches) {
    const cv::Mat matches_img =
        draw_matches(this->img_ref, this->img_cur, k0, k1, matches);
    cv::imshow("Matches", matches_img);
    cv::waitKey(1);
  }

  // Update or add feature track
  std::map<int, bool> tracks_updated;
  std::map<int, bool> idx_updated;
  this->fea_ref.clear();

  for (size_t i = 0; i < matches.size(); i++) {
    const int f0_idx = matches[i].queryIdx;
    const int f1_idx = matches[i].trainIdx;
    auto fea0 = f0[f0_idx];
    auto fea1 = f1[f1_idx];

    if (fea0.track_id != -1) {
      this->features.updateTrack(this->counter_frame_id, fea0.track_id, fea1);
    } else {
      this->features.addTrack(this->counter_frame_id, fea0, fea1);
    }

    this->fea_ref.push_back(fea1);
    tracks_updated.insert({fea1.track_id, true});
    idx_updated.insert({f1_idx, true});
  }

  // Drop dead feature tracks
  const std::vector<TrackID> tracks_tracking = this->features.tracking;
  for (auto track_id : tracks_tracking) {
    if (tracks_updated.count(track_id) == 0) {
      this->features.removeTrack(track_id, true);
    }
  }

  // Update tracker stats
  this->stats.update(this->features.tracking.size(),
                     this->features.lost.size());

  // Update list of reference and unmatched features
  // -- Clear unmatched
  this->unmatched.clear();
  // -- Calculate replenish size
  int replenish_size = this->max_features - this->features.tracking.size();
  replenish_size = (replenish_size > 0) ? replenish_size : 0;
  // -- Replenish features tracking (to make sure we don't runout over time)
  for (int i = 0; i < (int) std::min(f1.size(), (size_t) replenish_size); i++) {
    if (idx_updated.find(i) == idx_updated.end()) {
      this->unmatched.push_back(f1[i]);
    }
  }

  return 0;
}

int FeatureTracker::initialize(const cv::Mat &img_cur) {
  img_cur.copyTo(this->img_ref);
  this->img_size = img_cur.size();
  return this->detect(img_cur, this->fea_ref);
}

int FeatureTracker::update(const cv::Mat &img_cur) {
  // Keep track of current image
  img_cur.copyTo(this->img_cur);

  // Initialize feature tracker
  if (this->fea_ref.size() == 0) {
    this->initialize(img_cur);
    return 0;
  }

  // Detect
  Features features;
  if (this->detect(img_cur, features) != 0) {
    return -1;
  }

  // Match features
  std::vector<cv::DMatch> matches;
  if (this->match(features, matches) != 0) {
    return -2;
  }

  // Update
  img_cur.copyTo(this->img_ref);

  return 0;
}

std::ostream &operator<<(std::ostream &os, const FeatureTracker &tracker) {
  os << "tracking: [";
  for (auto track_id : tracker.features.tracking) {
    os << track_id << ", ";
  }
  if (tracker.features.tracking.size()) {
    os << "\b\b]" << std::endl;
  } else {
    os << "]" << std::endl;
  }

  os << "lost : [";
  for (auto track_id : tracker.features.lost) {
    os << track_id << ", ";
  }
  if (tracker.features.lost.size()) {
    os << "\b\b]" << std::endl;
  } else {
    os << "]" << std::endl;
  }

  os << "buffer: [";
  for (auto kv : tracker.features.buffer) {
    os << kv.first << ", ";
  }
  if (tracker.features.buffer.size()) {
    os << "\b\b]" << std::endl;
  } else {
    os << "]" << std::endl;
  }

  return os;
}

} //  namespace prototype
