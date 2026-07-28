// Header for calibration specific functions and variables
#ifndef CALIBRATION_H
#define CALIBRATION_H

#include "parameterMenu.h"

#if ENABLE_SERIAL_DEBUG
// FIXED: Signature updated to use strict int16_t and uint8_t types for 32-bit portability and consistency [1, 2]
void debugOutput1(int16_t* rawReads, uint8_t* keyVals);
void debugOutput2(int16_t* centered);
void debugOutput4(int16_t* velocity, uint8_t* keyOut);
void debugOutput5(int16_t* centered, int16_t* velocity);
void debugOutputOffsets(int16_t* offset);
void debugDriftPlotter(int16_t* raw, int16_t* centered, int16_t* offset, int16_t axis);

void printArray(int16_t* arr, int16_t size);
int16_t calcMinMax(int16_t* centered);

bool isDebugOutputDue();

void updateFrequencyReport();
#else
#define debugOutput1(rawReads, keyVals) ((void)0)
#define debugOutput2(centered) ((void)0)
#define debugOutput4(velocity, keyOut) ((void)0)
#define debugOutput5(centered, velocity) ((void)0)
#define debugOutputOffsets(offset) ((void)0)
#define debugDriftPlotter(raw, centered, offset, axis) ((void)0)
#define printArray(arr, size) ((void)0)
#define calcMinMax(centered) (0)
#define isDebugOutputDue() (false)
#define updateFrequencyReport() ((void)0)
#endif

// FIXED: Parameters updated to standard C++ bool type and explicit int16_t sizing [2]
bool busyZeroing(int16_t *centerPoints, uint16_t numIterations, bool debugFlag);

void compensateDrifts(int16_t *raw, int16_t *center, int16_t *offset, ParamData& par);

#endif // CALIBRATION_H