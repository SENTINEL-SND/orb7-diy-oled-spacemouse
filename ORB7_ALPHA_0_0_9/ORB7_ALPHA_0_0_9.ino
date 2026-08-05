/*
 * SPACE MOUSE PRO EMULATOR (6DOF DIY) - MAIN INO ENTRY FILE
 * Firmware Version: ALPHA 0.0.8
 * Architecture: ATmega32U4 (Arduino Pro Micro, 5V, 16 MHz)
 */

#include <Arduino.h>
#include <avr/wdt.h> // Watchdog timer support for hardware software reboot
#include <Wire.h> // Included to support global Wire I2C timeout configurations

// The user-specific settings, pin mappings, and hardware configuration definitions
#include "config.h"
#include "parameterMenu.h"

// Include inbuilt Arduino HID library by NicoHood: https://github.com/NicoHood/HID
#include "HID.h"

// Header file for calibration output, filters, and helper routines
#include "calibration.h"
#include "calibrationChecks.h"

// Header executing the core kinematic math across all 6 degrees of freedom
#include "kinematics.h"

// Header for processing physical hardware tactile switches
#include "spaceKeys.h"

// Header for HID USB composite emulation
#include "SpaceMouseHID.h"

// Header managing the high-speed RAM-less SSD1306 OLED interface
#include "oledDisplay.h"

// Header for WebHID bi-directional communication protocol
#include "webHID.h"

// Fallback safety to ensure the ADC runs fast enough to maintain ultra-low loop latency
#ifndef ADC_PRESCALER_PRESET
  #define ADC_PRESCALER_PRESET 0x05
#endif

void setup();
void loop();
#ifdef HALLEFFECT
void setAnalogReferenceVoltage(int dbg);
#endif

// Global motion tracking arrays explicitly promoted to 16-bit to ensure cross-platform ABI safety
int16_t rawReads[8];
int16_t centered[8];
int16_t centerPoints[8];
int16_t offsets[8];

// Resulting calculated velocities mapping directly to the HID protocol expected bounds (-350 to +350)
int16_t velocity[6];

// Global parameter definitions persisted directly to EEPROM
ParamStorage parStorage;

// Conditional Struct Initialization:
// Trims sizeof(ParamData) from 542 bytes down to 2 bytes when serial debugging is disabled,
// reclaiming massive amounts of Flash memory for the OLED graphics routines.
#if ENABLE_SERIAL_DEBUG
ParamData par = {.values = &parStorage,
                 .description = {
                     {PARAM_TYPE_BOOL, "", NULL},                                         // param 0 (unused)
                     {PARAM_TYPE_INT, "DEADZONE", &parStorage.deadzone},                  // 1
                     {PARAM_TYPE_INT, "SENS_TX", &parStorage.transX_sensitivity_q7},       // 2
                     {PARAM_TYPE_INT, "SENS_TY", &parStorage.transY_sensitivity_q7},       // 3
                     {PARAM_TYPE_INT, "SENS_PTZ", &parStorage.pos_transZ_sensitivity_q7},  // 4
                     {PARAM_TYPE_INT, "SENS_NTZ", &parStorage.neg_transZ_sensitivity_q7},  // 5
                     {PARAM_TYPE_INT, "GATE_NTZ", &parStorage.gate_neg_transZ},           // 6
                     {PARAM_TYPE_INT, "GATE_RX", &parStorage.gate_rotX},                  // 7
                     {PARAM_TYPE_INT, "GATE_RY", &parStorage.gate_rotY},                  // 8
                     {PARAM_TYPE_INT, "GATE_RZ", &parStorage.gate_rotZ},                  // 9
                     {PARAM_TYPE_INT, "GATE_TR", &parStorage.gate_trans},                 // 10
                     {PARAM_TYPE_INT, "SENS_RX", &parStorage.rotX_sensitivity_q7},         // 11
                     {PARAM_TYPE_INT, "SENS_RY", &parStorage.rotY_sensitivity_q7},         // 12
                     {PARAM_TYPE_INT, "SENS_RZ", &parStorage.rotZ_sensitivity_q7},         // 13
                     {PARAM_TYPE_INT, "MODFUNC", &parStorage.modFunc},                    // 14
                     {PARAM_TYPE_INT, "MOD_A", &parStorage.slope_at_zero_q8},             // 15
                     {PARAM_TYPE_INT, "MOD_B", &parStorage.slope_at_end_q8},              // 16
                     {PARAM_TYPE_BOOL, "INVX", &parStorage.invX},                         // 17
                     {PARAM_TYPE_BOOL, "INVY", &parStorage.invY},                         // 18
                     {PARAM_TYPE_BOOL, "INVZ", &parStorage.invZ},                         // 19
                     {PARAM_TYPE_BOOL, "INVRX", &parStorage.invRX},                       // 20
                     {PARAM_TYPE_BOOL, "INVRY", &parStorage.invRY},                       // 21
                     {PARAM_TYPE_BOOL, "INVRZ", &parStorage.invRZ},                       // 22
                     {PARAM_TYPE_BOOL, "SWITCHXY", &parStorage.switchXY},                 // 23
                     {PARAM_TYPE_BOOL, "SWITCHYZ", &parStorage.switchYZ},                 // 24
                     {PARAM_TYPE_BOOL, "EXCLUSIVE", &parStorage.exclusiveMode},           // 25
                     {PARAM_TYPE_INT, "EXCL_HYST", &parStorage.exclusiveHysteresis},      // 26
                     {PARAM_TYPE_BOOL, "COMP_EN", &parStorage.compEnabled},               // 27
                     {PARAM_TYPE_INT, "COMP_NR", &parStorage.compNoOfPoints},             // 28
                     {PARAM_TYPE_INT, "COMP_WAIT", &parStorage.compWaitTime},             // 29
                     {PARAM_TYPE_INT, "COMP_MDIFF", &parStorage.compMinMaxDiff},          // 30
                     {PARAM_TYPE_INT, "COMP_CDIFF", &parStorage.compCenterDiff},          // 31
                     {PARAM_TYPE_INT, "GLB_SENS", &parStorage.globalSens},                 // 32
                     {PARAM_TYPE_INT, "KEY2_SHORT", &parStorage.key2_shortcut},           // 33
                     {PARAM_TYPE_INT, "KEY1_SHORT", &parStorage.key1_shortcut}            // 34
                 }};
#else
ParamData par = {.values = &parStorage};
#endif

// Physical button states minimized to 8-bit tracking preventing SRAM waste
uint8_t keyVals[NUMKEYS];
uint8_t keyOut[NUMKEYS];
uint8_t keyState[NUMKEYS];

/**
 * @brief Setup the SpaceMouse firmware, triggered by system reset or power-on.
 */
void setup() {
  // Clear Watchdog reset flags and disable Watchdog timer immediately upon boot
  MCUSR = 0;
  wdt_disable();

  // Directly configure the hardware multiplexer ADC prescaler via the ADCSRA register.
  // E.g., 0x05 scales the clock to 500 kHz, safely reducing per-read sampling time from 52us down to 26us.
  ADCSRA = (ADCSRA & 0xF8) | (ADC_PRESCALER_PRESET & 0x07);

// Initialize configuration definitions from EEPROM block with XOR checks
#if PARAM_IN_EEPROM > 0
  getParametersFromEEPROM(par);
#endif

// Initialize hardware GPIO states for buttons, utilizing internal MCU resistors
#if NUMKEYS > 0
  setupKeys();
#endif

#ifdef HALLEFFECT
  // Establish the ADC reference voltage baseline (typically 2.56V INTERNAL for Hall effect ranges)
  setAnalogReferenceVoltage(0);
#endif

#if ENABLE_SERIAL_DEBUG
  Serial.begin(115200);
#endif

  // 1. Awaken the display via I2C and render the splash boot sequence
  initOledDisplay();

  // 2. Render visual instructions requesting the user to maintain the 3D knob at rest
  showCalibrationScreen();

  // 3. Calibrate base rest coordinates non-blockingly (Zeroing Sequence)
#if ENABLE_SERIAL_DEBUG
  // When debugging, enforce the legacy sequence directly
  busyZeroing(centerPoints, 1000, true);
#else
  // Active boot logic loop attempts to zero the knob up to 3 times before giving up.
  // Replaces the infinite hardware lockout loop when uncalibrated magnetic plates are first installed.
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
      delay(1500); // Give the user time to release mechanical pressure
      showCalibrationScreen();
    }
  }

  // Gracefully transition boot execution even if calibration attempts are entirely exhausted
  if (!calSuccess) {
    showCalibrationFailedScreen();
    delay(2500); 
  }
#endif

  // Ensure thermal drift baseline is clean before allowing operational tracking
  for (uint8_t i = 0; i < 8; i++) {
    offsets[i] = 0;
  }
}

/**
 * @brief Primary Operational Loop processing raw analog reads to USB HID reports efficiently.
 */
void loop() {
  static int debug = STARTDEBUG;
  
#if ENABLE_SERIAL_DEBUG
  static bool showMenu = false;

  // Evaluate the serial buffer strictly if debug monitoring is requested
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

  // Engage the Serial CLI parameter menu sequence
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

  // --- 1. CORE PIPELINE: Sample hardware ADCs employing oversampling and EMA smoothing
  readAllFromJoystick(rawReads);

  // --- 2. Evaluate physical hardware button debounce logic
#if NUMKEYS > 0
  readAllFromKeys(keyVals);
#endif

#if ENABLE_SERIAL_DEBUG
#ifdef HALLEFFECT
  if ((debug == 1) || (debug == 10)) {
    debugOutput1(rawReads, keyVals);
  }
#else
  if (debug == 1) {
    debugOutput1(rawReads, keyVals);
  }
#endif

  if (debug == 11) {
    busyZeroing(centerPoints, 2000, true);
    debug = -1;
  }
#endif // ENABLE_SERIAL_DEBUG

  // --- 3. Evaluate environmental drift compensation logic independently per axis
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
  if (debug == 31) {
    debugOutput2(offsets);
  }
#endif

  // --- 4. Offset Subtraction: Normalize data removing mechanical center bounds and drift calculations
  for (uint8_t i = 0; i < 8; i++) {
    centered[i] = rawReads[i] - centerPoints[i] + offsets[i];
  }

#if ENABLE_SERIAL_DEBUG
  if (debug == 20) {
    if (calcMinMax(centered) == 0) { 
      debug = -1;                    
    }
  }

  if (debug == 2) {
    debugOutput2(centered);
  }
#endif

  // --- 5. Apply physical noise filtering (Deadzones) and scale data dynamically to symmetrical arrays
  FilterAnalogReadOuts(centered, par);

#if ENABLE_SERIAL_DEBUG
  if (debug == 3) {
    debugOutput2(centered);
  }
#endif

  // --- 6. Formulate 6DOF vector space translations handling logic gates and mapping curve multipliers
  calculateKinematic(centered, velocity, par);

  // --- 7. Resolve discrete key combinations triggering OLED interactions
#if NUMKEYS > 0
  evalKeys(keyVals, keyOut, keyState);
  
  // Forward button presses directly into the OLED UI logic hierarchy
  processMenuInput(keyState, par);
#endif

#if ENABLE_SERIAL_DEBUG
  if (debug == 4) {
    debugOutput4(velocity, keyOut);
  }

  if (debug == 5) {
    debugOutput5(centered, velocity);
  }
#endif

  // Execute Kill-Key override mutations if designated by the configuration
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
  if (debug == 6) {
    debugOutput4(velocity, keyOut);
  }
#endif

  // Mutate axis arrangements to comply with specific CAD engine orientations if activated
  if (par.values->switchYZ == 1) {
    switchYZ(velocity);
  }
  if (par.values->switchXY == 1) {
    switchXY(velocity);
  }

  // --- 8. Resolve translation OR rotation dominance muting mechanisms
  if (par.values->exclusiveMode == 1) {
    exclusiveMode(velocity, par.values->exclusiveHysteresis);
  }
  
  // --- WEBHID TELEMETRY: Stream synchronized values to the browser at loop end
  streamWebHIDRawData(rawReads);

#if ENABLE_SERIAL_DEBUG
  if (debug == 61) {
    debugOutput4(velocity, keyOut);
  }
#endif

  // --- 9. KINEMATIC SUPPRESSION AND KEY BLOCK DURING OLED NAVIGATION ---
  // Overrides the HID payload buffers with zeroes if the user is actively scrolling through OLED parameters.
  // Ensures settings manipulation doesn't violently whip the 3D viewport canvas on the host PC.
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
    // Standard execution pushes finalized 6DOF kinematic matrix to the PluggableUSB core dispatcher
    SpaceMouseHID.send_command(velocity[ROTX], velocity[ROTY], velocity[ROTZ], velocity[TRANSX],
                               velocity[TRANSY], velocity[TRANSZ], keyState, debug);
  }

#if ENABLE_SERIAL_DEBUG
  if (debug == 7) {
    updateFrequencyReport();
  }
#endif

  // Empty the USB RX buffers continuously avoiding queue blockages and processing WebHID payloads
  SpaceMouseHID.receiveHostData(par);

  // --- 10. Update pixels incrementally on the OLED UI preserving high loop refresh rates
  updateOledDisplay(velocity, keyState, par);

} // end loop()

#ifdef HALLEFFECT
/**
 * @brief Modifies the underlying analog comparator baseline voltage scale
 */
void setAnalogReferenceVoltage(int dbg) {
#if ENABLE_SERIAL_DEBUG
  if (dbg == 1) { 
    analogReference(DEFAULT); // Fallback to 5V standard operation for mechanical diagnostics
  } else { 
    analogReference(INTERNAL); // 2.56V
  }
#else
  // Bypass serial comparisons completely pushing the 2.56V ref configuration directly
  analogReference(INTERNAL);
#endif

  // Permit comparator hardware to settle physically before resuming standard pipeline actions
  delay(100);
  int tempReads[8];
  for (uint8_t i = 0; i < 8; i++) { 
    readAllFromJoystick(tempReads);
  }
}
#endif