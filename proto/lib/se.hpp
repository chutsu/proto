#ifndef PROTO_SE_HPP
#define PROTO_SE_HPP

#include "core.hpp"

namespace proto {

/****************************** PARAMETERS ***********************************/

typedef ssize_t id_t;

struct param_t {
  EIGEN_MAKE_ALIGNED_OPERATOR_NEW;
  bool fixed = false;
  bool marginalize = false;

  std::string type;
  id_t id = -1;
  timestamp_t ts = 0;
  long local_size = 0;
  long global_size = 0;
  vecx_t param;

  std::vector<id_t> factor_ids;

  param_t() {}

  param_t(const std::string &type_,
          const id_t id_,
          const timestamp_t &ts_,
          const long local_size_,
          const long global_size_,
          const bool fixed_=false)
    : fixed{fixed_},
      type{type_},
      id{id_},
      ts{ts_},
      local_size{local_size_},
      global_size{global_size_},
      param{zeros(global_size_, 1)} {}

  param_t(const std::string &type_,
          const id_t id_,
          const long local_size_,
          const long global_size_,
          const bool fixed_=false)
    : param_t{type_, id_, 0, local_size_, global_size_, fixed_} {}

  virtual ~param_t() {}

  void mark_marginalize() {
    marginalize = true;
    type = "marg_" + type;
  }

  virtual void plus(const vecx_t &) = 0;
  virtual void perturb(const int i, const real_t step_size) = 0;
};

struct pose_t : param_t {
  EIGEN_MAKE_ALIGNED_OPERATOR_NEW;

  pose_t() {}

  pose_t(const id_t id_,
         const timestamp_t &ts_,
         const vec_t<7> &pose,
         const bool fixed_=false)
      : param_t{"pose_t", id_, ts_, 6, 7, fixed_} {
    param = pose;
  }

  pose_t(const id_t id_,
         const timestamp_t &ts_,
         const mat4_t &T,
         const bool fixed_=false)
      : param_t{"pose_t", id_, ts_, 6, 7, fixed_} {
    const quat_t q{tf_quat(T)};
    const vec3_t r{tf_trans(T)};

    param(0) = q.w();
    param(1) = q.x();
    param(2) = q.y();
    param(3) = q.z();

    param(4) = r(0);
    param(5) = r(1);
    param(6) = r(2);
  }

  quat_t rot() const {
    return quat_t{param[0], param[1], param[2], param[3]};
  }

  vec3_t trans() const {
    return vec3_t{param[4], param[5], param[6]};
  }

  mat4_t tf() const {
    return proto::tf(rot(), trans());
  }

  quat_t rot() { return static_cast<const pose_t &>(*this).rot(); }
  vec3_t trans() { return static_cast<const pose_t &>(*this).trans(); }
  mat4_t tf() { return static_cast<const pose_t &>(*this).tf(); }

  void set_trans(const vec3_t &r) {
    param(4) = r(0);
    param(5) = r(1);
    param(6) = r(2);
  }

  void set_rot(const quat_t &q) {
    param(0) = q.w();
    param(1) = q.x();
    param(2) = q.y();
    param(3) = q.z();
  }

  void set_rot(const mat3_t &C) {
    quat_t q{C};
    param(0) = q.w();
    param(1) = q.x();
    param(2) = q.y();
    param(3) = q.z();
  }

  void plus(const vecx_t &dx) {
    // Rotation component
    const vec3_t dalpha{dx(0), dx(1), dx(2)};
    const quat_t dq = quat_delta(dalpha);
    const quat_t q = rot();
    const quat_t q_updated = dq * q;
    param(0) = q_updated.w();
    param(1) = q_updated.x();
    param(2) = q_updated.y();
    param(3) = q_updated.z();

    // Translation component
    param(4) += dx(3);
    param(5) += dx(4);
    param(6) += dx(5);
  }

  void perturb(const int i, const real_t step_size) {
    if (i >= 0 && i < 3) {
      const auto T_WS_diff = tf_perturb_rot(this->tf(), step_size, i);
      this->set_rot(tf_rot(T_WS_diff));
      this->set_trans(tf_trans(T_WS_diff));
    } else if (i >= 3 && i <= 5) {
      const auto T_WS_diff = tf_perturb_trans(this->tf(), step_size, i - 3);
      this->set_rot(tf_rot(T_WS_diff));
      this->set_trans(tf_trans(T_WS_diff));
    } else {
      FATAL("Invalid perturbation index [%d]!", i);
    }
  }
};

struct fiducial_pose_t : pose_t {
  EIGEN_MAKE_ALIGNED_OPERATOR_NEW;

  fiducial_pose_t() {}

  fiducial_pose_t(const id_t id_, const mat4_t &T, const bool fixed_=false)
    : pose_t{id_, 0, T, fixed_} {
    this->type = "fiducial_pose_t";
  }
};

struct extrinsic_t : pose_t {
  EIGEN_MAKE_ALIGNED_OPERATOR_NEW;

  extrinsic_t() {}

  extrinsic_t(const id_t id_, const mat4_t &T, const bool fixed_=false)
    : pose_t{id_, 0, T, fixed_} {
    this->type = "extrinsic_t";
  }
};

struct landmark_t : param_t {
  EIGEN_MAKE_ALIGNED_OPERATOR_NEW;

  landmark_t() {}

  landmark_t(const id_t id_, const vec3_t &p_W_, const bool fixed_=false)
    : param_t{"landmark_t", id_, 3, 3, fixed_} {
    param = p_W_;
  }

  void plus(const vecx_t &dx) { param += dx; }
  void perturb(const int i, const real_t step_size) { param[i] += step_size; }
};

struct camera_params_t : param_t {
  EIGEN_MAKE_ALIGNED_OPERATOR_NEW;

  int cam_index = 0;
  int resolution[2] = {0, 0};
  std::string proj_model;
  std::string dist_model;
  long proj_size = 0;
  long dist_size = 0;

  camera_params_t() {}

  camera_params_t(const id_t id_,
                  const int cam_index_,
                  const int resolution_[2],
                  const vecx_t &proj_params_,
                  const vecx_t &dist_params_,
                  const bool fixed_=false)
    : param_t{"camera_params_t", id_, proj_params_.size() + dist_params_.size(),
              proj_params_.size() + dist_params_.size(),
              fixed_},
      cam_index{cam_index_},
      resolution{resolution_[0], resolution_[1]},
      proj_size{proj_params_.size()},
      dist_size{dist_params_.size()} {
    param.resize(proj_size + dist_size);
    param.head(proj_size) = proj_params_;
    param.tail(dist_size) = dist_params_;
  }

  vecx_t proj_params() { return param.head(proj_size); }
  vecx_t dist_params() { return param.tail(dist_size); }
  void plus(const vecx_t &dx) { param += dx; }
  void perturb(const int i, const real_t step_size) { param(i) += step_size; }
};

struct sb_params_t : param_t {
  EIGEN_MAKE_ALIGNED_OPERATOR_NEW;

  sb_params_t() {}

  sb_params_t(const id_t id_,
             const timestamp_t &ts_,
             const vec3_t &v_,
             const vec3_t &ba_,
             const vec3_t &bg_,
             const bool fixed_=false)
    : param_t{"sb_params_t", id_, ts_, 9, 9, fixed_} {
    param << v_, ba_, bg_;
  }

  void plus(const vecx_t &dx) { param += dx; }
  void perturb(const int i, const real_t step_size) { param(i) += step_size; }
};

typedef std::vector<pose_t> poses_t;
typedef std::vector<landmark_t> landmarks_t;
typedef std::vector<vec2_t> keypoints_t;

void pose_print(const std::string &prefix, const pose_t &pose);
void landmarks_print(const landmarks_t &landmarks);
void keypoints_print(const keypoints_t &keypoints);

/******************************** FACTORS ************************************/

struct factor_t {
  EIGEN_MAKE_ALIGNED_OPERATOR_NEW;
  bool marginalize = false;

  std::string type = "factor_t";
  id_t id = 0;

  matx_t covar;
  matx_t info;
  matx_t sqrt_info;

  std::vector<param_t *> params;
  vecx_t residuals;
  matxs_t jacobians;

  factor_t() {}

  factor_t(const id_t id_,
           const matx_t &covar_,
           const std::vector<param_t *> &params_)
    : id{id_}, covar{covar_}, info{covar_.inverse()}, params{params_} {
    Eigen::LLT<matx_t> llt_info(info);
    sqrt_info = llt_info.matrixL().transpose();
  }

  factor_t(const id_t id_, const matx_t &covar_, param_t * &param_)
    : id{id_}, covar{covar_}, info{covar_.inverse()}, params{param_} {
    Eigen::LLT<matx_t> llt_info(info);
    sqrt_info = llt_info.matrixL().transpose();
  }

  virtual ~factor_t() {}
  virtual int eval(const bool jacs=true) = 0;
};

int check_jacobians(factor_t *factor,
                    const int param_idx,
                    const std::string &jac_name,
                    const real_t step_size,
                    const real_t threshold);

struct pose_factor_t : factor_t {
  EIGEN_MAKE_ALIGNED_OPERATOR_NEW;
  const mat4_t pose_meas;

  pose_factor_t(const id_t id_,
                const mat_t<6, 6> &covar_,
                param_t *param_)
      : factor_t{id_, covar_, param_}, pose_meas{tf(param_->param)} {
    type = "pose_factor_t";
    residuals = zeros(6, 1);
    jacobians.push_back(zeros(6, 6));
  }

  int eval(const bool jacs=true) {
    assert(params.size() == 1);

    // Calculate delta pose
    const mat4_t pose_est = tf(params[0]->param);
    const mat4_t delta_pose = pose_meas * pose_est.inverse();

    // Calculate pose error
    const quat_t dq = tf_quat(delta_pose);
    const vec3_t dtheta = 2 * dq.coeffs().head<3>();
    residuals.head<3>() = dtheta;
    residuals.tail<3>() = tf_trans(pose_meas) - tf_trans(pose_est);

    // Calculate jacobian
    // clang-format off
    if (jacs) {
      jacobians[0].setIdentity();
      jacobians[0] *= -1.0;
      mat3_t dq_mul_xyz;
      dq_mul_xyz << dq.w(), -dq.z(), dq.y(),
                    dq.z(), dq.w(), -dq.x(),
                    -dq.y(), dq.x(), dq.w();
      jacobians[0].block<3, 3>(0, 0) = -dq_mul_xyz;
    }
    // clang-format on

    return 0;
  }
};

struct extrinsic_factor_t : pose_factor_t {
  EIGEN_MAKE_ALIGNED_OPERATOR_NEW;
  const mat4_t pose_meas;

  extrinsic_factor_t(const id_t id_,
                     const mat_t<6, 6> &covar_,
                     param_t *param_)
    : pose_factor_t{id_, covar_, param_} {}
};

struct speed_bias_factor_t : factor_t {
  EIGEN_MAKE_ALIGNED_OPERATOR_NEW;
  const vec_t<9> sb_meas;

  speed_bias_factor_t(const id_t id_,
                      const mat_t<9, 9> &covar_,
                      param_t * param_)
      : factor_t{id_, covar_, param_}, sb_meas{param_->param} {
    type = "speed_bias_factor_t";
    residuals = zeros(9, 1);
    jacobians.push_back(zeros(9, 9));
  }

  int eval(const bool jacs=true) {
    assert(params.size() == 1);

    // Calculate delta sb
    const vec_t<9> sb_est = params[0]->param;
    const vec_t<9> error = sb_meas - sb_est;
    residuals = error;

    // Calculate jacobian
    if (jacs) {
      jacobians[0] = -1.0 * I(9);
    }

    return 0;
  }
};

struct camera_params_factor_t : factor_t {
  EIGEN_MAKE_ALIGNED_OPERATOR_NEW;
  const vecx_t meas;

  camera_params_factor_t(const id_t id_,
                         const matx_t &covar_,
                         param_t *param_)
      : factor_t(id_, covar_, {param_}), meas{param_->param} {
    type = "camera_params_factor_t";
    residuals = zeros(9, 1);
    jacobians.push_back(zeros(9, 9));
  }

  int eval(const bool jacs=true) {
    assert(params.size() == 1);

    // Calculate delta sb
    const vecx_t est = params[0]->param;
    const vecx_t error = meas - est;
    residuals = error;

    // Calculate jacobian
    if (jacs) {
      jacobians[0] = -1.0 * I(est.rows());
    }

    return 0;
  }
};

struct landmark_factor_t : factor_t {
  EIGEN_MAKE_ALIGNED_OPERATOR_NEW;
  const vecx_t meas;

  landmark_factor_t(const id_t id_,
                    const matx_t &covar_,
                    param_t *param_)
      : factor_t(id_, covar_, {param_}), meas{param_->param} {
    type = "landmark_factor_t";
    residuals = zeros(3, 1);
    jacobians.push_back(zeros(3, 3));
  }

  int eval(const bool jacs=true) {
    assert(params.size() == 1);

    // Calculate delta sb
    const vecx_t est = params[0]->param;
    const vecx_t error = meas - est;
    residuals = error;

    // Calculate jacobian
    if (jacs) {
      jacobians[0] = -1.0 * I(est.rows());
    }

    return 0;
  }
};

template <typename CM>
struct ba_factor_t : factor_t {
  EIGEN_MAKE_ALIGNED_OPERATOR_NEW;

  int cam_index = 0;
  int resolution[2] = {0, 0};

  timestamp_t ts = 0;
  vec2_t z{0.0, 0.0};

  ba_factor_t(const id_t id_,
              const timestamp_t &ts_,
              const vec2_t &z_,
              const mat2_t &covar_,
              const std::vector<param_t *> &params_)
      : factor_t{id_, covar_, params_}, ts{ts_}, z{z_} {
    type = "ba_factor_t";
    residuals = zeros(2, 1);
    jacobians.push_back(zeros(2, 6));  // T_WC
    jacobians.push_back(zeros(2, 3));  // p_W
    jacobians.push_back(zeros(2, CM::params_size));  // Camera params

    auto cam_params = static_cast<camera_params_t *>(params_[2]);
    cam_index = cam_params->cam_index;
    resolution[0] = cam_params->resolution[0];
    resolution[1] = cam_params->resolution[1];
  }

  int eval(const bool jacs=true) {
    assert(params.size() == 3);

    // Map out parameters
    const mat4_t T_WC = tf(params[0]->param);
    const vec3_t p_W{params[1]->param};
    const CM cm{resolution, params[2]->param};

    // Transform point from world to camera frame
    const mat4_t T_CW = T_WC.inverse();
    const vec3_t p_C = tf_point(T_CW, p_W);

    // Project point in camera frame to image plane
    vec2_t z_hat;
    mat_t<2, 3> J_h;
    int retval = cm.project(p_C, z_hat, J_h);
    if (retval != 0) {
      // switch (retval) {
      // case -1: LOG_ERROR("Point is not infront of camera!"); break;
      // case -2: LOG_ERROR("Projected point is outside the image plane!"); break;
      // }
      jacobians[0] = zeros(2, 6);  // T_WC
      jacobians[1] = zeros(2, 3);  // p_W
      jacobians[2] = zeros(2, CM::params_size);  // Projection model
      return 0;
    }

    // Calculate residual
    residuals = sqrt_info * (z - z_hat);

    // Calculate Jacobians
    if (jacs) {
      const vec2_t p{p_C(0) / p_C(2), p_C(1) / p_C(2)};
      const mat3_t C_WC = tf_rot(T_WC);
      const mat3_t C_CW = C_WC.transpose();
      const vec3_t r_WC = tf_trans(T_WC);

      // -- Jacobian w.r.t. camera pose T_WC
      jacobians[0].block(0, 0, 2, 3) = -1 * sqrt_info * J_h * C_CW * skew(p_W - r_WC);
      jacobians[0].block(0, 3, 2, 3) = -1 * sqrt_info * J_h * -C_CW;
      // -- Jacobian w.r.t. landmark
      jacobians[1] = -1 * sqrt_info * J_h * C_CW;
      // -- Jacobian w.r.t. camera parameters
      jacobians[2] = -1 * sqrt_info * cm.J_params(p);
    }

    return 0;
  }
};

template <typename CM>
struct calib_mono_factor_t : factor_t {
  EIGEN_MAKE_ALIGNED_OPERATOR_NEW;

  int cam_index = 0;
  int resolution[2] = {0, 0};

  timestamp_t ts = 0;
  int tag_id = 0;
  int tag_corner = 0;
  vec3_t r_FFi{0.0, 0.0, 0.0};
  vec2_t z{0.0, 0.0};

  calib_mono_factor_t(const id_t id_,
                      const timestamp_t &ts_,
                      const int tag_id_,
                      const int tag_corner_,
                      const vec3_t &r_FFi_,
                      const vec2_t &z_,
                      const mat2_t &covar_,
                      const std::vector<param_t *> &params_)
      : factor_t{id_, covar_, params_}, ts{ts_},
        tag_id{tag_id_}, tag_corner{tag_corner_}, r_FFi{r_FFi_}, z{z_} {
    type = "calib_mono_factor_t";
    residuals = zeros(2, 1);
    jacobians.push_back(zeros(2, 6));  // T_WC
    jacobians.push_back(zeros(2, 6));  // T_WF
    jacobians.push_back(zeros(2, CM::params_size));  // Camera params

    auto cam_params = static_cast<camera_params_t *>(params_[2]);
    cam_index = cam_params->cam_index;
    resolution[0] = cam_params->resolution[0];
    resolution[1] = cam_params->resolution[1];
  }

  int eval(const bool jacs=true) {
    assert(params.size() == 3);

    // Map out parameters
    const mat4_t T_WC = tf(params[0]->param);
    const mat4_t T_WF = tf(params[1]->param);
    const CM cm{resolution, params[2]->param};

    // Transform target point to camera frame
    const mat4_t T_CW = T_WC.inverse();
    const vec3_t r_CFi = tf_point(T_CW * T_WF, r_FFi);

    // Project point in camera frame to image plane
    vec2_t z_hat;
    mat_t<2, 3> J_h;
    int retval = cm.project(r_CFi, z_hat, J_h);
    if (retval != 0) {
      // switch (retval) {
      // case -1: LOG_ERROR("Point is not infront of camera!"); break;
      // case -2: LOG_ERROR("Projected point is outside the image plane!"); break;
      // }
      jacobians[0] = zeros(2, 6);  // T_WC
      jacobians[1] = zeros(2, 6);  // T_WF
      jacobians[2] = zeros(2, CM::params_size);  // Projection model
      return 0;
    }

    // Calculate residual
    residuals = sqrt_info * (z - z_hat);

    // Calculate Jacobians
    if (jacs) {
      const vec2_t p{r_CFi(0) / r_CFi(2), r_CFi(1) / r_CFi(2)};
      const vec3_t r_WFi = tf_point(T_WF, r_FFi);
      const mat3_t C_WC = tf_rot(T_WC);
      const mat3_t C_CW = C_WC.transpose();
      const vec3_t r_WC = tf_trans(T_WC);
      const mat3_t C_WF = tf_rot(T_WF);

      // -- Jacobian w.r.t. camera pose T_WC
      jacobians[0].block(0, 0, 2, 3) = -1 * sqrt_info * J_h * C_CW * skew(r_WFi - r_WC);
      jacobians[0].block(0, 3, 2, 3) = -1 * sqrt_info * J_h * -C_CW;
      // -- Jacobian w.r.t. fiducial pose T_WF
      jacobians[1].block(0, 0, 2, 3) = -1 * sqrt_info * J_h * C_CW * -skew(C_WF * r_FFi);
      jacobians[1].block(0, 3, 2, 3) = -1 * sqrt_info * J_h * C_CW * I(3);
      // -- Jacobian w.r.t. camera parameters
      jacobians[2] = -1 * sqrt_info * cm.J_params(p);
    }

    return 0;
  }
};

template <typename CM>
struct cam_factor_t : factor_t {
  EIGEN_MAKE_ALIGNED_OPERATOR_NEW;

  int cam_index = 0;
  int resolution[2] = {0, 0};

  timestamp_t ts = 0;
  vec2_t z{0.0, 0.0};

  cam_factor_t(const id_t id_,
               const timestamp_t &ts_,
               const vec2_t &z_,
               const mat2_t &covar_,
               const std::vector<param_t *> &params_)
      : factor_t{id_, covar_, params_}, ts{ts_}, z{z_} {
    type = "cam_factor_t";
    residuals = zeros(2, 1);
    jacobians.push_back(zeros(2, 6));  // T_WS
    jacobians.push_back(zeros(2, 6));  // T_SC
    jacobians.push_back(zeros(2, 3));  // p_W
    jacobians.push_back(zeros(2, CM::params_size));  // Camera params

    auto cam_params = static_cast<camera_params_t *>(params_[3]);
    cam_index = cam_params->cam_index;
    resolution[0] = cam_params->resolution[0];
    resolution[1] = cam_params->resolution[1];
  }

  int eval(const bool jacs=true) {
    assert(params.size() == 4);

    // Map out parameters
    const mat4_t T_WS = tf(params[0]->param);
    const mat4_t T_SC = tf(params[1]->param);
    const vec3_t p_W{params[2]->param};
    const CM cm{resolution, params[3]->param};

    // Transform point from world to camera frame
    const mat4_t T_WC = T_WS * T_SC;
    const mat4_t T_CW = T_WC.inverse();
    const vec3_t p_C = tf_point(T_CW, p_W);

    // Project point in camera frame to image plane
    vec2_t z_hat;
    mat_t<2, 3> J_h;
    int retval = cm.project(p_C, z_hat, J_h);
    if (retval != 0) {
      // LOG_ERROR("Failed to project point!");
      // switch (retval) {
      // case -1: LOG_ERROR("Point is not infront of camera!"); break;
      // case -2: LOG_ERROR("Projected point is outside the image plane!"); break;
      // }
      // return -1;
      jacobians[0] = zeros(2, 6);  // T_WS
      jacobians[1] = zeros(2, 6);  // T_SC
      jacobians[2] = zeros(2, 3);  // p_W
      jacobians[3] = zeros(2, CM::params_size);  // Camera params
      return 0;
    }

    // Calculate residual
    residuals = sqrt_info * (z - z_hat);

    // Calculate Jacobians
    if (jacs) {
      const mat3_t C_SC = tf_rot(T_SC);
      const mat3_t C_CS = C_SC.transpose();
      const mat3_t C_WS = tf_rot(T_WS);
      const mat3_t C_SW = C_WS.transpose();
      const mat3_t C_CW = C_CS * C_SW;
      const vec3_t r_WS = tf_trans(T_WS);
      const vec2_t p{p_C(0) / p_C(2), p_C(1) / p_C(2)};

      // -- Jacobian w.r.t. sensor pose T_WS
      jacobians[0].block(0, 0, 2, 3) = -1 * J_h * C_CS * C_SW * skew(p_W - r_WS);
      jacobians[0].block(0, 3, 2, 3) = -1 * J_h * C_CS * -C_SW;
      // -- Jacobian w.r.t. sensor-camera extrinsic pose T_SCi
      jacobians[1].block(0, 0, 2, 3) = -1 * J_h * C_CS * skew(C_SC * p_C);
      jacobians[1].block(0, 3, 2, 3) = -1 * J_h * -C_CS;
      // -- Jacobian w.r.t. landmark
      jacobians[2] = -1 * J_h * C_CW;
      // -- Jacobian w.r.t. camera model
      jacobians[3] = -1 * cm.J_params(p);
    }

    return 0;
  }
};

struct imu_factor_t : factor_t {
  EIGEN_MAKE_ALIGNED_OPERATOR_NEW;

  const int imu_index = -1;
  const timestamps_t imu_ts;
  const vec3s_t imu_accel;
  const vec3s_t imu_gyro;
  const vec3_t g{0.0, 0.0, 9.81};

  mat_t<15, 15> P = zeros(15, 15);  // Covariance matrix
  mat_t<12, 12> Q = zeros(12, 12);  // noise matrix
  mat_t<15, 15> F = zeros(15, 15);  // Transition matrix

  // Delta position, velocity and rotation between timestep i and j
  // (i.e start and end of imu measurements)
  vec3_t dp{0.0, 0.0, 0.0};
  vec3_t dv{0.0, 0.0, 0.0};
  quat_t dq{1.0, 0.0, 0.0, 0.0};

  // Accelerometer and gyroscope biases
  vec3_t bg{0.0, 0.0, 0.0};
  vec3_t ba{0.0, 0.0, 0.0};

  imu_factor_t(const id_t id_,
               const int imu_index_,
               const timestamps_t imu_ts_,
               const vec3s_t imu_accel_,
               const vec3s_t imu_gyro_ ,
               const mat_t<15, 15> &covar_,
               const std::vector<param_t *> &params_)
      : factor_t{id_, covar_, params_},
        imu_index{imu_index_},
        imu_ts{imu_ts_},
        imu_accel{imu_accel_},
        imu_gyro{imu_gyro_} {
    type = "imu_factor_t";
    residuals = zeros(15, 1);
    jacobians.push_back(zeros(15, 6));  // T_WS at timestep i
    jacobians.push_back(zeros(15, 9));  // Speed and bias at timestep i
    jacobians.push_back(zeros(15, 6));  // T_WS at timestep j
    jacobians.push_back(zeros(15, 9));  // Speed and bias at timestep j

    propagate(imu_ts_, imu_accel_, imu_gyro_);
  }

  void reset() {
    P = zeros(15, 15);
    F = zeros(15, 15);

    dp = zeros(3);
    dv = zeros(3);
    dq = quat_t{1.0, 0.0, 0.0, 0.0};
    ba = zeros(3);
    bg = zeros(3);
  }

  void propagate(const timestamps_t &ts,
                 const vec3s_t &a_m,
                 const vec3s_t &w_m) {
    assert(ts.size() == a_m.size());
    assert(w_m.size() == a_m.size());

    real_t dt_prev = ns2sec(ts[1] - ts[0]);
    for (size_t i = 0; i < w_m.size(); i++) {
      // Calculate dt
      real_t dt = 0.0;
      if ((i + 1) < w_m.size()) {
        dt = ns2sec(ts[i + 1] - ts[i]);
        dt_prev = dt;
      } else {
        dt = dt_prev;
      }
      // printf("i: %zu\n", i);

      // Update relative position and velocity
      dp = dp + dv * dt + 0.5 * (dq * (a_m[i] - ba)) * dt * dt;
      dv = dv + (dq * (a_m[i] - ba)) * dt;

      // Update relative rotation
      const real_t scalar = 1.0;
      const vec3_t vector = 0.5 * (w_m[i] - bg) * dt;
      const quat_t dq_i{scalar, vector(0), vector(1), vector(2)};
      dq = dq * dq_i;

      // Transition matrix F
      const mat3_t C_ji = dq.toRotationMatrix();
      mat_t<15, 15> F_i = zeros(15, 15);
      F_i.block<3, 3>(0, 3) = I(3);
      F_i.block<3, 3>(3, 6) = -C_ji * skew(a_m[i] - ba);
      F_i.block<3, 3>(3, 9) = -C_ji;
      F_i.block<3, 3>(6, 6) = -skew(w_m[i] - bg);
      F_i.block<3, 3>(6, 12) = -I(3);

      // Input matrix G
      mat_t<15, 12> G_i = zeros(15, 12);
      G_i.block<3, 3>(3, 0) = -C_ji;
      G_i.block<3, 3>(6, 3) = -I(3);
      G_i.block<3, 3>(9, 6) = I(3);
      G_i.block<3, 3>(12, 9) = I(3);

      // Update covariance matrix
      const mat_t<15, 15> I_Fi_dt = (I(15) + F * dt);
      const mat_t<15, 12> Gi_dt = (G_i * dt);
      P = I_Fi_dt * P * I_Fi_dt.transpose() + Gi_dt * Q * Gi_dt.transpose();

      // Update Jacobian
      F = I_Fi_dt * F;
    }
  }

  int eval(const bool jacs=true) {
    // Map out parameters
    // -- Sensor pose at timestep i
    const mat4_t T_i = tf(params[0]->param);
    const mat3_t C_i = tf_rot(T_i);
    const mat3_t C_i_inv = C_i.transpose();
    const quat_t q_i = tf_quat(T_i);
    const vec3_t r_i = tf_trans(T_i);
    // -- Speed and bias at timestamp i
    const vec_t<9> sb_i{params[1]->param};
    const vec3_t v_i = sb_i.segment<3>(0);
    const vec3_t ba_i = sb_i.segment<3>(3);
    const vec3_t bg_i = sb_i.segment<3>(6);
    // -- Sensor pose at timestep j
    const mat4_t T_j = tf(params[2]->param);
    const quat_t q_j = tf_quat(T_j);
    const vec3_t r_j = tf_trans(T_j);
    // -- Speed and bias at timestep j
    const vec_t<9> sb_j{params[3]->param};
    const vec3_t v_j = sb_j.segment<3>(0);
    const vec3_t ba_j = sb_j.segment<3>(3);
    const vec3_t bg_j = sb_j.segment<3>(6);

    // Obtain Jacobians for gyro and accel bias
    const mat3_t dp_dbg = F.block<3, 3>(0, 9);
    const mat3_t dp_dba = F.block<3, 3>(0, 12);
    const mat3_t dv_dbg = F.block<3, 3>(3, 9);
    const mat3_t dv_dba = F.block<3, 3>(3, 12);
    const mat3_t dq_dbg = F.block<3, 3>(6, 12);

    // Calculate residuals
    const real_t dt_ij = ns2sec(imu_ts.back() - imu_ts.front());
    const real_t dt_ij_sq = dt_ij * dt_ij;
    const vec3_t dbg = bg_i - bg;
    const vec3_t dba = ba_i - ba;
    const vec3_t alpha = dp + dp_dbg * dbg + dp_dba * dba;
    const vec3_t beta = dv + dv_dbg * dbg + dv_dba * dba;
    const quat_t gamma = dq * quat_delta(dq_dbg * dbg);

    const quat_t q_i_inv = q_i.inverse();
    const quat_t q_j_inv = q_j.inverse();
    const quat_t gamma_inv = gamma.inverse();

    // clang-format off
    residuals << C_i_inv * (r_j - r_i - v_i * dt_ij + 0.5 * g * dt_ij_sq) - alpha,
                 C_i_inv * (v_j - v_i + g * dt_ij) - beta,
                 2.0 * (gamma_inv * (q_i_inv * q_j)).vec(),
                 ba_j - ba_i,
                 bg_j - bg_i;
    // clang-format on

    // Calculate jacobians
    if (jacs) {
      // clang-format off
      // -- Sensor pose at i Jacobian
      jacobians[0] = zeros(15, 6);
      jacobians[0].block<3, 3>(0, 0) = skew(C_i_inv * (r_j - r_i - v_i * dt_ij + 0.5 * g * dt_ij_sq));
      jacobians[0].block<3, 3>(0, 3) = -C_i_inv;
      jacobians[0].block<3, 3>(3, 0) = skew(C_i_inv * (v_j - v_i + g * dt_ij));
      jacobians[0].block<3, 3>(6, 0) = -quat_mat_xyz(quat_lmul(q_j_inv * q_i) * quat_rmul(gamma));
      // -- Speed and bias at i Jacobian
      jacobians[1] = zeros(15, 9);
      jacobians[1].block<3, 3>(0, 0) = -C_i_inv * dt_ij;
      jacobians[1].block<3, 3>(0, 3) = -dp_dba;
      jacobians[1].block<3, 3>(0, 6) = -dp_dbg;
      jacobians[1].block<3, 3>(3, 0) = -C_i_inv;
      jacobians[1].block<3, 3>(3, 3) = -dv_dba;
      jacobians[1].block<3, 3>(3, 6) = -dv_dbg;
      jacobians[1].block<3, 3>(9, 3) = -I(3);
      jacobians[1].block<3, 3>(12, 6) = -I(3);
      // -- Sensor pose at j Jacobian
      jacobians[2] = zeros(15, 6);
      jacobians[2].block<3, 3>(0, 3) = C_i_inv;
      jacobians[2].block<3, 3>(6, 0) = quat_lmul_xyz(gamma_inv * q_i_inv * q_j_inv);
      // -- Speed and bias at j Jacobian
      jacobians[3] = zeros(15, 9);
      jacobians[3].block<3, 3>(3, 0) = C_i_inv;
      jacobians[3].block<3, 3>(9, 3) = I(3);
      jacobians[3].block<3, 3>(12, 6) = I(3);
      // clang-format on
    }

    return 0;
  }
};

void imu_propagate(const imu_data_t &imu_data,
                   const vec3_t &g,
                   const vec_t<7> &pose_i,
                   const vec_t<9> &sb_i,
                   vec_t<7> &pose_j,
                   vec_t<9> &sb_j);

struct marg_factor_t : factor_t {
  EIGEN_MAKE_ALIGNED_OPERATOR_NEW;

  marg_factor_t() {
    type = "marg_factor_t";
  }

  void add_param() {
    // Add parameter to be marginalized

  }

  int eval(const bool jacs=true) {

    // Calculate jacobians
    if (jacs) {

    }

    return 0;
  }
};

/********************************* GRAPH ************************************/

struct graph_t {
  id_t next_param_id = 0;
  id_t next_factor_id = 0;

  std::map<id_t, factor_t *> factors;
  std::map<id_t, param_t *> params;
  std::unordered_map<id_t, size_t> param_index; // id - column start
  std::vector<std::string> param_order{"pose_t",
                                       "camera_params_t",
                                       "landmark_t"};

  graph_t() {}

  ~graph_t() {
    for (const auto &kv : factors) {
      delete kv.second;
    }
    factors.clear();

    for (const auto &kv : params) {
      delete kv.second;
    }
    params.clear();
  }
};

id_t graph_add_pose(graph_t &graph,
                    const timestamp_t &ts,
                    const vec_t<7> &pose,
                    const bool fixed=false);

id_t graph_add_pose(graph_t &graph,
                    const timestamp_t &ts,
                    const mat4_t &pose,
                    const bool fixed=false);

id_t graph_add_fiducial_pose(graph_t &graph,
                             const mat4_t &pose,
                             const bool fixed=false);

id_t graph_add_extrinsic(graph_t &graph,
                         const mat4_t &pose,
                         const bool fixed=false);

id_t graph_add_landmark(graph_t &graph,
                        const vec3_t &landmark,
                        const bool fixed=false);

id_t graph_add_camera(graph_t &graph,
                      const int cam_index,
                      const int resolution[2],
                      const vecx_t &proj_params,
                      const vecx_t &dist_params,
                      bool fixed=false);

id_t graph_add_speed_bias(graph_t &graph,
                          const timestamp_t &ts,
                          const vec3_t &v,
                          const vec3_t &ba,
                          const vec3_t &bg);

id_t graph_add_speed_bias(graph_t &graph,
                          const timestamp_t &ts,
                          const vec_t<9> &sb);

vecx_t graph_get_estimate(graph_t &graph, id_t id);

id_t graph_add_pose_factor(graph_t &graph,
                           const id_t pose_id,
                           const mat_t<6, 6> &covar = I(6));

id_t graph_add_camera_params_factor(graph_t &graph,
                                    const id_t cam_params_id,
                                    const matx_t &covar);

id_t graph_add_landmark_factor(graph_t &graph,
                               const id_t landmark_id,
                               const mat_t<3, 3> &covar=I(3));

template <typename CM>
id_t graph_add_ba_factor(graph_t &graph,
                         const timestamp_t &ts,
                         const id_t cam_pose_id,
                         const id_t landmark_id,
                         const id_t cam_params_id,
                         const vec2_t &z,
                         const mat2_t &covar = 0.25 * I(2)) {

  // Create factor
  const id_t f_id = graph.next_factor_id++;
  std::vector<param_t *> params{
    graph.params[cam_pose_id],
    graph.params[landmark_id],
    graph.params[cam_params_id],
  };
  auto factor = new ba_factor_t<CM>{f_id, ts, z, covar, params};

  // Add factor to graph
  graph.factors[f_id] = factor;

  // Point params to factor
  for (auto *param : params) {
    param->factor_ids.push_back(f_id);
  }

  return f_id;
}

template <typename CM>
id_t graph_add_calib_mono_factor(graph_t &graph,
                                 const timestamp_t &ts,
                                 const id_t cam_pose_id,
                                 const id_t fiducial_id,
                                 const id_t cam_params_id,
                                 const int tag_id,
                                 const int tag_corner,
                                 const vec3_t &r_FFi,
                                 const vec2_t &z,
                                 const mat2_t &covar = I(2)) {

  // Create factor
  const id_t f_id = graph.next_factor_id++;
  std::vector<param_t *> params{
    graph.params[cam_pose_id],
    graph.params[fiducial_id],
    graph.params[cam_params_id],
  };
  auto factor = new calib_mono_factor_t<CM>{
    f_id, ts,
    tag_id, tag_corner, r_FFi,
    z, covar, params
  };

  // Add factor to graph
  graph.factors[f_id] = factor;

  // Point params to factor
  for (auto *param : params) {
    param->factor_ids.push_back(f_id);
  }

  return f_id;
}

template <typename CM>
id_t graph_add_cam_factor(graph_t &graph,
                          const timestamp_t &ts,
                          const id_t sensor_pose_id,
                          const id_t imu_cam_pose_id,
                          const id_t landmark_id,
                          const id_t cam_params_id,
                          const vec2_t &z,
                          const mat2_t &covar = I(2)) {
  // Create factor
  const id_t f_id = graph.next_factor_id++;
  std::vector<param_t *> params{
    graph.params[sensor_pose_id],
    graph.params[imu_cam_pose_id],
    graph.params[landmark_id],
    graph.params[cam_params_id]
  };
  auto factor = new cam_factor_t<CM>{f_id, ts, z, covar, params};

  // Add factor to graph
  graph.factors[f_id] = factor;

  // Point params to factor
  for (auto *param : params) {
    param->factor_ids.push_back(f_id);
  }

  return f_id;
}

id_t graph_add_imu_factor(graph_t &graph,
                          const int imu_index,
                          const timestamps_t &imu_ts,
                          const vec3s_t &imu_accel,
                          const vec3s_t &imu_gyro,
                          const id_t pose0_id,
                          const id_t sb0_id,
                          const id_t pose1_id,
                          const id_t sb1_id);

// Note: this function does not actually perform marginalization, it simply
// marks it to be marginalized.
void graph_mark_param(graph_t &graph, const id_t param_id);
void graph_rm_param(graph_t &graph, const id_t param_id);
void graph_rm_factor(graph_t &graph, const id_t factor_id);
vecx_t graph_residuals(graph_t &graph);
matx_t graph_jacobians(graph_t &graph, size_t *marg_size, size_t *remain_size);
void graph_eval(graph_t &graph, matx_t &H, vecx_t &g,
                size_t *marg_size, size_t *remain_size);
vecx_t graph_get_state(const graph_t &graph);
void graph_set_state(graph_t &graph, const vecx_t &x);
void graph_print_params(const graph_t &graph);
void graph_update(graph_t &graph, const vecx_t &dx, const size_t offset=0);

/*****************************************************************************
 *                               TINY SOLVER
 ****************************************************************************/

struct tiny_solver_t {
  // Optimization parameters
  bool verbose = false;
  int max_iter = 10;
  real_t lambda = 1e-4;
  real_t cost_change_threshold = 1e-1;
  real_t time_limit = 0.01;
  real_t update_factor = 10.0;

  // Optimization data
  int iter = 0;
  real_t cost = 0.0;
  real_t solve_time = 0.0;
  matx_t H;
  vecx_t g;
  vecx_t e;

  vecx_t x;
  vecx_t dx;

  // Marginalization
  std::string marg_type = "sibley";
  size_t marg_size = 0;
  size_t remain_size = 0;

  tiny_solver_t() {}

  void load_config(const config_t &config, const std::string &prefix="") {
    const std::string key = (prefix == "") ? "" : prefix + ".";
    parse(config, key + "verbose", verbose);
    parse(config, key + "marg_type", marg_type);
    parse(config, key + "max_iter", max_iter);
    parse(config, key + "time_limit", time_limit);
    parse(config, key + "lambda", lambda);
  }

  real_t eval(graph_t &graph) {
    graph_eval(graph, H, g, &marg_size, &remain_size);
    e = graph_residuals(graph);
    return 0.5 * e.transpose() * e;
  }

  void update(graph_t &graph, const real_t lambda_k) {
    assert(H.size() != 0);
    assert(g.size() != 0);

    // -- Marginalize?
    if (marg_size) {
      if (marg_type == "sibley") {
        if (schurs_complement(H, g, marg_size, H.rows() - marg_size) != 0) {
          marg_size = 0;
        }
      } else if (marg_type == "drop") {
        marg_size = 0;
      } else {
        FATAL("marg_type [%s] not implemented!\n", marg_type.c_str());
      }
    }
    // -- Damp the Hessian matrix H
    const matx_t H_diag = (H.diagonal().asDiagonal());
    H = H + lambda_k * H_diag;
    // -- Solve for dx
    dx = H.ldlt().solve(g);
    // -- Update
    graph_update(graph, dx, marg_size);
  }

  int solve(graph_t &graph)  {
    struct timespec solve_tic = tic();
    real_t lambda_k = lambda;

    // Solve
    for (iter = 0; iter < max_iter; iter++) {
      // Cost k
      x = graph_get_state(graph);
      graph_eval(graph, H, g, &marg_size, &remain_size);
      const matx_t H_diag = (H.diagonal().asDiagonal());
      H = H + lambda_k * H_diag;
      dx = H.ldlt().solve(g);
      e = graph_residuals(graph);
      cost = 0.5 * e.transpose() * e;

      // Cost k+1
      graph_update(graph, dx);
      // graph_eval(graph, H, g, &marg_size, &remain_size);
      // const matx_t H_diag_kp1 = (H.diagonal().asDiagonal());
      // H = H + lambda_k * H_diag_kp1;
      // dx = H.ldlt().solve(g);
      e = graph_residuals(graph);
      const real_t cost_k = 0.5 * e.transpose() * e;

      // cost = eval(graph);
      // x = graph_get_state(graph);
      //
      // update(graph, lambda);
      // const real_t cost_k = eval(graph);

      const real_t cost_delta = cost_k - cost;
      const real_t solve_time = toc(&solve_tic);
      const real_t iter_time = (iter == 0) ? 0 : (solve_time / iter);

      if (verbose) {
        printf("iter[%d] ", iter);
        printf("cost[%.2e] ", cost);
        printf("cost_k[%.2e] ", cost_k);
        printf("cost_delta[%.2e] ", cost_delta);
        printf("lambda[%.2e] ", lambda_k);
        printf("iter_time[%.4f] ", iter_time);
        printf("solve_time[%.4f]  ", solve_time);
        printf("\n");

        // // Calculate reprojection error
        // size_t nb_keypoints = e.size() / 2.0;
        // real_t sse = 0.0;
        // for (size_t i = 0; i < nb_keypoints; i++) {
        //   sse += pow(e.segment(i * 2, 2).norm(), 2);
        // }
        // const real_t rmse = sqrt(sse / nb_keypoints);
        // printf("rmse reproj error: %.2f\n", rmse);
      }

      // Determine whether to accept update
      if (cost_k < cost) {
        // Accept update
        // printf("improvement!\n");
        lambda_k /= update_factor;
        cost = cost_k;
      } else {
        // Reject update
        // printf("no improvement!\n");
        graph_set_state(graph, x); // Restore state
        lambda_k *= update_factor;
      }

      // Termination criterias
      if (fabs(cost_delta) < cost_change_threshold) {
        break;
      } else if ((solve_time + iter_time) > time_limit) {
        break;
      }
    }

    solve_time = toc(&solve_tic);
    if (verbose) {
      printf("cost: %.2e\t", cost);
      printf("solver took: %.4fs\n", solve_time);
    }

    return 0;
  }
};


/*******************************************************************************
 *                           SLIDING WINDOW FILTER
 ******************************************************************************/

struct state_info_t {
  timestamp_t ts = -1;
  id_t pose_id = -1;
  id_t sb_id = -1;
  std::vector<id_t> factor_ids;
  std::vector<id_t> feature_ids;

  state_info_t(const timestamp_t ts_) : ts{ts_} {}
};

struct swf_t {
  graph_t graph;
  tiny_solver_t solver;
  int window_limit = 10;

  std::deque<state_info_t> window;
  std::vector<id_t> camera_ids;
  std::vector<id_t> extrinsics_ids;
  std::vector<id_t> feature_ids;
  std::deque<id_t> pose_ids;
  std::deque<id_t> sb_ids;

  ordered_set_t<id_t> marg_param_ids;
  ordered_set_t<id_t> marg_factor_ids;

  real_t imu_rate = 0.0;
  vec3_t g{0.0, 0.0, -9.81};

  swf_t() {}

  size_t window_size() { return window.size(); }
  size_t nb_cams() { return camera_ids.size(); }
  size_t nb_features() { return feature_ids.size(); }
  size_t nb_extrinsics() { return extrinsics_ids.size(); }
  size_t nb_poses() { return pose_ids.size(); }
  size_t nb_speed_biases() { return sb_ids.size(); }

  void print_info() {
    printf("window size: %zu\n", window_size());
    printf("nb camera_ids: %zu\n", camera_ids.size());
    printf("nb feature_ids: %zu\n", feature_ids.size());
    printf("nb extrinsics_ids: %zu\n", extrinsics_ids.size());
    printf("nb pose_ids: %zu\n", pose_ids.size());
    printf("nb sb_ids: %zu\n", sb_ids.size());
  }

  void print_window() {
    printf("window size: %zu\n", window_size());
    int i = 0;
    for (const auto &state : window) {
      printf("state [%d]:\n", i++);
      printf("ts: %ld\n", state.ts);
      printf("factor_ids size: %ld\n", state.factor_ids.size());
      printf("feature_ids size: %ld\n", state.feature_ids.size());
      printf("pose_id: %ld\n", state.pose_id);
      printf("sb_id: %ld\n", state.sb_id);
      printf("\n");
    }
  }

  void add_imu(const config_t &config) {
    const std::string prefix = "imu0";
    parse(config, "imu0.rate", imu_rate);
    parse(config, "imu0.g", g);
  }

  void add_camera(const int cam_index,
                  const int resolution[2],
                  const vecx_t &proj_params,
                  const vecx_t &dist_params) {
    auto camera_id = graph_add_camera(graph,
                                      cam_index,
                                      resolution,
                                      proj_params,
                                      dist_params);
    camera_ids.push_back(camera_id);
  }

  void add_camera(const config_t &config, const int cam_index) {
    const std::string prefix = "cam" + std::to_string(cam_index);

    std::vector<int> cam_res;
    std::string proj_model;
    std::string dist_model;
    vecx_t proj_params;
    vecx_t dist_params;
    parse(config, prefix + ".resolution", cam_res);
    parse(config, prefix + ".proj_model", proj_model);
    parse(config, prefix + ".dist_model", dist_model);

    bool has_proj_params = yaml_has_key(config, prefix + ".proj_params");
    bool has_dist_params = yaml_has_key(config, prefix + ".dist_params");
    bool has_lens_hfov = yaml_has_key(config, prefix + ".lens_hfov");
    bool has_lens_vfov = yaml_has_key(config, prefix + ".lens_vfov");

    if (has_proj_params && has_dist_params) {
      parse(config, prefix + ".proj_params", proj_params);
      parse(config, prefix + ".dist_params", dist_params);

    } else if (has_lens_hfov && has_lens_vfov) {
      real_t lens_hfov;
      real_t lens_vfov;
      parse(config, prefix + ".lens_hfov", lens_hfov);
      parse(config, prefix + ".lens_vfov", lens_vfov);
      const real_t fx = pinhole_focal(cam_res[0], lens_hfov);
      const real_t fy = pinhole_focal(cam_res[1], lens_vfov);
      const real_t cx = cam_res[0] / 2.0;
      const real_t cy = cam_res[1] / 2.0;
      proj_params = vec4_t{fx, fy, cx, cy};
      dist_params = vec4_t{0.01, 0.001, 0.01, 0.001};

    } else {
      FATAL("Insufficient info to init proj and dist params!");
    }

    add_camera(cam_index, cam_res.data(), proj_params, dist_params);
  }

  void add_extrinsics(const int cam_index, const mat4_t &extrinsics) {
    // Check to see if camera index exists
    try {
      camera_ids.at(cam_index);
    } catch (const std::out_of_range &exception) {
      FATAL("camera [%d] not added yet!", cam_index);
    }

    // Add extrinsics
    auto imucam_id = graph_add_extrinsic(graph, extrinsics);
    extrinsics_ids.push_back(imucam_id);
  }

  void add_extrinsics(config_t &config, const int cam_index) {
    const std::string prefix = "T_SC" + std::to_string(cam_index);
    mat4_t extrinsics;
    parse(config, prefix, extrinsics);
    add_extrinsics(cam_index, extrinsics);
  }

  id_t add_feature(const vec3_t &feature) {
    auto feature_id = graph_add_landmark(graph, feature);
    feature_ids.push_back(feature_id);
    return feature_id;
  }

  id_t add_pose(const timestamp_t ts, const mat4_t &pose) {
    auto pose_id = graph_add_pose(graph, ts, pose);
    pose_ids.push_back(pose_id);
    window.emplace_back(ts);
    window.back().pose_id = pose_id;
    return pose_id;
  }

  id_t add_pose(const timestamp_t ts, const vec_t<7> &pose) {
    return add_pose(ts, tf(pose));
  }

  id_t add_speed_bias(const timestamp_t ts,
                      const vec3_t &v,
                      const vec3_t &ba,
                      const vec3_t &bg) {
    assert(window_size() != 0);
    auto sb_id = graph_add_speed_bias(graph, ts, v, ba, bg);
    sb_ids.push_back(sb_id);
    window.back().sb_id = sb_id;
    return sb_id;
  }

  id_t add_speed_bias(const timestamp_t ts, const vec_t<9> &sb) {
    const auto v = sb.segment(0, 3);
    const auto ba = sb.segment(3, 3);
    const auto bg = sb.segment(6, 3);
    return add_speed_bias(ts, v, ba, bg);
  }

  id_t add_pose_prior(const id_t pose_id) {
    assert(window_size() != 0);

    const auto prior_id = graph_add_pose_factor(graph, pose_id, I(6));
    window.back().factor_ids.push_back(prior_id);
    return prior_id;
  }

  id_t add_ba_factor(const timestamp_t ts,
                     const int cam_index,
                     const id_t pose_id,
                     const id_t feature_id,
                     const vec2_t &z) {
    assert(nb_cams() != 0);
    assert(window_size() != 0);

    const id_t cam_id = camera_ids.at(cam_index);
    const auto factor_id = graph_add_ba_factor<pinhole_radtan4_t>(
      graph, ts, pose_id, feature_id, cam_id, z);
    window.back().factor_ids.push_back(factor_id);
    window.back().feature_ids.push_back(feature_id);


    return factor_id;
  }

  id_t add_imu_factor(const timestamp_t ts, const imu_data_t &imu_data) {
    // Expect pose, speed and biases to be initialized first
    assert(pose_ids.size() > 0);
    assert(sb_ids.size() > 0);

    // Propagate imu measurements
    const id_t pose_i_id = pose_ids.back();
    const id_t sb_i_id = sb_ids.back();
    const vec_t<7> pose_i = graph.params[pose_i_id]->param;
    const vec_t<9> sb_i = graph.params[sb_i_id]->param;
    vec_t<7> pose_j = pose_i;
    vec_t<9> sb_j = sb_i;
    imu_propagate(imu_data, g, pose_i, sb_i, pose_j, sb_j);
    const id_t pose_j_id = add_pose(ts, pose_j);
    const id_t sb_j_id = add_speed_bias(ts, sb_j);

    // Add imu factor
    const int imu_idx = 0;
    const auto factor_id = graph_add_imu_factor(
      graph, imu_idx,
      imu_data.timestamps, imu_data.accel, imu_data.gyro,
      pose_i_id, sb_i_id, pose_j_id, sb_j_id);
    window.back().factor_ids.push_back(factor_id);
    window.back().pose_id = pose_j_id;
    window.back().sb_id = sb_j_id;

    return factor_id;
  }

  id_t add_cam_factor(const timestamp_t ts,
                      const int cam_index,
                      const id_t pose_id,
                      const id_t feature_id,
                      const vec2_t &z) {
    assert(nb_cams() != 0);
    assert(window_size() != 0);

    const auto imucam_id = extrinsics_ids.at(cam_index);
    const auto cam_id = camera_ids.at(cam_index);
    const auto factor_id = graph_add_cam_factor<pinhole_radtan4_t>(
      graph, ts, pose_id, imucam_id, feature_id, cam_id, z);
    window.back().factor_ids.push_back(factor_id);
    window.back().feature_ids.push_back(feature_id);

    return factor_id;
  }

  void pre_marginalize() {
    // Mark oldest pose or speed bias for marginalization
    {
      auto &state = window.front();
      if (state.pose_id != -1) {
        const auto &param = graph.params[state.pose_id];
        param->mark_marginalize();
        marg_param_ids.insert(param->id);

        auto factor_ids = param->factor_ids;
        for (const auto &factor_id : factor_ids) {
          if (graph.factors.count(factor_id)) {
            const auto &factor = graph.factors[factor_id];
            factor->marginalize = true;
            marg_factor_ids.insert(factor->id);

            auto &ids = param->factor_ids;
            auto erase_idx = std::remove(ids.begin(), ids.end(), factor_id);
            ids.erase(erase_idx, ids.end());
          }
        }
      }
      if (state.sb_id != -1) {
        const auto &param = graph.params[state.sb_id];
        param->mark_marginalize();
        marg_param_ids.insert(param->id);

        for (const auto &factor_id : param->factor_ids) {
          if (graph.factors.count(factor_id)) {
            const auto &factor = graph.factors[factor_id];
            factor->marginalize = true;
            marg_factor_ids.insert(factor->id);

            auto &ids = param->factor_ids;
            auto erase_idx = std::remove(ids.begin(), ids.end(), factor_id);
            ids.erase(erase_idx, ids.end());
          }
        }
      }
    }
  }

  void marginalize() {
    // for (const auto &param_id : marg_param_ids) {
    //   // printf("remove param[%ld]\n", param_id);
    //   graph_rm_param(graph, param_id);
    // }
    for (const auto &factor_id : marg_factor_ids) {
      // printf("remove factor[%ld]\n", factor_id);
      graph_rm_factor(graph, factor_id);
    }

    marg_param_ids.clear();
    marg_factor_ids.clear();
    window.pop_front();
  }

  int solve() {
    if ((int) window.size() <= window_limit) {
      return 0;
    }

    pre_marginalize();
    solver.solve(graph);
    marginalize();

    return 0;
  }

  int load_config(const std::string &config_path) {
    config_t config{config_path};
    parse(config, "swf.window_limit", window_limit);

    // Add imu
    if (yaml_has_key(config, "imu0")) {
      graph.param_order = {"pose_t",
                           "sb_params_t",
                           "camera_params_t",
                           "extrinsic_t",
                           "landmark_t"};
      add_imu(config);
    }

    // Add camera
    for (int cam_idx = 0; cam_idx < 5; cam_idx++) {
      std::string prefix = "cam" + std::to_string(cam_idx);
      if (yaml_has_key(config, prefix)) {
        add_camera(config, cam_idx);
      }
    }

    // Add imu-cam extrinsics
    for (int cam_idx = 0; cam_idx < 5; cam_idx++) {
      std::string imucam_key = "T_SC" + std::to_string(cam_idx);
      if (yaml_has_key(config, imucam_key)) {
        mat4_t pose;
        parse(config, imucam_key, pose);
        add_extrinsics(cam_idx, pose);
      }
    }

    // Solver options
    solver.load_config(config, "solver");

    return 0;
  }

  int save_poses(const std::string &save_path) {
    FILE *est_csv = fopen(save_path.c_str(), "w");
    if (est_csv == NULL) {
      LOG_ERROR("Failed to open [%s] to save poses!", save_path.c_str());
      return -1;
    }

    for (const auto &id : pose_ids) {
      const auto ts = graph.params[id]->ts;
      const auto pose = graph.params[id]->param;
      save_pose(est_csv, ts, pose);
    }
    fclose(est_csv);

    return 0;
  }
};

/*****************************************************************************
 *                             Bundle Adjustment
 ****************************************************************************/

struct ba_data_t {
  mat3_t cam_K;

  poses_t cam_poses;
  pose_t target_pose;
  int nb_frames;

  std::vector<keypoints_t> keypoints;
  int **point_ids;
  int nb_ids;

  real_t **points;
  int nb_points;

  ba_data_t(const std::string &data_path) {
    cam_K = load_camera(data_path);
    cam_poses = load_camera_poses(data_path);
    target_pose = load_target_pose(data_path)[0];
    nb_frames = cam_poses.size();
    keypoints = load_keypoints(data_path);
    point_ids = load_point_ids(data_path, &nb_ids);
    points = load_points(data_path, &nb_points);
  }

  ~ba_data_t() {
    // Point IDs
    for (int i = 0; i < nb_frames; i++) {
      free(point_ids[i]);
    }
    free(point_ids);

    // Points
    for (int i = 0; i < nb_points; i++) {
      free(points[i]);
    }
    free(points);
  }

  static poses_t load_poses(const std::string &csv_path) {
    FILE *csv_file = fopen(csv_path.c_str(), "r");
    char line[1024] = {0};
    poses_t poses;

    size_t pose_index = 0;
    while (fgets(line, 1024, csv_file) != NULL) {
      if (line[0] == '#') {
        continue;
      }

      char entry[1024] = {0};
      real_t data[7] = {0};
      int index = 0;
      for (size_t i = 0; i < strlen(line); i++) {
        char c = line[i];
        if (c == ' ') {
          continue;
        }

        if (c == ',' || c == '\n') {
          data[index] = strtod(entry, NULL);
          memset(entry, '\0', sizeof(char) * 100);
          index++;
        } else {
          entry[strlen(entry)] = c;
        }
      }

      quat_t q{data[0], data[1], data[2], data[3]};
      vec3_t r{data[4], data[5], data[6]};
      poses.emplace_back(pose_index, pose_index, tf(q, r));
      pose_index++;
    }
    fclose(csv_file);

    return poses;
  }

  static keypoints_t parse_keypoints_line(const char *line) {
    char entry[100] = {0};
    int kp_ready = 0;
    vec2_t kp{0.0, 0.0};
    int kp_index = 0;
    bool first_element_parsed = false;

    // Parse line
    keypoints_t keypoints;

    for (size_t i = 0; i < strlen(line); i++) {
      char c = line[i];
      if (c == ' ') {
        continue;
      }

      if (c == ',' || c == '\n') {
        if (first_element_parsed == false) {
          first_element_parsed = true;
        } else {
          // Parse keypoint
          if (kp_ready == 0) {
            kp(0) = strtod(entry, NULL);
            kp_ready = 1;

          } else {
            kp(1) = strtod(entry, NULL);
            keypoints.push_back(kp);
            kp_ready = 0;
            kp_index++;
          }
        }

        memset(entry, '\0', sizeof(char) * 100);
      } else {
        entry[strlen(entry)] = c;
      }
    }

    return keypoints;
  }

  static std::vector<keypoints_t> load_keypoints(const std::string &data_path) {
    char keypoints_csv[1000] = {0};
    strcat(keypoints_csv, data_path.c_str());
    strcat(keypoints_csv, "/keypoints.csv");

    FILE *csv_file = fopen(keypoints_csv, "r");
    std::vector<keypoints_t> keypoints;

    char line[1024] = {0};
    while (fgets(line, 1024, csv_file) != NULL) {
      if (line[0] == '#') {
        continue;
      }
      keypoints.push_back(parse_keypoints_line(line));
    }
    fclose(csv_file);

    return keypoints;
  }

  static mat3_t load_camera(const std::string &data_path) {
    // Setup csv path
    char cam_csv[1000] = {0};
    strcat(cam_csv, data_path.c_str());
    strcat(cam_csv, "/camera.csv");

    // Parse csv file
    int nb_rows = 0;
    int nb_cols = 0;
    real_t **cam_K = csv_data(cam_csv, &nb_rows, &nb_cols);
    if (cam_K == NULL) {
      FATAL("Failed to load csv file [%s]!", cam_csv);
    }
    if (nb_rows != 3 || nb_cols != 3) {
      LOG_ERROR("Error while parsing camera file [%s]!", cam_csv);
      LOG_ERROR("-- Expected 3 rows got %d instead!", nb_rows);
      LOG_ERROR("-- Expected 3 cols got %d instead!", nb_cols);
      FATAL("Invalid camera file [%s]!", cam_csv);
    }

    // Flatten 2D array to 1D array
    mat3_t K;
    for (int i = 0; i < nb_rows; i++) {
      for (int j = 0; j < nb_cols; j++) {
        K(i, j) = cam_K[i][j];
      }
      free(cam_K[i]);
    }
    free(cam_K);

    return K;
  }

  static poses_t load_camera_poses(const std::string &data_path) {
    char cam_poses_csv[1000] = {0};
    strcat(cam_poses_csv, data_path.c_str());
    strcat(cam_poses_csv, "/camera_poses.csv");
    return load_poses(cam_poses_csv);
  }

  static poses_t load_target_pose(const std::string &data_path) {
    char target_pose_csv[1000] = {0};
    strcat(target_pose_csv, data_path.c_str());
    strcat(target_pose_csv, "/target_pose.csv");
    return load_poses(target_pose_csv);
  }

  static real_t **load_points(const std::string &data_path, int *nb_points) {
    char points_csv[1000] = {0};
    strcat(points_csv, data_path.c_str());
    strcat(points_csv, "/points.csv");

    // Initialize memory for points
    *nb_points = csv_rows(points_csv);
    real_t **points = (real_t **) malloc(sizeof(real_t *) * *nb_points);
    for (int i = 0; i < *nb_points; i++) {
      points[i] = (real_t *) malloc(sizeof(real_t) * 3);
    }

    // Load file
    FILE *infile = fopen(points_csv, "r");
    if (infile == NULL) {
      fclose(infile);
      return NULL;
    }

    // Loop through data
    char line[1024] = {0};
    size_t len_max = 1024;
    int point_idx = 0;
    int col_idx = 0;

    while (fgets(line, len_max, infile) != NULL) {
      if (line[0] == '#') {
        continue;
      }

      char entry[100] = {0};
      for (size_t i = 0; i < strlen(line); i++) {
        char c = line[i];
        if (c == ' ') {
          continue;
        }

        if (c == ',' || c == '\n') {
          points[point_idx][col_idx] = strtod(entry, NULL);
          memset(entry, '\0', sizeof(char) * 100);
          col_idx++;
        } else {
          entry[strlen(entry)] = c;
        }
      }

      col_idx = 0;
      point_idx++;
    }

    // Cleanup
    fclose(infile);

    return points;
  }

  static int **load_point_ids(const std::string &data_path, int *nb_points) {
    char csv_path[1000] = {0};
    strcat(csv_path, data_path.c_str());
    strcat(csv_path, "/point_ids.csv");
    return load_iarrays(csv_path, nb_points);
  }
};

} // namespace proto
#endif // PROTO_SE_HPP
