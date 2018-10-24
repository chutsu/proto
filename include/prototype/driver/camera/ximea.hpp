#ifndef ATL_CORE_VISION_CAMERA_XIMEA_HPP
#define ATL_CORE_VISION_CAMERA_XIMEA_HPP

#include <m3api/xiApi.h>

#include "atl/vision/camera/camera.hpp"

namespace atl {

#define XIMEA_CHECK(RETVAL, WHERE)                                             \
  if (RETVAL != XI_OK) {                                                       \
    std::cout << "Error after " << WHERE << std::endl;                         \
    if (this->ximea) {                                                         \
      xiCloseDevice(this->ximea);                                              \
      return -1;                                                               \
    }                                                                          \
    goto ximea_error;                                                          \
  }

class XIMEA : public Camera {
public:
  HANDLE ximea;

  XIMEA();

  int initialize();
  int setGain(float gain_db);
  int setExposure(float exposure_time_us);
  int getFrame(cv::Mat &image);
  int changeMode(std::string mode);
};

} // namespace atl
#endif
