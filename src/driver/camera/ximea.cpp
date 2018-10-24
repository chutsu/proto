#include "prototype/driver/camera/ximea.hpp"

namespace prototype {

XIMEA::XIMEA() { this->ximea = NULL; }

int XIMEA::initialize() {
  int ds_type;
  int ds_rate;
  int img_format;
  XI_RETURN retval;
  int image_height;
  int image_width;
  int offset_x;
  int offset_y;

  // setup
  ds_type = 0;
  ds_rate = 1;
  // img_format = XI_RAW8;
  img_format = XI_MONO8;

  image_width = 1280;
  image_height = 480 * 2;
  // offset_y = 100;
  // offset_x = 100;
  offset_y = 0;
  offset_x = 0;

  this->ximea = NULL;
  retval = XI_OK;

  LOG_INFO("Starting Ximea Camera\n");

  // clang-format off
  // open camera device
  retval = xiOpenDevice(0, &this->ximea);
  XIMEA_CHECK(retval, "xiOpenDevice");

  // downsampling type
  retval = xiSetParamInt(this->ximea, XI_PRM_DOWNSAMPLING_TYPE, ds_type);
  XIMEA_CHECK(retval, "xiSetParam (downsampling type)");

  // downsampling
  retval = xiSetParamInt(this->ximea, XI_PRM_DOWNSAMPLING, ds_rate);
  XIMEA_CHECK(retval, "xiSetParam (downsampling rate)");

  // image format
  retval = xiSetParamInt(this->ximea, XI_PRM_IMAGE_DATA_FORMAT, img_format);
  XIMEA_CHECK(retval, "xiSetParam (image format)");

  // windowing
  retval = xiSetParamInt(this->ximea, XI_PRM_WIDTH, image_width);
  XIMEA_CHECK(retval, "xiSetParam (image_width)");

  retval = xiSetParamInt(this->ximea, XI_PRM_HEIGHT, image_height);
  XIMEA_CHECK(retval, "xiSetParam (image_height)");

  retval = xiSetParamInt(this->ximea, XI_PRM_OFFSET_X, offset_x);
  XIMEA_CHECK(retval, "xiSetParam (offset_x)");

  retval = xiSetParamInt(this->ximea, XI_PRM_OFFSET_Y, offset_y);
  XIMEA_CHECK(retval, "xiSetParam (offset_y)");

  // Gain and Exposure
  this->setExposure(this->config.exposure_value);
  this->setGain(this->config.gain_value);

  // buffer policy
  // retval = xiSetParamInt(this->ximea, XI_PRM_BUFFER_POLICY, XI_BP_SAFE);
  // checkState(retval, "xiSetParam (buffer policy)");

  // start acquisition
  retval = xiStartAcquisition(this->ximea);
  XIMEA_CHECK(retval, "xiStartAcquisition");
  // clang-format on

  // return
  LOG_INFO("Ximea Camera Started Successfully\n");
  return 0;

ximea_error:
  return -1;
}

int XIMEA::setGain(float gain_db) {
  XI_RETURN retval;

  retval = xiSetParamFloat(this->ximea, XI_PRM_GAIN, gain_db);
  XIMEA_CHECK(retval, "xiSetParam (exposure time set)");
  return 0;

ximea_error:
  return -1;
}

int XIMEA::setExposure(float exposure_time_us) {
  XI_RETURN retval;

  retval = xiSetParamInt(this->ximea, XI_PRM_EXPOSURE, exposure_time_us);
  XIMEA_CHECK(retval, "xiSetParam (exposure time set)");
  return 0;

ximea_error:
  return -1;
}

int XIMEA::changeMode(std::string mode) {
  // pre-check
  if (this->configs.find(mode) == this->configs.end()) {
    return -1;
  }
  // update camera settings
  this->config = this->configs[mode];
  return 0;
}

int XIMEA::getFrame(cv::Mat &image) {
  int retval;
  XI_IMG ximea_img;

  // setup
  memset(&ximea_img, 0, sizeof(ximea_img));
  ximea_img.size = sizeof(XI_IMG);

  // get the image
  retval = xiGetImage(this->ximea, 1000, &ximea_img);
  if (retval != XI_OK) {
    LOG_ERROR("Error after xiGetImage (%d)\n", retval);
    return -1;

  } else {
    // when ximea frame is mono
    // cv::Mat(frame.height, frame.width, CV_8U, frame.bp);

    // when ximea frame is rgb color (XI_RGB24 ONLY)
    // cv::Mat(ximea_img.height, ximea_img.width, CV_8UC3, ximea_img.bp)
    //   .copyTo(image);
    cv::Mat(ximea_img.height, ximea_img.width, CV_8U, ximea_img.bp)
        .copyTo(image);

    // resize the image to reflect camera mode
    cv::resize(image, image, cv::Size(640, 480));
    // cv::Size(this->config.image_width, this->config.image_height));
    return 0;
  }
}

} // namespace prototype
