addpath(genpath("proto"));
graphics_toolkit("fltk");
pkg load statistics;

function data = calib_data_add_noise(data)
  nb_poses = length(data.time);

  % Add noise to camera position and rotation
  for i = 1:nb_poses
    % Position
    % data.r_WC{i} += normrnd([0; 0; 0], 1e-2);
    data.r_WC{i} += normrnd([0; 0; 0], 1e-1);

    % Rotation
    dq = quat_delta(normrnd([0; 0; 0], 1e-1));
    q_WC = data.q_WC{i};
    data.q_WC{i} = quat_mul(dq, q_WC);
  endfor

  % Add noise to point data
  data.p_data += normrnd(zeros(3, length(data.p_data)), 1e-2);
endfunction

function retval = ba_nb_poses(data)
  retval = length(data.r_WC);
endfunction

function retval = ba_nb_measurements(data)
  nb_poses = ba_nb_poses(data);
  retval = 0;
  for i = 1:nb_poses
    retval += length(data.z_data{i});
  endfor
endfunction

function J = intrinsics_point_jacobian(camera)
  proj_params = camera.param(1:4);
  fx = proj_params(1);
  fy = proj_params(2);

  J = zeros(2, 2);
  J(1, 1) = fx;
  J(2, 2) = fy;
endfunction

function J = project_jacobian(p_C)
  x = p_C(1);
  y = p_C(2);
  z = p_C(3);

  J = zeros(2, 3);
  J(1, 1) = 1.0 / z;
  J(2, 2) = 1.0 / z;
  J(1, 3) = -x / z**2;
  J(2, 3) = -y / z**2;
endfunction

function retval = check_J_cam_rot(K, T_WC, p_W, J_cam_rot, step_size, threshold)
  fdiff = zeros(2, 3);
  z = [0; 0];
  e = z - pinhole_project(K, T_WC, p_W);

  for i = 1:3
    T_WC_diff = perturb_rot(T_WC, step_size, i);
    e_prime = z - pinhole_project(K, T_WC_diff, p_W);
    fdiff(1:2, i) = (e_prime - e) / step_size;
  endfor

  retval = check_jacobian("J_cam_rot", fdiff, J_cam_rot, threshold);
endfunction

function retval = check_J_cam_pos(K, T_WC, p_W, J_cam_pos, step_size, threshold)
  fdiff = zeros(2, 3);
  z = [0; 0];
  e = z - pinhole_project(K, T_WC, p_W);

  for i = 1:3
    T_WC_diff = perturb_trans(T_WC, step_size, i);
    e_prime = z - pinhole_project(K, T_WC_diff, p_W);
    fdiff(1:2, i) = (e_prime - e) / step_size;
  endfor

  retval = check_jacobian("J_cam_pos", fdiff, J_cam_pos, threshold);
endfunction

function retval = check_J_point(K, T_WC, p_W, J_point, step_size, threshold)
  fdiff = zeros(2, 3);
  dr = eye(3) * step_size;
  z = [0; 0];
  e = z - pinhole_project(K, T_WC, p_W);

  for i = 1:3
    p_W_diff = p_W + dr(1:3, i);
    e_prime = z - pinhole_project(K, T_WC, p_W_diff);
    fdiff(1:2, i) = (e_prime - e) / step_size;
  endfor

  retval = check_jacobian("J_point", fdiff, J_point, threshold);
endfunction

function J = camera_rotation_jacobian(q_WC, r_WC, p_W)
  C_WC = quat2rot(q_WC);
  J = C_WC' * skew(p_W - r_WC);
endfunction

function J = camera_translation_jacobian(q_WC)
  C_WC = quat2rot(q_WC);
  J = -C_WC';
endfunction

function J = target_point_jacobian(q_WC)
  C_WC = quat2rot(q_WC);
  J = C_WC';
endfunction

function J = ba_jacobian(data, check_jacobians=false)
  nb_poses = ba_nb_poses(data);
  nb_points = data.target.nb_rows * data.target.nb_cols;
  nb_measurements = ba_nb_measurements(data);

  % Setup jacobian
  J_rows = nb_measurements * 2;
  J_cols = (nb_poses * 6) + (nb_points * 3);
  J = zeros(J_rows, J_cols);

  % Loop over camera poses
  pose_idx = 1;
  meas_idx = 1;
  for i = 1:nb_poses
    % Form camera pose transform T_WC
    q_WC = data.q_WC{i};
    r_WC = data.r_WC{i};
    T_WC = tf(q_WC, r_WC);

    % Invert T_WC to T_CW and decompose
    T_CW = tf_inv(T_WC);
    C_CW = tf_rot(T_CW);
    r_CW = tf_trans(T_CW);

    # Get point ids and keypoint measurements at time k
    p_ids = data.point_ids_data{i};
    z_k = data.z_data{i};

    % Loop over observations at time k
    for j = 1:length(z_k)
      % Transform point from world to camera frame
      p_W = data.p_data(:, p_ids(j));
      p_C = dehomogeneous(T_CW * homogeneous(p_W));

      % Fill in camera pose jacobian
      % -- Setup row start, row end, column start and column end
      rs = ((meas_idx - 1) * 2) + 1;
      re = rs + 1;
      cs = ((pose_idx - 1) * 6) + 1;
      ce = cs + 5;
      % -- Form jacobians
      J_K = intrinsics_point_jacobian(data.camera);
      J_P = project_jacobian(p_C);
      J_C = camera_rotation_jacobian(q_WC, r_WC, p_W);
      J_r = camera_translation_jacobian(q_WC);
      J_cam_rot = -1 * J_K * J_P * J_C;
      J_cam_pos = -1 * J_K * J_P * J_r;
      % -- Fill in the big jacobian
      J(rs:re, cs:ce) = [J_cam_rot, J_cam_pos];
      % J(rs:re, cs:ce) = 255 * ones(2, 6);

      % Fill in point elements
      % -- Setup row start, row end, column start and column end
      cs = (nb_poses * 6) + ((p_ids(j) - 1) * 3) + 1;
      ce = cs + 2;
      % -- Fill in the big jacobian
      J_point = -1 * J_K * J_P * target_point_jacobian(q_WC);
      J(rs:re, cs:ce) = J_point;
      % J(rs:re, cs:ce) = 255 * ones(2, 3);

      % Test jacobians
      if check_jacobians
        step_size = 1.0e-8;
        threshold = 1.0e-4;
        check_J_cam_rot(K, T_WC, p_W, J_cam_rot, step_size, threshold);
        check_J_cam_pos(K, T_WC, p_W, J_cam_pos, step_size, threshold);
        check_J_point(K, T_WC, p_W, J_point, step_size, threshold);
      endif

      meas_idx++;
    endfor

    pose_idx++;
  endfor

  % figure();
  % cmap = colormap();
  % imshow(J, cmap);
  % ginput();
endfunction

function residuals = ba_residuals(data)
  residuals = [];

  % Target pose
  C_WT = quat2rot(data.q_WT);
  r_WT = data.r_WT;
  T_WT = tf(C_WT, r_WT);

  % Loop over time
  for k = 1:length(data.time)
    % Form camera pose
    q_WC = data.q_WC{k};
    r_WC = data.r_WC{k};
    T_WC = tf(q_WC, r_WC);

    % Get point ids and measurements at time step k
    point_ids = data.point_ids_data{k};
    z_k = data.z_data{k};

    % Loop over observations at time k
    for i = 1:length(z_k)
      % Form calibration target point in world frame
      p_id = point_ids(i);
      p_W = data.p_data(:, p_id);

      % Calculate reprojection error
      z_hat = pinhole_project(data.camera.param(1:4), T_WC, p_W);
      e = z_k(1:2, i) - z_hat;
      residuals = [residuals; e];
    endfor
  endfor
endfunction

function cost = ba_cost(e)
  cost = 0.5 * e' * e;
endfunction

function data = ba_update(data, e, E, sigma=[1.0; 1.0])
  assert(size(sigma) == [2, 1]);

  % Form weight matrix
  nb_measurements = ba_nb_measurements(data);
  W = diag(repmat(sigma, nb_measurements, 1));

  % Solve Gauss-Newton system [H dx = g]: Solve for dx
  H = (E' * W * E);
  g = -E' * W * e;
  % cond(H)
  % H = nearestSPD(H);
  H = H + 0.1 * eye(size(H));
  % cond(H)
  % dx = H \ g;
  dx = pinv(H) * g;

  % Update camera poses
  nb_poses = length(data.time);
  for i = 1:nb_poses
    s = ((i - 1) * 6) + 1;
    dalpha = dx(s:s+2);
    dr_WC = dx(s+3:s+5);

    % Update camera rotation
    dq = quat_delta(dalpha);
    data.q_WC{i} = quat_mul(dq, data.q_WC{i});

    % Update camera position
    data.r_WC{i} += dr_WC;
  endfor

  % Update points
  for i = 1:length(data.p_data)
    s = (nb_poses * 6) + ((i - 1) * 3) + 1;
    dp_W = dx(s:s+2);
    data.p_data(1:3, i) += dp_W;
  endfor
endfunction

function plot_data(data)
  nb_poses = length(data.time);
  q_WT = data.q_WT;
  r_WT = data.r_WT;
  T_WT = tf(q_WT, r_WT);

  figure();
  clf();
  hold on;
  for i = 1:nb_poses
    q_WC = data.q_WC{i};
    r_WC = data.r_WC{i};
    T_WC = tf(q_WC, r_WC);

    calib_target_draw(data.target, T_WT);
    draw_camera(T_WC);
    draw_frame(T_WC, 0.05);
  endfor
  xlabel("x [m]");
  ylabel("y [m]");
  zlabel("z [m]");
  view(3);
  axis 'equal';
  pause;
endfunction

function plot_compare_data(title_name, data_gnd, data_est)
  nb_poses = length(data_gnd.time);
  q_WT = data_gnd.q_WT;
  r_WT = data_gnd.r_WT;
  T_WT = tf(q_WT, r_WT);

  figure();
  clf();
  hold on;
  % Ground truth data
  for i = 1:nb_poses
    q_WC = data_gnd.q_WC{i};
    r_WC = data_gnd.r_WC{i};
    T_WC = tf(q_WC, r_WC);

    calib_target_draw(data_gnd.target, T_WT);
    draw_camera(T_WC, 0.05, "r-");
    % draw_frame(T_WC, 0.05);
  endfor

  % Estimated data
  for i = 1:nb_poses
    q_WC = data_est.q_WC{i};
    r_WC = data_est.r_WC{i};
    T_WC = tf(q_WC, r_WC);

    draw_camera(T_WC, 0.05, "b-");
    % draw_frame(T_WC, 0.05);
  endfor

  scatter3(data_est.p_data(1, :),
           data_est.p_data(2, :),
            data_est.p_data(3, :),
           10.0,
            'b');

  % Plot settings
  title(title_name);
  xlabel("x [m]");
  ylabel("y [m]");
  zlabel("z [m]");
  view(3);
  axis 'equal';
endfunction

% Setup data
% -- Create calibration target
calib_target = calib_target_init(5, 5);
C_WT = euler321(deg2rad([90.0, 0.0, -90.0]));
r_WT = [1.0; 0.0; 0.0];
T_WT = tf(C_WT, r_WT);
% -- Create camera
cam_idx = 0;
image_width = 640;
image_height = 480;
resolution = [image_width; image_height];
fov = 90.0;
fx = focal_length(image_width, fov);
fy = focal_length(image_height, fov);
cx = image_width / 2;
cy = image_height / 2;
proj_model = "pinhole";
dist_model = "radtan4";
proj_params = [fx; fy; cx; cy];
dist_params = [-0.01; 0.01; 1e-4; 1e-4];
camera = camera_init(cam_idx, resolution,
                     proj_model, dist_model,
                     proj_params, dist_params);
% -- Create data
% data = trajectory_simulate(camera, chessboard);
% nb_poses = 20;
nb_poses = 10;
data_gnd = calib_sim(calib_target, T_WT, camera, nb_poses);
% data = calib_data_add_noise(data_gnd);
data = data_gnd;

function save_dataset(save_path="/tmp/ba_data/", data)
  % Create directory to save dataset
  mkdir(save_path);

  % fieldnames(data)
  % fieldnames(data.camera)
  % fieldnames(data.target)

  % -- Save camera matrix
  % cam_file = fopen(strcat(save_path, "cam.csv"), "w");
  % fprintf(cam_file, "%f,%f,%f\n", data.cam_K(0));
  % fclose(cam_file);
  cam_file = fopen(strcat(save_path, "camera.csv"), "w");
  K = data.camera.K;
  fprintf(cam_file, "#camera_K\n");
  fprintf(cam_file, "%f,%f,%f\n", K(1, 1), K(1, 2), K(1, 3));
  fprintf(cam_file, "%f,%f,%f\n", K(2, 1), K(2, 2), K(2, 3));
  fprintf(cam_file, "%f,%f,%f\n", K(3, 1), K(3, 2), K(3, 3));
  fclose(cam_file);

  % -- Save camera poses
  cam_poses_file = fopen(strcat(save_path, "camera_poses.csv"), "w");
  fprintf(cam_poses_file, "#qw,qx,qy,qz,rx,ry,rz\n");
  for k = 1:length(data.q_WC)
    q = data.q_WC{k};
    r = data.r_WC{k};
    fprintf(cam_poses_file, "%f,%f,%f,%f,", q(1), q(2), q(3), q(4));
    fprintf(cam_poses_file, "%f,%f,%f", r(1), r(2), r(3));
    fprintf(cam_poses_file, "\n");
  endfor
  fclose(cam_poses_file);

  % -- Save target pose
  target_pose_file = fopen(strcat(save_path, "target_pose.csv"), "w");
  fprintf(target_pose_file, "#qw,qx,qy,qz,rx,ry,rz\n");
  q = data.q_WT;
  r = data.r_WT;
  fprintf(target_pose_file, "%f,%f,%f,%f,", q(1), q(2), q(3), q(4));
  fprintf(target_pose_file, "%f,%f,%f", r(1), r(2), r(3));
  fprintf(target_pose_file, "\n");
  fclose(target_pose_file);

  % -- Save measured keypoints
  keypoints_file = fopen(strcat(save_path, "keypoints.csv"), "w");
  fprintf(keypoints_file, "#size,keypoints\n");
  for i = 1:length(data.z_data)
    z_data = data.z_data{i};
    fprintf(keypoints_file, "%d,", length(z_data) * 2);
    for j = 1:length(z_data)
      fprintf(keypoints_file, "%f,%f", z_data(1, j), z_data(2, j));
      if j != length(z_data)
        fprintf(keypoints_file, ",");
      end
    endfor
    fprintf(keypoints_file, "\n");
  endfor
  fclose(keypoints_file);

  % -- Save point ids
  point_ids_file = fopen(strcat(save_path, "point_ids.csv"), "w");
  fprintf(point_ids_file, "#size,point_ids\n");
  for i = 1:length(data.point_ids_data)
    id = data.point_ids_data{i};
    fprintf(point_ids_file, "%d,", length(id));
    for j = 1:length(id)
      fprintf(point_ids_file, "%d", id(j) - 1);  # to correct for 0-index
      if j != length(id)
        fprintf(point_ids_file, ",");
      end
    endfor
    fprintf(point_ids_file, "\n");
  endfor
  fclose(point_ids_file);

  % -- Save points
  points_file = fopen(strcat(save_path, "points.csv"), "w");
  fprintf(points_file, "#x,y,z\n");
  for i = 1:length(data.p_data)
    p = data.p_data(1:3, i);
    fprintf(points_file, "%f,%f,%f", p(1), p(2), p(3));
    if j != length(id)
      fprintf(points_file, ",");
    end
    fprintf(points_file, "\n");
  endfor
  fclose(points_file);
end

% Optimize
% plot_compare_data("Before Bundle Adjustment", data_gnd, data);
max_iter = 20;
cost_prev = 0.0;
for i = 1:max_iter
  E = ba_jacobian(data);
  e = ba_residuals(data);

  data = ba_update(data, e, E);
  cost = ba_cost(e);
  printf("iter: %d\t cost: %.4e\n", i, cost);

  % Termination criteria
  cost_diff = abs(cost - cost_prev);
  if cost_diff < 1e-6
    printf("Done!\n");
    break;
  endif
  cost_prev = cost;
endfor
% plot_compare_data("After Bundle Adjustment", data_gnd, data);
% % ginput();
