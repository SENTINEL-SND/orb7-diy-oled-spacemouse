// File for parameter-menu specific functions heavily optimized to save Flash/SRAM space

#include <Arduino.h>
#include <EEPROM.h>
#include "parameterMenu.h"

/// @brief Generates a fast, low-overhead 8-bit XOR checksum over the entire parameters struct.
/// Guarantees that the loaded EEPROM data block is structurally coherent and not partially written.
static uint8_t calculateChecksum(const ParamStorage &storage) {
  const uint8_t *ptr = (const uint8_t *)&storage;
  uint8_t checksum = 0;
  for (size_t i = 0; i < sizeof(ParamStorage); i++) {
    checksum ^= ptr[i];
  }
  return checksum;
}

/// @brief Enforces strict operational boundaries against corrupt or manually manipulated EEPROM data.
/// Prevents division-by-zero, array out-of-bounds, and mathematical asymptote explosions.
void sanitizeParameters(ParamData &par) {
  if (par.values->gate_transX < 0 || par.values->gate_transX > 100) par.values->gate_transX = GATE_TX;
  if (par.values->globalSens < 10 || par.values->globalSens > 300) par.values->globalSens = 100;
  if (par.values->slope_at_zero_q8 < 26 || par.values->slope_at_zero_q8 > 768) par.values->slope_at_zero_q8 = SLOPE_A_Q8;
  if (par.values->slope_at_end_q8 < 26 || par.values->slope_at_end_q8 > 402) par.values->slope_at_end_q8 = SLOPE_B_Q8;
  if (par.values->exclusiveHysteresis < 0 || par.values->exclusiveHysteresis > 500) par.values->exclusiveHysteresis = EXCL_HYST;
  if (par.values->oledSleepTimer < -1 || par.values->oledSleepTimer > 3) par.values->oledSleepTimer = 2;

  // Sanitize key shortcut indices (Range 0..31) for ALL 4 hardware buttons
  if (par.values->keyL_shortcut < 0 || par.values->keyL_shortcut >= 32) par.values->keyL_shortcut = 2;  // SM_T
  if (par.values->keyR_shortcut < 0 || par.values->keyR_shortcut >= 32) par.values->keyR_shortcut = 1;  // SM_FIT
  if (par.values->key2_shortcut < 0 || par.values->key2_shortcut >= 32) par.values->key2_shortcut = 13; // SM_2
  if (par.values->key1_shortcut < 0 || par.values->key1_shortcut >= 32) par.values->key1_shortcut = 12; // SM_1

  if (par.values->gate_transY < 0 || par.values->gate_transY > 100) par.values->gate_transY = GATE_TY;
  if (par.values->gate_transZ < 0 || par.values->gate_transZ > 100) par.values->gate_transZ = GATE_TZ;
  if (par.values->gate_rotX < 0 || par.values->gate_rotX > 100) par.values->gate_rotX = GATE_RX;
  if (par.values->gate_rotY < 0 || par.values->gate_rotY > 100) par.values->gate_rotY = GATE_RY;
  if (par.values->gate_rotZ < 0 || par.values->gate_rotZ > 100) par.values->gate_rotZ = GATE_RZ;
  if (par.values->deadzoneLevel < 0 || par.values->deadzoneLevel > DEADZONE_MAX) par.values->deadzoneLevel = DEADZONE_DEFAULT;

  // Sanitize modifier curve function selection against corrupt values
  if (par.values->modFunc != 0 && par.values->modFunc != 1 && par.values->modFunc != 3) par.values->modFunc = MODFUNC;

  // Sanitize Q7 Sensitivities against <= 0 values that would trigger division-by-zero or disable kinematic axes
  if (par.values->transX_sensitivity_q7 <= 0) par.values->transX_sensitivity_q7 = SENS_TX_Q7;
  if (par.values->transY_sensitivity_q7 <= 0) par.values->transY_sensitivity_q7 = SENS_TY_Q7;
  if (par.values->pos_transZ_sensitivity_q7 <= 0) par.values->pos_transZ_sensitivity_q7 = SENS_PTZ_Q7;
  if (par.values->neg_transZ_sensitivity_q7 <= 0) par.values->neg_transZ_sensitivity_q7 = SENS_NTZ_Q7;
  if (par.values->rotX_sensitivity_q7 <= 0) par.values->rotX_sensitivity_q7 = SENS_RX_Q7;
  if (par.values->rotY_sensitivity_q7 <= 0) par.values->rotY_sensitivity_q7 = SENS_RY_Q7;
  if (par.values->rotZ_sensitivity_q7 <= 0) par.values->rotZ_sensitivity_q7 = SENS_RZ_Q7;

  // Sanitize dynamic Calibration limits against inverted or impossible polarities
  for (uint8_t i = 0; i < 8; i++) {
    if (par.values->minVals[i] >= 0 || par.values->minVals[i] < -1023) par.values->minVals[i] = -400;
    if (par.values->maxVals[i] <= 0 || par.values->maxVals[i] > 1023) par.values->maxVals[i] = 175;
  }
}

void getParametersFromEEPROM(ParamData &par) {
  long magicNumber = 0L;
  EEPROM.get(BASE_ADDRESS_MAGIC, magicNumber);

  bool checksumValid = false;
  if (magicNumber == MAGIC_NUMBER) {
    EEPROM.get(BASE_ADDRESS_PAR, *par.values);

    // Retrieve stored 8-bit XOR checksum from sequential address directly succeeding the payload
    uint8_t storedChecksum = 0;
    int checksumAddress = BASE_ADDRESS_PAR + sizeof(ParamStorage);
    EEPROM.get(checksumAddress, storedChecksum);

    // Compare stored with dynamically computed checksum
    if (storedChecksum == calculateChecksum(*par.values)) {
      checksumValid = true;
    }
  }

  // Fallback to factory defaults if the memory block fails integrity checks
  if (!checksumValid) {
    putParametersToEEPROM(par);
  }

  // Enforce operational boundary sanitization
  sanitizeParameters(par);
}

void putParametersToEEPROM(ParamData &par) {
  long magicNumber = MAGIC_NUMBER;
  EEPROM.put(BASE_ADDRESS_PAR, *par.values);
  EEPROM.put(BASE_ADDRESS_MAGIC, magicNumber);

  // Calculate and commit the XOR checksum immediately following the struct payload
  uint8_t checksum = calculateChecksum(*par.values);
  int checksumAddress = BASE_ADDRESS_PAR + sizeof(ParamStorage);
  EEPROM.put(checksumAddress, checksum);
}

#if ENABLE_SERIAL_DEBUG
long invalidNum = 0xFFFFFFFF;

/// @brief Fully non-blocking serial character accumulator replacing the legacy synchronous Serial.parseFloat().
/// Prevents the main loop and USB HID reports from freezing while waiting for user inputs.
int userInput(double &value) {
  static char rxBuffer[16] = {0};
  static uint8_t rxIndex = 0;

  if (Serial.available() > 0) {
    char ch = Serial.read();
    char lowerCh = toLowerCase(ch);

    if (ch == '\n' || ch == '\r') {
      if (rxIndex > 0) {
        value = atof(rxBuffer);
        rxIndex = 0;
        rxBuffer[0] = '\0';
        return 1;
      } else {
        return 0;
      }
    }

    if (lowerCh == 'q' || ch == 27) {
      rxIndex = 0;
      rxBuffer[0] = '\0';
      return 2;
    }

    if (isDigit(ch) || ch == '.' || ch == '-') {
      if (rxIndex < 15) {
        rxBuffer[rxIndex++] = ch;
        rxBuffer[rxIndex] = '\0';
      }
      return 0;
    }

    if (ch != ' ' && ch != '\t') {
      rxIndex = 0;
      rxBuffer[0] = '\0';
      return 4;
    }
  }

  return 0;
}

int parameterMenu(ParamData &par) {
  static int state = 0;
  static int menuMode = -1;

  if (state == 0 || state == 1) {
    Serial.println(F("\r\n[1] List [3] Load [4] Save"));
    menuMode = -1;
    state = 2;
  }

  if (state == 2) {
    double num;
    int result = userInput(num);
    if (result == 1) {
      menuMode = (int)num;
      state = 3;
    } 
    else if (result == 2) {
      state = 0;
    } 
    else if (result == 3 || result == 4) {
      state = 1;
    }
  }

  if (state == 3) {
    switch (menuMode) {
    case 1:
      printAllParameters(par, true);
      state = 1;
      break;
    case 2:
      if (editParameters(par) == 0) {
        state = 1;
      }
      break;
    case 3:
      getParametersFromEEPROM(par);
      state = 1;
      break;
    case 4:
      putParametersToEEPROM(par);
      state = 1;
      break;
    default:
      state = 1;
    }
  }
  return state;
}

int editParameters(ParamData &par) {
  static int state = 0;
  static bool isFloat;
  static int parIndex = 0;
  static double parValue = 0.0;
  int result = 0;

  if (state == 0) {
    parIndex = 0;
    parValue = 0.0;
    state = 1;
  }

  if (state == 1) {
    printAllParameters(par, true);
    state = 2;
  }

  if (state == 2) {
    double num;
    result = userInput(num);
    if (result == 1) {
      parIndex = (int)num;
      state = 3;
    } else if (result == 2) {
      state = 0;
    } else if (result != 0) {
      state = 1;
    }
  }

  if (state == 3) {
    if (parIndex >= 1 && parIndex <= NUM_PARAMS) {
      isFloat = printOneParameter(parIndex, par, false, true);
      state = 4;
    } else {
      state = 1;
    }
  }

  if (state == 4) {
    result = userInput(parValue);
    if (result == 1) {
      state = 5;
    } else if (result != 0) {
      state = 1;
    }
  }

  if (state == 5) {
    writeParameter(parIndex, parValue, par);
    state = 1;
  }

  return state;
}

void printParameterName(int i, ParamData &par, bool formatted) {
  Serial.print(par.description[i].name);
  if (formatted) {
    int c = MAX_PARAM_NAME_LEN - strlen(par.description[i].name);
    for (int n = 0; n < c; n++) {
      Serial.print(' ');
    }
  }
}

void printAllParameters(ParamData &par, bool num) {
  for (int i = 1; i <= NUM_PARAMS; i++) {
    printOneParameter(i, par, true, num);
  }
}

bool printOneParameter(int i, ParamData &par, bool line, bool numbering) {
  bool isFloat = false;
  if (i >= 1 && i <= NUM_PARAMS) {
    // Treat Q7 and Q8 parameters logically as floats in serial CLI outputs to preserve human readability
    isFloat = (par.description[i].type == PARAM_TYPE_FLOAT) ||
              (i == 2 || i == 3 || i == 4 || i == 5 || i == 11 || i == 12 || i == 13 || i == 15 || i == 16);
    if (numbering) {
      if (i <= 9) Serial.print(' ');
      Serial.print(i);
      Serial.print(' ');
    }
    printParameterName(i, par, true);
    Serial.print(' ');
    double value = readParameter(i, par);
    if (isFloat) {
      Serial.print(value);
    } else {
      Serial.print((int)trunc(value));
    }
    if (line) {
      Serial.println();
    }
  }
  return isFloat;
}

/// @brief Reads and translates parameters for the Serial CLI, converting fixed-point back to float.
double readParameter(int i, ParamData &par) {
  double value = NAN;
  if (i >= 1 && i <= NUM_PARAMS) {
    if (i == 2 || i == 3 || i == 4 || i == 5 || i == 11 || i == 12 || i == 13) {
      value = (double)(*(int16_t *)par.description[i].storage) / 128.0; // Decode Q7 back to standard decimal
    } else if (i == 15 || i == 16) {
      value = (double)(*(int16_t *)par.description[i].storage) / 256.0; // Decode Q8 back to standard decimal
    } else {
      switch (par.description[i].type) {
      case PARAM_TYPE_BOOL:
        value = *(int8_t *)par.description[i].storage;
        break;
      case PARAM_TYPE_INT:
        value = *(int16_t *)par.description[i].storage;
        break;
      case PARAM_TYPE_FLOAT:
        value = *(float *)par.description[i].storage;
        break;
      }
    }
  }
  return value;
}

/// @brief Receives human inputs from the Serial CLI and safely converts them to fixed-point integers.
void writeParameter(int i, double value, ParamData &par) {
  if (i >= 1 && i <= NUM_PARAMS) {
    if (i == 2 || i == 3 || i == 4 || i == 5 || i == 11 || i == 12 || i == 13) {
      // Clamped Q7 conversion against double precision overflow bounds (16-bit safe max)
      double q7_val = value * 128.0;
      if (q7_val < 1.0) q7_val = 1.0;
      if (q7_val > 32767.0) q7_val = 32767.0;
      *(int16_t *)par.description[i].storage = (int16_t)trunc(q7_val);
    } else if (i == 15 || i == 16) {
      // Clamped Q8 conversion against double precision overflow & strict math domain errors
      double q8_val = value * 256.0;
      if (q8_val < 26.0) q8_val = 26.0; // Constrain curve bottom minimum to 0.1
      if (q8_val > 768.0) q8_val = 768.0; // Constrain curve maximum to 3.0
      *(int16_t *)par.description[i].storage = (int16_t)trunc(q8_val);
    } else {
      switch (par.description[i].type) {
      case PARAM_TYPE_BOOL:
        *((int8_t *)par.description[i].storage) = (int8_t)trunc(value);
        break;
      case PARAM_TYPE_INT:
        *((int16_t *)par.description[i].storage) = (int16_t)trunc(value);
        break;
      case PARAM_TYPE_FLOAT:
        *((float *)par.description[i].storage) = (float)value;
        break;
      }
    }
  }
}
#endif
