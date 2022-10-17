#!/bin/sh
set -e

run_gdb() {
  gdb \
    -ex=run \
    -ex=bt \
    -ex="set confirm off" \
    -ex=quit \
    --args "$1" "$2" "$3"
}

run_memcheck() {
  valgrind --leak-check=full $1 $2 $3
}

###############################################################################
# PYTHON
###############################################################################

# python3 scripts/codegen.py
# python3 scripts/comment_converter.py

# ctags proto/proto.c proto/proto.py
# python3 proto/proto.py
# python3 proto/proto.py TestNetwork.test_http_parse_request
# python3 proto/proto.py TestNetwork.test_websocket_hash
# python3 proto/proto.py TestNetwork.test_websocket_encode_frame
# python3 proto/proto.py TestNetwork.test_debug_server
# python3 proto/proto.py TestLinearAlgebra
# python3 proto/proto.py TestTransform
# python3 proto/proto.py TestTransform.test_quat2rot
# python3 proto/proto.py TestTransform.test_rot2quat
# python3 proto/proto.py TestTransform.test_rot2euler
# python3 proto/proto.py TestTransform.test_quat_inv
# python3 proto/proto.py TestTransform.test_quat_conj
# python3 proto/proto.py TestTransform.test_quat_slerp
# python3 proto/proto.py TestCV
python3 proto/proto.py TestCV.test_harris_corner
# python3 proto/proto.py TestFactors
# python3 proto/proto.py TestFactors.test_pose_factor
# python3 proto/proto.py TestFactors.test_ba_factor
# python3 proto/proto.py TestFactors.test_vision_factor
# python3 proto/proto.py TestFactors.test_calib_vision_factor
# python3 proto/proto.py TestFactors.test_two_state_vision_factor
# python3 proto/proto.py TestFactors.test_imu_buffer
# python3 proto/proto.py TestFactors.test_imu_buffer_with_interpolation
# python3 proto/proto.py TestFactors.test_imu_factor_propagate
# python3 proto/proto.py TestFactors.test_imu_factor
# python3 proto/proto.py TestFactorGraph
# python3 proto/proto.py TestFactorGraph.test_factor_graph_solve_vo
# python3 proto/proto.py TestFactorGraph.test_factor_graph_solve_io
# python3 proto/proto.py TestFactorGraph.test_factor_graph_solve_vio
# python3 proto/proto.py TestFeatureTracking
# python3 proto/proto.py TestFeatureTracking.test_feature_grid_cell_index
# python3 proto/proto.py TestFeatureTracking.test_feature_grid_count
# python3 proto/proto.py TestFeatureTracking.test_spread_keypoints
# python3 proto/proto.py TestFeatureTracking.test_grid_detect
# python3 proto/proto.py TestFeatureTracking.test_optflow_track
# python3 proto/proto.py TestFeatureTracker
# python3 proto/proto.py TestFeatureTracker.test_detect
# python3 proto/proto.py TestFeatureTracker.test_detect_overlaps
# python3 proto/proto.py TestFeatureTracker.test_detect_nonoverlaps
# python3 proto/proto.py TestFeatureTracker.test_detect_new
# python3 proto/proto.py TestFeatureTracker.test_update
# python3 proto/proto.py TestTracker
# python3 proto/proto.py TestTracker.test_tracker_process_features
# python3 proto/proto.py TestTracker.test_tracker_vision_callback
# python3 proto/proto.py TestCalibration
# python3 proto/proto.py TestCalibration.test_aprilgrid
# python3 proto/proto.py TestCalibration.test_calibrator
# python3 proto/proto.py TestEuroc
# python3 proto/proto.py TestKitti
# python3 proto/proto.py TestSimulation
# python3 proto/proto.py TestSimulation.test_create_3d_features
# python3 proto/proto.py TestSimulation.test_create_3d_features_perimeter
# python3 proto/proto.py TestSimulation.test_sim_camera_frame
# python3 proto/proto.py TestSimulation.test_sim_data
# python3 proto/proto.py TestSimulation.test_sim_feature_tracker
# python3 proto/proto.py TestSimulation.test_sim_arm
# python3 proto/proto.py TestViz.test_multiplot
# python3 proto/proto.py TestViz.test_server

# python3 proto/robot_arm.py

###############################################################################
# C
###############################################################################

# make clean
# time make build
# time make tests

# make format_code
# ctags -R lib
# time make clean
# time make build
# time make debug
# export ASAN_OPTIONS=print_legend=0 && time make tests
# export ASAN_OPTIONS=print_legend=0 \
#   && cd ./proto/build/bin/ \
#   && ./test_proto --target test_mat_transpose

run_all_tests() {
  # cd ~/projects/proto
  # time make build
  # cd ./proto/build/bin;
  # ./test_proto
  # cd -

  tmux send-keys -t dev -R C-l C-m
  tmux send-keys -t dev -R "\
    cd ~/projects/proto
    time make build
    cd ./proto/build/bin;
    ./test_proto
    cd -
  " C-m C-m
  exit
}

run_test() {
  # time make build
  # cd ./proto/build/bin;
  # # ./test_proto --target "$1"
  # # valgrind ./test_proto --target "$1"
  # gdb -ex run -ex bt -ex quit --args ./test_proto --target "$1"
  # cd -

  tmux send-keys -t dev -R C-l C-m
  tmux send-keys -t dev -R "\
    time make build
    cd ./proto/build/bin
    ./test_proto --target $1
    cd -
  " C-m C-m
  exit
}

run_sbgc_tests() {
  touch proto/proto.c;
  time make build
  cd ./proto/build/bin;
  ./test_sbgc
  cd -
}

dev_tiscam() {
  tmux send-keys -t dev -R C-l C-m
  tmux send-keys -t dev -R "\
  cd ~/projects/proto/proto && make test_tis && ./build/bin/test_tis
  " C-m C-m
  exit
}

# run_sbgc_tests
# dev_tiscam

# CAM0_SERIAL=19220362
# CAM1_SERIAL=19220363

# gst-launch-1.0 \
#   compositor name=comp \
#     sink_0::alpha=1 sink_0::xpos=0 sink_0::ypos=0 \
#     sink_1::alpha=0.5 sink_1::xpos=320 sink_1::ypos=0 \
#     ! videoconvert \
#     ! xvimagesink \
#   videotestsrc pattern=1 ! "video/x-raw" ! comp.sink_0 \
#   videotestsrc pattern=2 ! "video/x-raw" ! comp.sink_1

# gst-launch-1.0 \
#   compositor name=comp \
#     sink_0::alpha=1 sink_0::xpos=0 sink_0::ypos=0 \
#     sink_1::alpha=1 sink_1::xpos=744 sink_1::ypos=0 \
#     ! videoconvert \
#     ! xvimagesink \
#   tcambin serial=$CAM0_SERIAL ! videoconvert ! comp.sink_0 \
#   tcambin serial=$CAM1_SERIAL ! videoconvert ! comp.sink_1

# gst-launch-1.0 \
#   compositor name=comp ! videoconvert ! ximagesink \
#   tcamsrc serial=$CAM0_SERIAL ! videoconvert ! comp. \
#   tcamsrc serial=$CAM1_SERIAL ! videoconvert ! comp.

# gst-launch-1.0 \
#   compositor name=comp \
#     sink_0::alpha=1 sink_0::xpos=0 sink_0::ypos=0 \
#     sink_1::alpha=1 sink_1::xpos=320 sink_1::ypos=0 \
#     ! videoconvert \
#     ! xvimagesink \
#   videotestsrc pattern="red" ! videoconvert ! comp.sink_0 \
#   videotestsrc pattern="green" ! videoconvert ! comp.sink_1

# gst-launch-1.0 \
#   tcambin serial=$CAM0_SERIAL \
#   ! videoconvert \
#   ! autovideosink


# format="video/x-bayer, format=gbrg, width=640, height=480,framerate=30/1"
# displayformat="video/x-raw, format=GRAY8, width=640, height=480,framerate=30/1"
# gst-launch-1.0 tcambin ! $format ! capssetter join=false replace=true caps="$displayformat" ! videoconvert ! videoscale !  ximagesink

# PROTO
# run_all_tests
# PROTO-LOGGING
# run_test test_debug
# run_test test_log_error
# run_test test_log_warn
# PROTO-FILE_SYSTEM
# run_test test_path_file_name
# run_test test_path_file_ext
# run_test test_path_dir_name
# run_test test_path_join
# run_test test_list_files
# run_test test_list_files_free
# run_test test_file_read
#  run_test test_file_copy
# PROTO-DATA
# run_test test_malloc_string
# run_test test_dsv_rows
# run_test test_dsv_cols
# run_test test_dsv_fields
# run_test test_dsv_data
# run_test test_dsv_free
# PROTO-DATA-STRUCTURE
# run_test test_darray_new_and_destroy
# run_test test_darray_push_pop
# run_test test_darray_contains
# run_test test_darray_copy
# run_test test_darray_new_element
# run_test test_darray_set_and_get
# run_test test_darray_update
# run_test test_darray_remove
# run_test test_darray_expand_and_contract
# run_test test_list_new_and_destroy
# run_test test_list_push_pop
# run_test test_list_shift
# run_test test_list_unshift
# run_test test_list_remove
# run_test test_list_remove_destroy
# run_test test_stack_new_and_destroy
# run_test test_stack_push
# run_test test_stack_pop
# run_test test_queue_new_and_destroy
# run_test test_queue_enqueue_dequeue
# run_test test_hashmap_new_destroy
# run_test test_hashmap_clear_destroy
# run_test test_hashmap_get_set
# run_test test_hashmap_delete
# run_test test_hashmap_traverse
# PROTO-TIME
# run_test test_tic
# run_test test_toc
# run_test test_mtoc
# run_test test_time_now
# PROTO-NETWORK
# run_test test_tcp_server_setup
# run_test test_http_parse_request
# run_test test_websocket_hash
# run_test test_ws_handshake_respond
# run_test test_ws_server
# PROTO-MATHS
# run_test test_min
# run_test test_max
# run_test test_randf
# run_test test_deg2rad
# run_test test_rad2deg
# run_test test_fltcmp
# run_test test_fltcmp2
# run_test test_pythag
# run_test test_lerp
# run_test test_lerp3
# run_test test_sinc
# run_test test_mean
# run_test test_median
# run_test test_var
# run_test test_stddev
# PROTO-LINEAR_ALGEBRA
# run_test test_eye
# run_test test_ones
# run_test test_protos
# run_test test_mat_set
# run_test test_mat_val
# run_test test_mat_copy
# run_test test_mat_row_set
# run_test test_mat_col_set
# run_test test_mat_block_get
# run_test test_mat_block_set
# run_test test_mat_diag_get
# run_test test_mat_diag_set
# run_test test_mat_triu
# run_test test_mat_tril
# run_test test_mat_trace
# run_test test_mat_transpose
# run_test test_mat_add
# run_test test_mat_sub
# run_test test_mat_scale
# run_test test_vec_add
# run_test test_vec_sub
# run_test test_dot
# run_test test_skew
# run_test test_check_jacobian
# PROTO-SVD
# run_test test_svd
# run_test test_svdcomp
# run_test test_pinv
# PROTO-CHOL
# run_test test_chol
# run_test test_chol_lls_solve
# run_test test_chol_lls_solve2
# run_test test_chol_Axb
# PROTO-TIME
# PROTO-TRANSFORMS
# run_test test_tf_set_rot
# run_test test_tf_set_trans
# run_test test_tf_trans
# run_test test_tf_rot
# run_test test_tf_quat
# run_test test_tf_inv
# run_test test_tf_point
# run_test test_tf_hpoint
# run_test test_tf_perturb_rot
# run_test test_tf_perturb_trans
# run_test test_quat2rot
# PROTO-POSE
# run_test test_pose_init
# run_test test_pose_set_get_quat
# run_test test_pose_set_get_trans
# run_test test_pose2tf
# run_test test_load_poses
# PROTO-CV
# run_test test_lie_Exp_Log
# run_test test_linear_triangulation
# run_test test_radtan4_distort
# run_test test_radtan4_point_jacobian
# run_test test_radtan4_params_jacobian
# run_test test_equi4_distort
# run_test test_equi4_point_jacobian
# run_test test_equi4_params_jacobian
# run_test test_pinhole_focal
# run_test test_pinhole_K
# run_test test_pinhole_project
# run_test test_pinhole_projection_matrix
# run_test test_pinhole_point_jacobian
# run_test test_pinhole_params_jacobian
# run_test test_pinhole_radtan4_project
# run_test test_pinhole_radtan4_project_jacobian
# run_test test_pinhole_radtan4_params_jacobian
# run_test test_pinhole_equi4_project
# run_test test_pinhole_equi4_project_jacobian
# run_test test_pinhole_equi4_params_jacobian
# PROTO-SIM
# memcheck run_test test_load_sim_features
# memcheck run_test test_load_sim_imu_data
# memcheck run_test test_load_sim_cam_frame
# memcheck run_test test_load_sim_cam_data
# PROTO-SF
# run_test test_pose_setup
# run_test test_speed_bias_setup
# run_test test_landmark_setup
# run_test test_extrinsics_setup
# run_test test_camera_setup
# run_test test_pose_factor_setup
# run_test test_pose_factor_eval
# run_test test_ba_factor_setup
# run_test test_ba_factor_eval
# run_test test_ba_factor_ceres_eval
# run_test test_cam_factor_setup
# run_test test_cam_factor_eval
# run_test test_cam_factor_ceres_eval
# run_test test_imu_buf_setup
# run_test test_imu_buf_add
# run_test test_imu_buf_clear
# run_test test_imu_buf_copy
# run_test test_imu_buf_print
# run_test test_imu_factor_propagate_step
# run_test test_imu_factor_setup
# run_test test_imu_factor_eval
# run_test test_ceres_solver
# run_test test_solver_setup
# run_test test_solver_print
# run_test test_solver_eval
# PROTO-SIM
# run_test test_load_sim_features
# run_test test_load_sim_imu_data
# run_test test_load_sim_cam_frame
# run_test test_load_sim_cam_data
# PROTO-GUI
# run_test test_gl_zeros
# run_test test_gl_ones
# run_test test_gl_eye
# run_test test_gl_matf_set
# run_test test_gl_matf_val
# run_test test_gl_transpose
# run_test test_gl_equals
# run_test test_gl_vec3_cross
# run_test test_gl_dot
# run_test test_gl_norm
# run_test test_gl_normalize
# run_test test_gl_perspective
# run_test test_gl_lookat
# run_test test_shader_compile
# run_test test_shader_link
# run_test test_gl_prog_setup
# run_test test_gl_camera_setup
# run_test test_gui
# run_test test_imshow

# valgrind --leak-check=full ./test_traj_eval
# time ./test_traj_eval

# ./test_gui
# gdb -ex run -ex bt -args ./test_gui --target test_gui_setup

# tmux send-keys -t dev -R C-l C-m
# tmux send-keys -t dev -R "\
#   cd ~/projects/proto \
#   && time make build \
#   && cd ./proto/build/bin \
#   && ./test_proto --target test_cam_factor_eval \
#   && cd ~/projects/proto
# " C-m C-m
# exit


###############################################################################
# ARDUINO
###############################################################################

# arduino-cli compile -b teensy:avr:teensy40 firmware
# arduino-cli upload -b teensy:avr:teensy40 -p usb1/1-6 firmware
# python3 firmware/firmware_debugger.py

# tmux send-keys -t dev -R C-l C-m
# tmux send-keys -t dev -R "\
#   cd ~/projects/proto \
#   && arduino --upload firmware/firmware.ino
# " C-m C-m
# exit

# && cu -l /dev/ttyACM0 -s 115200
