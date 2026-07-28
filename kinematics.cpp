// This file contains all functions to calculate the kinematics

#include "config.h"
#include <Arduino.h>
#include <math.h>

#define sign(x) ((x) < 0 ? -1 : ((x) > 0 ? 1 : 0)) // Define Signum Function

#include "parameterMenu.h"
#include "kinematics.h"
#include "calibration.h"

// Do not change this! Use independent sensitivity multipliers.
#define TOTALSENSITIVITY 350

// Ultra-fast 16-bit inline mapping helper to bypass register pushing overhead of fastMap16
inline int16_t mapToSensitivity(int32_t numer, int32_t denom, int16_t offset) {
  // Safety guard against division by zero and null dynamic boundaries
  if (denom <= 0) return 0; 
  return (int16_t)(numer * TOTALSENSITIVITY / denom + offset);
}

// Flash-optimized signed 32-bit division helper (eliminates sign logic overhead)
static int16_t divideBySensitivity(int16_t val, int16_t sens_q7) {
  if (val == 0 || sens_q7 <= 0) return 0;
  int32_t numer = (int32_t)val << 7;
  return (int16_t)(numer / sens_q7);
}

/// @brief Function to modify the input value according to mathematical curves using optimized cached trig math
int modifierFunction(int x, ParamData& par) {
  if (x == 0) return 0; // Immediate bypass for idle axis to save massive CPU cycles

  x = constrain(x, -TOTALSENSITIVITY, +TOTALSENSITIVITY);
  int32_t abs_x = (x < 0) ? -x : x;
  int32_t sign_x = (x < 0) ? -1 : 1;

  int32_t y;
  
  // Handle both mathematical curves (1 = Squared, 3 = Squared Tangent)
  if(par.values->modFunc == 1 || par.values->modFunc == 3){
    // Restored full curve flexibility with low-latency parameter caching
    static int16_t last_slope_a = -1;
    static int16_t last_slope_b = -1;
    static float cached_a = 0.0f;
    static float cached_b = 0.0f;
    static float cached_tan_b = 0.0f;

    // Recalculate parameters and base tangent only if they are changed by the user in settings
    if (par.values->slope_at_zero_q8 != last_slope_a || par.values->slope_at_end_q8 != last_slope_b) {
      last_slope_a = par.values->slope_at_zero_q8;
      last_slope_b = par.values->slope_at_end_q8;
      cached_a = (float)last_slope_a * (1.0f / 256.0f);
      cached_b = (float)last_slope_b * (1.0f / 256.0f);
      cached_tan_b = tanf(cached_b);
    }

    // Normalized input: xn = abs_x / 350.0
    float xn = (float)abs_x * (1.0f / 350.0f);

    if (par.values->modFunc == 3) {
      // modFunc == 3: Squared tangent function y = tan(b*(|x|^a*sign(x)))/tan(b)
      if (fabsf(cached_tan_b) > 1e-6f) { // Safety guard against division by zero
        float y_val = tanf(cached_b * powf(xn, cached_a)) / cached_tan_b;
        y = (int32_t)(y_val * 350.0f);
      } else {
        y = abs_x;
      }
    } else {
      // modFunc == 1: Squared function y = (|x|/350)^a * sign(x)
      float y_val = powf(xn, cached_a);
      y = (int32_t)(y_val * 350.0f);
    }
  } else {
    // Linear bypass (modFunc == 0)
    y = abs_x;
  }

  y = constrain(y, 0, TOTALSENSITIVITY);
  return (int)(y * sign_x);
}

/// @brief Function to read and store analogue voltages for each joystick axis using optimized 2x oversampling and integer EMA filtering
void readAllFromJoystick(int16_t *rawReads){
  static const uint8_t pinList[8] = PINLIST;
  static const uint8_t invertList[8] = INVERTLIST;
  const uint8_t OVERSAMPLES = 2;
  
  // EMA filter coefficient shift (SHIFT=2 means alpha = 1 / 2^2 = 0.25 for quick but smooth response)
  const uint8_t EMA_FILTER_SHIFT = 2; 
  
  static int16_t filterState[8];
  static bool isFirstRun = true;

  // Seed the filter states with the raw readings on first startup to avoid slow calibration climb
  if (isFirstRun) {
    for (uint8_t i = 0; i < 8; i++) {
      filterState[i] = analogRead(pinList[i]);
    }
    isFirstRun = false;
  }

  for (uint8_t i = 0; i < 8; i++) {
    uint16_t sum = 0; 
    for (uint8_t j = 0; j < OVERSAMPLES; j++) {
      sum += analogRead(pinList[i]);
    }

    uint16_t averagedRead = sum >> 1; 

    // Apply non-stuck integer Exponential Moving Average (EMA) filtering
    int16_t diff = (int16_t)averagedRead - filterState[i];
    int16_t step = diff >> EMA_FILTER_SHIFT;
    if (step == 0 && diff != 0) {
      step = (diff > 0) ? 1 : -1;
    }
    filterState[i] += step;

    if (invertList[i] == 1) {
      rawReads[i] = 1023 - filterState[i];
    } else {
      rawReads[i] = filterState[i];
    }
  }
}

/// @brief Takes centered values, applies a deadzone and maps the values dynamically to +/- 350.
void FilterAnalogReadOuts(int16_t* centered, ParamData& par){
  int16_t dz = par.values->deadzone;

  for(uint8_t i = 0; i < 8; i++){
    int16_t val = centered[i];
    if (val < dz && val > -dz){
      centered[i] = 0;
    }else{
      if(val < 0){ 
        // Isolate denominator calculation to protect against corrupt/blank EEPROM values
        int32_t denom = (int32_t)(-dz - par.values->minVals[i]);
        
        // Only apply constrain and mapping if calibration bounds are valid (denom > 0)
        if (denom > 0) {
          int16_t clamped_val = constrain(val, (int16_t)par.values->minVals[i], (int16_t)-dz);
          centered[i] = mapToSensitivity((int32_t)(clamped_val - par.values->minVals[i]), denom, -TOTALSENSITIVITY);
        } else {
          centered[i] = 0; // Safe fail-silent state
        }
      }else{ 
        // Isolate denominator calculation to protect against corrupt/blank EEPROM values
        int32_t denom = (int32_t)(par.values->maxVals[i] - dz);
        
        // Only apply constrain and mapping if calibration bounds are valid (denom > 0)
        if (denom > 0) {
          int16_t clamped_val = constrain(val, dz, (int16_t)par.values->maxVals[i]);
          centered[i] = mapToSensitivity((int32_t)(clamped_val - dz), denom, 0);
        } else {
          centered[i] = 0; // Safe fail-silent state
        }
      }
    }
  }
}

void _calculateKinematicSensors(int16_t* centered, int16_t* velocity, bool prio_z_exclusive){
  #ifndef HALLEFFECT
    uint8_t cntN = 0;
    uint8_t cntP = 0;
    if(centered[AX] < 0){cntN += 1;} if(centered[AX] > 0){cntP += 1;}
    if(centered[BX] < 0){cntN += 1;} if(centered[BX] > 0){cntP += 1;}
    if(centered[CX] < 0){cntN += 1;} if(centered[CX] > 0){cntP += 1;}
    if(centered[DX] < 0){cntN += 1;} if(centered[DX] > 0){cntP += 1;}

    bool zMove = ((cntP >= 3 && cntN == 0) || (cntN >= 3 && cntP == 0));

    velocity[TRANSX] = (-centered[CY] +centered[AY]);
    velocity[TRANSY] = (-centered[BY] +centered[DY]);
    velocity[TRANSZ] = (-centered[AX] -centered[BX] -centered[CX] -centered[DX]);

    if (prio_z_exclusive && zMove) 
    {
      velocity[ROTX] = 0;
      velocity[ROTY] = 0;
      velocity[ROTZ] = 0;
    }
    else
    {
      velocity[ROTX] = (-centered[CX] + centered[AX]);
      velocity[ROTY] = (-centered[BX] + centered[DX]);
      velocity[ROTZ] = (+centered[AY] + centered[BY] + centered[CY] + centered[DY]);
    }
  #else
    velocity[TRANSX] = (centered[HES1] -centered[HES0] +centered[HES6] -centered[HES7]) / 2;
    velocity[TRANSY] = (centered[HES2] -centered[HES3] +centered[HES9] -centered[HES8]) / 2;
    velocity[TRANSZ] = (centered[HES0] +centered[HES1] +centered[HES2] +centered[HES3] +centered[HES6] +centered[HES7] +centered[HES8] +centered[HES9]) / 4;
    velocity[ROTX]   = (centered[HES0] +centered[HES1] -centered[HES6] -centered[HES7]) / 2;
    velocity[ROTY]   = (centered[HES8] +centered[HES9] -centered[HES2] -centered[HES3]) / 2;
    velocity[ROTZ]   = (centered[HES0] +centered[HES2] +centered[HES6] +centered[HES8] -centered[HES1] -centered[HES3] -centered[HES7] -centered[HES9]) / 4;
  #endif
}

void calculateKinematic(int16_t* centered, int16_t* velocity, ParamData& par){
  _calculateKinematicSensors(centered, velocity, par.values->exclusiveMode);

  // Optimized boolean checks for direct inversion branches saving several Flash bytes
  if(par.values->invX){velocity[TRANSX] = -velocity[TRANSX];}
  if(par.values->invY){velocity[TRANSY] = -velocity[TRANSY];}
  if(par.values->invZ){velocity[TRANSZ] = -velocity[TRANSZ];}
  if(par.values->invRX){velocity[ROTX]   = -velocity[ROTX];}
  if(par.values->invRY){velocity[ROTY]   = -velocity[ROTY];}
  if(par.values->invRZ){velocity[ROTZ]   = -velocity[ROTZ];}

  // Gated Translation X
  if (velocity[TRANSX] != 0) {
    velocity[TRANSX] = modifierFunction(divideBySensitivity(velocity[TRANSX], par.values->transX_sensitivity_q7), par);
    if (abs(velocity[TRANSX]) < par.values->gate_neg_transZ) {
      velocity[TRANSX] = 0;
    }
  }

  // Gated Translation Y
  if (velocity[TRANSY] != 0) {
    velocity[TRANSY] = modifierFunction(divideBySensitivity(velocity[TRANSY], par.values->transY_sensitivity_q7), par);
    if (abs(velocity[TRANSY]) < par.values->gate_neg_transZ) {
      velocity[TRANSY] = 0;
    }
  }

  // Symmetrically scaled and gated Translation Z
  if (velocity[TRANSZ] != 0) {
    if (velocity[TRANSZ] < 0) {
      velocity[TRANSZ] = modifierFunction(divideBySensitivity(velocity[TRANSZ], par.values->neg_transZ_sensitivity_q7), par);                           
    } else {                                                                                  
      velocity[TRANSZ] = modifierFunction(divideBySensitivity(velocity[TRANSZ], par.values->pos_transZ_sensitivity_q7), par);
    }

    if (abs(velocity[TRANSZ]) < par.values->gate_neg_transZ) {
      velocity[TRANSZ] = 0;
    }
  }

  if (velocity[ROTX] != 0) {
    velocity[ROTX] = modifierFunction(divideBySensitivity(velocity[ROTX], par.values->rotX_sensitivity_q7), par);                                 
    if (abs(velocity[ROTX]) < par.values->gate_rotX) {
      velocity[ROTX] = 0;
    }
  }

  if (velocity[ROTY] != 0) {
    velocity[ROTY] = modifierFunction(divideBySensitivity(velocity[ROTY], par.values->rotY_sensitivity_q7), par); 
    if (abs(velocity[ROTY]) < par.values->gate_rotY) {
      velocity[ROTY] = 0;
    }
  }

  if (velocity[ROTZ] != 0) {
    velocity[ROTZ] = modifierFunction(divideBySensitivity(velocity[ROTZ], par.values->rotZ_sensitivity_q7), par); 
    if (abs(velocity[ROTZ]) < par.values->gate_rotZ) {
      velocity[ROTZ] = 0;
    }
  }

  // Apply master global sensitivity scaling on all 6 output axes with 32-bit overflow prevention
  if (par.values->globalSens != 100) {
    for (uint8_t i = 0; i < 6; i++) {
      int32_t scaledVal = ((int32_t)velocity[i] * par.values->globalSens) / 100;
      velocity[i] = (int16_t)constrain(scaledVal, -TOTALSENSITIVITY, TOTALSENSITIVITY);
    }
  }
} 

void switchXY(int16_t *velocity){
  int16_t tmp      = velocity[TRANSX];
  velocity[TRANSX] = velocity[TRANSY];
  velocity[TRANSY] = tmp;

  tmp = velocity[ROTX];
  velocity[ROTX] = velocity[ROTY];
  velocity[ROTY] = tmp;
}

void switchYZ(int16_t *velocity){
  int16_t tmp      = velocity[TRANSY];
  velocity[TRANSY] = velocity[TRANSZ];
  velocity[TRANSZ] = tmp;

  tmp = velocity[ROTY];
  velocity[ROTY] = velocity[ROTZ];
  velocity[ROTZ] = tmp;
}

void exclusiveMode(int16_t *velocity, int16_t hysteresis){  

  // Enhanced Exclusive Mode with Auto-Reset on Relaxation (Neutral Unlocking)
  // 0 = NEUTRAL, 1 = TRANSLATION, 2 = ROTATION
  static uint8_t mode = 0; 

  uint16_t totalRot   = abs(velocity[ROTX]  ) + abs(velocity[ROTY]  ) + abs(velocity[ROTZ]  );
  uint16_t totalTrans = abs(velocity[TRANSX]) + abs(velocity[TRANSY]) + abs(velocity[TRANSZ]);

  // Relaxation threshold: If total force drops below 35 during hand release/direction change,
  // unlock mode to NEUTRAL (0) so the next gesture registers instantly.
  const uint16_t RELAX_THRESHOLD = 35;

  if (totalRot < RELAX_THRESHOLD && totalTrans < RELAX_THRESHOLD) {
    mode = 0; // Reset to NEUTRAL on motion relaxation
  }

  if (mode == 0) {
    // In NEUTRAL: Evaluate pure dominance without hysteresis barrier
    if (totalRot > totalTrans && totalRot >= 15) {
      mode = 2; // Latch ROTATION
    } else if (totalTrans > totalRot && totalTrans >= 15) {
      mode = 1; // Latch TRANSLATION
    }
  } else if (mode == 1) { // Currently in TRANSLATION
    // Must overcome hysteresis barrier to force mid-motion switch to ROTATION
    if (totalRot > (totalTrans + (uint16_t)hysteresis)) {
      mode = 2;
    }
  } else if (mode == 2) { // Currently in ROTATION
    // Must overcome hysteresis barrier to force mid-motion switch to TRANSLATION
    if (totalTrans > (totalRot + (uint16_t)hysteresis)) {
      mode = 1;
    }
  }

  if (mode == 2) { // Active ROTATION: Mute Translation
    velocity[TRANSX] = 0;
    velocity[TRANSY] = 0;
    velocity[TRANSZ] = 0;
  } else if (mode == 1) { // Active TRANSLATION: Mute Rotation
    velocity[ROTX] = 0;
    velocity[ROTY] = 0;
    velocity[ROTZ] = 0;
  }
}