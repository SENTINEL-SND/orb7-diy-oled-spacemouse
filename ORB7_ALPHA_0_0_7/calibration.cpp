#include <Arduino.h>
#include "calibration.h"
#include "kinematics.h"
#include "config.h"

// A peak-to-peak span above this value triggers a mechanical warning during zeroing.
#define ZEROING_NOISE_WARNING 25

// Sanity boundaries defining the expected hardware midpoint response.
// Values outside this range indicate severe magnetic misalignment, unpowered sensors, or disconnected hardware.
#ifndef HALLEFFECT
  #define CENTERPOINTWARNINGMIN (512 - 128)
  #define CENTERPOINTWARNINGMAX (512 + 128)
#else
  // Wide sanity bounds for 8 Hall Effect sensors operating on 2.56V Internal Reference or 5V Reference.
  // Accommodates resting voltages from magnet offset (100 LSB to 1000 LSB) while flagging disconnected (0) or shorted (1023) pins.
  #define CENTERPOINTWARNINGMIN (100)
  #define CENTERPOINTWARNINGMAX (1000)
#endif

#if ENABLE_SERIAL_DEBUG
#ifndef HALLEFFECT
char const *axisNames[] = {"AX:", "AY:", "BX:", "BY:", "CX:", "CY:", "DX:", "DY:"}; 
#else
char const *axisNames[] = {"H0:", "H1:", "H2:", "H3:", "H6:", "H7:", "H8:", "H9:"}; 
#endif
char const *velNames[] = {"TX:", "TY:", "TZ:", "RX:", "RY:", "RZ:"}; 

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

#ifndef HALLEFFECT
#define MINMAX_MINWARNING (-250)
#define MINMAX_MAXWARNING (+250)
#else
#define MINMAX_MINWARNING (-100)
#define MINMAX_MAXWARNING (+100)
#endif

// Semi-automatic minimum and maximum extreme bounding calibration tracker.
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

// Performs blocking sampling loop to determine average rest position of all sensors.
bool busyZeroing(int16_t *centerPoints, uint16_t numIterations, bool debugFlag){
  bool noWarningsOccured = true;

  // Runtime safety guard against zero iterations division exception
  if (numIterations == 0) numIterations = 1;

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

#if ENABLE_SERIAL_DEBUG
  int16_t noiseSpan[8];
#endif

  for (uint8_t i = 0; i < 8; i++){
    centerPoints[i] = mean[i] / numIterations;
    int16_t span = maxValue[i] - minValue[i];

#if ENABLE_SERIAL_DEBUG
    noiseSpan[i] = span;
#endif

    // Identify mechanical instability or interference during calibration procedure
    if (span > ZEROING_NOISE_WARNING){ noWarningsOccured = false; }
    if (centerPoints[i] < CENTERPOINTWARNINGMIN || centerPoints[i] > CENTERPOINTWARNINGMAX){ noWarningsOccured = false; }
  }

#if ENABLE_SERIAL_DEBUG
  if (debugFlag){
    Serial.println(F("##  Min - Mean - Max -> Noise Span"));
    for (uint8_t i = 0; i < 8; i++){
      Serial.print(axisNames[i]); Serial.print(" "); Serial.print(minValue[i]); Serial.print(" - ");
      Serial.print(centerPoints[i]); Serial.print(" - "); Serial.print(maxValue[i]); Serial.print(" -> "); Serial.print(noiseSpan[i]); Serial.print(" ");
      if (noiseSpan[i] > ZEROING_NOISE_WARNING){ Serial.print(F(" Moved axis?")); }
      if (centerPoints[i] < CENTERPOINTWARNINGMIN || centerPoints[i] > CENTERPOINTWARNINGMAX){ Serial.print(F(" Axis not centered?")); }
      Serial.println("");
    }
    Serial.println(F("Using mean as zero position."));
  }
#endif
  return noWarningsOccured;
}
