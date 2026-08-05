// Header for parameter-menu specific functions and variables
// Manages the persistent EEPROM storage struct and the Serial CLI definitions.

#ifndef PARAMETERMENU_H
  #define PARAMETERMENU_H

  #include <Arduino.h> // Required for int16_t, int8_t and standard types
  #include "config.h"

  // Defines the absolute number of persistent parameters managed by the EEPROM (37 individual parameters)
  #define NUM_PARAMS         37   

  #define MAX_PARAM_NAME_LEN 10   // maximum length of any parameter name for the Serial CLI

  // Magic Number enforces data structural alignment.
  // Changing this value intentionally forces a factory reset on the next boot,
  // which is highly recommended when adding or removing parameters in the struct below.
  #define MAGIC_NUMBER       1234567853L
  #define BASE_ADDRESS_MAGIC 0
  #define BASE_ADDRESS_PAR   4

  #define PARAM_TYPE_BOOL    1
  #define PARAM_TYPE_INT     2
  #define PARAM_TYPE_FLOAT   3

  // Fixed-point scaling macros calculated at compile-time to prevent runtime floating-point overhead.
  // Q7 implies 7 bits of fractional precision (Multiplier: 128.0).
  // Q8 implies 8 bits of fractional precision (Multiplier: 256.0).
  #define SENS_TX_Q7 ((int16_t)(SENS_TX * 128.0f))
  #define SENS_TY_Q7 ((int16_t)(SENS_TY * 128.0f))
  #define SENS_PTZ_Q7 ((int16_t)(SENS_PTZ * 128.0f))
  #define SENS_NTZ_Q7 ((int16_t)(SENS_NTZ * 128.0f))
  #define SENS_RX_Q7 ((int16_t)(SENS_RX * 128.0f))
  #define SENS_RY_Q7 ((int16_t)(SENS_RY * 128.0f))
  #define SENS_RZ_Q7 ((int16_t)(SENS_RZ * 128.0f))
  #define SLOPE_A_Q8 ((int16_t)(MOD_A * 256.0f))
  #define SLOPE_B_Q8 ((int16_t)(MOD_B * 256.0f))

  // Core configuration structure written directly to the ATmega32U4 EEPROM.
  // Relies exclusively on fixed-width integer types (int8_t, int16_t) to guarantee memory alignment
  // and prevent cross-platform padding issues.
  typedef struct _ParamStorage {
    int16_t deadzone               = DEADZONE;

    // Float sensitivity variables replaced by Q7 fixed-point 16-bit integers
    int16_t transX_sensitivity_q7     = SENS_TX_Q7;
    int16_t transY_sensitivity_q7     = SENS_TY_Q7;
    int16_t pos_transZ_sensitivity_q7 = SENS_PTZ_Q7;
    int16_t neg_transZ_sensitivity_q7 = SENS_NTZ_Q7;
    int16_t gate_neg_transZ           = GATE_NTZ; 
    int16_t gate_rotX                 = GATE_RX;
    int16_t gate_rotY                 = GATE_RY;
    int16_t gate_rotZ                 = GATE_RZ;
    int16_t gate_trans                = GATE_TRANS; // MicroGate for Pan X/Y!

    int16_t rotX_sensitivity_q7       = SENS_RX_Q7;
    int16_t rotY_sensitivity_q7       = SENS_RY_Q7;
    int16_t rotZ_sensitivity_q7       = SENS_RZ_Q7;

    int16_t modFunc                   = MODFUNC;         
    int16_t slope_at_zero_q8          = SLOPE_A_Q8; // Replaces 'slope_at_zero' as Q8 fixed-point integer
    int16_t slope_at_end_q8           = SLOPE_B_Q8; // Replaces 'slope_at_end' as Q8 fixed-point integer

    int8_t  invX                   = INVX;
    int8_t  invY                   = INVY;
    int8_t  invZ                   = INVZ;
    int8_t  invRX                  = INVRX;
    int8_t  invRY                  = INVRY;
    int8_t  invRZ                  = INVRZ;

    int8_t  switchXY               = SWITCHXY;
    int8_t  switchYZ               = SWITCHYZ;
    int8_t  exclusiveMode          = EXCLUSIVE;
    int16_t exclusiveHysteresis    = EXCL_HYST;

    int8_t  compEnabled            = COMP_EN;
    int16_t compNoOfPoints         = COMP_NR;
    int16_t compWaitTime           = COMP_WAIT;
    int16_t compMinMaxDiff         = COMP_MDIFF;
    int16_t compCenterDiff         = COMP_CDIFF;
    
    // Global Sensitivity initialized at 100%
    int16_t globalSens             = 100; 

    // OLED Inactivity Sleep Timer (0 = OFF, 1 = 1m, 2 = 3m, 3 = 5m)
    int8_t  oledSleepTimer         = 2; 

    // Dynamic key shortcuts for ALL 4 physical hardware buttons (Fully customizable)
    int8_t  keyL_shortcut          = 2;  // Front Left  (Key L / keys[1]) - Default: SM_T (Top = 2)
    int8_t  keyR_shortcut          = 1;  // Front Right (Key R / keys[0]) - Default: SM_FIT (Fit = 1)
    int8_t  key2_shortcut          = 13; // Back Left   (Key 2 / keys[2]) - Default: SM_2 (Button 2 = 13)
    int8_t  key1_shortcut          = 12; // Back Right  (Key 1 / keys[3]) - Default: SM_1 (Button 1 = 12)

    // Dynamic calibration limits bounding the analog hardware range to the HID logical output
    int16_t minVals[8]             = {-400, -400, -400, -400, -400, -400, -400, -400};
    int16_t maxVals[8]             = {175, 175, 175, 175, 175, 175, 175, 175};
  } ParamStorage;

#if ENABLE_SERIAL_DEBUG
  // Descriptor strings and types strictly allocated only when Serial Debugging is active
  typedef struct _ParamDescription {
    int   type;
    char  name[MAX_PARAM_NAME_LEN+1];
    void* storage;
  } ParamDescription;
#endif

  typedef struct _ParamData {
    ParamStorage*     values;
#if ENABLE_SERIAL_DEBUG
    // Compiles out the heavy descriptive table when debugging is off, saving huge amounts of SRAM/Flash
    ParamDescription  description[NUM_PARAMS+1];
#endif
  } ParamData;

  // EEPROM utilities are always compiled as they are required by the OLED interface
  void getParametersFromEEPROM(ParamData& par);
  void putParametersToEEPROM(ParamData& par);

#if ENABLE_SERIAL_DEBUG
  int    userInput(double& value);
  double readParameter(int i, ParamData& par);
  void   writeParameter(int i, double value, ParamData& par);
  void   printParameterName(int i, ParamData& par, bool formatted);
  bool   printOneParameter(int i, ParamData& par, bool line, bool num);
  void   printAllParameters(ParamData& par, bool num);
  int    editParameters(ParamData& par);
  int    parameterMenu(ParamData& par);
#else
  // Macros silencing function calls at compile-time when Serial Debug is disabled
  #define userInput(value) (0)
  #define readParameter(i, par) (0.0)
  #define writeParameter(i, value, par) ((void)0)
  #define printParameterName(i, par, formatted) ((void)0)
  #define printOneParameter(i, par, line, num) (false)
  #define printAllParameters(par, num) ((void)0)
  #define editParameters(par) (0)
  #define parameterMenu(par) (0)
#endif

#endif // PARAMETERMENU_H