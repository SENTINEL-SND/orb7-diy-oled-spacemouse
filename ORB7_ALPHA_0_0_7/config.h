// Config File for << HALL-EFFECT SPACEMOUSE >>
// Firmware version is defined centrally in release.h
#ifndef CONFIG_h
#define CONFIG_h

#include "release.h"

// 1 = Enable serial monitor debugging (diagnostics), 0 = Disable for maximum Flash memory savings
#define ENABLE_SERIAL_DEBUG 0

// 1 = Enable OLED display functions, 0 = Disable OLED completely to save space for debugging
#define ENABLE_OLED 1

// 1 = Load persistent parameters from EEPROM at boot, 0 = Use in-memory defaults at boot.
// OLED parameter saves remain available when the OLED is enabled.
#define PARAM_IN_EEPROM 1
// 1 = Enable Serial CLI programming mode commands, 0 = Disable
#define ENABLE_PROGMODE 0

#undef DEBUG_KEYS
#undef DEBUG_ADC

// Serial debug output report interval in milliseconds (prevents serial buffer flooding)
#define DEBUGDELAY 100

/* ADC Acquisition
=====================================================
These parameters control low-level hardware sampling. The active firmware uses
small integer oversampling and does not use EMA or adaptive drift compensation.

ADC_OVERSAMPLES:     Number of analog samples per sensor read cycle.
                     Recommended values are 1, 2, 4 or 8; keep the value at or below 64.
ADC_PRESCALER_PRESET: ADC Clock Prescaler setting for the ATmega32U4 hardware multiplexer.
                      (0x07 = 128 / 125kHz [Arduino Default], 0x06 = 64 / 250kHz [Stable/Fast], 0x05 = 32 / 500kHz [Overclock]).
*/
#define ADC_OVERSAMPLES 2
#define ADC_PRESCALER_PRESET 0x05

/* Debugging Instructions
=========================
When ENABLE_SERIAL_DEBUG is 1, set STARTDEBUG to boot directly into a debug mode,
or type the number in the Serial Monitor.
-1: Debugging off. Standard operation mode.
 1: Report raw joystick values on 5V ref. (0-1023)
10: Report raw joystick values on 2.56V ref. (0-1023)
11: Auto calibrate centers and show sensor noise spans.
  2: Report centered sensor values (approximately -1023 to +1023).
20: Find min/max-values over 20s (move stick).
 3: Report normalized sensor values (-350 to +350).
 4: Report final velocity (translation/rotation) values (-350 to +350).
  5: Report centered sensor and final velocity values side by side.
 6: Report velocity after kill-keys.
61: Report velocity after axis-switch and ExclusiveMode.
 7: Report main loop frequency (Hz).
 8: Report HID key bitmask output.
30: Open Serial CLI parameter menu (EEPROM editing).
*/
#define STARTDEBUG -1 // Runtime changes require ENABLE_SERIAL_DEBUG = 1.

// Hardware uses HallEffect sensors instead of resistive joystick potentiometers
#define HALLEFFECT

/* First Calibration: Hall effect sensors pin assignment
=========================================================
Default physical layout when looking from above:
 *    back(USB)     
 *      7   6              Y+
 *        |                .
 *   8    |    3           .
 *     ---+---        X-...Z+...X+
 *   9    |    2           .
 *        |                .
 *      0   1              Y-
*/
// Check the correct wiring with the debug output=1
#define PINLIST {A0, A1, A2, A3, A6, A7, A8, A9}
// HES0, HES1, HES2, HES3, HES6, HES7, HES8, HES9

// Set to 1 to invert specific Hall sensors.
// Use this when measured polarity is opposite to the expected assembly direction.
#define INVERTLIST {0, 0, 0, 0, 0, 0, 0, 0}
// HES0, HES1, HES2, HES3, HES6, HES7, HES8, HES9

/* Second calibration: Getting MIN and MAX values
=================================================
Use Debug 20 or the OLED "Cal. Limits" screen to populate these bounds.
These dynamic limits map the hardware sensor swing to the +/- 350 HID bounds.
*/
// Insert measured values like this:
//              {HES0, HES1, HES2, HES3, HES6, HES7, HES8, HES9}
#define MINVALS {-400, -400, -400, -400, -400, -400, -400, -400}
#define MAXVALS {175, 175, 175, 175, 175, 175, 175, 175}

/* Third calibration: Axis Response
===================================================
These values are factory defaults. Once EEPROM contains a valid parameter block,
the persisted values take precedence over these definitions.

Sensitivity values are divisors applied after the 8-to-6 kinematic matrix.
Smaller values increase response; larger values reduce response.

Each final 6DOF axis has one independent base gate applied after the 8-to-6 matrix.
DEADZONE_DEFAULT is a user comfort level from 0..100% that proportionally raises
all six base gates inside the same continuous deadzone layer. At 100%, the default
DEADZONE_BOOST_AT_MAX value adds 200% to each base gate (3x total threshold).
Gate units are normalized matrix units, not ADC counts and not percentages.
The comparison is inclusive: values at or below the effective gate are suppressed.
There is no sensor-level deadzone or post-curve gate.
*/
// =========================
// TRANSLATION SENSITIVITY
// =========================
#define SENS_TX  0.8
#define SENS_TY  1
#define SENS_PTZ 2     // Positive Z: pushing down
#define SENS_NTZ 1     // Negative Z: pulling up

// =========================
// ROTATION SENSITIVITY
// =========================
#define SENS_RX  1.2
#define SENS_RY  1.2
#define SENS_RZ  0.90

// =========================
// AXIS GATES
// =========================
#define GATE_TX 6
#define GATE_TY 6
#define GATE_TZ 25

#define GATE_RX 5
#define GATE_RY 5
#define GATE_RZ 5

// =========================
// DEADZONE
// =========================
// Gate units are normalized matrix units, not ADC counts or percentages.
// Values at or below the effective gate are suppressed.
// The deadzone is an additional comfort level, not a sensor-level filter.
// effective gate = base gate x (1 + 2 x deadzone level / 100)
// Therefore 0% = 1x, 50% = 2x and 100% = 3x the base gate.
// There is no post-curve gate.
#define DEADZONE_DEFAULT      0
#define DEADZONE_MAX          100
#define DEADZONE_BOOST_AT_MAX 200

/* Fifth calibration: Modifier Function
========================================
Modifies the post-gate, post-sensitivity response into ergonomic curves.
The curve normalizes the absolute input as xn = abs(x) / 350 before applying
the selected function, then restores the signed +/-350 HID range.
0: Linear (y = x)
1: Power function (y = 350 * xn^a * sign(x))
3: Tangent-power function (y = 350 * tan(b * xn^a) / tan(b) * sign(x))
Recommendation for CAD: MODFUNC 3
*/
#define MODFUNC 3  // Used as default value as long as the data hasn't been saved in the EEPROM
#define MOD_A 1.15 // exponent "a", recommended: 1.0 ... 3.0
#define MOD_B 1.35 // factor "b", recommended: 1.0 ... 1.57 (Must not exceed Pi/2)

/* Sixth calibration: Direction
================================
Invert resulting axes depending on the target CAD software preference.
(Often required to match 3DConnexion Windows driver orientations).
*/
// Switch between 0 or 1 as desired
#define INVX 0  // pan left/right
#define INVY 1  // pan up/down
#define INVZ 1  // zoom in/out
#define INVRX 0 // Rotate around X axis (tilt front/back)
#define INVRY 1 // Rotate around Y axis (tilt left/right)
#define INVRZ 1 // Rotate around Z axis (twist left/right)

// Switch Zoom direction with Up/Down Movement
#define SWITCHYZ 0 // change to 1 to switch Y and Z axis
#define SWITCHXY 0 // change to 1 to switch X and Y axis

/* Exclusive mode
==================
Only permits translation OR rotation at a given time.
When the current gesture relaxes, the dominance latch returns to neutral so the
next gesture can select translation or rotation again.
*/
#define EXCLUSIVE 1
#define EXCL_HYST 50 // Hysteresis barrier to switch modes mid-motion
#define EXCL_RELAX_THRESHOLD 35 // Combined motion below which the latch returns to neutral

/* Key Support (ORB7 Physical Redesign Layout)
==============================================
Define hardware buttons attached to the SpaceMouse.
Pin 5  : Front Right (Key R)
Pin 0  : Front Left  (Key L)
Pin 1  : Back Left   (Key 2)
Pin 7  : Back Right  (Key 1)
*/
#define NUMKEYS 4

// Pin assignment for physical keys: {Front Right, Front Left, Back Left, Back Right}
#define KEYLIST {5, 0, 1, 7}

// Legacy compatibility count; the active HID path uses NUMKEYS.
#define NUMHIDKEYS 4

// 3DConnexion HID Standard Key Mappings
#define SM_MENU 0  // Key "Menu"
#define SM_FIT 1   // Key "Fit"
#define SM_T 2     // Key "Top"
#define SM_R 4     // Key "Right"
#define SM_F 5     // Key "Front"
#define SM_RCW 8   // Key "Roll 90°CW"
#define SM_1 12    // Key "1"
#define SM_2 13    // Key "2"
#define SM_3 14    // Key "3"
#define SM_4 15    // Key "4"
#define SM_ESC 22  // Key "ESC"
#define SM_ALT 23  // Key "ALT"
#define SM_SHFT 24 // Key "SHIFT"
#define SM_CTRL 25 // Key "CTRL"
#define SM_ROT 26  // Key "Rotate"

// Retained legacy defaults for the two rear key positions.
// Current firmware stores all four key assignments in the persistent parameter block.
#define BUTTONLIST {SM_2, SM_1}

/* Kill-Key Feature
--------------------
Buttons dedicated to mute translation or rotation globally.
The active firmware path supports 0 (disabled) or 2 kill keys.
*/
#define NUMKILLKEYS 0
#define KILLROT 2
#define KILLTRANS 3

// Time in ms required to allow a new button press (Instant press / Delayed release debounce)
#define DEBOUNCE_KEYS_MS 50

#endif // CONFIG_h
