// Header for spaceKeys.cpp
// Handles physical hardware pin configuration and tactile button debounce routines for the SpaceMouse

#ifndef SPACE_KEYS_H
#define SPACE_KEYS_H

#include <Arduino.h>

/// @brief Configures MCU internal pull-up resistors for all defined button pins.
void setupKeys();

/// @brief Reads raw digital states directly from the defined GPIO hardware pins.
/// @param keyVals Pointer to array where un-debounced physical states will be stored.
void readAllFromKeys(uint8_t* keyVals);

/// @brief Processes raw inputs applying an Instant Press / Delayed Release algorithm.
/// @param keyVals Array containing recent raw hardware reads.
/// @param keyOut Event array indicating an instantaneous button trigger (active exactly one loop cycle).
/// @param keyState Persisted logic state tracking if a key is currently held down.
void evalKeys(uint8_t* keyVals, uint8_t* keyOut, uint8_t* keyState);

#endif // SPACE_KEYS_H