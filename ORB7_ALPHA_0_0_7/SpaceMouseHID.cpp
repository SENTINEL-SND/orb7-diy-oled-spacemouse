#include <Arduino.h>
#include "config.h"
#include "SpaceMouseHID.h"
#include "parameterMenu.h"
#include "webHID.h" // Injects the WebHID protocol processor

extern ParamData par; 

SpaceMouseHID_::SpaceMouseHID_() : PluggableUSBModule(2, 1, endpointTypes) {
  endpointTypes[0] = EP_TYPE_INTERRUPT_IN;
  endpointTypes[1] = EP_TYPE_INTERRUPT_OUT;
  PluggableUSB().plug(this);
  nextState = ST_INIT; 
  ledState = false;
}

int SpaceMouseHID_::getInterface(uint8_t *interfaceNumber) {
  interfaceNumber[0] += 1;
  SpaceMouseHIDDescriptor interfaceDescriptor = {
      D_INTERFACE(USBControllerInterface, 2, USB_DEVICE_CLASS_HUMAN_INTERFACE, 0, 0),
      SPACEMOUSE_D_HIDREPORT(sizeof(SpaceMouseReportDescriptor)),
      D_ENDPOINT(USB_ENDPOINT_IN(USBControllerEndpointIn), USB_ENDPOINT_TYPE_INTERRUPT, USB_EP_SIZE,
                 0),
      D_ENDPOINT(USB_ENDPOINT_OUT(USBControllerEndpointOut), USB_ENDPOINT_TYPE_INTERRUPT,
                 USB_EP_SIZE, 0),
  };
  return USB_SendControl(0, &interfaceDescriptor, sizeof(interfaceDescriptor));
}

int SpaceMouseHID_::getDescriptor(USBSetup &setup) {
  if (setup.bmRequestType != REQUEST_DEVICETOHOST_STANDARD_INTERFACE) return 0;
  if (setup.wValueH != HID_REPORT_DESCRIPTOR_TYPE) return 0;
  if (setup.wIndex != pluggedInterface) return 0;

  protocol = HID_REPORT_PROTOCOL;
  return USB_SendControl(TRANSFER_PGM, SpaceMouseReportDescriptor, sizeof(SpaceMouseReportDescriptor));
}

bool SpaceMouseHID_::setup(USBSetup &setup) {
  if (pluggedInterface != setup.wIndex) return false;

  uint8_t request = setup.bRequest;
  uint8_t requestType = setup.bmRequestType;

  if (requestType == REQUEST_DEVICETOHOST_CLASS_INTERFACE) {
    if (request == HID_GET_REPORT || request == HID_GET_PROTOCOL) return true;
  }
  if (requestType == REQUEST_HOSTTODEVICE_CLASS_INTERFACE) {
    if (request == HID_SET_PROTOCOL) {
      protocol = setup.wValueL;
      return true;
    }
    if (request == HID_SET_IDLE) {
      idle = setup.wValueL;
      return true;
    }
    if (request == HID_SET_REPORT) return true;
  }
  return false;
}

int SpaceMouseHID_::write(const uint8_t *buffer, size_t size) {
  return USB_Send(USBControllerTX, buffer, size);
}

int SpaceMouseHID_::SendReport(uint8_t id, const void *data, int len) {
  auto ret = USB_Send(USBControllerTX, &id, 1);
  if (ret < 0) return ret;
  auto ret2 = USB_Send(USBControllerTX | TRANSFER_RELEASE, data, len);
  if (ret2 < 0) return ret2;
  return ret + ret2;
}

/// @brief Reads incoming Host PC packets and routes them to LEDs or WebHID API
void SpaceMouseHID_::receiveHostData(ParamData& par) {
  uint8_t numBytes = USB_Available(USBControllerRX);
  if (numBytes > 0) {
    uint8_t buffer[65]; // Local stack buffer
    uint8_t readLen = (numBytes > 65) ? 65 : numBytes;
    
    // Safely store actual received bytes from USB_Recv to prevent processing uninitialized memory
    int actualRead = USB_Recv(USBControllerRX, buffer, readLen);

    if (actualRead > 0) {
      if (buffer[0] == 4) { 
        if (actualRead >= 2) { 
          ledState = (buffer[1] == 1);
        }
      } else if (buffer[0] == 5) { 
        processWebHIDPacket(&buffer[1], (uint8_t)(actualRead - 1), par);
      }
    }
  }
}

bool SpaceMouseHID_::getLEDState() {
  return ledState;
}

bool SpaceMouseHID_::send_command(int16_t rx, int16_t ry, int16_t rz, int16_t x, int16_t y,
                                  int16_t z, uint8_t *keys, int debug) {
  unsigned long now = millis();
  bool hasSentNewData = false;

#if (NUMKEYS > 0)
  static uint8_t keyData[4];
  static uint8_t prevKeyData[4];
  prepareKeyBytes(keys, keyData, debug);
#endif

  switch (nextState) {
  case ST_INIT:
    lastHIDsentRep = now;
    nextState = ST_START;
    break;

  case ST_START:
    if (countTransZeros < 3 || countRotZeros < 3 || (x != 0 || y != 0 || z != 0 || rx != 0 || ry != 0 || rz != 0)) {
      nextState = ST_SENDTRANS;
    } else {
#if (NUMKEYS > 0)
      if (memcmp(keyData, prevKeyData, 4) != 0) nextState = ST_SENDKEYS;
#endif
      if (nextState == ST_START && IsNewHidReportDue(now)) lastHIDsentRep = now - HIDUPDATERATE_MS;
    }
    break;

  case ST_SENDTRANS:
    if (IsNewHidReportDue(now)) {
      uint8_t trans[12] = {(byte)(x & 0xFF),  (byte)(x >> 8),  (byte)(y & 0xFF),  (byte)(y >> 8),
                           (byte)(z & 0xFF),  (byte)(z >> 8),  (byte)(rx & 0xFF), (byte)(rx >> 8),
                           (byte)(ry & 0xFF), (byte)(ry >> 8), (byte)(rz & 0xFF), (byte)(rz >> 8)};

      SendReport(1, trans, 12);
      lastHIDsentRep += HIDUPDATERATE_MS;
      hasSentNewData = true;

      // Saturate zero counters at 255 to prevent uint8_t wraparound back to 0 during idle rest
      if (x == 0 && y == 0 && z == 0) {
        if (countTransZeros < 255) countTransZeros++;
      } else {
        countTransZeros = 0;
      }
      
      if (rx == 0 && ry == 0 && rz == 0) {
        if (countRotZeros < 255) countRotZeros++;
      } else {
        countRotZeros = 0;
      }
      
#if (NUMKEYS > 0)
      if (memcmp(keyData, prevKeyData, 4) != 0) nextState = ST_SENDKEYS;
      else nextState = ST_START;
#else
      nextState = ST_START;
#endif
    }
    break;

#if (NUMKEYS > 0)
  case ST_SENDKEYS:
    if (IsNewHidReportDue(now)) {
      SendReport(3, keyData, 4); 
      lastHIDsentRep += HIDUPDATERATE_MS;
      memcpy(prevKeyData, keyData, 4); 
      hasSentNewData = true;
      nextState = ST_START;
    }
    break;
#endif

  default:
    nextState = ST_START;
    break;
  }
  return hasSentNewData;
}

bool SpaceMouseHID_::IsNewHidReportDue(unsigned long now) {
  if (now - lastHIDsentRep > 2 * HIDUPDATERATE_MS) lastHIDsentRep = now - HIDUPDATERATE_MS;
  return (now - lastHIDsentRep >= HIDUPDATERATE_MS);
}

#if (NUMKEYS > 0)
void SpaceMouseHID_::prepareKeyBytes(uint8_t *keys, uint8_t *keyData, int debug) {
  for (int i = 0; i < 4; i++) keyData[i] = 0;
  for (uint8_t i = 0; i < NUMKEYS; i++) {
#if (NUMKILLKEYS > 0)
    // Mask designated kill keys to prevent triggering USB HID shortcuts when toggling rotational/translational mute
    if (i == KILLROT || i == KILLTRANS) continue;
#endif
    if (keys[i]) {
      uint8_t bitNum = 0;
      if (i == 0) bitNum = par.values->keyR_shortcut;       // keys[0] = Front Right (Key R)
      else if (i == 1) bitNum = par.values->keyL_shortcut;  // keys[1] = Front Left  (Key L)
      else if (i == 2) bitNum = par.values->key2_shortcut;  // keys[2] = Back Left   (Key 2)
      else if (i == 3) bitNum = par.values->key1_shortcut;  // keys[3] = Back Right  (Key 1)

      if (bitNum < 32) keyData[bitNum / 8] |= (1 << (bitNum % 8)); 
    }
  }
}
#endif

SpaceMouseHID_ SpaceMouseHID;