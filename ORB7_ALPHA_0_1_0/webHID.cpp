// This file handles the WebHID 2-way communication logic.
// It directly bridges the ATmega32U4 EEPROM and sensor streams to modern Web Browsers.

#include <Arduino.h>
#include <avr/wdt.h> // Watchdog timer support for hardware software reset
#include "config.h"
#include "webHID.h"
#include "SpaceMouseHID.h"
#include "calibration.h"

// Import global variables directly from main loop (zero Flash cost)
extern int16_t centered[8];
extern int16_t offsets[8];
extern int16_t velocity[6];
extern int16_t centerPoints[8];

#if NUMKEYS > 0
extern uint8_t keyState[NUMKEYS];
#endif

// State tracking variable determining if the browser requested live telemetry
static bool isStreamingRaw = false;

// Frame limiter to prevent the telemetry stream from choking the main kinematics loop
static unsigned long lastStreamTime = 0;

void processWebHIDPacket(uint8_t* payload, uint8_t length, ParamData& par) {
  if (length < 1) return; 

  uint8_t cmd = payload[0];
  
  // Zero-initialize buffer to prevent stack memory leak when sending short packets
  uint8_t txBuffer[64] = {0}; 
  bool sendConfigBack = false;

  switch (cmd) {
    case WEBHID_CMD_GET_CONFIG:
      sendConfigBack = true;
      break;

    case WEBHID_CMD_SET_CONFIG:
      if (length >= 2) {
        // PREVENT MINVALS CORRUPTION: Copy only user parameters (up to offsetof minVals = 57 bytes)
        uint8_t maxParamBytes = offsetof(ParamStorage, minVals);
        uint8_t bytesToCopy = (maxParamBytes > (length - 1)) ? (length - 1) : maxParamBytes;
        memcpy(par.values, &payload[1], bytesToCopy);
        
        // Execute boundary checks to safeguard against zero-division in kinematics and corrupt curve parameters
        if (par.values->deadzone < 0 || par.values->deadzone > 200) par.values->deadzone = DEADZONE;
        if (par.values->globalSens < 10 || par.values->globalSens > 300) par.values->globalSens = 100;
        if (par.values->modFunc != 0 && par.values->modFunc != 1 && par.values->modFunc != 3) par.values->modFunc = MODFUNC;
        if (par.values->transX_sensitivity_q7 <= 0) par.values->transX_sensitivity_q7 = SENS_TX_Q7;
        if (par.values->transY_sensitivity_q7 <= 0) par.values->transY_sensitivity_q7 = SENS_TY_Q7;
        if (par.values->pos_transZ_sensitivity_q7 <= 0) par.values->pos_transZ_sensitivity_q7 = SENS_PTZ_Q7;
        if (par.values->neg_transZ_sensitivity_q7 <= 0) par.values->neg_transZ_sensitivity_q7 = SENS_NTZ_Q7;
        if (par.values->rotX_sensitivity_q7 <= 0) par.values->rotX_sensitivity_q7 = SENS_RX_Q7;
        if (par.values->rotY_sensitivity_q7 <= 0) par.values->rotY_sensitivity_q7 = SENS_RY_Q7;
        if (par.values->rotZ_sensitivity_q7 <= 0) par.values->rotZ_sensitivity_q7 = SENS_RZ_Q7;
        if (par.values->compNoOfPoints <= 0 || par.values->compNoOfPoints > 500) par.values->compNoOfPoints = COMP_NR;
        if (par.values->gate_trans < 0 || par.values->gate_trans > 100) par.values->gate_trans = GATE_TRANS;

        putParametersToEEPROM(par);   
        sendConfigBack = true;
      }
      break;

    case WEBHID_CMD_STREAM_RAW:
      if (length >= 2) {
        isStreamingRaw = (payload[1] == 0x01);
      }
      break;

    case WEBHID_CMD_REZERO:
      busyZeroing(centerPoints, 1000, false);
      txBuffer[0] = WEBHID_CMD_REZERO;
      SpaceMouseHID.SendReport(5, txBuffer, 64);
      break;

    case WEBHID_CMD_FACTORY_RESET:
      *par.values = ParamStorage();
      putParametersToEEPROM(par);
      sendConfigBack = true;
      break;

    case WEBHID_CMD_SET_CALIBRATION:
      if (length >= 33) {
        // Write calibration boundaries using a dedicated secondary payload
        memcpy(par.values->minVals, &payload[1], 16);
        memcpy(par.values->maxVals, &payload[17], 16);
        
        // Execute boundary sanitization for dynamic limits
        for (uint8_t i = 0; i < 8; i++) {
          if (par.values->minVals[i] >= 0 || par.values->minVals[i] < -1023) par.values->minVals[i] = -400;
          if (par.values->maxVals[i] <= 0 || par.values->maxVals[i] > 1023) par.values->maxVals[i] = 175;
        }

        putParametersToEEPROM(par);
      }
      break;

    case WEBHID_CMD_GET_CALIBRATION:
      // Retrieve boundaries directly from EEPROM and send strictly back to Web Studio
      txBuffer[0] = WEBHID_CMD_GET_CALIBRATION;
      memcpy(&txBuffer[1], par.values->minVals, 16);
      memcpy(&txBuffer[17], par.values->maxVals, 16);
      SpaceMouseHID.SendReport(5, txBuffer, 64);
      break;

    case WEBHID_CMD_RESTART:
      // Acknowledge restart request to browser before triggering Watchdog reset
      txBuffer[0] = WEBHID_CMD_RESTART;
      SpaceMouseHID.SendReport(5, txBuffer, 64);
      delay(50);
      
      // Enable Watchdog Timer at 60ms timeout and enter infinite loop to force clean MCU hardware reset
      wdt_enable(WDTO_60MS);
      while(1);
      break;

    default:
      break; 
  }

  if (sendConfigBack) {
    txBuffer[0] = cmd;
    uint8_t bytesToCopy = (sizeof(ParamStorage) > 59) ? 59 : sizeof(ParamStorage);
    memcpy(&txBuffer[1], par.values, bytesToCopy);

    // Inject Dynamic Firmware Release Version into trailing free bytes of buffer (Offsets 60..62)
    txBuffer[60] = FW_VERSION_MAJOR;
    txBuffer[61] = FW_VERSION_MINOR;
    txBuffer[62] = FW_VERSION_PATCH;

    SpaceMouseHID.SendReport(5, txBuffer, 64);
  }
}

void streamWebHIDRawData(int16_t* rawReads) {
  if (!isStreamingRaw) return;

  unsigned long now = millis();
  // OPTIMIZED: Reduced stream delay from 30ms (33Hz) to 10ms (100Hz) for high-speed dynamic calibration peak capture
  if (now - lastStreamTime < 10) return; 
  lastStreamTime = now;

  // Zero-initialize telemetry buffer to prevent leaking memory in unused payload bytes
  uint8_t txBuffer[64] = {0};
  txBuffer[0] = WEBHID_CMD_STREAM_RAW;

  // Calculate true physical unmapped centered deltas for web telemetry & calibration wizard
  int16_t physicalCentered[8];
  for (uint8_t i = 0; i < 8; i++) {
    physicalCentered[i] = rawReads[i] - centerPoints[i] + offsets[i];
  }
  
  memcpy(&txBuffer[1], rawReads, 16);          // Bytes 1-16: rawReads
  memcpy(&txBuffer[17], physicalCentered, 16); // Bytes 17-32: physical centered deltas (Raw - Center + Offset)
  memcpy(&txBuffer[33], offsets, 16);          // Bytes 33-48: offsets
  memcpy(&txBuffer[49], velocity, 12);         // Bytes 49-60: velocity (6DOF)
  
#if NUMKEYS > 0
  // Byte 61: Pack digital states for all 4 physical hardware buttons into a single 8-bit bitmask
  // Bit 0 = Key R (Front Right / keys[0])
  // Bit 1 = Key L (Front Left  / keys[1])
  // Bit 2 = Key 2 (Back Left   / keys[2])
  // Bit 3 = Key 1 (Back Right  / keys[3])
  uint8_t keysBitmask = 0;
  for (uint8_t i = 0; i < NUMKEYS && i < 8; i++) {
    if (keyState[i]) {
      keysBitmask |= (1 << i);
    }
  }
  txBuffer[61] = keysBitmask;
#endif

  SpaceMouseHID.SendReport(5, txBuffer, 64);
}