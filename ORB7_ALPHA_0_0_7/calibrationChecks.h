#ifndef CALIBRATION_CHECKS_h
#define CALIBRATION_CHECKS_h

/* Compile-time parameter validation checks
 * This file contains all static assertions and preprocessor checks
 * to validate configuration parameters in config.h during compilation.
 * Prevents flashing unstable configurations to the MCU.
 */

#include "config.h"

// Check that NUMKILLKEYS does not exceed total number of keys
#if (NUMKILLKEYS > NUMKEYS)
#error "Number of Kill Keys can not be larger than total number of keys"
#endif

// Check that kill key indices are within valid range to prevent out-of-bounds array access
#if (NUMKILLKEYS > 0 && ((KILLROT >= NUMKEYS) || (KILLTRANS >= NUMKEYS)))
#error "Index of killkeys must be smaller than the total number of keys"
#endif

// Check KEYLIST size matches NUMKEYS declaration
#if NUMKEYS > 0
constexpr int _keyListCompile[] = KEYLIST;
static_assert(sizeof(_keyListCompile) / sizeof(_keyListCompile[0]) == NUMKEYS,
              "KEYLIST element count does not match NUMKEYS definition in config.h");

// Recursive constexpr function to evaluate array membership at compile time
constexpr bool _isValueInArray(const int *arr, int size, int idx, int value) {
  return (idx >= size)         ? false
         : (arr[idx] == value) ? true
                               : _isValueInArray(arr, size, idx + 1, value);
}
#endif

// Static assertions for user config sanity & zero-division prevention in ADC filtering
static_assert(ADC_OVERSAMPLES >= 1, "ADC_OVERSAMPLES in config.h must be at least 1");
static_assert(EXCL_RELAX_THRESHOLD > 0, "EXCL_RELAX_THRESHOLD in config.h must be greater than 0");

#endif // CALIBRATION_CHECKS_h
