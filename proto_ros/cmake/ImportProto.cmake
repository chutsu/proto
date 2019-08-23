# PROTO_FOUND
# PROTO_INCLUDE_DIRS
# PROTO_LIBRARIES

# Dependencies
FIND_PACKAGE(OpenCV REQUIRED)
FIND_PACKAGE(Ceres REQUIRED)
FIND_PACKAGE(SuiteSparse REQUIRED)
FIND_PACKAGE(OpenMP REQUIRED)
FIND_PACKAGE(GeographicLib 1.34 REQUIRED)
FIND_PACKAGE(Eigen3 REQUIRED)
FIND_PACKAGE(OpenGL REQUIRED)
FIND_PACKAGE(glfw3 REQUIRED)
FIND_PACKAGE(assimp REQUIRED)
FIND_PACKAGE(glm REQUIRED)

# -- glad
# INCLUDE_DIRECTORIES(/usr/local/src/glad/include)
FIND_LIBRARY(GLAD_LIBRARY LIBRARY NAMES glad)
FIND_PATH(GLAD_INCLUDE_DIR glad/glad.h
          HINTS /usr/local/src/glad/include)

# -- imgui
FIND_LIBRARY(IMGUI_LIBRARY LIBRARY NAMES imgui)
FIND_LIBRARY(IMGUI_IMPL_LIBRARY LIBRARY NAMES imgui_impl)
FIND_PATH(IMGUI_INCLUDE_DIR imgui.h
          HINTS /usr/local/src/imgui)
FIND_PATH(IMGUI_IMPL_INCLUDE_DIR imgui_impl_opengl3.h
          HINTS /usr/local/src/imgui/examples)

# Find proto
FIND_LIBRARY(PROTO_LIBRARY LIBRARY NAMES proto)

# Set PROTO_FOUND
IF(PROTO_LIBRARY)
  SET(PROTO_FOUND TRUE)
ELSE ()
  SET(PROTO_FOUND FALSE)
  MESSAGE(FATAL_ERROR "Failed to find libproto!")
ENDIF()

# Set PROTO_LIBRARIES
SET(
  PROTO_LIBRARIES
  ${OpenCV_LIBS}
  ${SUITESPARSE_LIBRARIES}
  ${CERES_LIBRARIES}
  yaml-cpp
  apriltags
  ${GeographicLib_LIBRARIES}
  glad
  glfw
  dl
  ${ASSIMP_LIBRARIES}
  ${OPENGL_gl_LIBRARY}
  imgui
  imgui_impl
  # proto
)
SET(
  PROTO_INCLUDE_DIRS
  ${EIGEN3_INCLUDE_DIR}
  ${SUITESPARSE_INCLUDE_DIRS}
  ${CERES_INCLUDE_DIRS}
  ${GLAD_INCLUDE_DIR}
  ${IMGUI_INCLUDE_DIR}
  ${IMGUI_IMPL_INCLUDE_DIR}
)
