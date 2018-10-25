#include "prototype/core/data.hpp"

namespace prototype {

int csvrows(const std::string &file_path) {
  int nb_rows;
  std::string line;
  std::ifstream infile(file_path);

  // load file
  if (infile.good() != true) {
    printf(E_CSV_DATA_LOAD, file_path.c_str());
    return -1;
  }

  // obtain number of lines
  nb_rows = 0;
  while (std::getline(infile, line)) {
    nb_rows++;
  }

  return nb_rows;
}

int csvcols(const std::string &file_path) {
  int nb_elements;
  std::string line;
  bool found_separator;
  std::ifstream infile(file_path);

  // setup
  nb_elements = 1;
  found_separator = false;

  // load file
  if (infile.good() != true) {
    printf(E_CSV_DATA_LOAD, file_path.c_str());
    return -1;
  }

  // obtain number of commas
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
  int line_no;
  int nb_rows;
  int nb_cols;
  std::string line;
  std::ifstream infile(file_path);
  std::vector<double> vdata;
  std::string element;
  double value;

  // load file
  if (infile.good() != true) {
    return -1;
  }

  // obtain number of rows and cols
  nb_rows = csvrows(file_path);
  nb_cols = csvcols(file_path);

  // header line?
  if (header) {
    std::getline(infile, line);
    nb_rows -= 1;
  }

  // load data
  line_no = 0;
  data.resize(nb_rows, nb_cols);
  while (std::getline(infile, line)) {
    std::istringstream ss(line);

    // load data row
    for (int i = 0; i < nb_cols; i++) {
      std::getline(ss, element, ',');
      value = atof(element.c_str());
      data(line_no, i) = value;
    }

    line_no++;
  }

  return 0;
}

int mat2csv(const std::string &file_path, const matx_t &data) {
  std::ofstream outfile(file_path);

  // open file
  if (outfile.good() != true) {
    printf(E_CSV_DATA_OPEN, file_path.c_str());
    return -1;
  }

  // save matrix
  for (int i = 0; i < data.rows(); i++) {
    for (int j = 0; j < data.cols(); j++) {
      outfile << data(i, j);

      if ((j + 1) != data.cols()) {
        outfile << ",";
      }
    }
    outfile << "\n";
  }

  // close file
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

} //  namespace prototype
