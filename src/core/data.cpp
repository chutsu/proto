#include "prototype/core/data.hpp"

namespace prototype {

int filerows(const std::string &file_path) {
  // Load file
  std::ifstream infile(file_path);
  if (infile.good() != true) {
    return -1;
  }

  // Obtain number of lines
  int nb_rows = 0;
  std::string line;
  while (std::getline(infile, line)) {
    nb_rows++;
  }

  return nb_rows;
}

int csvrows(const std::string &file_path) {
  // Load file
  std::ifstream infile(file_path);
  if (infile.good() != true) {
    return -1;
  }

  // Obtain number of lines
  int nb_rows = 0;
  std::string line;
  while (std::getline(infile, line)) {
    nb_rows++;
  }

  return nb_rows;
}

int csvcols(const std::string &file_path) {
  int nb_elements = 1;
  bool found_separator = false;

  // Load file
  std::ifstream infile(file_path);
  if (infile.good() != true) {
    return -1;
  }

  // Obtain number of commas
  std::string line;
  std::getline(infile, line);
  for (size_t i = 0; i < line.length(); i++) {
    if (line[i] == ',') {
      found_separator = true;
      nb_elements++;
    }
  }

  return (found_separator) ? nb_elements : 0;
}

int csv2mat(const std::string &file_path, const bool header, matx_t &data) {
  // Load file
  std::ifstream infile(file_path);
  if (infile.good() != true) {
    return -1;
  }

  // Obtain number of rows and cols
  int nb_rows = csvrows(file_path);
  int nb_cols = csvcols(file_path);

  // Skip header line?
  std::string line;
  if (header) {
    std::getline(infile, line);
    nb_rows -= 1;
  }

  // Load data
  int line_no = 0;
  std::vector<double> vdata;
  data = zeros(nb_rows, nb_cols);

  while (std::getline(infile, line)) {
    std::istringstream ss(line);

    // Load data row
    std::string element;
    for (int i = 0; i < nb_cols; i++) {
      std::getline(ss, element, ',');
      const double value = atof(element.c_str());
      data(line_no, i) = value;
    }

    line_no++;
  }

  return 0;
}

int mat2csv(const std::string &file_path, const matx_t &data) {
  // Open file
  std::ofstream outfile(file_path);
  if (outfile.good() != true) {
    return -1;
  }

  // Save matrix
  for (int i = 0; i < data.rows(); i++) {
    for (int j = 0; j < data.cols(); j++) {
      outfile << data(i, j);

      if ((j + 1) != data.cols()) {
        outfile << ",";
      }
    }
    outfile << "\n";
  }

  // Close file
  outfile.close();
  return 0;
}

void print_progress(const double percentage) {
  const char *PBSTR =
      "||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||";
  const int PBWIDTH = 60;

  int val = (int) (percentage * 100);
  int lpad = (int) (percentage * PBWIDTH);
  int rpad = PBWIDTH - lpad;
  printf("\r%3d%% [%.*s%*s]", val, lpad, PBSTR, rpad, "");
  fflush(stdout);
}

mat4_t interp_pose(const mat4_t &p0, const mat4_t &p1, const double alpha) {
  // Decompose start pose
  const vec3_t trans0 = tf_trans(p0);
  const quat_t quat0{tf_rot(p0)};

  // Decompose end pose
  const vec3_t trans1 = tf_trans(p1);
  const quat_t quat1{tf_rot(p1)};

  // Interpolate translation and rotation
  const auto trans_interp = lerp(trans0, trans1, alpha);
  const auto quat_interp = quat0.slerp(alpha, quat1);

  return tf(quat_interp, trans_interp);
}

void interp_poses(const std::vector<long> &timestamps,
                  const mat4s_t &poses,
                  const std::vector<long> &interp_ts,
                  mat4s_t &interped_poses,
                  const double threshold) {
  assert(timestamps.size() > 0);
  assert(timestamps.size() == poses.size());
  assert(interp_ts.size() > 0);
  assert(timestamps[0] < interp_ts[0]);

  // Interpolation variables
  long t_start = 0;
  long t_end = 0;
  mat4_t pose0 = I(4);
  mat4_t pose1 = I(4);

  size_t interp_idx = 0;
  for (size_t i = 0; i < timestamps.size(); i++) {
    const long ts = timestamps[i];
    const mat4_t T = poses[i];

    const double diff = (ts - interp_ts[interp_idx]) * 1e-9;
    if (diff < threshold) {
      // Set interpolation start point
      t_start = ts;
      pose0 = T;

    } else if (diff > threshold) {
      // Set interpolation end point
      t_end = ts;
      pose1 = T;

      // Calculate alpha
      const double numerator = (interp_ts[interp_idx] - t_start) * 1e-9;
      const double denominator = (t_end - t_start) * 1e-9;
      const double alpha = numerator / denominator;

      // Interpoate translation and rotation and add to results
      interped_poses.push_back(interp_pose(pose0, pose1, alpha));
      interp_idx++;

      // Shift interpolation current end point to start point
      t_start = t_end;
      pose0 = pose1;

      // Reset interpolation end point
      t_end = 0;
      pose1 = I(4);
    }

    // Check if we're done
    if (interp_idx == interp_ts.size()) {
      break;
    }
  }
}

} //  namespace prototype
