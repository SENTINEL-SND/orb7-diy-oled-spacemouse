#ifndef CALIBRATION_CHECKS_h
#define CALIBRATION_CHECKS_h

/* Compile-time parameter validation checks
 * This file contains all static assertions and preprocessor checks
 * to validate configuration parameters in config.h during compilation.
 */

// Check that NUMKILLKEYS does not exceed total number of keys
#if (NUMKILLKEYS > NUMKEYS)
#error "Number of Kill Keys can not be larger than total number of keys"
#endif

// FIXED: Symmetrical boundary check updated to >= to prevent out-of-bounds heap/stack memory access during runtime
#if (NUMKILLKEYS > 0 && ((KILLROT >= NUMKEYS) || (KILLTRANS >= NUMKEYS)))
#error "Index of killkeys must be smaller than the total number of keys"
#endif

// Check KEYLIST size matches NUMKEYS
#if NUMKEYS > 0
constexpr int _keyListCompile[] = KEYLIST;
static_assert(sizeof(_keyListCompile) / sizeof(_keyListCompile[0]) == NUMKEYS,
              "KEYLIST element count does not match NUMKEYS definition in config.h");

constexpr bool _isValueInArray(const int *arr, int size, int idx, int value) {
  return (idx >= size)         ? false
         : (arr[idx] == value) ? true
                               : _isValueInArray(arr, size, idx + 1, value);
}
#endif

#endif // CALIBRATION_CHECKS_h