// This file handles the WebHID 2-way communication logic.
// It directly bridges the ATmega32U4 EEPROM and sensor streams to modern Web Browsers.

#include <Arduino.h>
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
  uint8_t txBuffer[64];
  bool sendConfigBack = false;

  switch (cmd) {
    case WEBHID_CMD_GET_CONFIG:
      sendConfigBack = true;
      break;

    case WEBHID_CMD_SET_CONFIG:
      if (length >= 2) {
        // PREVENT MINVALS CORRUPTION: Copy only user parameters (up to offsetof minVals = 56 bytes)
        uint8_t maxParamBytes = offsetof(ParamStorage, minVals);
        uint8_t bytesToCopy = (maxParamBytes > (length - 1)) ? (length - 1) : maxParamBytes;
        memcpy(par.values, &payload[1], bytesToCopy);
        
        // Execute boundary checks to safeguard against zero-division in kinematics
        if (par.values->deadzone < 0 || par.values->deadzone > 200) par.values->deadzone = DEADZONE;
        if (par.values->globalSens < 10 || par.values->globalSens > 300) par.values->globalSens = 100;
        if (par.values->transX_sensitivity_q7 <= 0) par.values->transX_sensitivity_q7 = SENS_TX_Q7;
        if (par.values->transY_sensitivity_q7 <= 0) par.values->transY_sensitivity_q7 = SENS_TY_Q7;
        if (par.values->pos_transZ_sensitivity_q7 <= 0) par.values->pos_transZ_sensitivity_q7 = SENS_PTZ_Q7;
        if (par.values->neg_transZ_sensitivity_q7 <= 0) par.values->neg_transZ_sensitivity_q7 = SENS_NTZ_Q7;
        if (par.values->rotX_sensitivity_q7 <= 0) par.values->rotX_sensitivity_q7 = SENS_RX_Q7;
        if (par.values->rotY_sensitivity_q7 <= 0) par.values->rotY_sensitivity_q7 = SENS_RY_Q7;
        if (par.values->rotZ_sensitivity_q7 <= 0) par.values->rotZ_sensitivity_q7 = SENS_RZ_Q7;
        if (par.values->compNoOfPoints <= 0 || par.values->compNoOfPoints > 500) par.values->compNoOfPoints = COMP_NR;

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

    default:
      break; 
  }

  if (sendConfigBack) {
    txBuffer[0] = cmd;
    uint8_t bytesToCopy = (sizeof(ParamStorage) > 63) ? 63 : sizeof(ParamStorage);
    memcpy(&txBuffer[1], par.values, bytesToCopy);
    SpaceMouseHID.SendReport(5, txBuffer, 64);
  }
}

void streamWebHIDRawData(int16_t* rawReads) {
  if (!isStreamingRaw) return;

  unsigned long now = millis();
  if (now - lastStreamTime < 30) return; 
  lastStreamTime = now;

  uint8_t txBuffer[64];
  txBuffer[0] = WEBHID_CMD_STREAM_RAW;
  
  memcpy(&txBuffer[1], rawReads, 16);  // Bytes 1-16: rawReads
  memcpy(&txBuffer[17], centered, 16); // Bytes 17-32: centered
  memcpy(&txBuffer[33], offsets, 16);  // Bytes 33-48: offsets
  memcpy(&txBuffer[49], velocity, 12); // Bytes 49-60: velocity (6DOF)
  
#if NUMKEYS > 0
  txBuffer[61] = keyState[0]; 
#if NUMKEYS > 1
  txBuffer[62] = keyState[1]; 
#endif
#endif

  SpaceMouseHID.SendReport(5, txBuffer, 64);
}