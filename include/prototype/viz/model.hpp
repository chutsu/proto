#ifndef MODEL_HPP
#define MODEL_HPP

#include <glad/glad.h>
#include <glm/glm.hpp>

#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>

#include "play/common.hpp"
#include "play/mesh.hpp"
#include "play/shader.hpp"
#include "play/texture.hpp"
#include "play/camera.hpp"

#include <string>
#include <fstream>
#include <sstream>
#include <iostream>
#include <map>
#include <vector>

struct glmodel_t {
  glprog_t program;

  std::vector<gltexture_t> textures_loaded;
  std::vector<glmesh_t> meshes;
  std::string directory;
  bool gamma_correction = false;

  glm::mat4 T_SM = glm::mat4(1.0f);

  glmodel_t(const std::string &path,
            const std::string &vs,
            const std::string &fs,
            bool gamma = false);
};

void glmodel_draw(glmodel_t &model, const glcamera_t &camera);
void glmodel_load(glmodel_t &model, const std::string &path);
void glmodel_process_node(glmodel_t &model, aiNode *node, const aiScene *scene);
glmesh_t glmodel_process_mesh(glmodel_t &model, aiMesh *mesh, const aiScene *scene);
std::vector<gltexture_t> glmodel_load_textures(glmodel_t &model,
                                               aiMaterial *mat,
                                               aiTextureType type,
                                               std::string typeName);
unsigned int texture_from_file(const std::string &dir,
                               const char *fp,
                               bool gamma = false);

#endif // MODEL_HPP
