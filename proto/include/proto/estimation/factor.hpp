#ifndef PROTO_ESTIMATION_FACTOR_HPP
#define PROTO_ESTIMATION_FACTOR_HPP

#include <ceres/ceres.h>

#include "proto/core/core.hpp"
#include "proto/vision/vision.hpp"

namespace proto {

/*****************************************************************************
 *                                FACTOR
 ****************************************************************************/

struct factor_t {
  size_t id = 0;
  std::vector<size_t> param_ids;
  vecx_t residuals;
  matxs_t jacobians;

  factor_t() {}

  factor_t(const size_t id_)
    : id{id_} {}

  virtual ~factor_t() {}

  virtual bool eval(real_t const *const *parameters) = 0;
};

/*****************************************************************************
 *                               BA FACTOR
 ****************************************************************************/

template <typename CM>
struct ba_factor_t : factor_t {
  EIGEN_MAKE_ALIGNED_OPERATOR_NEW;

  const timestamp_t ts = 0;
  const int cam_index = 0;
  const int img_w = 0;
  const int img_h = 0;
  const vec2_t z{0.0, 0.0};
  const mat2_t info = I(2);

  ba_factor_t(const size_t id_,
              const timestamp_t &ts_,
              const int cam_index_,
              const int img_w_,
              const int img_h_,
              const vec2_t &z_,
              const mat2_t &info_ = I(2))
      : factor_t{id_},
        ts{ts_}, cam_index{cam_index_},
        img_w{img_w_}, img_h{img_h_},
        z{z_}, info{info_} {
    jacobians.push_back(zeros(2, 6));  // T_WS
    jacobians.push_back(zeros(2, 3));  // p_W
    jacobians.push_back(zeros(2, CM::proj_params_size));  // Projection model
    jacobians.push_back(zeros(2, CM::dist_params_size));  // Distortion model
  }

  bool eval(real_t const *const *params) {
    assert(param_ids.size() == 5);

    // Map out parameters
    const mat4_t T_WC = tf(params[0]);
    const vec3_t p_W{params[1]};
    const CM cm{img_w, img_h, params[2], params[3]};

    // Transform point from world to camera frame
    const mat4_t T_CW = T_WC.inverse();
    const vec3_t p_C = tf_point(T_CW, p_W);

    // Project point in camera frame to image plane
    vec2_t z_hat;
    mat_t<2, 3> J_h;
    int retval = cm.project(p_C, z_hat, J_h);
    if (retval != 0) {
      LOG_ERROR("Failed to project point!");
      switch (retval) {
      case -1: LOG_ERROR("Point is not infront of camera!"); break;
      case -2: LOG_ERROR("Projected point is outside the image plane!"); break;
      }
      return false;
    }

    // Calculate residual
    residuals = z - z_hat;

    // Calculate Jacobians
    const vec2_t p{p_C(0) / p_C(2), p_C(1) / p_C(2)};
    const vec2_t p_dist = cm.distortion.distort(p);
    const mat3_t C_WC = tf_rot(T_WC);
    const mat3_t C_CW = C_WC.transpose();
    const vec3_t r_WC = tf_trans(T_WC);

    // -- Jacobian w.r.t. sensor pose T_WS
    jacobians[0].block(0, 0, 2, 3) = -1 * J_h * C_CW * skew(p_W - r_WC);
    jacobians[0].block(0, 3, 2, 3) = -1 * J_h * -C_CW;
    // -- Jacobian w.r.t. landmark
    jacobians[1] = -1 * J_h * C_CW;
    // -- Jacobian w.r.t. projection model
    jacobians[2] = -1 * cm.J_proj(p_dist);
    // -- Jacobian w.r.t. distortion model
    jacobians[3] = -1 * cm.J_dist(p);

    return 0;
  }
};

/*****************************************************************************
 *                             CAMERA FACTOR
 ****************************************************************************/

template <typename CM>
struct cam_factor_t : factor_t {
  EIGEN_MAKE_ALIGNED_OPERATOR_NEW;

  const timestamp_t ts = 0;
  const int cam_index = 0;
  const int img_w = 0;
  const int img_h = 0;
  const vec2_t z{0.0, 0.0};
  const mat2_t info = I(2);

  cam_factor_t(const size_t id_,
               const timestamp_t &ts_,
               const int cam_index_,
               const int img_w_,
               const int img_h_,
               const vec2_t &z_,
               const mat2_t &info_ = I(2))
      : factor_t{id_},
        ts{ts_}, cam_index{cam_index_},
        img_w{img_w_}, img_h{img_h_},
        z{z_}, info{info_} {
    jacobians.push_back(zeros(2, 6));  // T_WS
    jacobians.push_back(zeros(2, 6));  // T_SC
    jacobians.push_back(zeros(2, 3));  // p_W
    jacobians.push_back(zeros(2, 4));  // Camera model
    jacobians.push_back(zeros(2, 4));  // Distortion model
  }

  bool eval(real_t const *const *params) {
    assert(param_ids.size() == 5);

    // Map out parameters
    const mat4_t T_WS = tf(params[0]);
    const mat4_t T_SC = tf(params[1]);
    const vec3_t p_W{params[2]};
    const CM cam_model{img_w, img_h, params[3], params[4]};

    // Transform point from world to camera frame
    const mat4_t T_WC = T_WS * T_SC;
    const mat4_t T_CW = T_WC.inverse();
    const vec3_t p_C = tf_point(T_CW, p_W);

    // Project point in camera frame to image plane
    vec2_t z_hat;
    mat_t<2, 3> J_h;
    int retval = cam_model.project(p_C, z_hat, J_h);
    if (retval != 0) {
      LOG_ERROR("Failed to project point!");
      switch (retval) {
      case -1: LOG_ERROR("Point is not infront of camera!"); break;
      case -2: LOG_ERROR("Projected point is outside the image plane!"); break;
      }
      return false;
    }

    // Calculate residual
    residuals = z - z_hat;

    // Calculate Jacobians
    const vec2_t p{p_C(0) / p_C(2), p_C(1) / p_C(2)};
    const vec2_t p_dist = cam_model.distortion.distort(p);
    const vec3_t p_S = tf_point(T_SC, p_C);
    const mat3_t C_SC = tf_rot(T_SC);
    const mat3_t C_CS = C_SC.transpose();
    const mat3_t C_WS = tf_rot(T_WS);
    const mat3_t C_CW = C_CS * C_WS.transpose();

    // -- Jacobian w.r.t. sensor pose T_WS
    jacobians[0].block(0, 0, 2, 3) = -1 * J_h * -C_CW * -skew(C_WS * p_S);
    jacobians[0].block(0, 3, 2, 3) = -1 * J_h * -C_CW;
    // -- Jacobian w.r.t. sensor-camera extrinsic pose T_SCi
    jacobians[1].block(0, 0, 2, 3) = -1 * J_h * C_CS * skew(C_SC * p_C);
    jacobians[1].block(0, 3, 2, 3) = -1 * J_h * -C_CS;
    // -- Jacobian w.r.t. landmark
    jacobians[2] = -1 * J_h * C_CW;
    // -- Jacobian w.r.t. camera model
    jacobians[3] = -1 * cam_model.J_proj(p_dist);
    // -- Jacobian w.r.t. distortion model
    jacobians[4] = -1 * cam_model.J_dist(p);

    return 0;
  }
};

/*****************************************************************************
 *                              IMU FACTOR
 ****************************************************************************/

struct imu_factor_t : factor_t {
  EIGEN_MAKE_ALIGNED_OPERATOR_NEW;

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

  imu_factor_t(const size_t id_,
               const timestamps_t imu_ts_,
               const vec3s_t imu_accel_,
               const vec3s_t imu_gyro_ )
      : factor_t{id_},
        imu_ts{imu_ts_},
        imu_accel{imu_accel_},
        imu_gyro{imu_gyro_} {
    residuals = zeros(15, 1);
    jacobians.push_back(zeros(15, 6));  // T_WS at timestep i
    jacobians.push_back(zeros(15, 9));  // Speed and bias at timestep i
    jacobians.push_back(zeros(15, 6));  // T_WS at timestep j
    jacobians.push_back(zeros(15, 9));  // Speed and bias at timestep j
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

  bool eval(real_t const *const *params) {
    // Map out parameters
    // -- Sensor pose at timestep i
    const mat4_t T_i = tf(params[0]);
    const mat3_t C_i = tf_rot(T_i);
    const mat3_t C_i_inv = C_i.transpose();
    const quat_t q_i = tf_quat(T_i);
    const vec3_t r_i = tf_trans(T_i);
    // -- Speed and bias at timestamp i
    const vec_t<9> sb_i{params[1]};
    const vec3_t v_i = sb_i.segment<3>(0);
    const vec3_t ba_i = sb_i.segment<3>(3);
    const vec3_t bg_i = sb_i.segment<3>(6);
    // -- Sensor pose at timestep j
    const mat4_t T_j = tf(params[2]);
    const quat_t q_j = tf_quat(T_j);
    const vec3_t r_j = tf_trans(T_j);
    // -- Speed and bias at timestep j
    const vec_t<9> sb_j{params[3]};
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

    residuals << C_i_inv * (r_j - r_i - v_i * dt_ij + 0.5 * g * dt_ij_sq) - alpha,
                 C_i_inv * (v_j - v_i + g * dt_ij) - beta,
                 2.0 * (gamma.inverse() * (q_i.inverse() * q_j)).vec(),
                 ba_j - ba_i,
                 bg_j - bg_i;

    // Calculate jacobians
    // clang-format off
    // -- Sensor pose at i Jacobian
    jacobians[0] = zeros(15, 6);
    jacobians[0].block<3, 3>(0, 0) = skew(C_i_inv * (r_j - r_i - v_i * dt_ij + 0.5 * g * dt_ij_sq));
    jacobians[0].block<3, 3>(0, 3) = -C_i_inv;
    jacobians[0].block<3, 3>(3, 0) = skew(C_i_inv * (v_j - v_i + g * dt_ij));
    jacobians[0].block<3, 3>(6, 0) = -(quat_lmul(q_j.inverse() * q_i) * quat_rmul(gamma)).bottomRightCorner<3, 3>();
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
    jacobians[2].block<3, 3>(6, 0) = quat_lmul(gamma.inverse() * q_i.inverse() * q_j.inverse()).bottomRightCorner<3, 3>();
    // -- Speed and bias at j Jacobian
    jacobians[3] = zeros(15, 9);
    jacobians[3].block<3, 3>(3, 0) = C_i_inv;
    jacobians[3].block<3, 3>(9, 3) = I(3);
    jacobians[3].block<3, 3>(12, 6) = I(3);
    // clang-format on

    return true;
  }
};

/*****************************************************************************
 *                              FACTOR GRAPH
 ****************************************************************************/

struct graph_t {
  std::unordered_map<size_t, param_t *> params;
  std::deque<factor_t *> factors;
  size_t param_size = 0;
  size_t residual_size = 0;

  graph_t() {}

  ~graph_t() {
    for (const auto &factor : factors) {
      delete factor;
    }
    factors.clear();

    for (const auto &kv : params) {
      delete kv.second;
    }
    params.clear();
  }
};

size_t graph_add_pose(graph_t &graph,
                      const timestamp_t &ts,
                      const mat4_t &pose) {
  const auto id = graph.params.size();
  const auto pose_param = new pose_t{id, ts, pose};
  graph.params.insert({id, pose_param});
  return id;
}

size_t graph_add_landmark(graph_t &graph, const vec3_t &landmark) {
  const auto id = graph.params.size();
  const auto param = new landmark_t{id, landmark};
  graph.params.insert({id, param});
  return id;
}

size_t graph_add_cam_params(graph_t &graph,
                            const int cam_index,
                            const vecx_t &params) {
  const auto id = graph.params.size();
  const auto param = new camera_param_t{id, cam_index, params};
  graph.params.insert({id, param});
  return id;
}

size_t graph_add_dist_params(graph_t &graph,
                             const int cam_index,
                             const vecx_t &params) {
  const auto id = graph.params.size();
  const auto param = new dist_param_t{id, cam_index, params};
  graph.params.insert({id, param});
  return id;
}

size_t graph_add_sb_params(graph_t &graph,
                           const timestamp_t &ts,
                           const vec3_t &v,
                           const vec3_t &ba,
                           const vec3_t &bg) {
  const auto id = graph.params.size();
  const auto param = new sb_param_t{id, ts, v, ba, bg};
  graph.params.insert({id, param});
  return id;
}

template <typename CM>
size_t graph_add_ba_factor(graph_t &graph,
                           const timestamp_t &ts,
                           const int cam_idx,
                           const int img_w,
                           const int img_h,
                           const size_t cam_pose_id,
                           const size_t landmark_id,
                           const size_t cam_param_id,
                           const size_t dist_param_id,
                           const vec2_t &z,
                           const mat2_t &info = I(2)) {
  // Create factor
  const auto f_id = graph.factors.size();
  auto factor = new ba_factor_t<CM>{
    ts, f_id,
    cam_idx, img_w, img_h,
    z, info
  };
  factor->param_ids.push_back(cam_pose_id);
  factor->param_ids.push_back(landmark_id);
  factor->param_ids.push_back(cam_param_id);
  factor->param_ids.push_back(dist_param_id);

  // Add factor to graph
  graph.factors.push_back(factor);

  return f_id;
}

template <typename CM>
size_t graph_add_cam_factor(graph_t &graph,
                            const timestamp_t &ts,
                            const int cam_idx,
                            const int img_w,
                            const int img_h,
                            const size_t sensor_pose_id,
                            const size_t imu_cam_pose_id,
                            const size_t landmark_id,
                            const size_t cam_param_id,
                            const size_t dist_param_id,
                            const vec2_t &z,
                            const mat2_t &info = I(2)) {
  // Create factor
  const auto f_id = graph.factors.size();
  auto factor = new cam_factor_t<CM>{
    ts, f_id,
    cam_idx, img_w, img_h,
    z, info
  };
  factor->param_ids.push_back(sensor_pose_id);
  factor->param_ids.push_back(imu_cam_pose_id);
  factor->param_ids.push_back(landmark_id);
  factor->param_ids.push_back(cam_param_id);
  factor->param_ids.push_back(dist_param_id);

  // Add factor to graph
  graph.factors.push_back(factor);

  return f_id;
}

size_t graph_add_imu_factor(graph_t &graph,
                            const int imu_index,
                            const timestamps_t &imu_ts,
                            const vec3s_t &imu_accel,
                            const vec3s_t &imu_gyro,
                            const size_t pose0_id,
                            const size_t sb0_id,
                            const size_t pose1_id,
                            const size_t sb1_id) {
  // Create factor
  const auto f_id = graph.factors.size();
  auto factor = new imu_factor_t(imu_index, imu_ts, imu_gyro, imu_accel);
  factor->param_ids.push_back(pose0_id);
  factor->param_ids.push_back(sb0_id);
  factor->param_ids.push_back(pose1_id);
  factor->param_ids.push_back(sb1_id);

  // Add factor to graph
  graph.factors.push_back(factor);

  return f_id;
}

void graph_eval(graph_t &graph) {
  vecx_t r = zeros(graph.residual_size);
  matx_t J = zeros(graph.residual_size, graph.param_size);

  // Determine parameter order
  std::map<real_t *, size_t> param_idx;
  std::map<real_t *, size_t> pose_blocks;
  std::map<real_t *, size_t> calib_blocks;
  std::map<real_t *, size_t> landmark_blocks;
  // size_t param_counter = 0;

  for (const auto &factor : graph.factors) {

    std::vector<real_t *> factor_params;
    for (size_t i = 0; i < factor->param_ids.size(); i++) {
      const auto param_id = factor->param_ids[i];
      const auto param = graph.params[param_id];
      factor_params.push_back(param->data());

        if (typeid(param) == typeid(pose_t)) {

        } else if (typeid(param) == typeid(landmark_t)) {

        }

      // param_idx[param->data()] = param_counter;
      // param_counter += param->local_size;
    }

    factor->eval(factor_params.data());
  }

  // // Loop over time
  // real_t *params[10] = {0};
  // for (const auto &factor : graph.factors) {
  //   for (size_t i = 0; i < factor->param_ids.size(); i++) {
  //     const auto param_id = factor->param_ids[i];
  //     params[i] = graph.params[param_id]->data();
  //   }
  //
  //   // factor->eval(params, );
  //   memset(params, '\0', sizeof(real_t *) * 10);
  // }
}

vecx_t graph_residuals(graph_t &graph) {
  vecx_t e;
  return e;
}

matx_t graph_jacobian(graph_t &graph) {
  matx_t E;
  return E;
}

void graph_update(graph_t &graph, const vecx_t &dx) {

}

/*****************************************************************************
 *                               TINY SOLVER
 ****************************************************************************/

struct tiny_solver_t {
  // Optimization parameters
  int max_iter = 10;
  real_t lambda = 1e-2;

  real_t cost = 0.0;

  tiny_solver_t() {}

  int solve(graph_t &graph)  {
    int max_iter = 10;
    real_t cost_prev = 0.0;

    for (int iter = 0; iter < max_iter; iter++) {
      struct timespec t_start = tic();
      const vecx_t e = graph_residuals(graph);
      const matx_t E = graph_jacobian(graph);

      // Form weight matrix
      // W = diag(repmat(sigma, data->nb_measurements, 1));

      // Solve Gauss-Newton system [H dx = g]: Solve for dx
      matx_t H = E.transpose() * E; // Hessian approx: H = J^t J
      matx_t H_diag = (H.diagonal().asDiagonal());
      // H = H + lambda * I(E.cols());  // original LM damping
      H = H + lambda * H_diag;  // R. Fletcher trust region mod
      const vecx_t g = -E.transpose() * e;
      const vecx_t dx = H.ldlt().solve(g);   // Cholesky decomp

      cost = 0.5 * static_cast<real_t>(e.transpose() * e);
      printf("iter[%d] cost[%.4e] time: %fs\n", iter, cost, toc(&t_start));

      // Termination criteria
      real_t cost_diff = fabs(cost - cost_prev);
      if (cost_diff < 1.0e-3) {
        printf("Done!\n");
        break;
      }
      cost_prev = cost;
    }

    return 0;
  }

};

} // namespace proto
#endif // PROTO_ESTIMATION_FACTOR_HPP
