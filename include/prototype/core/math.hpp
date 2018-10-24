/**
 * @file
 * @defgroup math math
 * @ingroup util
 */
#ifndef PROTOTYPE_CORE_MATH_HPP
#define PROTOTYPE_CORE_MATH_HPP

#include <iostream>
#include <sstream>
#include <string>
#include <vector>

#include <Eigen/Dense>
#include <Eigen/Geometry>

#include "prototype/core/log.hpp"

namespace prototype {
/**
 * @addtogroup math
 * @{
 */

#ifndef __EIGEN_TYPEDEF__
#define __EIGEN_TYPEDEF__
typedef Eigen::Vector2d Vec2;
typedef Eigen::Vector3d Vec3;
typedef Eigen::Vector4d Vec4;
typedef Eigen::Matrix<double, 5, 1> Vec5;
typedef Eigen::Matrix<double, 6, 1> Vec6;
typedef Eigen::VectorXd VecX;

typedef Eigen::Matrix2d Mat2;
typedef Eigen::Matrix3d Mat3;
typedef Eigen::Matrix4d Mat4;
typedef Eigen::MatrixXd MatX;
typedef Eigen::Matrix<double, 3, 4> Mat34;

typedef Eigen::Quaterniond Quaternion;
#endif

/**
 * Print shape of a matrix
 *
 * @param name Name of matrix
 * @param A Matrix
 */
void print_shape(const std::string &name, const MatX &A);

/**
 * Print shape of a vector
 *
 * @param name Name of vector
 * @param v Vector
 */
void print_shape(const std::string &name, const VecX &v);

/**
 * Print array
 *
 * @param name Name of array
 * @param array Target array
 * @param size Size of target array
 */
void print_array(const std::string &name,
                 const double *array,
                 const size_t size);

/**
 * Array to string
 *
 * @param array Target array
 * @param size Size of target array
 * @returns String of array
 */
std::string array2str(const double *array, const size_t size);

/**
 * Convert double array to Eigen::Vector
 *
 * @param x Input array
 * @param size Size of input array
 * @param y Output vector
 */
void array2vec(const double *x, const size_t size, VecX &y);

/**
 * Vector to array
 *
 * @param v Vector
 * @returns Array
 */
double *vec2array(const VecX &v);

/**
 * Matrix to array
 *
 * @param m Matrix
 * @returns Array
 */
double *mat2array(const MatX &m);

/**
 * Vector to array
 *
 * @param v Vector
 * @param out Output array
 */
void vec2array(const VecX &v, double *out);

/**
 * Matrix to array
 *
 * @param m Matrix
 * @param out Output array
 */
void mat2array(const MatX &m, double *out);

/**
 * Matrix to list of vectors
 *
 * @param m Matrix
 * @returns Vectors
 */
std::vector<VecX> mat2vec(const MatX &m, bool row_wise = true);

/**
 * Matrix to list of vectors of size 3
 *
 * @param m Matrix
 * @returns Vectors
 */
std::vector<Vec3> mat2vec3(const MatX &m, bool row_wise = true);

/**
 * Matrix to list of vectors of size 3
 *
 * @param m Matrix
 * @returns Vectors
 */
std::vector<Vec2> mat2vec2(const MatX &m, bool row_wise = true);

/**
 * Vector to string
 *
 * @param v Vector
 * @param brackets Brakcets around vector string
 *
 * @returns Vector as a string
 */
std::string vec2str(const VecX &v, bool brackets=true);

/**
 * Create random integer
 *
 * @param ub Upper bound
 * @param lb Lower bound
 * @return Random integer
 */
int randi(const int ub, const int lb);

/**
 * Create random double
 *
 * @param ub Upper bound
 * @param lb Lower bound
 * @return Random floating point
 */
double randf(const double ub, const double lb);

/**
 * Sign of number
 *
 * @param x Number to check sign
 * @return
 *    - 0: Number is zero
 *    - 1: Positive number
 *    - -1: Negative number
 */
int sign(const double x);

/**
 * Floating point comparator
 *
 * @param f1 First value
 * @param f2 Second value
 * @return
 *    - 0: if equal
 *    - 1: if f1 > f2
 *    - -1: if f1 < f2
 */
int fltcmp(const double f1, const double f2);

/**
 * Calculate median given an array of numbers
 *
 * @param v Array of numbers
 * @return Median of given array
 */
double median(const std::vector<double> &v);

/**
 * Degrees to radians
 *
 * @param d Degree to be converted
 * @return Degree in radians
 */
double deg2rad(const double d);

/**
 * Radians to degree
 *
 * @param r Radian to be converted
 * @return Radian in degrees
 */
double rad2deg(const double r);

/**
 * Load std::vector of doubles to an Eigen::Matrix
 *
 * @param x Matrix values
 * @param rows Number of matrix rows
 * @param cols Number of matrix colums
 * @param y Output matrix
 */
void load_matrix(const std::vector<double> &x,
                 const int rows,
                 const int cols,
                 MatX &y);

/**
 * Load an Eigen::Matrix into a std::vector of doubles
 *
 * @param A Matrix
 * @param x Output vector of matrix values
 */
void load_matrix(const MatX A, std::vector<double> &x);

/**
 * Calculate binomial coefficient
 *
 * @param n
 * @param k
 * @returns Binomial coefficient
 */
double binomial(const double n, const double k);

/**
 * Wrap angle in degrees to 180
 *
 * @param d Degrees
 * @return Angle wraped to 180
 */
double wrapTo180(const double d);

/**
 * Wrap angle in degrees to 360
 *
 * @param d Degrees
 * @return Angle wraped to 360
 */
double wrapTo360(const double d);

/**
 * Wrap angle in radians to PI
 *
 * @param r Radians
 * @return Angle wraped to PI
 */
double wrapToPi(const double r);

/**
 * Wrap angle in radians to 2 PI
 *
 * @param r Radians
 * @return Angle wraped to 2 PI
 */
double wrapTo2Pi(const double r);

/**
 * Mean vector
 *
 * @param x List of vectors
 * @return Mean vector
 */
Vec3 mean(const std::vector<Vec3> &x);

/**
 * Cross-Track error based on waypoint line between p1, p2, and robot position
 *
 * @param p1 Waypoint 1
 * @param p2 Waypoint 2
 * @param pos Robot position
 * @return Cross track error
 */
double cross_track_error(const Vec2 &p1, const Vec2 &p2, const Vec2 &pos);

/**
 * Check if point `pos` is left or right of line formed by `p1` and `p2`
 *
 * @param p1 Waypoint 1
 * @param p2 Waypoint 2
 * @param pos Robot position
 * @returns
 *    - 1: Point is left of waypoint line formed by `p1` and `p2`
 *    - 2: Point is right of waypoint line formed by `p1` and `p2`
 *    - 0: Point is colinear with waypoint line formed by `p1` and `p2`
 */
int point_left_right(const Vec2 &p1, const Vec2 &p2, const Vec2 &pos);

/**
 * Calculate closest point given waypoint line between `p1`, `p2` and robot
 * position
 *
 * @param p1 Waypoint 1
 * @param p2 Waypoint 2
 * @param p3 Robot position
 * @param closest Closest point
 * @returns
 *    Unit number denoting where the closest point is on waypoint line. For
 *    example, a return value of 0.5 denotes the closest point is half-way
 *    (50%) of the waypoint line, alternatively a negative number denotes the
 *    closest point is behind the first waypoint.
 */
double
closest_point(const Vec2 &p1, const Vec2 &p2, const Vec2 &p3, Vec2 &closest);

/**
 * Linear interpolation between two points
 *
 * @param a First point
 * @param b Second point
 * @param mu Unit number
 */
Vec2 lerp(const Vec2 &a, const Vec2 &b, const double mu);

/** @} group math */
} //  namespace prototype
#endif // PROTOTYPE_CORE_MATH_HPP
