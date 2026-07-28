#include <Arduino.h>
#include <Wire.h> // Included to support global Wire timeout configurations

// The user specific settings, like pin mappings or special configuration variables and
// sensitivities are stored in config.h. Please open config_sample.h, adjust your settings and save
// it as config.h
#include "config.h"
#include "parameterMenu.h"

// Include inbuilt Arduino HID library by NicoHood: https://github.com/NicoHood/HID
#include "HID.h"

// header file for calibration output and helper routines
#include "calibration.h"
#include "calibrationChecks.h"

// header to calculate the kinematics of the mouse
#include "kinematics.h"

// header file for reading the keys
#include "spaceKeys.h"

// header for HID emulation of the spacemouse
#include "SpaceMouseHID.h"

// Header for the OLED display
#include "oledDisplay.h"

void setup();
void loop();
#ifdef HALLEFFECT
void setAnalogReferenceVoltage(int dbg);
#endif

// FIXED: Variables globally promoted to strict int16_t for 32-bit portability and consistency [2]
int16_t rawReads[8];
int16_t centered[8];
int16_t centerPoints[8];
int16_t offsets[8];

// Resulting calculated velocities / movements
// int16_t to match what the HID protocol expects.
int16_t velocity[6];

// global parameters (also stored in EEPROM)
ParamStorage parStorage;

// CONDITIONAL STRUCT INITIALIZATION
// Cuts sizeof(ParamData) from 542 bytes to 2 bytes when serial debugging is disabled (Saves massive Flash)
#if ENABLE_SERIAL_DEBUG
ParamData par = {.values = &parStorage,
                 .description = {
                     {PARAM_TYPE_BOOL, "", NULL},                        // param 0 is unused
                     {PARAM_TYPE_INT, "DEADZONE", &parStorage.deadzone}, //       1
                     {PARAM_TYPE_INT, "SENS_TX", &parStorage.transX_sensitivity_q7},      //       2
                     {PARAM_TYPE_INT, "SENS_TY", &parStorage.transY_sensitivity_q7},      //       3
                     {PARAM_TYPE_INT, "SENS_PTZ", &parStorage.pos_transZ_sensitivity_q7}, //       4
                     {PARAM_TYPE_INT, "SENS_NTZ", &parStorage.neg_transZ_sensitivity_q7}, //       5
                     {PARAM_TYPE_INT, "GATE_NTZ", &parStorage.gate_neg_transZ},          //       6
                     {PARAM_TYPE_INT, "GATE_RX", &parStorage.gate_rotX},                 //       7
                     {PARAM_TYPE_INT, "GATE_RY", &parStorage.gate_rotY},                 //       8
                     {PARAM_TYPE_INT, "GATE_RZ", &parStorage.gate_rotZ},                 //       9
                     {PARAM_TYPE_INT, "SENS_RX", &parStorage.rotX_sensitivity_q7},        //      10
                     {PARAM_TYPE_INT, "SENS_RY", &parStorage.rotY_sensitivity_q7},        //      11
                     {PARAM_TYPE_INT, "SENS_RZ", &parStorage.rotZ_sensitivity_q7},        //      12
                     {PARAM_TYPE_INT, "MODFUNC", &parStorage.modFunc},                   //      13
                     {PARAM_TYPE_INT, "MOD_A", &parStorage.slope_at_zero_q8},            //      14
                     {PARAM_TYPE_INT, "MOD_B", &parStorage.slope_at_end_q8},             //      15  <-- FIXED: MOD_B restored dynamically [5]
                     {PARAM_TYPE_BOOL, "INVX", &parStorage.invX},                        //      16  <-- Shifted indices
                     {PARAM_TYPE_BOOL, "INVY", &parStorage.invY},                        //      17
                     {PARAM_TYPE_BOOL, "INVZ", &parStorage.invZ},                        //      18
                     {PARAM_TYPE_BOOL, "INVRX", &parStorage.invRX},                      //      19
                     {PARAM_TYPE_BOOL, "INVRY", &parStorage.invRY},                      //      20
                     {PARAM_TYPE_BOOL, "INVRZ", &parStorage.invRZ},                      //      21
                     {PARAM_TYPE_BOOL, "SWITCHXY", &parStorage.switchXY},                //      22
                     {PARAM_TYPE_BOOL, "SWITCHYZ", &parStorage.switchYZ},                //      23
                     {PARAM_TYPE_BOOL, "EXCLUSIVE", &parStorage.exclusiveMode},          //      24
                     {PARAM_TYPE_INT, "EXCL_HYST", &parStorage.exclusiveHysteresis},     //      25
                     {PARAM_TYPE_BOOL, "EXCL_PRIOZ", &parStorage.prioZexclusiveMode},    //      26
                     {PARAM_TYPE_BOOL, "COMP_EN", &parStorage.compEnabled},              //      27
                     {PARAM_TYPE_INT, "COMP_NR", &parStorage.compNoOfPoints},            //      28
                     {PARAM_TYPE_INT, "COMP_WAIT", &parStorage.compWaitTime},            //      29
                     {PARAM_TYPE_INT, "COMP_MDIFF", &parStorage.compMinMaxDiff},         //      30
                     {PARAM_TYPE_INT, "COMP_CDIFF", &parStorage.compCenterDiff},         //      31
                     {PARAM_TYPE_INT, "GLB_SENS", &parStorage.globalSens}                //      32
                 }};
#else
ParamData par = {.values = &parStorage};
#endif

// FIXED: Replaced standard int type array with uint8_t to prevent SRAM waste [6]
uint8_t keyVals[NUMKEYS];

// key event, after debouncing. It is 1 only for a single sample
uint8_t keyOut[NUMKEYS];

// state of the key, which stays 1 as long as the key is pressed
uint8_t keyState[NUMKEYS];

/**
 * @brief Setup the SpaceMouse, called by system-start
 */
void setup() {
  // FIXED: Set ADC prescaler to 32 (ADPS2=1, ADPS1=0, ADPS0=1) to double clock speed to 500 kHz
  // This reduces physical analog conversion time per read from 52 us to only 26 us safely!
  ADCSRA = (ADCSRA & 0xF8) | 0x05;

// Get parameters from EEPROM
#if PARAM_IN_EEPROM > 0
  getParametersFromEEPROM(par);
#endif

// setup the keys e.g. to internal pull-ups
#if NUMKEYS > 0
  setupKeys();
#endif

#ifdef HALLEFFECT
  // Set the ADC reference voltage to 2,56V if HALLEFFECT is defined, 5V otherwise.
  // It is important the reference Voltage is set before the Zeroing of the sensors is executed.
  setAnalogReferenceVoltage(0);
#endif

#if ENABLE_SERIAL_DEBUG
  // Setup Serial only if serial interface parser is compiled (timeout purged to prevent blockages)
  Serial.begin(115200);
#endif

  // 1. Play boot-screen splash loading animation (Initializes Wire I2C internally)
  initOledDisplay();

  // 2. Clear display and show visual instruction to keep hands off during calibration
  showCalibrationScreen();

  // 3. Calibrate centers in background
  // Enforce loop-protection only if serial debug is disabled. This bypasses
  // the catch-22 lock when calibrating a completely uncalibrated magnet plate.
#if ENABLE_SERIAL_DEBUG
  busyZeroing(centerPoints, 1000, true);
#else
  uint8_t attempts = 0;
  bool calSuccess = false;
  while (attempts < 3) {
    if (busyZeroing(centerPoints, 1000, false)) {
      calSuccess = true;
      break;
    }
    attempts++;
    if (attempts < 3) {
      showCalibrationWarningScreen();
      delay(1500); // Wait for the user to completely release the knob
      showCalibrationScreen();
    }
  }

  // FIXED: Replaced blocking physical bypass loop with automatic warning & fallback boot [4]
  if (!calSuccess) {
    showCalibrationFailedScreen();
    delay(2500); // Show bypass notification on OLED briefly before continuing boot
  }
#endif

  for (uint8_t i = 0; i < 8; i++) {
    offsets[i] = 0;
  }
}

/**
 * @brief Main-loop of the SpaceMouse, called cyclic by system
 */
void loop() {
  static int debug = STARTDEBUG;
  
#if ENABLE_SERIAL_DEBUG
  static bool showMenu = false;

  //--- check if the user entered a debug mode via serial interface
  if ((debug != 20) && (debug != 30)) { 
    double num;

    int state = userInput(num);
#if ENABLE_PROGMODE > 0
    if (state == 10) {
      executeProgCommand(par);
      state = 0;
    }
#endif
    if (state == 1) {
      debug = (int)num;
      Serial.println(debug);
#ifdef HALLEFFECT
      setAnalogReferenceVoltage(debug);
#endif
    }
    if (state == 2 || state == 3 || state == 4) {
      debug = 99;
    }
    if ((state != 0) && (debug == 0 || debug == 99)) {
      showMenu = true;
    }

    if (showMenu) {
      Serial.print(F("\r\n\r\nSpaceMouse FW"));
      Serial.print(F(FW_RELEASE));
      Serial.println(F(" - Debug Modes"));
      Serial.println(F("ESC stop running mode, leave menu (ESC, Q)"));
      Serial.println(F("  1 raw sensors ADC values full range, max. 0..1023"));
#ifdef HALLEFFECT
      Serial.println(F(" 10 raw sensors ADC values used range, max. 0..1023"));
#endif
      Serial.println(F("  2 centered values -500..+500"));
      Serial.println(F(" 11 auto calibrate centers, show deadzones"));
      Serial.println(F(" 20 find min/max-values over 20s (move stick)"));
      Serial.println(F("  3 centered values w.deadzones -350..+350"));
      Serial.println(F(" 31 drift compensation offsets"));
      Serial.println(F("  4 velocity- (trans-/rot-)values -350..+350"));
      Serial.println(F("  5 centered- & velocity-values, (3) and (4)"));
      Serial.println(F("  6 velocity after kill-keys and keys"));
      Serial.println(F(" 61 velocity after axis-switch, exclusive"));
      Serial.println(F("  7 loop-frequency-test"));
      Serial.println(F("  8 key-test, button-codes to send"));
#if PARAM_IN_EEPROM > 0
      Serial.println(F(" 30 parameters (load, save, edit, view)"));
#endif
      Serial.print(F("mode::"));
      showMenu = false;
    }
  }

  //--- run parameter-menu
  if (debug == 30) {
#if PARAM_IN_EEPROM > 0
    if (parameterMenu(par) == 0) {
      showMenu = true;
      debug = 0;
    }
#else
    showMenu = true;
    debug = 0;
#endif
  }
#endif // ENABLE_SERIAL_DEBUG

  //--- Read joystick values. 0-1023
  readAllFromJoystick(rawReads);

//--- Reading of key presses
#if NUMKEYS > 0
  readAllFromKeys(keyVals);
#endif

#if ENABLE_SERIAL_DEBUG
// Report back 0-1023 raw ADC 10-bit values if enabled
#ifdef HALLEFFECT
  if ((debug == 1) || (debug == 10)) {
    debugOutput1(rawReads, keyVals);
  }
#else
  if (debug == 1) {
    debugOutput1(rawReads, keyVals);
  }
#endif

  //--- calibrate the joystick
  if (debug == 11) {
    busyZeroing(centerPoints, 2000, true);
    debug = -1;
  }
#endif // ENABLE_SERIAL_DEBUG

  //--- Calculate drift compensation offsets
  if (par.values->compEnabled == 1) {
#if ENABLE_SERIAL_DEBUG
    if (debug != 20) { 
      compensateDrifts(rawReads, centerPoints, offsets, par);
    } else {
      for (uint8_t i = 0; i < 8; i++) {
        offsets[i] = 0;
      }
    }
#else
    compensateDrifts(rawReads, centerPoints, offsets, par);
#endif
  } else {
    for (uint8_t i = 0; i < 8; i++) {
      offsets[i] = 0;
    }
  }

#if ENABLE_SERIAL_DEBUG
  // Report compensation-offset values
  if (debug == 31) {
    debugOutput2(offsets);
  }
#endif

  //--- Subtract centre position and drift-offsets from measured position to determine movement.
  for (uint8_t i = 0; i < 8; i++) {
    centered[i] = rawReads[i] - centerPoints[i] + offsets[i];
  }

#if ENABLE_SERIAL_DEBUG
  //--- calibrate MinMax values
  if (debug == 20) {
    if (calcMinMax(centered) == 0) { 
      debug = -1;                    
    }
  }

  // Report centered joystick values if enabled.
  if (debug == 2) {
    debugOutput2(centered);
  }
#endif

  //--- Set movement values to zero if movement is below deadzone threshold, scale to +/-350
  FilterAnalogReadOuts(centered, par);

#if ENABLE_SERIAL_DEBUG
  // Report centered joystick values. Filtered for deadzone.
  if (debug == 3) {
    debugOutput2(centered);
  }
#endif

  //--- Calculate the kinematic (centered->velocity)
  calculateKinematic(centered, velocity, par);

//--- evaluate keys
#if NUMKEYS > 0
  evalKeys(keyVals, keyOut, keyState);
  
  // Handle menu navigation inputs and actions
  processMenuInput(keyState, par);
#endif

#if ENABLE_SERIAL_DEBUG
  // report translation and rotation values if enabled
  if (debug == 4) {
    debugOutput4(velocity, keyOut);
  }

  if (debug == 5) {
    debugOutput5(centered, velocity);
  }
#endif

// if the kill-key feature is enabled, rotations or translations are killed
#if (NUMKILLKEYS == 2)
  if (keyVals[KILLROT] == LOW) {
    velocity[ROTX] = 0;
    velocity[ROTY] = 0;
    velocity[ROTZ] = 0;
  }
  if (keyVals[KILLTRANS] == LOW) {
    velocity[TRANSX] = 0;
    velocity[TRANSY] = 0;
    velocity[TRANSZ] = 0;
  }
#endif

#if ENABLE_SERIAL_DEBUG
  // report velocity and keys after possible kill-key feature
  if (debug == 6) {
    debugOutput4(velocity, keyOut);
  }
#endif

  //--- exchange axis if desired
  if (par.values->switchYZ == 1) {
    switchYZ(velocity);
  }
  if (par.values->switchXY == 1) {
    switchXY(velocity);
  }

  // exclusive mode: rotation OR translation
  if (par.values->exclusiveMode == 1) {
    exclusiveMode(velocity, par.values->exclusiveHysteresis);
  }

#if ENABLE_SERIAL_DEBUG
  if (debug == 61) {
    debugOutput4(velocity, keyOut);
  }
#endif

  // --- KINEMATIC SUPPRESSION AND KEY BLOCK DURING OLED NAVIGATION ---
  if (isOledMenuOpen()) {
    for (uint8_t i = 0; i < 6; i++) {
      velocity[i] = 0;
    }
#if NUMKEYS > 0
    uint8_t suppressedKeys[NUMKEYS] = {0};
    SpaceMouseHID.send_command(0, 0, 0, 0, 0, 0, suppressedKeys, debug);
#else
    SpaceMouseHID.send_command(0, 0, 0, 0, 0, 0, NULL, debug);
#endif
  } else {
    SpaceMouseHID.send_command(velocity[ROTX], velocity[ROTY], velocity[ROTZ], velocity[TRANSX],
                               velocity[TRANSY], velocity[TRANSZ], keyState, debug);
  }

#if ENABLE_SERIAL_DEBUG
  if (debug == 7) {
    updateFrequencyReport();
  }
#endif

  // Check for the LED state by calling updateLEDState.
  SpaceMouseHID.updateLEDState();

  // Update the OLED display layout
  updateOledDisplay(velocity, keyState, par);

} // end loop()

#ifdef HALLEFFECT
/**
 * @brief Set the analog reference to 5V for debug 1 and to 2.56V otherwise
 */
void setAnalogReferenceVoltage(int dbg) {
#if ENABLE_SERIAL_DEBUG
  if (dbg == 1) { 
    analogReference(DEFAULT);
  } else { 
    analogReference(INTERNAL);
  }
#else
  // Directly configure Internal 2.56V to bypass debugging code and save Flash bytes
  analogReference(INTERNAL);
#endif

  delay(100);
  int tempReads[8];
  for (uint8_t i = 0; i < 8; i++) { 
    readAllFromJoystick(tempReads);
  }
}
#endif