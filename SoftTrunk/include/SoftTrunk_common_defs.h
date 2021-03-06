/**
 * @file SoftTrunk_common_defs.h
 * @brief defines various definitions for the Soft Trunk that are used across different files.
 */

#ifndef SOFTTRUNK_SOFTTRUNK_COMMON_DEFS_H
#define SOFTTRUNK_SOFTTRUNK_COMMON_DEFS_H


#include <Eigen/Dense>

/**
 * @brief how many PCC elements there are
 */
#define N_SEGMENTS 2
/**
 * @brief number of chambers in a single segment. 3 or 4 is supported.
 */
#define N_CHAMBERS 4
/**
 * @brief IP address of this computer
 */
#define LOCAL_ADDRESS "192.168.1.111"
/**
 * @brief IP address of computer running Motive
 */
#define MOTIVE_ADDRESS "192.168.1.105"
/**
 * @brief IP address of Festo valves
 */
#define VALVE_ADDRESS "192.168.1.101"
/**
 * @brief baseline pressure of arm. The average of the pressures sent to a segment should be this pressure.
 * for DragonSkin 30, set to 300.
 * for DragonSkin 10, set to 150.
 * (not throughly examined- a larger or smaller value may be better)
 */
#define P_OFFSET 150
/**
 * @brief define max pressure that can be sent out. Useful to prevent puncture of the arm with too high a pressure.
 * for DragonSkin 30, set to 1200.
 * for DragonSkin 10, set to 400.
 * (not throughly examined- a larger or smaller value may be better)
 */
#define MAX_PRESSURE 400
/**
 * @brief mass of each segment, in kg
 */
#define MASSES {0.12, 0.12, 0.12}
/**
 * @brief length of each segment, in m
 */
#define LENGTHS {0.11, 0.11, 0.11}
/**
 * @brief radius of the soft trunk, in meters.
 */
#define R_TRUNK 0.03
/**
 * @brief Maps from index of each chamber to ID of valve array. Should be ordered in: {root stage(first is chamber along x positive axis, other chambers are counted clockwise from that) -> second stage ...}
 */
#define VALVE_MAP {11, 4, 9, 10, 8, 6, 5, 7}

#define PI 3.141592
/**
 * @brief period of one control step, in seconds. must be a value longer than the control loop.
 */
#define CONTROL_PERIOD 0.005

/**
 * @brief defines a matrix of size (NUM_ELEMENTS*2,1), for convenience
 */
typedef Eigen::Matrix<double, N_SEGMENTS * 2, 1> Vector2Nd;
/**
 * @brief defines a matrix of size (NUM_ELEMENTS*2,NUM_ELEMENTS*2), for convenience
 */
typedef Eigen::Matrix<double, N_SEGMENTS * 2, N_SEGMENTS * 2> Matrix2Nd;

// set up ROS automatically if ROS is found
#ifdef CATKIN_FOUND
#define USE_ROS true  // do you want to publish joint states to ROS
#endif
#ifndef CATKIN_FOUND
#define USE_ROS false
#endif

#endif //SOFTTRUNK_SOFTTRUNK_COMMON_DEFS_H
