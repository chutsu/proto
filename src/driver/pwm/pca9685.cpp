#include "prototype/driver/pwm/pca9685.hpp"

namespace prototype {

int PCA9685::configure(const int freq) {
  // Setup
  this->i2c = I2C();
  if (this->i2c.setup() != 0) {
    LOG_INFO("Failed to open i2c connection!");
    return -1;
  }
  this->i2c.setSlave(PCA9685_I2C_ADDR);

  this->reset();
  this->setAllPWM(0);

  // Configure mode 2 register
  this->i2c.writeByte(PCA9685_MODE2, 0x04);
  this->i2c.writeByte(PCA9685_MODE1, 0x01);
  usleep(PCA9685_WAIT_MS);

  // Configure mode 1 register
  char mode_1;
  this->i2c.readByte(PCA9685_MODE1, &mode_1);
  mode_1 = mode_1 & ~0x10;
  this->i2c.writeByte(PCA9685_MODE1, mode_1);
  usleep(PCA9685_WAIT_MS);

  // Set frequency
  this->setPWMFrequency(freq);
  usleep(PCA9685_WAIT_MS);

  return 0;
}

void PCA9685::setPWMFrequency(const int freq) {
  // Setup
  this->i2c.setSlave(PCA9685_I2C_ADDR);

  // Set pca9685 to sleep
  char mode_1_old;
  this->i2c.readByte(PCA9685_MODE1, &mode_1_old);
  char mode_1_new = (mode_1_old & 0x7F) | 0x10;
  this->i2c.writeByte(PCA9685_MODE1, mode_1_new);

  // Set pwm prescaler
  float prescale = (25000000 / (4096.0 * freq)) - 1;
  prescale = floor(prescale + 0.5);
  // LOG_INFO("prescale: %d", (int) prescale);
  this->i2c.writeByte(PCA9685_PRE_SCALE, (int) prescale);
  this->i2c.writeByte(PCA9685_MODE1, mode_1_old);

  // Wait for oscillator
  usleep(PCA9685_WAIT_MS);

  // Reset
  this->i2c.writeByte(PCA9685_MODE1, mode_1_old | (1 << 7));
}

void PCA9685::setPWM(const int8_t channel, const int16_t off) {
  this->i2c.setSlave(PCA9685_I2C_ADDR);
  this->i2c.writeByte(PCA9685_LED0_ON_L + (4 * channel), 0 & 0xFF);
  this->i2c.writeByte(PCA9685_LED0_ON_H + (4 * channel), 0 >> 8);
  this->i2c.writeByte(PCA9685_LED0_OFF_L + (4 * channel), off & 0xFF);
  this->i2c.writeByte(PCA9685_LED0_OFF_H + (4 * channel), off >> 8);
}

void PCA9685::setAllPWM(const int16_t off) {
  this->i2c.setSlave(PCA9685_I2C_ADDR);
  this->i2c.writeByte(PCA9685_ALL_LED_ON_L, 0 & 0xFF);
  this->i2c.writeByte(PCA9685_ALL_LED_ON_H, 0 >> 8);
  this->i2c.writeByte(PCA9685_ALL_LED_OFF_L, off & 0xFF);
  this->i2c.writeByte(PCA9685_ALL_LED_OFF_H, off >> 8);
}

void PCA9685::reset() {
  this->i2c.setSlave(0x00);
  this->i2c.writeRawByte(0x06);
  usleep(PCA9685_WAIT_MS);
}

} // libr namespace
