// Header for WebHID bi-directional communication protocol
// Allows direct browser integration without installing drivers or background services.

#ifndef WEBHID_H
#define WEBHID_H

#include <Arduino.h>
#include "parameterMenu.h"

// Standard Command IDs for the WebHID Interface
#define WEBHID_CMD_GET_CONFIG    0x01
#define WEBHID_CMD_SET_CONFIG    0x02
#define WEBHID_CMD_STREAM_RAW    0x03
#define WEBHID_CMD_REZERO        0x04
#define WEBHID_CMD_FACTORY_RESET 0x05

/// @brief Processes incoming WebHID payloads sent by the browser.
/// @param payload Pointer to the raw USB packet data.
/// @param length Number of received bytes.
/// @param par Global parameters struct.
void processWebHIDPacket(uint8_t* payload, uint8_t length, ParamData& par);

/// @brief Streams the 8 raw Hall Effect sensor values and system kinematics to the browser.
/// @param rawReads Array containing the raw ADC inputs.
void streamWebHIDRawData(int16_t* rawReads);

#endif // WEBHID_H