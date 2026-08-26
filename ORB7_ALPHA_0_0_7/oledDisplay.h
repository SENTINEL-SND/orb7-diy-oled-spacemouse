// Header for OLED Display specific functions and UI state management.
// Handles the direct hardware interface with the SSD1306 controller.

#ifndef OLED_DISPLAY_H
#define OLED_DISPLAY_H

#include <Arduino.h>
#include "parameterMenu.h"
#include "config.h"

#if ENABLE_OLED

/// @brief Initializes the OLED display, configures the I2C bus at 400kHz, and shows the boot splash screen.
void initOledDisplay();

/// @brief Renders the visual calibration warning screen demanding the user to keep the knob at rest.
void showCalibrationScreen();

/// @brief Renders the warning screen if mechanical movement is detected during the boot-time calibration.
void showCalibrationWarningScreen();

/// @brief Renders the visual alert screen when boot calibration attempts are exhausted, notifying bypass.
void showCalibrationFailedScreen();

/// @brief Core UI rendering orchestrator. Updates all information displayed on the OLED screen.
/// @param velocity Array of the 6 calculated kinematic axes.
/// @param keyState Physical hardware button active levels.
/// @param par Structural reference to persistent system parameters.
void updateOledDisplay(int16_t* velocity, uint8_t* keyState, ParamData& par);

/// @brief Evaluates tactile button presses and hold actions to navigate the UI state machine.
/// @param keyState Physical button direct array readings.
/// @param par Structural reference to persistent system parameters.
void processMenuInput(uint8_t* keyState, ParamData& par);

/// @brief Checks if any configuration menu is currently open on the display.
/// Used to suppress kinematic 3D output while the user is actively editing settings.
/// @return True if a submenu is active, False if on the home screen.
bool isOledMenuOpen();

#else

// Stub implementations to completely bypass compilation overhead when the OLED feature is disabled in config.h.
// Ensures zero Flash memory footprint for UI logic when ENABLE_OLED is 0.
inline void initOledDisplay() {}
inline void showCalibrationScreen() {}
inline void showCalibrationWarningScreen() {}
inline void showCalibrationFailedScreen() {}
inline void updateOledDisplay(int16_t* velocity, uint8_t* keyState, ParamData& par) {}
inline void processMenuInput(uint8_t* keyState, ParamData& par) {}
inline bool isOledMenuOpen() { return false; }

#endif // ENABLE_OLED

#endif // OLED_DISPLAY_H