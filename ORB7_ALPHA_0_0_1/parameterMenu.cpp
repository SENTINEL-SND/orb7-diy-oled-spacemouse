// File for parameter-menu specific functions optimized to save Flash space

#include <Arduino.h>
#include <EEPROM.h>
#include "parameterMenu.h"

// Local helper function to calculate an 8-bit XOR checksum of the ParamStorage bytes
static uint8_t calculateChecksum(const ParamStorage &storage) {
  const uint8_t *ptr = (const uint8_t *)&storage;
  uint8_t checksum = 0;
  for (size_t i = 0; i < sizeof(ParamStorage); i++) {
    checksum ^= ptr[i];
  }
  return checksum;
}

void getParametersFromEEPROM(ParamData &par) {
  long magicNumber = 0L;
  EEPROM.get(BASE_ADDRESS_MAGIC, magicNumber);
  
  bool checksumValid = false;
  if (magicNumber == MAGIC_NUMBER) {
    EEPROM.get(BASE_ADDRESS_PAR, *par.values);
    
    // Retrieve stored 8-bit XOR checksum from sequential address
    uint8_t storedChecksum = 0;
    int checksumAddress = BASE_ADDRESS_PAR + sizeof(ParamStorage);
    EEPROM.get(checksumAddress, storedChecksum);
    
    // Compare stored with computed checksum
    if (storedChecksum == calculateChecksum(*par.values)) {
      checksumValid = true;
    }
  }

  if (!checksumValid) {
    // If checksum is corrupt or magic number is mismatched, flash default values
    putParametersToEEPROM(par); 
  }
}

void putParametersToEEPROM(ParamData &par) {
  long magicNumber = MAGIC_NUMBER;
  EEPROM.put(BASE_ADDRESS_PAR, *par.values);
  EEPROM.put(BASE_ADDRESS_MAGIC, magicNumber);
  
  // Compute and store 8-bit XOR checksum
  uint8_t checksum = calculateChecksum(*par.values);
  int checksumAddress = BASE_ADDRESS_PAR + sizeof(ParamStorage);
  EEPROM.put(checksumAddress, checksum);
}

#if ENABLE_SERIAL_DEBUG
long invalidNum = 0xFFFFFFFF;

// Fully non-blocking serial character accumulator to replace the legacy sychronous parser
int userInput(double &value) { 
  static char rxBuffer[16] = {0};
  static uint8_t rxIndex = 0;

  if (Serial.available() > 0) {
    char ch = Serial.read();
    char lowerCh = toLowerCase(ch);

    // 1. Carriage return or newline terminates input and processes buffer
    if (ch == '\n' || ch == '\r') {
      if (rxIndex > 0) {
        value = atof(rxBuffer);
        rxIndex = 0;
        rxBuffer[0] = '\0';
        return 1; // Completed entry
      } else {
        return 0; // Empty Enter key pressed (treated as no-op)
      }
    }

    // 2. Escape or 'q' exits the current menu state
    if (lowerCh == 'q' || ch == 27) {
      rxIndex = 0;
      rxBuffer[0] = '\0';
      return 2; // Exit code
    }

    // 3. Accumulate valid numeric characters
    if (isDigit(ch) || ch == '.' || ch == '-') {
      if (rxIndex < 15) {
        rxBuffer[rxIndex++] = ch;
        rxBuffer[rxIndex] = '\0';
      }
      return 0; // Typing in progress (treated as no-op to prevent state-machine reset)
    }

    // 4. Handle invalid chars (spaces and tabs are ignored silently, others trigger redraw)
    if (ch != ' ' && ch != '\t') {
      rxIndex = 0;
      rxBuffer[0] = '\0';
      return 4; // Trigger redraw/refresh
    }
  }

  // No new character was processed or still waiting for input
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
    // Treat Q7 and Q8 parameters as floats in serial CLI outputs to preserve usability
    isFloat = (par.description[i].type == PARAM_TYPE_FLOAT) || 
              (i == 2 || i == 3 || i == 4 || i == 5 || i == 10 || i == 11 || i == 12 || i == 14 || i == 15);
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

double readParameter(int i, ParamData &par) {
  double value = NAN;
  if (i >= 1 && i <= NUM_PARAMS) {
    // Intercept and scale internal Q7 / Q8 integers back to standard floats on the fly for CLI
    if (i == 2 || i == 3 || i == 4 || i == 5 || i == 10 || i == 11 || i == 12) {
      value = (double)(*(int16_t *)par.description[i].storage) / 128.0;
    } else if (i == 14 || i == 15) {
      value = (double)(*(int16_t *)par.description[i].storage) / 256.0;
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

void writeParameter(int i, double value, ParamData &par) {
  if (i >= 1 && i <= NUM_PARAMS) {
    // Scale incoming user decimals to internal Q7 / Q8 integers dynamically on parameter writes
    if (i == 2 || i == 3 || i == 4 || i == 5 || i == 10 || i == 11 || i == 12) {
      *(int16_t *)par.description[i].storage = (int16_t)trunc(value * 128.0);
    } else if (i == 14 || i == 15) {
      *(int16_t *)par.description[i].storage = (int16_t)trunc(value * 256.0);
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