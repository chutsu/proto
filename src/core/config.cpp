#include "prototype/core/config.hpp"

namespace prototype {

ConfigParser::ConfigParser() { this->config_loaded = false; }

void ConfigParser::addParam(const std::string &key,
                            bool *out,
                            const bool optional) {
  this->params.emplace_back(BOOL, key, out, optional);
}

void ConfigParser::addParam(const std::string &key,
                            int *out,
                            const bool optional) {
  this->params.emplace_back(INT, key, out, optional);
}

void ConfigParser::addParam(const std::string &key,
                            float *out,
                            const bool optional) {
  this->params.emplace_back(FLOAT, key, out, optional);
}

void ConfigParser::addParam(const std::string &key,
                            double *out,
                            const bool optional) {
  this->params.emplace_back(DOUBLE, key, out, optional);
}

void ConfigParser::addParam(const std::string &key,
                            std::string *out,
                            const bool optional) {
  this->params.emplace_back(STRING, key, out, optional);
}

void ConfigParser::addParam(const std::string &key,
                            std::vector<bool> *out,
                            const bool optional) {
  this->params.emplace_back(BOOL_ARRAY, key, out, optional);
}

void ConfigParser::addParam(const std::string &key,
                            std::vector<int> *out,
                            const bool optional) {
  this->params.emplace_back(INT_ARRAY, key, out, optional);
}

void ConfigParser::addParam(const std::string &key,
                            std::vector<float> *out,
                            const bool optional) {
  this->params.emplace_back(FLOAT_ARRAY, key, out, optional);
}

void ConfigParser::addParam(const std::string &key,
                            std::vector<double> *out,
                            const bool optional) {
  this->params.emplace_back(DOUBLE_ARRAY, key, out, optional);
}

void ConfigParser::addParam(const std::string &key,
                            std::vector<std::string> *out,
                            const bool optional) {
  this->params.emplace_back(STRING_ARRAY, key, out, optional);
}

void ConfigParser::addParam(const std::string &key,
                            Vec2 *out,
                            const bool optional) {
  this->params.emplace_back(VEC2, key, out, optional);
}

void ConfigParser::addParam(const std::string &key,
                            Vec3 *out,
                            const bool optional) {
  this->params.emplace_back(VEC3, key, out, optional);
}

void ConfigParser::addParam(const std::string &key,
                            Vec4 *out,
                            const bool optional) {
  this->params.emplace_back(VEC4, key, out, optional);
}

void ConfigParser::addParam(const std::string &key,
                            VecX *out,
                            const bool optional) {
  this->params.emplace_back(VECX, key, out, optional);
}

void ConfigParser::addParam(const std::string &key,
                            Mat2 *out,
                            const bool optional) {
  this->params.emplace_back(MAT2, key, out, optional);
}

void ConfigParser::addParam(const std::string &key,
                            Mat3 *out,
                            const bool optional) {
  this->params.emplace_back(MAT3, key, out, optional);
}

void ConfigParser::addParam(const std::string &key,
                            Mat4 *out,
                            const bool optional) {
  this->params.emplace_back(MAT4, key, out, optional);
}

void ConfigParser::addParam(const std::string &key,
                            MatX *out,
                            const bool optional) {
  this->params.emplace_back(MATX, key, out, optional);
}

void ConfigParser::addParam(const std::string &key,
                            cv::Mat *out,
                            const bool optional) {
  this->params.emplace_back(CVMAT, key, out, optional);
}

int ConfigParser::getYamlNode(const std::string &key,
                              const bool optional,
                              YAML::Node &node) {
  ASSERT(this->config_loaded == true, "Config file is not loaded!");
  std::string element;
  std::istringstream iss(key);
  std::vector<YAML::Node> traversal;

  // recurse down config key
  traversal.push_back(this->root);
  while (std::getline(iss, element, '.')) {
    traversal.push_back(traversal.back()[element]);
  }
  node = traversal.back();
  // Note:
  //
  //    yaml_node = yaml_node["some_level_deeper"];
  //
  // YAML::Node is mutable, by doing the above it destroys the parsed yaml
  // tree/graph, to avoid this problem we store the visited YAML::Node into
  // a std::vector and return the last visited YAML::Node

  // check key
  if (!node && optional == false) {
    LOG_ERROR("Opps [%s] missing in yaml file [%s]!",
              key.c_str(),
              this->file_path.c_str());
    return KEY_NOT_FOUND;
  } else if (!node && optional == true) {
    return OPTIONAL_KEY_NOT_FOUND;
  }

  return SUCCESS;
}

int ConfigParser::checkVector(const std::string &key,
                              const enum ConfigDataType type,
                              const bool optional,
                              YAML::Node &node) {
  ASSERT(this->config_loaded == true, "Config file is not loaded!");

  // check key
  int retval = this->getYamlNode(key, optional, node);
  if (retval != SUCCESS) {
    return retval;
  }

  int vector_size;
  switch (type) {
    case VEC2: vector_size = 2; break;
    case VEC3: vector_size = 3; break;
    case VEC4: vector_size = 4; break;
    default: return SUCCESS;
  }

  // check number of values
  if (node.size() != static_cast<size_t>(vector_size)) {
    LOG_ERROR("Vector [%s] should have %d values but config has %d!",
              key.c_str(),
              vector_size,
              static_cast<int>(node.size()));
    return INVALID_VECTOR;
  }

  return 0;
}

int ConfigParser::checkMatrix(const std::string &key,
                              const bool optional,
                              YAML::Node &node) {
  ASSERT(this->config_loaded == true, "Config file is not loaded!");

  // check key
  int retval = this->getYamlNode(key, optional, node);
  if (retval != SUCCESS) {
    return retval;
  }

  // check fields
  const std::string targets[3] = {"rows", "cols", "data"};
  for (int i = 0; i < 3; i++) {
    if (!node[targets[i]]) {
      LOG_ERROR("Key [%s] is missing for matrix [%s]!",
                targets[i].c_str(),
                key.c_str());
      return INVALID_MATRIX;
    }
  }

  return SUCCESS;
}

int ConfigParser::loadPrimitive(ConfigParam &param) {
  ASSERT(this->config_loaded == true, "Config file is not loaded!");

  // Pre-check
  YAML::Node node;
  int retval = this->getYamlNode(param.key, param.optional, node);
  if (retval != SUCCESS) {
    return retval;
  }

  // parse
  switch (param.type) {
    case BOOL: *static_cast<bool *>(param.data) = node.as<bool>(); break;
    case INT: *static_cast<int *>(param.data) = node.as<int>(); break;
    case FLOAT: *static_cast<float *>(param.data) = node.as<float>(); break;
    case DOUBLE: *static_cast<double *>(param.data) = node.as<double>(); break;
    case STRING:
      *static_cast<std::string *>(param.data) = node.as<std::string>();
      break;
    default: return INVALID_TYPE;
  }

  return SUCCESS;
}

int ConfigParser::loadArray(ConfigParam &param) {
  ASSERT(this->config_loaded == true, "Config file is not loaded!");

  // check parameters
  YAML::Node node;
  int retval = this->getYamlNode(param.key, param.optional, node);
  if (retval != SUCCESS) {
    return retval;
  }

  // parse
  switch (param.type) {
    case BOOL_ARRAY:
      for (auto n : node) {
        static_cast<std::vector<bool> *>(param.data)->push_back(n.as<bool>());
      }
      break;
    case INT_ARRAY:
      for (auto n : node) {
        static_cast<std::vector<int> *>(param.data)->push_back(n.as<int>());
      }
      break;
    case FLOAT_ARRAY:
      for (auto n : node) {
        static_cast<std::vector<float> *>(param.data)->push_back(n.as<float>());
      }
      break;
    case DOUBLE_ARRAY:
      for (auto n : node) {
        static_cast<std::vector<double> *>(param.data)
            ->push_back(n.as<double>());
      }
      break;
    case STRING_ARRAY:
      for (auto n : node) {
        static_cast<std::vector<std::string> *>(param.data)
            ->push_back(n.as<std::string>());
      }
      break;
    default: return INVALID_TYPE;
  }

  return 0;
}

int ConfigParser::loadVector(ConfigParam &param) {
  ASSERT(this->config_loaded == true, "Config file is not loaded!");

  // check parameter
  YAML::Node node;
  int retval = this->checkVector(param.key, param.type, param.optional, node);
  if (retval != SUCCESS) {
    return retval;
  }

  // parse
  switch (param.type) {
    case VEC2:
      *static_cast<Vec2 *>(param.data) << node[0].as<double>(),
          node[1].as<double>();
      break;

    case VEC3:
      *static_cast<Vec3 *>(param.data) << node[0].as<double>(),
          node[1].as<double>(), node[2].as<double>();
      break;

    case VEC4:
      *static_cast<Vec4 *>(param.data) << node[0].as<double>(),
          node[1].as<double>(), node[2].as<double>(), node[3].as<double>();
      break;

    case VECX: {
      VecX &vecx = *static_cast<VecX *>(param.data);
      vecx = VecX((int) node.size());
      for (size_t i = 0; i < node.size(); i++) {
        vecx(i) = node[i].as<double>();
      }
    } break;

    default: return INVALID_TYPE;
  }

  return SUCCESS;
}

int ConfigParser::loadMatrix(ConfigParam &param) {
  ASSERT(this->config_loaded == true, "Config file is not loaded!");

  // check parameter
  YAML::Node node;
  int retval = this->checkMatrix(param.key, param.optional, node);
  if (retval != SUCCESS) {
    return retval;
  }

  // parse
  int index = 0;
  int rows = node["rows"].as<int>();
  int cols = node["cols"].as<int>();

  switch (param.type) {
    case MAT2: {
      Mat2 &mat2 = *static_cast<Mat2 *>(param.data);
      mat2(0, 0) = node["data"][0].as<double>();
      mat2(0, 1) = node["data"][1].as<double>();

      mat2(1, 0) = node["data"][2].as<double>();
      mat2(1, 1) = node["data"][3].as<double>();
    } break;

    case MAT3: {
      Mat3 &mat3 = *static_cast<Mat3 *>(param.data);
      mat3(0, 0) = node["data"][0].as<double>();
      mat3(0, 1) = node["data"][1].as<double>();
      mat3(0, 2) = node["data"][2].as<double>();

      mat3(1, 0) = node["data"][3].as<double>();
      mat3(1, 1) = node["data"][4].as<double>();
      mat3(1, 2) = node["data"][5].as<double>();

      mat3(2, 0) = node["data"][6].as<double>();
      mat3(2, 1) = node["data"][7].as<double>();
      mat3(2, 2) = node["data"][8].as<double>();
    } break;

    case MAT4: {
      Mat4 &mat4 = *static_cast<Mat4 *>(param.data);
      for (int i = 0; i < rows; i++) {
        for (int j = 0; j < cols; j++) {
          mat4(i, j) = node["data"][index].as<double>();
          index++;
        }
      }
    } break;

    case MATX: {
      MatX &matx = *static_cast<MatX *>(param.data);
      matx.resize(rows, cols);
      for (int i = 0; i < rows; i++) {
        for (int j = 0; j < cols; j++) {
          matx(i, j) = node["data"][index].as<double>();
          index++;
        }
      }
    } break;

    case CVMAT: {
      cv::Mat &cvmat = *static_cast<cv::Mat *>(param.data);
      cvmat = cv::Mat(rows, cols, CV_64F);
      for (int i = 0; i < rows; i++) {
        for (int j = 0; j < cols; j++) {
          cvmat.at<double>(i, j) = node["data"][index].as<double>();
          index++;
        }
      }
    } break;
    default: return INVALID_TYPE;
  }

  return SUCCESS;
}

int ConfigParser::load(const std::string &config_file) {
  // Pre-check
  if (file_exists(config_file) == false) {
    LOG_ERROR("File not found: %s", config_file.c_str());
    return CONFIG_NOT_FOUND;
  }

  // load and parse file
  this->file_path = config_file;
  this->root = YAML::LoadFile(config_file);
  this->config_loaded = true;

  int retval;
  for (size_t i = 0; i < this->params.size(); i++) {
    switch (this->params[i].type) {
      // PRIMITIVE
      case BOOL:
      case INT:
      case FLOAT:
      case DOUBLE:
      case STRING:
        retval = this->loadPrimitive(this->params[i]);
        break;
      // ARRAY
      case BOOL_ARRAY:
      case INT_ARRAY:
      case FLOAT_ARRAY:
      case DOUBLE_ARRAY:
      case STRING_ARRAY:
        retval = this->loadArray(this->params[i]);
        break;
      // VECTOR
      case VEC2:
      case VEC3:
      case VEC4:
      case VECX:
        retval = this->loadVector(this->params[i]);
        break;
      // MAT
      case MAT2:
      case MAT3:
      case MAT4:
      case MATX:
      case CVMAT: retval = this->loadMatrix(this->params[i]); break;
      default: return INVALID_TYPE;
    }

    if (retval != SUCCESS && retval != OPTIONAL_KEY_NOT_FOUND) {
      return retval;
    }
  }

  return SUCCESS;
}

} //  namespace prototype
