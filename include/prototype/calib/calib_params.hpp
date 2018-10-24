/**
 * @file
 * @ingroup calibration
 */
#ifndef PROTOTYPE_CALIB_CALIB_PARAMS_HPP
#define PROTOTYPE_CALIB_CALIB_PARAMS_HPP

#include "prototype/core.hpp"
#include "calibration/camchain.hpp"

namespace prototype {
/**
 * @addtogroup calibration
 * @{
 */

/**
 * Calibration parameters
 */
struct CalibParams {
  Camchain camchain;

  double *tau_s = nullptr;
  double *tau_d = nullptr;
  double *w1 = nullptr;
  double *w2 = nullptr;
  double *theta1_offset = nullptr;
  double *theta2_offset = nullptr;
  double *Lambda1 = nullptr;
  double *Lambda2 = nullptr;
  int nb_measurements = 0;

  CalibParams();
  virtual ~CalibParams();

  /**
   * Load initial optimization params
   *
   * @param camchain_file Path to camchain file
   * @param joint_file Path to joint angles file
   * @returns 0 for success, -1 for failure
   */
  int load(const std::string &camchain_file, const std::string &joint_file);
};

/**
 * CalibParams to string
 */
std::ostream &operator<<(std::ostream &os, const CalibParams &m);

/** @} group calibration */
} //  namespace prototype
#endif // PROTOTYPE_CALIB_CALIB_PARAMS_HPP
