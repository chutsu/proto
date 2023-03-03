#include <iostream>
#include <string>
#include <thread>
#include <unistd.h>

#include <opencv2/opencv.hpp>
#include <ArduCamLib.h>
#define USE_SOFT_TRIGGER

int arducam_list_cameras() {
  ArduCamIndexinfo infos[16];
  const int num_cameras = ArduCam_scan(infos);
  if (num_cameras == 0) {
    return 0;
  }

  for (int i = 0; i < num_cameras; i++) {
    unsigned char *buf = infos[i].u8SerialNum;

    std::ostringstream os;
    os << buf[0] << buf[1] << buf[2] << buf[3] << "-";
    os << buf[4] << buf[5] << buf[6] << buf[7] << "-";
    os << buf[8] << buf[9] << buf[10] << buf[11];
    const auto serial = os.str();

    const auto cam_idx = infos[i].u8UsbIndex;
    printf("Camera Index:%4d\tSerial: %s\n", cam_idx, serial.c_str());
  }

  // Need to sleep 2 seconds atleast or else for some strange reason
  // ArduCameras would not initialize after this call
  sleep(2);

  return num_cameras;
}

class MT9V034C {
public:
  int index_;
  int img_w_ = 640;
  int img_h_ = 480;
  std::string serial_;
  ArduCamHandle handle_;

  MT9V034C(const int index) : index_{index} {
    // Set camera configuration
    ArduCamCfg cfg = {};
    cfg.u32Width = img_w_;
    cfg.u32Height = img_h_;
    cfg.emI2cMode = I2C_MODE_8_16;
    cfg.emImageFmtMode = FORMAT_MODE_RAW;
    cfg.u32I2cAddr = 0x90; // I2C address of camera
    cfg.u8PixelBits = 10;  // Bit width of the image generated by camera
    cfg.u8PixelBytes = 2;  // Number of bytes per pixel
    cfg.u32TransLvl = 0;

    // Open camera
    int ret_val = ArduCam_open(handle_, &cfg, index_);
    if (ret_val != USB_CAMERA_NO_ERROR) {
      printf("FAIL ret_val: %d\n", ret_val);
      return;
    }

    // Read camera serial number
    unsigned char buf[16];
    ArduCam_readUserData(handle_, 0x400 - 16, 16, buf);
    std::ostringstream os;
    os << buf[0] << buf[1] << buf[2] << buf[3] << "-";
    os << buf[4] << buf[5] << buf[6] << buf[7] << "-";
    os << buf[8] << buf[9] << buf[10] << buf[11];
    serial_ = os.str();

    // Set USB-Shield configurations
    // VRCMD = 0xD7, 0x4600, 0x0100, 1, 0x85
    // VRCMD = 0xD7, 0x4600, 0x0200, 1, 0x00
    // VRCMD = 0xD7, 0x4600, 0x0300, 1, 0xC0
    // VRCMD = 0xD7, 0x4600, 0x0300, 1, 0x40
    // VRCMD = 0xD7, 0x4600, 0x0400, 1, 0x00
    // VRCMD = 0xD7, 0x4600, 0x0A00, 1, 0x02
    // VRCMD = 0xF6, 0x0000, 0x0000, 3, 0x03, 0x04, 0x0C
    uint8_t cmd = 0xD7;
    uint8_t buf_all[7][3] = {{0x85}, {0x00}, {0xC0}, {0x40}, {0x00}, {0x02}};
    uint8_t buf_usb2[3] = {0x03, 0x04, 0x0C};
    ArduCam_setboardConfig(handle_, cmd, 0x4600, 0x0100, 1, buf_all[0]);
    ArduCam_setboardConfig(handle_, cmd, 0x4600, 0x0200, 1, buf_all[1]);
    ArduCam_setboardConfig(handle_, cmd, 0x4600, 0x0300, 1, buf_all[2]);
    ArduCam_setboardConfig(handle_, cmd, 0x4600, 0x0300, 1, buf_all[3]);
    ArduCam_setboardConfig(handle_, cmd, 0x4600, 0x0400, 1, buf_all[4]);
    ArduCam_setboardConfig(handle_, cmd, 0x4600, 0x0A00, 1, buf_all[5]);
    ArduCam_setboardConfig(handle_, cmd, 0, 0, 3, buf_usb2);

    // Set MT9V034C camera configurations
    ArduCam_writeSensorReg(handle_, 0x03, img_h_);
    ArduCam_writeSensorReg(handle_, 0x04, img_w_);
    ArduCam_writeSensorReg(handle_, 0x0D, 0x320);

    // Set Hardware-trigger mode
    int retval = ArduCam_setMode(handle_, EXTERNAL_TRIGGER_MODE);
    if (retval == USB_BOARD_FW_VERSION_NOT_SUPPORT_ERROR) {
      printf("Usb board firmware version not support single mode.\n");
      printf("Fail!\n");
    }
  }

  virtual ~MT9V034C() {
    ArduCam_close(handle_);
  }

  int update(cv::Mat &image) {
    const auto retval = ArduCam_isFrameReady(handle_);
#ifdef USE_SOFT_TRIGGER
    if (retval != 1) {
      ArduCam_softTrigger(handle_);
      return -1;
    }
#endif

    ArduCamOutData *frame = NULL;
    if (ArduCam_getSingleFrame(handle_, frame) != USB_CAMERA_NO_ERROR) {
      return -1;
    }

    const auto bytes = frame->pu8ImageData;
    const auto bit_width = frame->stImagePara.u8PixelBits;

    image = cv::Mat(img_h_, img_w_, CV_8UC1);
    unsigned char *tmp = (unsigned char *) malloc(img_w_ * img_h_);
    int idx = 0;
    for (int i = 0; i < img_w_ * img_h_ * 2; i += 2) {
      tmp[idx++] = ((bytes[i + 1] << 8 | bytes[i]) >> (bit_width - 8)) & 0xFF;
    }
    memcpy(image.data, tmp, img_w_ * img_h_);
    free(tmp);

    return 0;
  }
};

void imshow(cv::Mat &image) {
  char name[50] = {0};
  sprintf(name, "ArduCam%d", 0);

  cv::cvtColor(image, image, cv::COLOR_BayerRG2BGR);
  cv::resize(image,
             image,
             cv::Size(640, 480),
             (0, 0),
             (0, 0),
             cv::INTER_LINEAR);
  cv::imshow(name, image);
  cv::waitKey(1);
}

int main(int argc, char **argv) {
  const auto camera_num = arducam_list_cameras();
  MT9V034C camera{0};
  cv::Mat image;
  while (true) {
    if (camera.update(image) == 0) {
      imshow(image);
    }
  }

  return 0;
}
