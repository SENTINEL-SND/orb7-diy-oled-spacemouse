// Config File for << HALL-EFFECT SPACEMOUSE >>
// Firmware Version: v0.0.5
#ifndef CONFIG_h
#define CONFIG_h

#include "release.h"

// 1 = Enable serial monitor debugging (diagnostics), 0 = Disable for maximum Flash memory savings
#define ENABLE_SERIAL_DEBUG 0

// 1 = Enable OLED display functions, 0 = Disable OLED completely to save space for debugging
#define ENABLE_OLED 1

// 1 = Store and load parameters from EEPROM, 0 = Use hardcoded values only
#define PARAM_IN_EEPROM 1
// 1 = Enable Serial CLI programming mode commands, 0 = Disable
#define ENABLE_PROGMODE 0

#undef DEBUG_KEYS
#undef DEBUG_ADC

// Serial debug output report interval in milliseconds (prevents serial buffer flooding)
#define DEBUGDELAY 100

/* Advanced Signal Filtering & Hardware ADC Settings
=====================================================
These parameters control the low-level hardware sampling to eliminate noise without float math.

ADC_OVERSAMPLES:     Number of hardware analog samples taken per axis read cycle (1, 2, 4, 8).
                     Higher values reduce random noise but increase loop time slightly.
ADC_EMA_SHIFT:       Low-pass Exponential Moving Average (EMA) filter shift factor. 
                     Uses extremely fast bit-shifting for integer division.
                     (0 = disabled, 1 = 50%, 2 = 25%, 3 = 12.5% weight for new readings).
EXCL_RELAX_THRESHOLD: Force threshold (sum of axes) below which exclusive mode unlocks back to NEUTRAL.
ADC_PRESCALER_PRESET: ADC Clock Prescaler setting for the ATmega32U4 hardware multiplexer.
                      (0x07 = 128 / 125kHz [Arduino Default], 0x06 = 64 / 250kHz [Stable/Fast], 0x05 = 32 / 500kHz [Overclock]).
*/
#define ADC_OVERSAMPLES 2
#define ADC_EMA_SHIFT 2
#define EXCL_RELAX_THRESHOLD 35
#define ADC_PRESCALER_PRESET 0x05

/* Debugging Instructions
=========================
Set STARTDEBUG to boot directly into a specific debug mode, or type the number in the Serial Monitor.
-1: Debugging off. Standard operation mode.
 1: Report raw joystick values on 5V ref. (0-1023)
10: Report raw joystick values on 2.56V ref. (0-1023)
11: Auto calibrate centers and show deadzones.
 2: Report centered joystick values (-500 to +500).
20: Find min/max-values over 20s (move stick).
 3: Report centered values filtered with deadzones (-350 to +350).
31: Report drift compensation offsets.
 4: Report final velocity (translation/rotation) values (-350 to +350).
 5: Report centered (3) and velocity (4) side by side.
 6: Report velocity after kill-keys.
61: Report velocity after axis-switch and ExclusiveMode.
 7: Report main loop frequency (Hz).
 8: Report HID key bitmask output.
30: Open Serial CLI parameter menu (EEPROM editing).
*/
#define STARTDEBUG -1 // Can also be set over the serial interface, while the program is running!

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

// Set to 1 to invert specific hall sensors.
// Values should naturally decrease when the magnet is nearing the sensor.
#define INVERTLIST {0, 0, 0, 0, 0, 0, 0, 0}
// HES0, HES1, HES2, HES3, HES6, HES7, HES8, HES9

/* Second calibration: Tune Deadzone
=====================================
Deadzone filters out mechanical noise and micro-movements when the knob is at rest.
Recommended to keep this as small as possible (e.g., 10-15) for smooth operation.
*/
#define DEADZONE 10 

/* Third calibration: Getting MIN and MAX values
=================================================
Use Debug 20 or the OLED "Cal. Limits" screen to populate these bounds.
These dynamic limits map the hardware sensor swing to the +/- 350 HID bounds.
*/
// Insert measured values like this:
//              {HES0, HES1, HES2, HES3, HES6, HES7, HES8, HES9}
#define MINVALS {-400, -400, -400, -400, -400, -400, -400, -400}
#define MAXVALS {175, 175, 175, 175, 175, 175, 175, 175}

/* Fourth calibration: Sensitivity and Noise Gates
===================================================
Sensitivity values act as dividers. 
Smaller fraction (e.g., 0.8) = MORE sensitive (faster).
Larger value (e.g., 2.0) = LESS sensitive (slower).
Gates suppress cross-coupling (e.g., preventing rotation while translating).

GATE_TRANS: Dedicated micro-gate for Translation X and Y (Pan) to suppress 
            matrix cross-coupling and mechanical spring bleed near rest position.
            Filters out residual post-matrix velocity deltas (e.g. 1 to 5) while 
            keeping intentional panning movements 100% fluid and independent.
            Recommended range: 3 to 10 (Default: 6).
*/
#define SENS_TX 0.5
#define SENS_TY 0.5
#define SENS_PTZ 0.5 // sensitivity for positive translation z (pushing down)
#define SENS_NTZ 1 // sensitivity for negative translation z (pulling up)
#define GATE_TRANS 6 // Micro-gate filtering X/Y translation matrix bleed (3 to 10)
#define GATE_NTZ 15 // gate value below which negative z movements will be ignored.
#define GATE_RX 5 // Value under which rotX values will be forced to zero
#define GATE_RY 5 // Value under which roty values will be forced to zero
#define GATE_RZ 5 // Value under which rotz values will be forced to zero
#define SENS_RX 1.2
#define SENS_RY 1.2
#define SENS_RZ 0.90

/* Fifth calibration: Modifier Function
========================================
Modifies the linear hardware response into ergonomic curves.
0: Linear (y = x)
1: Squared function (y = abs(x)^a * sign(x))
3: Squared tangent function (y = tan(b * (abs(x)^a * sign(x))) / tan(b))
Recommendation for CAD: MODFUNC 3
*/
#define MODFUNC 3  // Used as default value as long as the data hasn't been saved in the EEPROM
#define MOD_A 1.15 // exponent "a", recommended: 1.0 ... 3.0
#define MOD_B 1.35 // factor "b", recommended: 1.0 ... 1.57 (Must not exceed Pi/2)

/* Sixth Calibration: Direction
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

/* Hallsensor Drift Compensation
================================
Dynamically tracks and eliminates center point wandering due to thermal expansion or mechanical fatigue.
*/
#define COMP_EN 1     // enable the compensation
#define COMP_NR 50    // number of points to build the mean-value (Must be 1-500)
#define COMP_WAIT 50 // [ms] time to wait and monitor before compensating
#define COMP_MDIFF 10 // [incr] maximum range of raw-values to be considered as only drift
#define COMP_CDIFF 50 // [incr] maximum distance from the center-value to be only drift

/* Exclusive mode
==================
Only permits translation OR rotation at a given time.
Now features 'Neutral Unlocking' which seamlessly transitions back to both allowed when the knob is released.
*/
#define EXCLUSIVE 1
#define EXCL_HYST 60 // Hysteresis barrier to switch modes mid-motion

/* Key Support (ORB7 Physical Redesign Layout)
==============================================
Define hardware buttons attached to the SpaceMouse.
Pin 5  : Front Right (Key R)
Pin 0  : Front Left  (Key L) -> Arduino RX0
Pin 1  : Back Left   (Key 2) -> Arduino TX1
Pin 7  : Back Right  (Key 1)
*/
#define NUMKEYS 4

// Pin assignment for physical keys: {Front Right, Front Left, Back Left, Back Right}
#define KEYLIST {5, 0, 1, 7}

// How many keys reported to HID 
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

// Mappings for keys[2] (Back Left) and keys[3] (Back Right).
// keys[0] (Front Right) and keys[1] (Front Left) are dynamically assigned via OLED/WebStudio.
#define BUTTONLIST {SM_2, SM_1}

/* Kill-Key Feature
--------------------
Buttons dedicated to mute translation or rotation globally.
*/
#define NUMKILLKEYS 0
#define KILLROT 2
#define KILLTRANS 3

// Time in ms required to allow a new button press (Instant press / Delayed release debounce)
#define DEBOUNCE_KEYS_MS 50

#endif // CONFIG_h