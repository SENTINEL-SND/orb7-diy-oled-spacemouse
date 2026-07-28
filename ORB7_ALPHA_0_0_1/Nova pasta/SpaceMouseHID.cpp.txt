/*
This class behaves as HID Device with two endpoints for in and out

It was created by reverse-engineering a Space Navigator and relating to the HID Library by Nico Hood
for reference. https://github.com/NicoHood/HID

This code is based on
https://forum.arduino.cc/t/solved-unable-to-receive-hid-reports-from-computer-using-pluggableusb/596793
*/

#include <Arduino.h>
#include "config.h"
#include "SpaceMouseHID.h"
#include "parameterMenu.h"

extern ParamData par; // Global access to dynamically assigned button shortcuts

SpaceMouseHID_::SpaceMouseHID_() : PluggableUSBModule(2, 1, endpointTypes) {
  endpointTypes[0] = EP_TYPE_INTERRUPT_IN;
  endpointTypes[1] = EP_TYPE_INTERRUPT_OUT;
  PluggableUSB().plug(this);
  nextState = ST_INIT; // init state machine with init state
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
  if (setup.bmRequestType != REQUEST_DEVICETOHOST_STANDARD_INTERFACE) {
    return 0;
  }
  if (setup.wValueH != HID_REPORT_DESCRIPTOR_TYPE) {
    return 0;
  }
  if (setup.wIndex != pluggedInterface) {
    return 0;
  }

  protocol = HID_REPORT_PROTOCOL;

  return USB_SendControl(TRANSFER_PGM, SpaceMouseReportDescriptor,
                         sizeof(SpaceMouseReportDescriptor));
}

bool SpaceMouseHID_::setup(USBSetup &setup) {
  if (pluggedInterface != setup.wIndex) {
    return false;
  }

  uint8_t request = setup.bRequest;
  uint8_t requestType = setup.bmRequestType;

  if (requestType == REQUEST_DEVICETOHOST_CLASS_INTERFACE) {
    if (request == HID_GET_REPORT) {
      return true;
    }
    if (request == HID_GET_PROTOCOL) {
      return true;
    }
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
    if (request == HID_SET_REPORT) {
      return true;
    }
  }

  return false;
}

int SpaceMouseHID_::write(const uint8_t *buffer, size_t size) {
  return USB_Send(USBControllerTX, buffer, size);
}

int SpaceMouseHID_::SendReport(uint8_t id, const void *data, int len) {
  auto ret = USB_Send(USBControllerTX, &id, 1);
  if (ret < 0)
    return ret;

  auto ret2 = USB_Send(USBControllerTX | TRANSFER_RELEASE, data, len);
  if (ret2 < 0)
    return ret2;

  return ret + ret2;
}

int SpaceMouseHID_::readSingleByte() {
  if (USB_Available(USBControllerRX)) {
    return USB_Recv(USBControllerRX);
  } else {
    return 0;
  }
}

#if ENABLE_SERIAL_DEBUG
void SpaceMouseHID_::printAllReports() {
  uint8_t numBytes = USB_Available(USBControllerRX);

  if (numBytes >= 2) {
    uint8_t data[2] = {0};
    USB_Recv(USBControllerRX, data, numBytes);
    for (int i = 0; i < numBytes; i++) {
      Serial.print(data[i], HEX);
      Serial.print(", ");
    }
    Serial.println(" ");
  }
}
#endif

bool SpaceMouseHID_::updateLEDState() {
  uint8_t numBytes = USB_Available(USBControllerRX);

  if (numBytes >= 2) {
    uint8_t data[2] = {0};
    USB_Recv(USBControllerRX, data, 2);
    if (data[0] == 4) {   // LED report id: 4
      if (data[1] == 1) { // if 1, led on!
        ledState = true;
      } else {
        ledState = false;
      }
    }
  }
  return ledState;
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

#ifdef ADV_HID_JIGGLE
  static bool toggleValue;
#endif

  switch (nextState) {
  case ST_INIT:
    lastHIDsentRep = now;
    nextState = ST_START;
#ifdef ADV_HID_JIGGLE
    toggleValue = false;
#endif
    break;

  case ST_START:
    if (countTransZeros < 3 || countRotZeros < 3 ||
        (x != 0 || y != 0 || z != 0 || rx != 0 || ry != 0 || rz != 0)) {
      nextState = ST_SENDTRANS;
    } else {
#if (NUMKEYS > 0)
      if (memcmp(keyData, prevKeyData, 4) != 0) {
        nextState = ST_SENDKEYS;
      }
#endif
      if (nextState == ST_START && IsNewHidReportDue(now)) {
        lastHIDsentRep = now - HIDUPDATERATE_MS;
      }
    }
    break;

  case ST_SENDTRANS:
    if (IsNewHidReportDue(now)) {
      uint8_t trans[12] = {(byte)(x & 0xFF),  (byte)(x >> 8),  (byte)(y & 0xFF),  (byte)(y >> 8),
                           (byte)(z & 0xFF),  (byte)(z >> 8),  (byte)(rx & 0xFF), (byte)(rx >> 8),
                           (byte)(ry & 0xFF), (byte)(ry >> 8), (byte)(rz & 0xFF), (byte)(rz >> 8)};

#ifdef ADV_HID_JIGGLE
      jiggleValues(trans, toggleValue);
#endif
      SendReport(1, trans, 12);
#ifdef ADV_HID_JIGGLE
      toggleValue = !toggleValue;
#endif
      lastHIDsentRep += HIDUPDATERATE_MS;
      hasSentNewData = true;

      if (x == 0 && y == 0 && z == 0) {
        countTransZeros++;
      } else {
        countTransZeros = 0;
      }
      if (rx == 0 && ry == 0 && rz == 0) {
        countRotZeros++;
      } else {
        countRotZeros = 0;
      }
#if (NUMKEYS > 0)
      if (memcmp(keyData, prevKeyData, 4) != 0) {
        nextState = ST_SENDKEYS;
      } else {
        nextState = ST_START;
      }
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

// FIXED: Clock resynchronization added to prevent cascading catch-up storms on loop lags [10]
bool SpaceMouseHID_::IsNewHidReportDue(unsigned long now) {
  // If loop lag exceeds double the rate, force clock resync to prevent USB endpoint saturation [10]
  if (now - lastHIDsentRep > 2 * HIDUPDATERATE_MS) {
    lastHIDsentRep = now - HIDUPDATERATE_MS;
  }
  return (now - lastHIDsentRep >= HIDUPDATERATE_MS);
}

bool SpaceMouseHID_::jiggleValues(uint8_t val[12], bool lastBit) {
  for (uint8_t i = 0; i < 12; i = i + 2) {
    if ((val[i] != 0 || val[i + 1] != 0) && lastBit) {
      val[i] |= 1;
    } else {
      val[i] &= 0xFE;
    }
  }
  return true;
}

#if (NUMKEYS > 0)
// Optimized to read button settings dynamically from ParamStorage
void SpaceMouseHID_::prepareKeyBytes(uint8_t *keys, uint8_t *keyData, int debug) {
  for (int i = 0; i < 4; i++) {
    keyData[i] = 0;
  }

  // keys[0] represents the Right (R) hardware button
  if (keys[0]) {
    uint8_t bitNum = par.values->keyR_shortcut;
    keyData[bitNum / 8] |= (1 << (bitNum % 8));
  }

  // keys[1] represents the Left (L) hardware button
  if (NUMKEYS > 1 && keys[1]) {
    uint8_t bitNum = par.values->keyL_shortcut;
    keyData[bitNum / 8] |= (1 << (bitNum % 8));
  }
}
#endif

SpaceMouseHID_ SpaceMouseHID;