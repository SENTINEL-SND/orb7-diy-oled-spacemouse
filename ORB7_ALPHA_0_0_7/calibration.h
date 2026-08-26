// Header for calibration specific functions and variables
#ifndef CALIBRATION_H
#define CALIBRATION_H

#include "parameterMenu.h"

#if ENABLE_SERIAL_DEBUG
// Debugging outputs are completely compiled out when ENABLE_SERIAL_DEBUG is 0.
// Type signatures strictly mapped to 16-bit to ensure cross-platform ABI alignment and SRAM safety.
void debugOutput1(int16_t* rawReads, uint8_t* keyVals);
void debugOutput2(int16_t* centered);
void debugOutput4(int16_t* velocity, uint8_t* keyOut);
void debugOutput5(int16_t* centered, int16_t* velocity);
void printArray(int16_t* arr, int16_t size);
int16_t calcMinMax(int16_t* centered);

bool isDebugOutputDue();

void updateFrequencyReport();
#else
// Stub definitions guaranteeing zero flash footprint when disabled
#define debugOutput1(rawReads, keyVals) ((void)0)
#define debugOutput2(centered) ((void)0)
#define debugOutput4(velocity, keyOut) ((void)0)
#define debugOutput5(centered, velocity) ((void)0)
#define printArray(arr, size) ((void)0)
#define calcMinMax(centered) (0)
#define isDebugOutputDue() (false)
#define updateFrequencyReport() ((void)0)
#endif

/// @brief Establishes baseline sensor rest states (Zeroing) with safety validation boundaries.
/// @param centerPoints Output array receiving the averaged zero-state coordinates.
/// @param numIterations Number of sampling loops to execute.
/// @param debugFlag Triggers serial evaluation prints if enabled.
/// @return True if successful without triggering mechanical displacement warnings.
bool busyZeroing(int16_t *centerPoints, uint16_t numIterations, bool debugFlag);

#endif // CALIBRATION_H
