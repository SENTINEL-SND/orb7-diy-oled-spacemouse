// Header for parameter-menu specific functions and variables
#ifndef PARAMETERMENU_H
  #define PARAMETERMENU_H

  #include <Arduino.h> // Required for int16_t, int8_t and standard types
  #include "config.h"

  // Updated to 32 parameters (slope_at_end restored as parameter 15)
  #define NUM_PARAMS         32   

  #define MAX_PARAM_NAME_LEN 10   // maximum length of any parameter name

  // Magic Number updated to force parameter realignment with CRC verification enabled
  #define MAGIC_NUMBER       1234567843L
  #define BASE_ADDRESS_MAGIC 0
  #define BASE_ADDRESS_PAR   4

  #define PARAM_TYPE_BOOL    1
  #define PARAM_TYPE_INT     2
  #define PARAM_TYPE_FLOAT   3

  // Fixed-point scaling macros calculated at compile-time to prevent runtime float math
  #define SENS_TX_Q7 ((int16_t)(SENS_TX * 128.0f))
  #define SENS_TY_Q7 ((int16_t)(SENS_TY * 128.0f))
  #define SENS_PTZ_Q7 ((int16_t)(SENS_PTZ * 128.0f))
  #define SENS_NTZ_Q7 ((int16_t)(SENS_NTZ * 128.0f))
  #define SENS_RX_Q7 ((int16_t)(SENS_RX * 128.0f))
  #define SENS_RY_Q7 ((int16_t)(SENS_RY * 128.0f))
  #define SENS_RZ_Q7 ((int16_t)(SENS_RZ * 128.0f))
  #define SLOPE_A_Q8 ((int16_t)(MOD_A * 256.0f))
  #define SLOPE_B_Q8 ((int16_t)(MOD_B * 256.0f)) // Fixed-point scaling for Parameter B

  typedef struct _ParamStorage {
    int16_t deadzone               = DEADZONE;

    // Float sensitivity variables replaced by Q7/Q0 fixed-point 16-bit integers
    int16_t transX_sensitivity_q7     = SENS_TX_Q7;
    int16_t transY_sensitivity_q7     = SENS_TY_Q7;
    int16_t pos_transZ_sensitivity_q7 = SENS_PTZ_Q7;
    int16_t neg_transZ_sensitivity_q7 = SENS_NTZ_Q7;
    int16_t gate_neg_transZ           = GATE_NTZ; // Gate NTZ is stored directly as raw integer
    int16_t gate_rotX                 = GATE_RX;
    int16_t gate_rotY                 = GATE_RY;
    int16_t gate_rotZ                 = GATE_RZ;

    // Float sensitivity variables replaced by Q7 fixed-point 16-bit integers
    int16_t rotX_sensitivity_q7       = SENS_RX_Q7;
    int16_t rotY_sensitivity_q7       = SENS_RY_Q7;
    int16_t rotZ_sensitivity_q7       = SENS_RZ_Q7;

    int16_t modFunc                   = MODFUNC;         
    int16_t slope_at_zero_q8          = SLOPE_A_Q8; // slope_at_zero replaced by Q8 fixed-point integer
    int16_t slope_at_end_q8           = SLOPE_B_Q8; // FIXED: slope_at_end restored as Q8 fixed-point integer

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
    int8_t  prioZexclusiveMode     = EXCL_PRIOZ;

    int8_t  compEnabled            = COMP_EN;
    int16_t compNoOfPoints         = COMP_NR;
    int16_t compWaitTime           = COMP_WAIT;
    int16_t compMinMaxDiff         = COMP_MDIFF;
    int16_t compCenterDiff         = COMP_CDIFF;
    
    // Global Sensitivity initialized at 100%
    int16_t globalSens             = 100; 

    // OLED Inactivity Sleep Timer (0 = OFF, 1 = 1m, 2 = 3m, 3 = 5m)
    int8_t  oledSleepTimer         = 2; 

    // Dynamic key shortcuts for Left (L) and Right (R) buttons
    int8_t  keyL_shortcut          = 2; // Default: SM_T (Top)
    int8_t  keyR_shortcut          = 1; // Default: SM_FIT (Fit)

    // Dynamic calibration limits stored in EEPROM
    int16_t minVals[8]             = {-400, -400, -400, -400, -400, -400, -400, -400};
    int16_t maxVals[8]             = {175, 175, 175, 175, 175, 175, 175, 175};
  } ParamStorage;

#if ENABLE_SERIAL_DEBUG
  typedef struct _ParamDescription {
    int   type;
    char  name[MAX_PARAM_NAME_LEN+1];
    void* storage;
  } ParamDescription;
#endif

  typedef struct _ParamData {
    ParamStorage*     values;
#if ENABLE_SERIAL_DEBUG
    // Compile-out the heavy descriptive table when debugging is off
    ParamDescription  description[NUM_PARAMS+1];
#endif
  } ParamData;

  // EEPROM utilities are always needed
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