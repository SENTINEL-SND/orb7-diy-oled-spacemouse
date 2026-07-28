#include <Arduino.h>
#include "calibration.h"
#include "kinematics.h"
#include "config.h"

// A dead zone above the following value will trigger a warning
#define DEADZONEWARNING 10

#ifndef HALLEFFECT
  // A centerpoint outside those values will trigger a warning (384..640)
  #define CENTERPOINTWARNINGMIN (512 - 128)
  #define CENTERPOINTWARNINGMAX (512 + 128)
#else
  // Centerpoint configuration parameters adjusted for Hall Effect sensors
  #define CENTERPOINTWARNINGMIN (400)
  #define CENTERPOINTWARNINGMAX (800)
#endif

#if ENABLE_SERIAL_DEBUG
#ifndef HALLEFFECT
char const *axisNames[] = {"AX:", "AY:", "BX:", "BY:", "CX:", "CY:", "DX:", "DY:"}; // 8
#else
char const *axisNames[] = {"H0:", "H1:", "H2:", "H3:", "H6:", "H7:", "H8:", "H9:"}; // 8
#endif
char const *velNames[] = {"TX:", "TY:", "TZ:", "RX:", "RY:", "RZ:"}; // 6

// FIXED: Parameters updated to explicit int16_t sizing for 32-bit portability
void printArray(int16_t* arr, int16_t size) {
  Serial.print("{");
  for (int16_t i = 0; i < size; i++) {
    Serial.print(arr[i]);
    if (i < size - 1) {
      Serial.print(", ");
    }
  }
  Serial.println("}");
}

// FIXED: Signature updated to use rawReads as int16_t* and keyVals as uint8_t* to prevent compile errors
void debugOutput1(int16_t* rawReads, uint8_t* keyVals) {
  if (isDebugOutputDue()) {
    for (uint8_t i = 0; i < 8; i++) {
      Serial.print(rawReads[i]);
      Serial.print("\t"); 
    }
    for (uint8_t i = 0; i < NUMKEYS; i++) {
      Serial.print(keyVals[i]);
      if (i < NUMKEYS - 1) {
        Serial.print("\t");
      }
    }
    Serial.println(); 
  }
}

// FIXED: Parameter updated to explicit int16_t sizing for 32-bit portability
void debugOutput2(int16_t* centered) {
  if (isDebugOutputDue()) {
    for (uint8_t i = 0; i < 8; i++) {
      Serial.print(centered[i]);
      if (i < 7) Serial.print("\t");
    }
    Serial.println(); 
  }
}

void debugOutput4(int16_t* velocity, uint8_t* keyOut) {
  if (isDebugOutputDue()) {
    for (uint8_t i = 0; i < 6; i++) {
      Serial.print(velocity[i]);
      Serial.print("\t");
    }
    for (uint8_t i = 0; i < NUMKEYS; i++) {
      Serial.print(keyOut[i]);
      if (i < NUMKEYS - 1) Serial.print("\t");
    }
    Serial.println(); 
  }
}

// FIXED: Parameters updated to explicit int16_t sizing for 32-bit portability
void debugOutput5(int16_t* centered, int16_t* velocity) {
  if (isDebugOutputDue()) {
    for (uint8_t i = 0; i < 8; i++) {
      Serial.print(centered[i]);
      Serial.print("\t");
    }
    for (uint8_t i = 0; i < 6; i++) {
      Serial.print(velocity[i]);
      if (i < 5) Serial.print("\t");
    }
    Serial.println(); 
  }
}

// FIXED: Parameter updated to explicit int16_t sizing for 32-bit portability
void debugOutputOffsets(int16_t* offset) {
  if (isDebugOutputDue()) {
    for (uint8_t i = 0; i < 8; i++) {
      Serial.print(offset[i]);
      if (i < 7) {
        Serial.print("\t");
      }
    }
    Serial.println();
  }
}

// FIXED: Parameters updated to explicit int16_t sizing for 32-bit portability
void debugDriftPlotter(int16_t* raw, int16_t* centered, int16_t* offset, int16_t axis) {
  if (isDebugOutputDue()) {
    Serial.print(raw[axis]);
    Serial.print("\t");
    Serial.print(centered[axis]);
    Serial.print("\t");
    Serial.print(offset[axis]);
    Serial.println();
  }
}

#ifndef HALLEFFECT
#define MINMAX_MINWARNING (-250)
#define MINMAX_MAXWARNING (+250)
#else
#define MINMAX_MINWARNING (-100)
#define MINMAX_MAXWARNING (+100)
#endif

// FIXED: Declarations and parameters updated to explicit int16_t sizing
int16_t calcMinMax(int16_t* centered) {    
  static int16_t minMaxCalcState = 0;  
  static int16_t minValue[8];          
  static int16_t maxValue[8];          
  static unsigned long startTime;  

  if (minMaxCalcState == 0) {
    delay(2000);
    for (uint8_t i = 0; i < 8; i++) {
      minValue[i] = +1023; 
      maxValue[i] = -1023; 
    }
    startTime = millis(); 
    minMaxCalcState = 1;  
    Serial.println(F("Start moving the SpaceMouse around for 20s!"));

  } else if (minMaxCalcState == 1) {
    if (millis() - startTime < 20000) {
      for (uint8_t i = 0; i < 8; i++) {
        if (centered[i] < minValue[i]) {minValue[i] = centered[i];}
        if (centered[i] > maxValue[i]) {maxValue[i] = centered[i];}
      }
    } else {
      Serial.println(F("\r\n\r\nStop moving. These are the results for the config.h"));
      minMaxCalcState = 2;
    }

  } else if (minMaxCalcState == 2) {
    Serial.print(F("#define MINVALS ")); printArray(minValue, 8);
    Serial.print(F("#define MAXVALS ")); printArray(maxValue, 8);
    #ifdef HALLEFFECT
      int16_t range[8];
      for (uint8_t i = 0; i < 8; i++) {
        range[i] = maxValue[i] - minValue[i];
      }
      Serial.print(F("Ranges are: ")); printArray(range, 8);
    #endif
    for(uint8_t i = 0; i < 8; i++){
      if(minValue[i] > MINMAX_MINWARNING){
        Serial.print(F("minValue[")); Serial.print(i); Serial.print("] "); Serial.print(axisNames[i]); Serial.print(F(" is small: ")); Serial.println(minValue[i]);
      }
      if(maxValue[i] < MINMAX_MAXWARNING){
        Serial.print(F("maxValue[")); Serial.print(i); Serial.print("] "); Serial.print(axisNames[i]); Serial.print(F(" is small: ")); Serial.println(maxValue[i]);
      }
    }
    minMaxCalcState = 0; 
  } else {
    minMaxCalcState = 0;
  }
  return minMaxCalcState;
}

bool isDebugOutputDue() {
  static unsigned long lastDebugOutput = 0;
  if (millis() - lastDebugOutput > DEBUGDELAY) {
    lastDebugOutput = millis();
    return true;
  } else {
    return false;
  }
}

void updateFrequencyReport() {
  static uint16_t iterationsPerSecond = 0;
  static unsigned long lastFrequencyUpdate = 0;
  iterationsPerSecond++;
  if (millis() - lastFrequencyUpdate > 1000) {
    Serial.print(F("Frequency: "));
    Serial.print(iterationsPerSecond);
    Serial.println(F(" Hz"));
    lastFrequencyUpdate = millis();
    iterationsPerSecond = 0;
  }
}
#endif

// FIXED: Sizing and type declarations updated to explicit int16_t and standard C++ bool
bool busyZeroing(int16_t *centerPoints, uint16_t numIterations, bool debugFlag){
  bool noWarningsOccured = true;
#if ENABLE_SERIAL_DEBUG
  if (debugFlag == true){
    #ifndef HALLEFFECT
      Serial.println(F("Zeroing Joysticks..."));
    #else
      Serial.println(F("Zeroing HALL Sensors..."));
    #endif
  }
#endif
  int16_t act[8];
  uint32_t mean[8] = {0};
  int16_t minValue[8];
  int16_t maxValue[8];
  for (uint8_t i = 0; i < 8; i++){
    minValue[i] = 1023; maxValue[i] = 0;
  }
  for (uint16_t count = 0; count < numIterations; count++){
    readAllFromJoystick(act);
    for (uint8_t i = 0; i < 8; i++){
      mean[i] += act[i];
      if (act[i] < minValue[i]){minValue[i] = act[i];}
      if (act[i] > maxValue[i]){maxValue[i] = act[i];}
    }
  }
  int16_t deadZone[8];
  int16_t maxDeadZone = 0;
  for (uint8_t i = 0; i < 8; i++){
    centerPoints[i] = mean[i] / numIterations;
    deadZone[i] = maxValue[i] - minValue[i];
    if (deadZone[i] > maxDeadZone){maxDeadZone = deadZone[i];}

    // Always validate movement limits even when serial debug is off to support boot loop logic
    if (deadZone[i] > DEADZONEWARNING){ noWarningsOccured = false; }
    if (centerPoints[i] < CENTERPOINTWARNINGMIN || centerPoints[i] > CENTERPOINTWARNINGMAX){ noWarningsOccured = false; }
  }
#if ENABLE_SERIAL_DEBUG
  if (debugFlag){
    Serial.println(F("##  Min - Mean- Max -> Dead Zone"));
    for (uint8_t i = 0; i < 8; i++){
      Serial.print(axisNames[i]); Serial.print(" "); Serial.print(minValue[i]); Serial.print(" - ");
      Serial.print(centerPoints[i]); Serial.print(" - "); Serial.print(maxValue[i]); Serial.print(" -> "); Serial.print(deadZone[i]); Serial.print(" ");
      if (deadZone[i] > DEADZONEWARNING){ Serial.print(F(" Moved axis?")); }
      if (centerPoints[i] < CENTERPOINTWARNINGMIN || centerPoints[i] > CENTERPOINTWARNINGMAX){ Serial.print(F(" Axis not centered?")); }
      Serial.println("");
    }
    Serial.println(F("Using mean as zero position."));
    Serial.print(F("Suggestion for config.h: ")); Serial.print(F("#define DEADZONE ")); Serial.println(maxDeadZone);
  }
#endif
  return noWarningsOccured;
}

// IMPLEMENTED: Decoupled per-axis drift compensation and adaptive muting logic
void compensateDrifts(int16_t *raw, int16_t *center, int16_t *offset, ParamData& par) {
  static int16_t consolidatedOffset[8] = {0};
  static int32_t cmpMean[8] = {0};
  static int16_t cmpMin[8] = {1023, 1023, 1023, 1023, 1023, 1023, 1023, 1023};
  static int16_t cmpMax[8] = {0};
  static int16_t cmpNo[8] = {0};
  static unsigned long cmpStart[8] = {0};
  static int16_t lastCenter[8] = {0};
  static bool isFirstRun = true;

  unsigned long now = millis();

  // Detect external recalibration (e.g., Re-Zero action or startup center update)
  bool centerChanged = false;
  for (uint8_t i = 0; i < 8; i++) {
    if (center[i] != lastCenter[i]) {
      centerChanged = true;
      lastCenter[i] = center[i];
    }
  }

  if (isFirstRun || centerChanged) {
    for (uint8_t i = 0; i < 8; i++) {
      cmpStart[i] = now;
      cmpMin[i] = 1023;
      cmpMax[i] = 0;
      cmpMean[i] = 0;
      cmpNo[i] = 0;
      consolidatedOffset[i] = 0;
      offset[i] = 0;
      lastCenter[i] = center[i];
    }
    isFirstRun = false;
    if (centerChanged) return;
  }

  // FIXED: Fetch and sanitize denominator explicitly against Division-by-Zero hardware crash
  int16_t validPts = par.values->compNoOfPoints;
  if (validPts <= 0) validPts = 1; 

  for (uint8_t i = 0; i < 8; i++) {
    int16_t r = raw[i];

    // Maintain running window boundary limits per sensor axis
    if (r < cmpMin[i]) cmpMin[i] = r;
    if (r > cmpMax[i]) cmpMax[i] = r;

    // Check individual stability bounds
    bool stable = true;
    if (abs(r - center[i]) > par.values->compCenterDiff) stable = false;
    if ((cmpMax[i] - cmpMin[i]) > par.values->compMinMaxDiff) stable = false;

    if (!stable) {
      // Axis is actively moving: fall back to the last stable consolidated offset instantly
      offset[i] = consolidatedOffset[i];

      // Reset tracking status for this independent axis
      cmpMin[i] = 1023;
      cmpMax[i] = 0;
      cmpMean[i] = 0;
      cmpNo[i] = 0;
      cmpStart[i] = now;
      continue;
    }

    // Adaptive Muting: Axis is stable (drifting). Zero out output reading dynamically.
    // Formula: centered[i] = raw[i] - center[i] + offset[i] -> centered[i] = 0
    offset[i] = center[i] - r;

    // Accumulate points if the minimum monitoring window duration is met
    if (now - cmpStart[i] >= (unsigned long)par.values->compWaitTime) {
      cmpMean[i] += r;
      cmpNo[i]++;

      if (cmpNo[i] >= validPts) {
        // Safe commit of new baseline drift offset for this axis
        consolidatedOffset[i] = center[i] - (int16_t)(cmpMean[i] / validPts);
        offset[i] = consolidatedOffset[i];

        // Reset buffer variables to start next window cycle
        cmpMin[i] = 1023;
        cmpMax[i] = 0;
        cmpMean[i] = 0;
        cmpNo[i] = 0;
        cmpStart[i] = now;
      }
    }
  }
}