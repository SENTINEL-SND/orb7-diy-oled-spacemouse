#include "config.h"
#include <Arduino.h>

#if NUMKEYS > 0

// Constantly mapped via Flash PROGMEM equivalents to conserve dynamic RAM
static const uint8_t keyList[NUMKEYS] = KEYLIST;

// Function to setup up all keys in keyList
// Configures hardware to use internal resistors, preventing the need for external pull-up components
void setupKeys() {
  for (uint8_t i = 0; i < NUMKEYS; i++) {
    pinMode(keyList[i], INPUT_PULLUP);
  }
}

// Function to read and store the digital states for each of the keys using 8-bit types
// Reduces array sizing constraints over classic int usage.
void readAllFromKeys(uint8_t *keyVals) {
  for (uint8_t i = 0; i < NUMKEYS; i++) {
    keyVals[i] = (uint8_t)digitalRead(keyList[i]);
  }
}

// Evaluate and debounce keys using an Instant Press and Delayed Release approach.
// Ensures immediate tactile response when clicked, but forgives mechanical switch bouncing upon release.
void evalKeys(uint8_t *keyVals, uint8_t *keyOut, uint8_t *keyState) {
  // Static timestamp array utilizing uint16_t to reduce SRAM memory footprint by 50% vs standard unsigned long.
  // Given that debounce times are tiny (e.g. 50ms), a 16-bit wrap-around (~65.5s) holds more than enough resolution.
  static uint16_t timestamp[NUMKEYS]; 
  uint16_t now = (uint16_t)millis();

  // Button Evaluation Loop
  for (uint8_t i = 0; i < NUMKEYS; i++) {
    keyOut[i] = 0; // Default assignment to eliminate redundant conditional branching in assembly
    
    // The keys are configured with pull_up, see setupKeys() and are pulled to ground when pressed.
    // Therefore, false (0) implies the key is physically pressed.
    if (!keyVals[i]) { 
      // Constantly reset the release timer while the button is physically held down
      timestamp[i] = now; 
      
      if (keyState[i] == 0) { 
        keyOut[i] = 1;   // Instant registration of the initial press
        keyState[i] = 1; // Mark logically as pressed
#ifdef DEBUG_KEYS
        Serial.println("");
        Serial.print("Key Pressed: "); 
        Serial.println(i);
#endif
      }
    } else {           // Physical button is NOT pressed (high signal from pull-up)
      if (keyState[i] == 1) { 
        // Only register the release once the physical signal has been stably high for DEBOUNCE_KEYS_MS
        if (now - timestamp[i] > (uint16_t)DEBOUNCE_KEYS_MS) { 
          keyState[i] = 0; // Mark logically as released
#ifdef DEBUG_KEYS
          Serial.println("");
          Serial.print("Key Released: "); 
          Serial.println(i);
#endif
        }
      }
    }
  }
}
#endif