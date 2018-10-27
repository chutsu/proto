#include "prototype/munit.hpp"
#include "feature2d/feature_container.hpp"

namespace prototype {

int test_FeatureContainerStats_update() {
  FeatureContainerStats stats;

  stats.update(1, 2);
  MU_CHECK(stats.tracking.size() == 1);
  MU_CHECK(stats.lost.size() == 1);
  MU_CHECK(stats.tracking.size() == stats.lost.size());

  return 0;
}

int test_FeatureContainerStats_save() {
  FeatureContainerStats stats;

  stats.update(1, 2);
  int retval = stats.save("/tmp/stat.csv");
  MU_CHECK(retval == 0);

  return 0;
}

int test_FeatureContainer_addTrack() {
  FeatureContainer features;
  Feature f1;
  Feature f2;

  features.addTrack(0, f1, f2);

  MU_CHECK_EQ(1, features.counter_track_id);

  MU_CHECK_EQ(0, f1.track_id);
  MU_CHECK_EQ(0, f2.track_id);

  MU_CHECK_EQ(1, (int) features.tracking.size());
  MU_CHECK_EQ(0, (int) features.lost.size());
  MU_CHECK_EQ(1, (int) features.buffer.size());

  return 0;
}

int test_FeatureContainer_removeTrack() {
  FeatureContainer features(2, 5);
  Feature f1;
  Feature f2;

  // Test remove from buffer as lost
  features.addTrack(0, f1, f2);
  features.removeTrack(0, true);

  MU_CHECK_EQ(0, (int) features.tracking.size());
  MU_CHECK_EQ(1, (int) features.lost.size());
  MU_CHECK_EQ(1, (int) features.buffer.size());

  // Clear lost and buffer for next test
  features.lost.clear();
  features.buffer.clear();

  // Test remove as not lost
  features.addTrack(1, f1, f2);
  features.removeTrack(1, false);

  MU_CHECK_EQ(0, (int) features.tracking.size());
  MU_CHECK_EQ(0, (int) features.lost.size());
  MU_CHECK_EQ(0, (int) features.buffer.size());

  return 0;
}

int test_FeatureContainer_removeTracks() {
  FeatureContainer features(2, 5);
  Feature f1;
  Feature f2;

  // Test remove as not lost
  features.addTrack(1, f1, f2);
  features.addTrack(2, f1, f2);
  features.addTrack(3, f1, f2);
  features.removeTracks({0, 1, 2}, false);

  MU_CHECK_EQ(0, (int) features.tracking.size());
  MU_CHECK_EQ(0, (int) features.lost.size());
  MU_CHECK_EQ(0, (int) features.buffer.size());

  return 0;
}

int test_FeatureContainer_updateTrack() {
  FeatureContainer features;
  Feature f1;
  Feature f2;
  Feature f3;

  features.addTrack(1, f1, f2);
  const int retval = features.updateTrack(1, 0, f3);

  MU_CHECK_EQ(0, retval);
  MU_CHECK_EQ(3, (int) features.buffer[0].trackedLength());

  return 0;
}

int test_FeatureContainer_purge() {
  FeatureContainer features;

  // Add features
  for (int i = 0; i < 10; i++) {
    Feature f1;
    Feature f2;
    features.addTrack(i, f1, f2);
  }

  // Purge
  const FeatureTracks tracks = features.purge(10);

  // Assert
  TrackID index = 0;
  MU_CHECK_EQ(10, tracks.size());
  MU_CHECK_EQ(0, features.buffer.size());
  for (auto t : tracks) {
    MU_CHECK_EQ(index, t.track_id);
    index++;
  }

  return 0;
}

void test_suite() {
  MU_ADD_TEST(test_FeatureContainerStats_update);
  MU_ADD_TEST(test_FeatureContainerStats_save);

  MU_ADD_TEST(test_FeatureContainer_addTrack);
  MU_ADD_TEST(test_FeatureContainer_removeTrack);
  MU_ADD_TEST(test_FeatureContainer_removeTracks);
  MU_ADD_TEST(test_FeatureContainer_updateTrack);
  MU_ADD_TEST(test_FeatureContainer_purge);
}

} // namespace prototype

MU_RUN_TESTS(prototype::test_suite);
