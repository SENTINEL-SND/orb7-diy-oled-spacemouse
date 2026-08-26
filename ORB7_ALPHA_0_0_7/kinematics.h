// This is the public header for the kinematics.cpp file
// It contains all functions and axis definitions which can be called from the main application.

#ifndef KINEMATICS_H
#define KINEMATICS_H

#include "parameterMenu.h"

/// @brief Applies configured mathematical curves (Linear, Squared, Squared Tangent) to kinematic values.
/// @param x Input raw velocity (-350 to +350)
/// @param par Global parameters struct containing the selected curve modifiers
/// @return Processed output velocity mapped symmetrically
int modifierFunction(int x, ParamData& par);

/// @brief Samples hardware ADC pins using a small arithmetic oversampling average.
/// @param rawReads Pointer to output array for the 8 sensor readings
void readAllFromJoystick(int16_t *rawReads);

/// @brief Maps centered sensor deltas through their individual min/max calibration bounds.
/// @param centered Array of 8 normalized sensor center deltas
/// @param par Global parameters containing the boundary definitions
void NormalizeAnalogReadOuts(int16_t* centered, ParamData& par);

/// @brief Processes the 6DOF matrix multiplication, axes inversions, and sensitivity scaling.
/// @param centered Normalized sensor data array
/// @param velocity Output array of calculated 6DOF velocity signals
/// @param par Global parameters for scales, gates, and inversions
void calculateKinematic(int16_t* centered, int16_t* velocity, ParamData& par);

/// @brief Swaps the X and Y translational and rotational axes
void switchXY(int16_t *velocity);

/// @brief Swaps the Y and Z translational and rotational axes
void switchYZ(int16_t *velocity);

/// @brief Enforces Exclusive Mode (Translation OR Rotation dominance) with a hysteresis lock and neutral relaxation.
void exclusiveMode(int16_t *velocity, int16_t hysteresis);

// The following constants provide readable index access to the sensor arrays.
// Standard resistive joystick axes array offsets
#define AX 0
#define AY 1
#define BX 2
#define BY 3
#define CX 4
#define CY 5
#define DX 6
#define DY 7

// SECTION HALLEFFECT
// Hardware sensor mapping offsets for Hall Effect matrices
#define HES0 0
#define HES1 1
#define HES2 2
#define HES3 3
#define HES6 4
#define HES7 5
#define HES8 6
#define HES9 7
// !SECTION HALLEFFECT

// Array offsets for the final output velocity payload
#define TRANSX 0
#define TRANSY 1
#define TRANSZ 2
#define ROTX   3
#define ROTY   4
#define ROTZ   5

#endif // KINEMATICS_H
