#ifndef OLED_DISPLAY_H
#define OLED_DISPLAY_H

#include <Arduino.h>
#include "parameterMenu.h"
#include "config.h"

#if ENABLE_OLED

/// @brief Initializes the OLED display and shows the welcome screen.
void initOledDisplay();

/// @brief Renders the visual calibration warning screen during setup.
void showCalibrationScreen();

/// @brief Renders the warning screen if movement is detected during calibration.
void showCalibrationWarningScreen();

/// @brief Renders the visual alert screen when calibration attempts are exhausted.
void showCalibrationFailedScreen();

/// @brief Updates all information displayed on the OLED screen.
/// @param velocity Array of 6 axes coordinates.
/// @param keyState Physical keyboard active levels.
/// @param par Struct holding persistent parameters.
void updateOledDisplay(int16_t* velocity, uint8_t* keyState, ParamData& par);

/// @brief Processes tactile button presses and hold actions.
/// @param keyState Physical button direct array readings.
/// @param par Struct holding persistent parameters.
void processMenuInput(uint8_t* keyState, ParamData& par);

/// @brief Checks if any configuration menu is currently open on the display.
bool isOledMenuOpen();

#else

// Stub implementations to completely bypass compilation overhead when OLED is disabled
inline void initOledDisplay() {}
inline void showCalibrationScreen() {}
inline void showCalibrationWarningScreen() {}
inline void showCalibrationFailedScreen() {}
inline void updateOledDisplay(int16_t* velocity, uint8_t* keyState, ParamData& par) {}
inline void processMenuInput(uint8_t* keyState, ParamData& par) {}
inline bool isOledMenuOpen() { return false; }

#endif // ENABLE_OLED

#endif // OLED_DISPLAY_H