// This file contains all functions to calculate the kinematics

#include "config.h"
#include <Arduino.h>
#include <math.h>

#define sign(x) ((x) < 0 ? -1 : ((x) > 0 ? 1 : 0)) // Define Signum Function

#include "parameterMenu.h"
#include "kinematics.h"
#include "calibration.h"

// Fallback safety definitions for hardware parameters if they are missing from an older config.h
#ifndef ADC_OVERSAMPLES
  #define ADC_OVERSAMPLES 2
#endif

#ifndef ADC_EMA_SHIFT
  #define ADC_EMA_SHIFT 2
#endif

#ifndef EXCL_RELAX_THRESHOLD
  #define EXCL_RELAX_THRESHOLD 35
#endif

// Do not change this! Standard baseline bound to match the HID logical scale. Use independent sensitivity multipliers instead.
#define TOTALSENSITIVITY 350

/// @brief Ultra-fast 16-bit inline mapping helper to bypass the standard Arduino map() register pushing overhead.
/// Also incorporates a strict guard against division by zero caused by corrupted EEPROM boundaries.
inline int16_t mapToSensitivity(int32_t numer, int32_t denom, int16_t offset) {
  // Safety guard against division by zero and null dynamic boundaries
  if (denom <= 0) return 0; 
  return (int16_t)(numer * TOTALSENSITIVITY / denom + offset);
}

/// @brief Flash-optimized signed 32-bit division helper utilizing Q7 fixed-point parameters.
/// Replaces heavy floating-point operations while preventing sign logic overflows.
static int16_t divideBySensitivity(int16_t val, int16_t sens_q7) {
  if (val == 0 || sens_q7 <= 0) return 0;
  int32_t numer = (int32_t)val << 7; // Shift left 7 bits to apply Q7 scaling division
  return (int16_t)(numer / sens_q7);
}

/// @brief Function to modify the input value according to mathematical curves.
/// Optimized to cache trigonometric processing, saving hundreds of CPU cycles per loop.
int modifierFunction(int x, ParamData& par) {
  if (x == 0) return 0; // Immediate bypass for idle axis to save massive CPU cycles

  // Constrain to physical domain limits before math operations
  x = constrain(x, -TOTALSENSITIVITY, +TOTALSENSITIVITY);
  int32_t abs_x = (x < 0) ? -x : x;
  int32_t sign_x = (x < 0) ? -1 : 1;

  int32_t y;
  
  // Handle both mathematical curves (1 = Squared, 3 = Squared Tangent)
  if(par.values->modFunc == 1 || par.values->modFunc == 3){
    // Restored full curve flexibility with low-latency parameter caching.
    // Memory remains static so tanf() and powf() constants aren't recalculated every microsecond.
    static int16_t last_slope_a = -1;
    static int16_t last_slope_b = -1;
    static float cached_a = 0.0f;
    static float cached_b = 0.0f;
    static float cached_tan_b = 0.0f;

    // Recalculate parameters and base tangent ONLY if they are changed by the user in EEPROM/Menu
    if (par.values->slope_at_zero_q8 != last_slope_a || par.values->slope_at_end_q8 != last_slope_b) {
      // Clamping to mathematically safe zones (Curve A: 0.1..3.0 | Curve B: 0.1..1.56 rad)
      // This strict bounding prevents asymptote explosions to infinity/NaN inside tanf().
      last_slope_a = constrain(par.values->slope_at_zero_q8, 26, 768);
      last_slope_b = constrain(par.values->slope_at_end_q8, 26, 402);
      
      // Convert Q8 fixed-point back to float exclusively for the modifier cache
      cached_a = (float)last_slope_a * (1.0f / 256.0f);
      cached_b = (float)last_slope_b * (1.0f / 256.0f);
      cached_tan_b = tanf(cached_b);
    }

    // Normalized input: xn = abs_x / 350.0
    float xn = (float)abs_x * (1.0f / 350.0f);

    if (par.values->modFunc == 3) {
      // modFunc == 3: Squared tangent function y = tan(b*(|x|^a*sign(x)))/tan(b)
      if (fabsf(cached_tan_b) > 1e-6f) { // Secondary safety guard against division by zero
        float y_val = tanf(cached_b * powf(xn, cached_a)) / cached_tan_b;
        y = (int32_t)(y_val * 350.0f);
      } else {
        y = abs_x; // Fallback to linear if cache corruption occurs
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

  // Ensure mathematical results never breach HID logical maximums
  y = constrain(y, 0, TOTALSENSITIVITY);
  return (int)(y * sign_x);
}

/// @brief Reads analog hardware using configured integer Exponential Moving Average (EMA) and hardware Oversampling.
void readAllFromJoystick(int16_t *rawReads){
  static const uint8_t pinList[8] = PINLIST;
  static const uint8_t invertList[8] = INVERTLIST;
  
  static int16_t filterState[8];
  static bool isFirstRun = true;

  // Seed the filter states with the raw readings on first startup. 
  // Prevents the system from slowly "climbing" to the target value due to EMA inertia.
  if (isFirstRun) {
    for (uint8_t i = 0; i < 8; i++) {
      filterState[i] = analogRead(pinList[i]);
    }
    isFirstRun = false;
  }

  // Runtime safety guard against division-by-zero if ADC_OVERSAMPLES is defined as 0
  const uint8_t safeOversamples = (ADC_OVERSAMPLES < 1) ? 1 : ADC_OVERSAMPLES;

  for (uint8_t i = 0; i < 8; i++) {
    uint16_t sum = 0; 
    for (uint8_t j = 0; j < safeOversamples; j++) {
      sum += analogRead(pinList[i]);
    }

    uint16_t averagedRead = sum / safeOversamples; 

    // Apply non-stuck integer Exponential Moving Average (EMA) filtering.
    // Extremely efficient: Replaces standard floats by using right bit-shifts for fractioning.
    int16_t diff = (int16_t)averagedRead - filterState[i];
    int16_t step = (ADC_EMA_SHIFT > 0) ? (diff >> ADC_EMA_SHIFT) : diff;
    
    // Prevent the integer math from getting "stuck" when the difference is smaller than the shift divisor
    if (step == 0 && diff != 0) {
      step = (diff > 0) ? 1 : -1;
    }
    filterState[i] += step;

    if (invertList[i] == 1) {
      rawReads[i] = 1023 - filterState[i]; // Hardware polarity inversion
    } else {
      rawReads[i] = filterState[i];
    }
  }
}

/// @brief Takes centered values, enforces deadzones, and scales inputs dynamically based on user calibration limits.
void FilterAnalogReadOuts(int16_t* centered, ParamData& par){
  // Sanitize deadzone parameter against negative values which would corrupt bounding logic
  int16_t dz = (par.values->deadzone < 0) ? 0 : par.values->deadzone;

  for(uint8_t i = 0; i < 8; i++){
    int16_t val = centered[i];
    if (val < dz && val > -dz){
      centered[i] = 0; // Filtered by deadzone, enforce absolute hardware zero
    }else{
      if(val < 0){ 
        // Isolate denominator calculation to protect against corrupt/blank EEPROM limits
        int32_t denom = (int32_t)(-dz - par.values->minVals[i]);
        
        // Only apply constrain and mapping if calibration bounds are valid (denom > 0)
        if (denom > 0) {
          int16_t clamped_val = constrain(val, (int16_t)par.values->minVals[i], (int16_t)-dz);
          centered[i] = mapToSensitivity((int32_t)(clamped_val - par.values->minVals[i]), denom, -TOTALSENSITIVITY);
        } else {
          centered[i] = 0; // Safe fail-silent state
        }
      }else{ 
        // Isolate denominator calculation to protect against corrupt/blank EEPROM limits
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

/// @brief Fuses the 8 independent sensor streams into the 6 physical Degrees of Freedom using matrix mixing.
void _calculateKinematicSensors(int16_t* centered, int16_t* velocity, bool prio_z_exclusive){
  #ifndef HALLEFFECT
    // Legacy resistive joystick matrix
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
    // Default Hall-Effect matrix mixing resolving polarities for typical physical magnet placements
    velocity[TRANSX] = (centered[HES1] -centered[HES0] +centered[HES6] -centered[HES7]) / 2;
    velocity[TRANSY] = (centered[HES2] -centered[HES3] +centered[HES9] -centered[HES8]) / 2;
    velocity[TRANSZ] = (centered[HES0] +centered[HES1] +centered[HES2] +centered[HES3] +centered[HES6] +centered[HES7] +centered[HES8] +centered[HES9]) / 4;
    velocity[ROTX]   = (centered[HES0] +centered[HES1] -centered[HES6] -centered[HES7]) / 2;
    velocity[ROTY]   = (centered[HES8] +centered[HES9] -centered[HES2] -centered[HES3]) / 2;
    velocity[ROTZ]   = (centered[HES0] +centered[HES2] +centered[HES6] +centered[HES8] -centered[HES1] -centered[HES3] -centered[HES7] -centered[HES9]) / 4;
  #endif
}

/// @brief Inline kinematic processor for scaling, mapping, and gating 6DOF signals
__attribute__((noinline)) static void processAxis(int16_t &vel, int16_t sens_q7, int16_t gate, ParamData& par) {
  if (vel != 0) {
    vel = modifierFunction(divideBySensitivity(vel, sens_q7), par);
    if (abs(vel) < gate) vel = 0;
  }
}

/// @brief Evaluates all 6DOF matrices, applies hardware polarity inversions, executes noise gates, and scales sensitivities.
void calculateKinematic(int16_t* centered, int16_t* velocity, ParamData& par){
  // Gather base unscaled velocities
  _calculateKinematicSensors(centered, velocity, par.values->exclusiveMode);

  // Optimized boolean checks for direct inversion branches saving several Flash bytes over multiplicative methods
  if(par.values->invX){velocity[TRANSX] = -velocity[TRANSX];}
  if(par.values->invY){velocity[TRANSY] = -velocity[TRANSY];}
  if(par.values->invZ){velocity[TRANSZ] = -velocity[TRANSZ];}
  if(par.values->invRX){velocity[ROTX]   = -velocity[ROTX];}
  if(par.values->invRY){velocity[ROTY]   = -velocity[ROTY];}
  if(par.values->invRZ){velocity[ROTZ]   = -velocity[ROTZ];}

  // Apply scales and mechanical noise cross-coupling gates
  processAxis(velocity[TRANSX], par.values->transX_sensitivity_q7, par.values->gate_neg_transZ, par);
  processAxis(velocity[TRANSY], par.values->transY_sensitivity_q7, par.values->gate_neg_transZ, par);

  // Translation Z handles asymmetrical sensitivities (push vs pull resistance)
  if (velocity[TRANSZ] != 0) {
    int16_t z_sens = (velocity[TRANSZ] < 0) ? par.values->neg_transZ_sensitivity_q7 : par.values->pos_transZ_sensitivity_q7;
    processAxis(velocity[TRANSZ], z_sens, par.values->gate_neg_transZ, par);
  }

  // Rotational axis processing
  processAxis(velocity[ROTX], par.values->rotX_sensitivity_q7, par.values->gate_rotX, par);
  processAxis(velocity[ROTY], par.values->rotY_sensitivity_q7, par.values->gate_rotY, par);
  processAxis(velocity[ROTZ], par.values->rotZ_sensitivity_q7, par.values->gate_rotZ, par);

  // Apply master global sensitivity scaling on all 6 output axes with 32-bit overflow prevention
  // Base 100 bypasses logic entirely to save CPU overhead
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

/// @brief Determines dominance and locks the output payload exclusively to either Translation OR Rotation.
void exclusiveMode(int16_t *velocity, int16_t hysteresis){  
  // Enhanced Exclusive Mode state machine:
  // 0 = NEUTRAL (System at rest or resolving dominance)
  // 1 = TRANSLATION locked
  // 2 = ROTATION locked
  static uint8_t mode = 0; 

  uint16_t totalRot   = abs(velocity[ROTX]  ) + abs(velocity[ROTY]  ) + abs(velocity[ROTZ]  );
  uint16_t totalTrans = abs(velocity[TRANSX]) + abs(velocity[TRANSY]) + abs(velocity[TRANSZ]);

  // Relaxation threshold: If total force drops below threshold during hand release/direction change,
  // unlock mode to NEUTRAL (0) so the next immediate gesture registers instantly without fighting hysteresis.
  if (totalRot < EXCL_RELAX_THRESHOLD && totalTrans < EXCL_RELAX_THRESHOLD) {
    mode = 0; 
  }

  // Underflow protection for negative hysteresis values originating from EEPROM corruption
  uint16_t hysteresis_safe = (hysteresis < 0) ? 0 : (uint16_t)hysteresis;

  if (mode == 0) {
    // In NEUTRAL: Evaluate pure dominance without any hysteresis barrier
    if (totalRot > totalTrans && totalRot >= 15) {
      mode = 2; // Latch ROTATION
    } else if (totalTrans > totalRot && totalTrans >= 15) {
      mode = 1; // Latch TRANSLATION
    }
  } else if (mode == 1) { 
    // Currently in TRANSLATION: Must mathematically overcome the hysteresis barrier to force a mid-motion switch
    if (totalRot > (totalTrans + hysteresis_safe)) {
      mode = 2;
    }
  } else if (mode == 2) { 
    // Currently in ROTATION: Must mathematically overcome the hysteresis barrier to force a mid-motion switch
    if (totalTrans > (totalRot + hysteresis_safe)) {
      mode = 1;
    }
  }

  // Mute non-dominant data
  if (mode == 2) { 
    velocity[TRANSX] = 0;
    velocity[TRANSY] = 0;
    velocity[TRANSZ] = 0;
  } else if (mode == 1) { 
    velocity[ROTX] = 0;
    velocity[ROTY] = 0;
    velocity[ROTZ] = 0;
  }
}