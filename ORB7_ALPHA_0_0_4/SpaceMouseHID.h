/*
This class behaves as a composite HID Device with two endpoints for in and out.

It was created by reverse-engineering a standard Space Navigator and heavily relating 
to the HID Library by Nico Hood for reference. https://github.com/NicoHood/HID

This specific implementation has been highly optimized to bundle the 6 Degrees of Freedom 
(Translation + Rotation) into a single unified 12-byte payload to reduce USB endpoint 
saturation and achieve ultra-low latency polling rates.
*/

#ifndef SpaceMouseHID_h
#define SpaceMouseHID_h
#include <Arduino.h>

// Ensure the code is compiling for a supported Architecture (ATmega32U4)
#ifndef ARDUINO_ARCH_AVR
#error "Unsupported Architecture"
#endif

#include "PluggableUSB.h"
#include "HID.h"
#include "parameterMenu.h"

// Macro to define the custom SpaceMouse HID report length
#define SPACEMOUSE_D_HIDREPORT(length) \
    {                                  \
        9, 0x21, 0x11, 0x01, 0, 1, 0x22, lowByte(length), highByte(length)}

// Struct holding the USB interface and endpoint descriptors required by PluggableUSB
typedef struct
{
    InterfaceDescriptor hid;
    HIDDescDescriptor desc;
    EndpointDescriptor in;
    EndpointDescriptor out;
} SpaceMouseHIDDescriptor;

// HID Report Descriptor defining the capabilities of the device to the host OS.
static const uint8_t SpaceMouseReportDescriptor[] PROGMEM = {
    0x05, 0x01,          // Usage Page (Generic Desktop)
    0x09, 0x08,          // Usage (Multi-Axis)
    0xA1, 0x01,          // Collection (Application) <-- TOP LEVEL COLLECTION 1 (SpaceMouse)
    
                         // Report 1: Unified Translation & Rotation (12 Bytes)
    0xA1, 0x00,          //   Collection (Physical)
    0x85, 0x01,          //     Report ID (1)
    0x16, 0xA2, 0xFE,    //     Logical Minimum (-350)
    0x26, 0x5E, 0x01,    //     Logical Maximum (350)
    0x36, 0x88, 0xFA,    //     Physical Minimum (-1400)
    0x46, 0x78, 0x05,    //     Physical Maximum (1400)
    0x55, 0x0C,          //     Unit Exponent (-4)
    0x65, 0x11,          //     Unit (System: SI Linear, Length: Centimeter)
    0x09, 0x30,          //     Usage (X)
    0x09, 0x31,          //     Usage (Y)
    0x09, 0x32,          //     Usage (Z)
    0x09, 0x33,          //     Usage (Rx)
    0x09, 0x34,          //     Usage (Ry)
    0x09, 0x35,          //     Usage (Rz)
    0x75, 0x10,          //     Report Size (16 bits per axis)
    0x95, 0x06,          //     Report Count (6 axes)
#ifdef ADV_HID_REL       
    0x81, 0x06,          //     Input (Data,Var,Rel,No Wrap,Linear,Preferred State,No Null Position)
#else
    0x81, 0x02,          //     Input (variable,absolute)
#endif
    0xC0,                //   End Collection

                         // Report 3: Keys (Up to 32 independent buttons)
    0xa1, 0x00,          //   Collection (Physical)
    0x85, 0x03,          //     Report ID (3)
    0x15, 0x00,          //     Logical Minimum (0)
    0x25, 0x01,          //     Logical Maximum (1)
    0x75, 0x01,          //     Report Size (1 bit per key)
    0x95, 0x20,          //     Report Count (32 keys)
    0x05, 0x09,          //     Usage Page (Button)
    0x19, 0x01,          //     Usage Minimum (Button #1)
    0x29, 0x20,          //     Usage Maximum (Button #32, needs 4 bytes payload)
    0x81, 0x02,          //     Input (Data,Var,Abs)
    0xC0,                //   End Collection

                         // Report 4: LEDs (Optional host-controlled indicators)
    0xA1, 0x02,          //   Collection (Logical)
    0x85, 0x04,          //     Report ID (4)
    0x05, 0x08,          //     Usage Page (LEDs)
    0x09, 0x4B,          //     Usage (Generic Indicator)
    0x15, 0x00,          //     Logical Minimum (0)
    0x25, 0x01,          //     Logical Maximum (1)
    0x95, 0x01,          //     Report Count (1)
    0x75, 0x01,          //     Report Size (1 bit)
    0x91, 0x02,          //     Output (Data,Var,Abs,No Wrap,Linear,Preferred State,No Null Position,Non-volatile)
    0x95, 0x01,          //     Report Count (1 byte padding)
    0x75, 0x07,          //     Report Size (7 bits padding)
    0x91, 0x03,          //     Output (Const,Var,Abs)
    0xC0,                //   End Collection

    0xC0,                // === END OF TOP LEVEL COLLECTION 1 === (Fechamos o SpaceMouse aqui!)

                         // Report 5: WebHID Bi-directional Interface (Vendor Defined)
    0x06, 0x00, 0xFF,    // Usage Page (Vendor Defined 0xFF00)
    0x09, 0x01,          // Usage (0x01)
    0xA1, 0x01,          // Collection (Application) <-- TOP LEVEL COLLECTION 2 (WebHID)
    0x85, 0x05,          //   Report ID (5)
    0x15, 0x00,          //   Logical Minimum (0)
    0x26, 0xFF, 0x00,    //   Logical Maximum (255)
    0x75, 0x08,          //   Report Size (8 bits)
    0x95, 0x40,          //   Report Count (64 bytes payload)
    0x09, 0x02,          //   Usage (0x02)
    0x81, 0x02,          //   Input (Data,Var,Abs)
    0x09, 0x03,          //   Usage (0x03)
    0x91, 0x02,          //   Output (Data,Var,Abs)
    0xC0                 // === END OF TOP LEVEL COLLECTION 2 ===
};

#define USBControllerInterface pluggedInterface
#define USBControllerEndpointIn pluggedEndpoint
#define USBControllerEndpointOut (pluggedEndpoint + 1)
#define USBControllerTX USBControllerEndpointIn
#define USBControllerRX USBControllerEndpointOut
#define HIDUPDATERATE_MS 8

enum SpaceMouseHIDStates
{
    ST_INIT,      
    ST_START,     
    ST_SENDTRANS, 
    ST_SENDKEYS   
};

class SpaceMouseHID_ : public PluggableUSBModule
{
public:
    SpaceMouseHID_();
    int write(const uint8_t *buffer, size_t size);
    int SendReport(uint8_t id, const void *data, int len);
    
    // Core function to read packets coming from Host (WebHID & LEDs)
    void receiveHostData(ParamData& par);
    
    bool getLEDState();
    bool send_command(int16_t rx, int16_t ry, int16_t rz, int16_t x, int16_t y, int16_t z, uint8_t *keys, int debug);

private:
    bool IsNewHidReportDue(unsigned long now);
    bool jiggleValues(uint8_t val[12], bool lastBit);

    SpaceMouseHIDStates nextState;
#if (NUMKEYS > 0)
    void prepareKeyBytes(uint8_t *keys, uint8_t *keyData, int debug);
#endif

    uint8_t countTransZeros = 10; 
    uint8_t countRotZeros = 10;
    unsigned long lastHIDsentRep; 
    bool ledState;

protected:
    uint8_t endpointTypes[2];
    uint8_t protocol;
    uint8_t idle;
    int getInterface(uint8_t *interfaceNumber);
    int getDescriptor(USBSetup &setup);
    bool setup(USBSetup &setup);
};

extern SpaceMouseHID_ SpaceMouseHID;

#endif // SpaceMouseHID_h