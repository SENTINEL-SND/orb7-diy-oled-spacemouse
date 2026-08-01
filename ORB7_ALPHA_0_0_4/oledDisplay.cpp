// This file handles the entire graphical user interface and parameter configuration directly on the device.
// It utilizes a "RAM-less" drawing technique via SSD1306AsciiWire to write pixels directly to the OLED's 
// internal memory, bypassing the need for a 1KB SRAM frame buffer on the ATmega32U4.

#include "config.h"

#if ENABLE_OLED

#include <Arduino.h>
#include <Wire.h>
#include "SSD1306Ascii.h"
#include "SSD1306AsciiWire.h"
#include "oledDisplay.h"
#include "calibration.h"

#define I2C_ADDRESS 0x3C

// Preventive safety definitions for standard SSD1306 Display power commands
#ifndef SSD1306_DISPLAYON
#define SSD1306_DISPLAYON 0xAF
#endif
#ifndef SSD1306_DISPLAYOFF
#define SSD1306_DISPLAYOFF 0xAE
#endif

// Initializes the SSD1306 display object in extremely lightweight direct-write mode
SSD1306AsciiWire oled;

// Tracks if the screen requires a complete structural wipe and redraw (e.g., state changes)
static bool forceFullViewRedraw = true;

// UI State machine tracker:
// 0 = Home screens (Axis visualizer)
// 1 = Main Menu
// 3 = Sensitivity & Deadzone Submenu
// 4 = Re-Zero Submenu
// 5 = OLED Setup Submenu (Sleep Timer / Power)
// 7 = Buttons Assignment Submenu
// 8 = Debug Submenu list
// 9 = Real-time Sensor Alignment Screen
// 10 = Interactive Limits Calibration Screen (Cal. Limits)
// 11 = Exclusive Mode Configurations Screen (Exclusive)
// 12 = Live Drift Offsets Monitoring Screen (Drift Offsets)
// 13 = Factory Reset Confirmation Screen (Factory Reset)
// 14 = Axis Direction & Inversion Submenu (Direction)
static uint8_t menuState = 0;
static uint8_t cursorIndex = 0;

// Unified submenu position selector tracking the currently highlighted line
static uint8_t submenuSelect = 0;
static bool oledPowerState = true;

// Edit sub-state tracker (false = navigating the list, true = modifying the highlighted value)
static bool isEditing = false;

// State machine tracking for the interactive limits calibration (State 10)
static uint8_t calState = 0;
static unsigned long calStartTime = 0;
static int16_t calMin[8];
static int16_t calMax[8];

// Static timer tracking continuous inactivity for the auto-sleep feature
static unsigned long lastActivityTime = 0;

// Pointers to global variables declared in the main .ino file, required for UI data display
extern int16_t centerPoints[8];
extern int16_t rawReads[8];
extern int16_t offsets[8];

// --- COMPACT MENU TABLES (PROGMEM) ---
const char menu_str0[] PROGMEM = " 1. Sensitivity";
const char menu_str1[] PROGMEM = " 2. Direction";
const char menu_str2[] PROGMEM = " 3. Display";
const char menu_str3[] PROGMEM = " 4. Buttons";
const char menu_str4[] PROGMEM = " 5. Debug";

const char* const menu_strings[5] PROGMEM = {
  menu_str0, menu_str1, menu_str2, menu_str3, menu_str4
};

const char debug_str0[] PROGMEM = " 1. Align Sensors";
const char debug_str1[] PROGMEM = " 2. Cal. Limits";
const char debug_str2[] PROGMEM = " 3. Re-Zero";
const char debug_str3[] PROGMEM = " 4. Exclusive";
const char debug_str4[] PROGMEM = " 5. Drift Offsets";
const char debug_str5[] PROGMEM = " 6. Factory Reset";

const char* const debug_strings[6] PROGMEM = {
  debug_str0, debug_str1, debug_str2, debug_str3, debug_str4, debug_str5
};

static const uint8_t shortcut_ids[15] = { 0, 1, 2, 4, 5, 8, 12, 13, 14, 15, 22, 23, 24, 25, 26 };
static const char shortcut_names[15][6] PROGMEM = {
  "MENU", "FIT", "TOP", "RIGHT", "FRONT", "ROLL", "1", "2", "3", "4", "ESC", "ALT", "SHIFT", "CTRL", "ROT"
};

static const char dir_labels[8][7] PROGMEM = {
  "INV TX", "INV TY", "INV TZ", "INV RX", "INV RY", "INV RZ", "SWP XY", "SWP YZ"
};

static const uint8_t destStates[5] PROGMEM = { 3, 14, 5, 7, 8 };

// --- LOW-LEVEL GRAPHICAL HELPER FUNCTION DECLARATIONS ---

// Flash-optimized wrapper replacing redundant setCursor + print combos
static void printAt(uint8_t x, uint8_t y, const __FlashStringHelper* text) {
  oled.setCursor(x, y);
  oled.print(text);
}

// Flash-optimized compact layout for submenus
static void printMenuLabel(uint8_t row, uint8_t index, const __FlashStringHelper* label) {
  oled.setCursor(6, row);
  oled.print(submenuSelect == index ? F("> ") : F("  "));
  oled.print(label);
}

static void printSpaces(uint8_t count) {
  while (count--) oled.print(' ');
}

// Fast 16-bit Q8 fixed point printing avoiding heavy uint32_t division libraries
static void printQ8Fixed(int16_t q8_val) {
  uint16_t val = (uint16_t)q8_val;
  uint8_t whole = val >> 8;
  uint8_t frac = ((val & 0xFF) * 100 + 128) >> 8;
  if (frac >= 100) { whole++; frac -= 100; }
  oled.print(whole);
  oled.print('.');
  if (frac < 10) oled.print('0');
  oled.print(frac);
}

static void printPaddedVal(int16_t val) {
  if (val < 10) printSpaces(3);
  else if (val < 100) printSpaces(2);
  else if (val < 1000) printSpaces(1);
  oled.print(val);
}

static void printSignedVal4(int16_t val) {
  int16_t absV = abs(val);
  oled.print(val >= 0 ? '+' : '-');
  if (absV < 10) oled.print(F("00"));
  else if (absV < 100) oled.print('0');
  oled.print(absV);
}

static uint8_t getShortcutIndex(uint8_t id) {
  for (uint8_t i = 0; i < 15; i++) {
    if (shortcut_ids[i] == id) return i;
  }
  return 0;
}

static void printShortcutName(uint8_t id) {
  char buf[6];
  strcpy_P(buf, shortcut_names[getShortcutIndex(id)]);
  oled.print(buf);
}

static void drawHorizontalLine(uint8_t page, uint8_t bitPattern) {
  oled.setCursor(0, page);
  for (uint8_t i = 0; i < 128; i++) {
    oled.ssd1306WriteRam(bitPattern);
  }
}

static void drawVerticalLine(uint8_t x, uint8_t startPage, uint8_t endPage) {
  for (uint8_t page = startPage; page <= endPage; page++) {
    oled.setCursor(x, page);
    oled.ssd1306WriteRam(0xFF);
  }
}

static void drawProgressBarOutline(uint8_t page) {
  oled.setCursor(10, page);
  oled.ssd1306WriteRam(0xFF);  
  for (uint8_t col = 11; col < 118; col++) {
    oled.ssd1306WriteRam(0x81);  
  }
  oled.ssd1306WriteRam(0xFF);  
}

static void drawProgressBarFill(uint8_t page, uint8_t barWidth) {
  oled.setCursor(11, page);
  for (uint8_t col = 11; col < 118; col++) {
    oled.ssd1306WriteRam((col - 11 < barWidth) ? 0xFF : 0x81);
  }
}

static void drawBiBar(uint8_t xStart, uint8_t xEnd, uint8_t xCenter, uint8_t page, int16_t val, int8_t& lastLen) {
  int16_t clampedVal = constrain(val, -350, 350);
  int8_t maxHalfWidth = (xEnd - xStart) / 2;
  int8_t len = (int8_t)(((int32_t)clampedVal * maxHalfWidth) / 350);

  if (len == lastLen) return; 
  lastLen = len;

  oled.setCursor(xStart, page);
  for (uint8_t col = xStart; col <= xEnd; col++) {
    if (col == xStart || col == xEnd || col == xCenter) {
      oled.ssd1306WriteRam(0xFF);  
    } else if (len > 0 && col > xCenter && col <= (xCenter + len)) {
      oled.ssd1306WriteRam(0xFF);  
    } else if (len < 0 && col < xCenter && col >= (xCenter + len)) {
      oled.ssd1306WriteRam(0xFF);  
    } else {
      oled.ssd1306WriteRam(0x81);  
    }
  }
}

static void drawHeader(const __FlashStringHelper* title, uint8_t x) {
  oled.clear();
  drawHorizontalLine(0, 0xFF);
  oled.setCursor(x, 0);
  oled.setInvertMode(true);
  oled.print(title);
  oled.setInvertMode(false);
}

static void printFooter() {
  printAt(4, 7, F("HL:BACK   HR:CONFIRM"));
}

static void printOledProtString(int8_t val) {
  if (val == 0) {
    oled.print(F("OFF"));
  } else {
    char m = '1';
    if (val == 2) m = '3';
    else if (val == 3) m = '5';
    oled.print(m);
    oled.print('m');
  }
}

static void adjustShortcut(ParamData& par, bool increment) {
  uint8_t current_id = (submenuSelect == 0) ? par.values->keyL_shortcut : par.values->keyR_shortcut;
  uint8_t idx = getShortcutIndex(current_id);
  if (increment) {
    idx++;
    if (idx > 14) idx = 0;
  } else {
    if (idx == 0) idx = 14;
    else idx--;
  }
  if (submenuSelect == 0) par.values->keyL_shortcut = shortcut_ids[idx];
  else par.values->keyR_shortcut = shortcut_ids[idx];
}

static void drawMenuList(const char* const* list, uint8_t size, uint8_t selected) {
  char buffer[18];
  for (uint8_t i = 0; i < size; i++) {
    oled.setCursor(0, i + 2);
    oled.setInvertMode(i == selected);
    strcpy_P(buffer, (char*)pgm_read_word(&(list[i])));
    oled.print(buffer);
  }
  oled.setInvertMode(false);
}

static void drawDirSubmenu(ParamData& par, uint8_t selected) {
  uint8_t topIdx = (selected < 5) ? 0 : (selected - 4);
  int8_t* flags = &par.values->invX; 

  char labelBuf[8];
  for (uint8_t row = 0; row < 5; row++) {
    uint8_t itemIdx = topIdx + row;
    oled.setCursor(6, row + 2);
    oled.print(itemIdx == selected ? F("> ") : F("  "));

    strcpy_P(labelBuf, dir_labels[itemIdx]);
    oled.print(labelBuf);
    oled.print(F(": "));

    oled.setInvertMode(itemIdx == selected);
    oled.print(flags[itemIdx] ? F("ON ") : F("OFF"));
    oled.setInvertMode(false);
    printSpaces(2);
  }
}

static void printSignedVal(int16_t val) {
  if (val >= 0) oled.print('+');
  oled.print(val);
  int16_t absV = abs(val);
  if (absV < 10) printSpaces(3);
  else if (absV < 100) printSpaces(2);
  else if (absV < 1000) printSpaces(1);
}

static void printOffsetLine(char label, int16_t offA, int16_t offB, uint8_t row) {
  oled.setCursor(18, row);
  oled.print(label);
  oled.print(F(": "));
  printSignedVal(offA);
  oled.print(F("| "));
  printSignedVal(offB);
}

void initOledDisplay() {
  oledPowerState = true;

  Wire.begin();
  Wire.setClock(400000L);  

#if defined(WIRE_HAS_TIMEOUT)
  Wire.setWireTimeout(3000, true);  
#endif

  oled.begin(&Adafruit128x64, I2C_ADDRESS);
  oled.setFont(System5x7); 
  oled.clear();

  drawHorizontalLine(0, 0x01);
  drawHorizontalLine(7, 0x80);

  printAt(34, 1, F("  O.R.B.7   "));
  printAt(31, 3, F(""));
  oled.print(F(FW_RELEASE));

  delay(800); // Brief splash display without heavy loading animation loop

  oled.clear();
  lastActivityTime = millis();
  forceFullViewRedraw = true;
}

// Unified helper for calibration warning/failed screens
static void showCalMsg(const __FlashStringHelper* title, uint8_t x, const __FlashStringHelper* line1, const __FlashStringHelper* line2) {
  drawHeader(title, x);
  printAt(14, 3, line1);
  printAt(20, 5, line2);
}

void showCalibrationScreen() {
  showCalMsg(F("CALIBRATION"), 31, F("Keep knob at rest"), F("Calibrating..."));
  delay(300);  
}

void showCalibrationWarningScreen() {
  showCalMsg(F("WARNING"), 41, F("Knob moved!"), F("Retrying..."));
}

void showCalibrationFailedScreen() {
  showCalMsg(F("WARNING"), 41, F("Calibration failed!"), F("Bypassing lock..."));
}

bool isOledMenuOpen() {
  return (menuState != 0);
}

// High-level UI Controller evaluating input actions and dispatching state transitions.
void processMenuInput(uint8_t* keyState, ParamData& par) {
  bool leftButton = (NUMKEYS > 1) ? keyState[1] : false;
  bool rightButton = (NUMKEYS > 0) ? keyState[0] : false;
  unsigned long now = millis();

  static unsigned long leftPressStart = 0;
  static unsigned long rightPressStart = 0;
  static bool leftHeldActive = false;
  static bool rightHeldActive = false;
  static bool leftHoldTriggered = false;
  static bool rightHoldTriggered = false;

  if (leftButton || rightButton) {
    lastActivityTime = now;
  }

  // --- LEFT BUTTON CONTROL ---
  if (leftButton) {
    if (!leftHeldActive) {
      leftHeldActive = true;
      leftPressStart = now;
      leftHoldTriggered = false;
    } else if (!leftHoldTriggered && (now - leftPressStart >= 1000)) {
      leftHoldTriggered = true;

      if (menuState == 1) {
        putParametersToEEPROM(par);
        menuState = 0;
        isEditing = false;
        forceFullViewRedraw = true;
      } else if (menuState >= 3) {
        if (isEditing) {
          isEditing = false;
        } else {
          if (menuState == 9 || menuState == 10 || menuState == 4 || menuState == 11 || menuState == 12 || menuState == 13) {
            menuState = 8;
          } else {
            menuState = 1; 
          }
        }
        forceFullViewRedraw = true;
      }
    }
  } else {
    if (leftHeldActive) {
      if (!leftHoldTriggered && (now - leftPressStart > 50)) {
        if (menuState == 1) {
          if (cursorIndex == 0) cursorIndex = 4;
          else cursorIndex--;
          forceFullViewRedraw = true;
        } else if (menuState == 3) {
          if (isEditing) {
            if (submenuSelect == 0) {
              if (par.values->globalSens > 10) par.values->globalSens -= 10;
            } else if (submenuSelect == 1) {
              if (par.values->deadzone > 0) par.values->deadzone--;
            } else if (submenuSelect == 2) {
              par.values->slope_at_zero_q8 -= 13;
              if (par.values->slope_at_zero_q8 < 26) par.values->slope_at_zero_q8 = 26;
            } else if (submenuSelect == 3) {
              par.values->slope_at_end_q8 -= 13;
              if (par.values->slope_at_end_q8 < 26) par.values->slope_at_end_q8 = 26; 
            } else if (submenuSelect == 4) {
              if (par.values->modFunc == 3) par.values->modFunc = 1;
              else if (par.values->modFunc == 1) par.values->modFunc = 0;
              else par.values->modFunc = 3;
            }
          } else {
            if (submenuSelect == 0) submenuSelect = 4;
            else submenuSelect--;
            forceFullViewRedraw = true;
          }
        } else if (menuState == 5) {
          if (isEditing) {
            if (par.values->oledSleepTimer == 0) par.values->oledSleepTimer = 3;
            else par.values->oledSleepTimer--;
          } else {
            if (submenuSelect == 0) submenuSelect = 1;
            else submenuSelect--;
            forceFullViewRedraw = true;
          }
        } else if (menuState == 7) {
          if (isEditing) {
            adjustShortcut(par, false);
          } else {
            submenuSelect = !submenuSelect;
            forceFullViewRedraw = true;
          }
        } else if (menuState == 8) {
          if (submenuSelect == 0) submenuSelect = 5;
          else submenuSelect--;
          forceFullViewRedraw = true;
        } else if (menuState == 10 && calState == 0) {
          menuState = 8;
          forceFullViewRedraw = true;
        } else if (menuState == 11) {
          if (isEditing) {
            if (submenuSelect == 0) {
              par.values->exclusiveMode = !par.values->exclusiveMode;
            } else if (submenuSelect == 1) {
              if (par.values->exclusiveHysteresis >= 5) par.values->exclusiveHysteresis -= 5;
              else par.values->exclusiveHysteresis = 0;
            }
          } else {
            if (submenuSelect == 0) submenuSelect = 1;
            else submenuSelect--;
            forceFullViewRedraw = true;
          }
        } else if (menuState == 14) {
          if (submenuSelect == 0) submenuSelect = 7;
          else submenuSelect--;
          forceFullViewRedraw = true;
        }
      }
      leftHeldActive = false;
    }
  }

  // --- RIGHT BUTTON CONTROL ---
  if (rightButton) {
    if (!rightHeldActive) {
      rightHeldActive = true;
      rightPressStart = now;
      rightHoldTriggered = false;
    } else if (!rightHoldTriggered && (now - rightPressStart >= 1000)) {
      rightHoldTriggered = true; 

      if (menuState == 0) {
        menuState = 1;
        cursorIndex = 0;
        isEditing = false;
        forceFullViewRedraw = true;
      } else if (menuState == 1) {
        menuState = pgm_read_byte(&destStates[cursorIndex]);
        submenuSelect = 0;
        isEditing = false;
        forceFullViewRedraw = true;
      } else if (menuState == 4) {
        drawHeader(F("RE-ZERO"), 41);
        printAt(20, 3, F("DO NOT TOUCH!"));
        printAt(17, 5, F("Calibrating..."));

        busyZeroing(centerPoints, 1000, false);

        drawHeader(F("RE-ZERO"), 41);
        printAt(32, 4, F("SUCCESSFUL!"));
        delay(1500);

        menuState = 8;
        isEditing = false;
        forceFullViewRedraw = true;
      } else if (menuState == 3 || menuState == 5 || menuState == 7 || menuState == 11) {
        isEditing = !isEditing;
        forceFullViewRedraw = true;
      } else if (menuState == 8) {
        if (submenuSelect == 0) menuState = 9;
        else if (submenuSelect == 1) { menuState = 10; calState = 0; }
        else if (submenuSelect == 2) menuState = 4;
        else if (submenuSelect == 3) { menuState = 11; submenuSelect = 0; }
        else if (submenuSelect == 4) menuState = 12;  
        else if (submenuSelect == 5) menuState = 13;  
        forceFullViewRedraw = true;
      } else if (menuState == 10 && calState == 0) {
        for (uint8_t i = 0; i < 8; i++) {
          calMin[i] = +1023;
          calMax[i] = -1023;
        }
        calStartTime = now;
        calState = 1; 
        forceFullViewRedraw = true;
      } else if (menuState == 13) {
        drawHeader(F("RESETTING"), 31);
        printAt(20, 3, F("Restoring..."));

        *par.values = ParamStorage(); 
        putParametersToEEPROM(par);    

        drawHeader(F("SUCCESS"), 44);
        printAt(14, 3, F("Factory Defaults"));
        printAt(34, 5, F("Restored!"));
        delay(1500);

        menuState = 8;
        forceFullViewRedraw = true;
      } else if (menuState == 14) {
        int8_t* flags = &par.values->invX;
        flags[submenuSelect] = !flags[submenuSelect];
        putParametersToEEPROM(par);
        forceFullViewRedraw = true;
      }
    }
  } else {
    if (rightHeldActive) {
      if (!rightHoldTriggered && (now - rightPressStart > 50)) {
        if (menuState == 1) {
          cursorIndex++;
          if (cursorIndex > 4) cursorIndex = 0;
          forceFullViewRedraw = true;
        } else if (menuState == 3) {
          if (isEditing) {
            if (submenuSelect == 0) {
              if (par.values->globalSens < 300) par.values->globalSens += 10;
            } else if (submenuSelect == 1) {
              if (par.values->deadzone < 200) par.values->deadzone++;
            } else if (submenuSelect == 2) {
              par.values->slope_at_zero_q8 += 13;
              if (par.values->slope_at_zero_q8 > 768) par.values->slope_at_zero_q8 = 768; 
            } else if (submenuSelect == 3) {
              par.values->slope_at_end_q8 += 13;
              if (par.values->slope_at_end_q8 > 402) par.values->slope_at_end_q8 = 402; 
            } else if (submenuSelect == 4) {
              if (par.values->modFunc == 0) par.values->modFunc = 1;
              else if (par.values->modFunc == 1) par.values->modFunc = 3;
              else par.values->modFunc = 0;
            }
          } else {
            submenuSelect++;
            if (submenuSelect > 4) submenuSelect = 0;
            forceFullViewRedraw = true;
          }
        } else if (menuState == 5) {
          if (isEditing) {
            par.values->oledSleepTimer++;
            if (par.values->oledSleepTimer > 3) par.values->oledSleepTimer = 0;
          } else {
            submenuSelect++;
            if (submenuSelect > 1) submenuSelect = 0;
            forceFullViewRedraw = true;
          }
        } else if (menuState == 7) {
          if (isEditing) {
            adjustShortcut(par, true);
          } else {
            submenuSelect = !submenuSelect;
            forceFullViewRedraw = true;
          }
        } else if (menuState == 8) {
          submenuSelect++;
          if (submenuSelect > 5) submenuSelect = 0;
          forceFullViewRedraw = true;
        } else if (menuState == 11) {
          if (isEditing) {
            if (submenuSelect == 0) {
              par.values->exclusiveMode = !par.values->exclusiveMode;
            } else if (submenuSelect == 1) {
              if (par.values->exclusiveHysteresis <= 295) par.values->exclusiveHysteresis += 5;
            }
          } else {
            submenuSelect = !submenuSelect;
            forceFullViewRedraw = true;
          }
        } else if (menuState == 12) {
          par.values->compEnabled = !par.values->compEnabled;
          putParametersToEEPROM(par);
          forceFullViewRedraw = true;
        } else if (menuState == 14) {
          submenuSelect++;
          if (submenuSelect > 7) submenuSelect = 0;
          forceFullViewRedraw = true;
        }
      }
      rightHeldActive = false;
    }
  }
}

static void printSensorLine(char label, int16_t valA, int16_t valB, uint8_t row) {
  oled.setCursor(0, row);
  oled.print(label);
  oled.print(F(": "));
  printPaddedVal(valA);
  oled.print('|');
  printPaddedVal(valB);
  oled.print(F(" ["));

  int16_t delta = abs(valB - valA);
  if (delta < 10) printSpaces(2);
  else if (delta < 100) printSpaces(1);
  oled.print(delta);
  oled.print(F("]"));

  if (delta > 50) oled.print(F(" !")); 
  else oled.print(F(" OK"));
}

void updateOledDisplay(int16_t* velocity, uint8_t* keyState, ParamData& par) {
  unsigned long now = millis();
  static unsigned long lastOledUpdate = 0;

  if (now - lastOledUpdate < 30) return;
  lastOledUpdate = now;

#if defined(WIRE_HAS_TIMEOUT)
  if (Wire.getWireTimeoutFlag()) {
    Wire.clearWireTimeoutFlag();
    oled.begin(&Adafruit128x64, I2C_ADDRESS);
    oled.setFont(System5x7);
    forceFullViewRedraw = true;
  }
#endif

  bool motionActive = false;
  for (uint8_t i = 0; i < 6; i++) {
    if (abs(velocity[i]) > 5) {
      motionActive = true;
      break;
    }
  }

  bool buttonActive = false;
#if NUMKEYS > 0
  for (uint8_t i = 0; i < NUMKEYS; i++) {
    if (keyState[i]) {
      buttonActive = true;
      break;
    }
  }
#endif

  if (motionActive || buttonActive || menuState == 9) {
    lastActivityTime = now;
    if (!oledPowerState) {
      oledPowerState = true;
      oled.ssd1306WriteCmd(SSD1306_DISPLAYON);
      forceFullViewRedraw = true;
    }
  }

  if (oledPowerState && par.values->oledSleepTimer > 0) {
    static int8_t lastSleepTimerParam = -1;
    static unsigned long cachedSleepTimeout = 0;

    if (par.values->oledSleepTimer != lastSleepTimerParam) {
      lastSleepTimerParam = par.values->oledSleepTimer;
      cachedSleepTimeout = ((unsigned long)lastSleepTimerParam * 2 - 1) * 60000UL;
    }

    if (now - lastActivityTime >= cachedSleepTimeout) {
      oledPowerState = false;
      oled.ssd1306WriteCmd(SSD1306_DISPLAYOFF);
      menuState = 0; 
      forceFullViewRedraw = true;
    }
  }

  if (!oledPowerState) return;

  static uint8_t lastBarWidth = 0xFF;
  static int16_t lastSensValue = 0xFFFF;
  static int16_t lastDeadValue = 0xFFFF;
  static uint8_t lastCursorIndex = 0xFF;
  static int16_t lastSlopeA = -1;
  static int16_t lastSlopeB = -1;
  static int16_t lastModFunc = -1;
  static int8_t lastPowerState = -1;
  static int8_t lastOledSleepTimer = -1;
  static int8_t lastKeyL = -1;
  static int8_t lastKeyR = -1;
  static uint8_t lastSubmenuSelect = 0xFF;
  static bool lastIsEditing = false;
  static uint8_t lastRemaining = 0xFF;
  static uint8_t lastProgressWidth = 0xFF;

  static int16_t lastTransVal[3] = { 9999, 9999, 9999 };
  static int16_t lastRotVal[3] = { 9999, 9999, 9999 };
  static int8_t lastTransBarLen[3] = { 127, 127, 127 };
  static int8_t lastRotBarLen[3] = { 127, 127, 127 };

  bool redraw = forceFullViewRedraw;

  if (redraw) {
    lastBarWidth = 0xFF;
    lastSensValue = 0xFFFF;
    lastDeadValue = 0xFFFF;
    lastCursorIndex = 0xFF;
    lastSlopeA = -1;
    lastSlopeB = -1;
    lastModFunc = -1;
    lastPowerState = -1;
    lastOledSleepTimer = -1;
    lastKeyL = -1;
    lastKeyR = -1;
    lastSubmenuSelect = 0xFF;
    lastIsEditing = false;
    lastRemaining = 0xFF;
    lastProgressWidth = 0xFF;

    for (uint8_t i = 0; i < 3; i++) {
      lastTransBarLen[i] = 127;
      lastRotBarLen[i] = 127;
      lastTransVal[i] = 9999;
      lastRotVal[i] = 9999;
    }
  }

  // ==========================================
  // STATE 1: MAIN OPTIONS SELECTION MENU
  // ==========================================
  if (menuState == 1) {
    if (redraw) {
      drawHeader(F("OPTIONS"), 44);
      forceFullViewRedraw = false;
    }
    if (cursorIndex != lastCursorIndex) {
      drawMenuList(menu_strings, 5, cursorIndex);
      lastCursorIndex = cursorIndex;
    }
    return;
  }

  // ==========================================
  // STATE 3: SENSITIVITY & DEADZONE SUBMENU
  // ==========================================
  if (menuState == 3) {
    if (redraw || par.values->globalSens != lastSensValue || par.values->deadzone != lastDeadValue || par.values->slope_at_zero_q8 != lastSlopeA || par.values->slope_at_end_q8 != lastSlopeB || par.values->modFunc != lastModFunc || submenuSelect != lastSubmenuSelect || isEditing != lastIsEditing) {
      if (redraw) {
        drawHeader(F("SENSITIVITY"), 31);
        printFooter();
        forceFullViewRedraw = false;
      }

      printMenuLabel(2, 0, F("Sens: "));
      oled.setInvertMode(submenuSelect == 0 && isEditing);
      oled.print(par.values->globalSens);
      oled.print('%');
      printSpaces(3);
      oled.setInvertMode(false);

      printMenuLabel(3, 1, F("Deadzone: "));
      oled.setInvertMode(submenuSelect == 1 && isEditing);
      oled.print(par.values->deadzone);
      printSpaces(3);
      oled.setInvertMode(false);

      printMenuLabel(4, 2, F("Curve A: "));
      oled.setInvertMode(submenuSelect == 2 && isEditing);
      printQ8Fixed(par.values->slope_at_zero_q8);
      printSpaces(2);
      oled.setInvertMode(false);

      printMenuLabel(5, 3, F("Curve B: "));
      oled.setInvertMode(submenuSelect == 3 && isEditing);
      printQ8Fixed(par.values->slope_at_end_q8);
      printSpaces(2);
      oled.setInvertMode(false);

      printMenuLabel(6, 4, F("Mode: "));
      oled.setInvertMode(submenuSelect == 4 && isEditing);
      if (par.values->modFunc == 3) oled.print(F("A/B "));
      else if (par.values->modFunc == 1) oled.print(F("SQR "));
      else oled.print(F("LIN "));
      oled.setInvertMode(false);

      lastSensValue = par.values->globalSens;
      lastDeadValue = par.values->deadzone;
      lastSlopeA = par.values->slope_at_zero_q8;
      lastSlopeB = par.values->slope_at_end_q8;
      lastModFunc = par.values->modFunc;
      lastSubmenuSelect = submenuSelect;
      lastIsEditing = isEditing;
    }
    return;
  }

  // ==========================================
  // STATE 4: RE-ZERO CALIBRATION
  // ==========================================
  if (menuState == 4) {
    if (redraw) {
      drawHeader(F("RE-ZERO"), 41);
      printAt(14, 2, F("Keep knob at rest"));
      printAt(26, 3, F("DO NOT TOUCH!"));
      printAt(20, 5, F("Hold R: Start"));
      drawHorizontalLine(6, 0x01);
      printFooter();
      forceFullViewRedraw = false;
    }
    return;
  }

  // ==========================================
  // STATE 5: DISPLAY SETTINGS (OLED SETUP)
  // ==========================================
  if (menuState == 5) {
    if (redraw || par.values->oledSleepTimer != lastOledSleepTimer || oledPowerState != lastPowerState) {
      if (redraw) {
        drawHeader(F("DISPLAY"), 41);
        printFooter();
        forceFullViewRedraw = false;
      }
      printMenuLabel(2, 0, F("SLEEP: "));
      oled.setInvertMode(submenuSelect == 0 && isEditing);
      printOledProtString(par.values->oledSleepTimer);
      oled.setInvertMode(false);
      printSpaces(2);

      printMenuLabel(3, 1, F("POWER: "));
      oled.setInvertMode(submenuSelect == 1 && isEditing);
      oled.print(oledPowerState ? F("ON") : F("OFF"));
      oled.setInvertMode(false);
      printSpaces(2);

      lastOledSleepTimer = par.values->oledSleepTimer;
      lastPowerState = oledPowerState;
    }
    return;
  }

  // ==========================================
  // STATE 7: BUTTONS SHORTCUT MAPPING
  // ==========================================
  if (menuState == 7) {
    if (redraw || par.values->keyL_shortcut != lastKeyL || par.values->keyR_shortcut != lastKeyR) {
      if (redraw) {
        drawHeader(F("BUTTONS"), 41);
        printFooter();
        forceFullViewRedraw = false;
      }
      printMenuLabel(2, 0, F("LEFT:  "));
      oled.setInvertMode(submenuSelect == 0 && isEditing);
      printShortcutName(par.values->keyL_shortcut);
      oled.setInvertMode(false);
      printSpaces(2);

      printMenuLabel(3, 1, F("RIGHT: "));
      oled.setInvertMode(submenuSelect == 1 && isEditing);
      printShortcutName(par.values->keyR_shortcut);
      oled.setInvertMode(false);
      printSpaces(2);

      lastKeyL = par.values->keyL_shortcut;
      lastKeyR = par.values->keyR_shortcut;
    }
    return;
  }

  // ==========================================
  // STATE 8: DEBUG SUBMENU
  // ==========================================
  if (menuState == 8) {
    if (redraw) {
      drawHeader(F("DEBUG"), 49);
      forceFullViewRedraw = false;
    }
    if (submenuSelect != lastSubmenuSelect || redraw) {
      drawMenuList(debug_strings, 6, submenuSelect);
      lastSubmenuSelect = submenuSelect;
    }
    return;
  }

  // ==========================================
  // STATE 9: HALL SENSORS PHYSICAL ALIGNMENT
  // ==========================================
  if (menuState == 9) {
    if (redraw) {
      drawHeader(F("SENSORS"), 41);
      printAt(0, 1, F("     A  |  B    DELTA"));
      forceFullViewRedraw = false;
    }

    printSensorLine('N', rawReads[4], rawReads[5], 2);
    printSensorLine('S', rawReads[0], rawReads[1], 3);
    printSensorLine('E', rawReads[2], rawReads[3], 4);
    printSensorLine('W', rawReads[6], rawReads[7], 5);

    int16_t delS = abs(rawReads[1] - rawReads[0]);
    int16_t delE = abs(rawReads[3] - rawReads[2]);
    int16_t delN = abs(rawReads[5] - rawReads[4]);
    int16_t delW = abs(rawReads[7] - rawReads[6]);

    oled.setCursor(0, 7);
    if (delS <= 50 && delE <= 50 && delN <= 50 && delW <= 50) {
      oled.print(F("SYSTEM BALANCED!   "));
    } else {
      oled.print(F("ADJUST ROT / BOLTS "));
    }
    return;
  }

  // ==========================================
  // STATE 10: DYNAMIC LIMITS CALIBRATION (CAL. LIMITS)
  // ==========================================
  if (menuState == 10) {
    if (calState == 0) {
      if (redraw) {
        drawHeader(F("CAL. LIMITS"), 31);
        printAt(14, 2, F("Keep hands ready"));
        printAt(8, 3, F("Move knob to extremes"));
        printAt(10, 4, F("during 20s test"));
        printAt(14, 6, F("Hold R (1s): Start"));
        printFooter();
        forceFullViewRedraw = false;
      }
    } else if (calState == 1) {
      unsigned long elapsed = now - calStartTime;
      for (uint8_t i = 0; i < 8; i++) {
        int16_t curVal = rawReads[i] - centerPoints[i] + offsets[i];
        if (curVal < calMin[i]) calMin[i] = curVal;
        if (curVal > calMax[i]) calMax[i] = curVal;
      }

      if (elapsed < 20000UL) { 
        if (redraw) {
          drawHeader(F("CALIBRATING"), 31);
          printAt(19, 2, F("Move knob fully"));
          lastRemaining = 0xFF;
          lastProgressWidth = 0xFF;
          lastBarWidth = 0xFF;
          forceFullViewRedraw = false;
        }

        uint8_t remaining = 20 - (elapsed / 1000);
        if (remaining != lastRemaining) {
          oled.setCursor(37, 4);
          oled.print(F("Time: "));
          oled.print(remaining);
          oled.print('s');
          printSpaces(2);
          lastRemaining = remaining;
        }

        uint8_t progressWidth = (elapsed * 106) / 20000UL;
        if (redraw) drawProgressBarOutline(5);
        if (progressWidth != lastProgressWidth || redraw) {
          drawProgressBarFill(5, progressWidth);
          lastProgressWidth = progressWidth;
        }
      } else {
        bool sanityPass = true;
        int16_t dz = (par.values->deadzone < 0) ? 0 : par.values->deadzone;

        for (uint8_t i = 0; i < 8; i++) {
          int16_t range = calMax[i] - calMin[i];
          if (range < 80 || calMin[i] >= -dz || calMax[i] <= dz) {
            sanityPass = false;
            break;
          }
        }

        if (sanityPass) {
          for (uint8_t i = 0; i < 8; i++) {
            par.values->minVals[i] = calMin[i];
            par.values->maxVals[i] = calMax[i];
          }
          putParametersToEEPROM(par);
          calState = 2;
        } else {
          calState = 3; 
        }
        calStartTime = now;
        forceFullViewRedraw = true;
      }
    } else if (calState == 2) {
      if (redraw) {
        drawHeader(F("SUCCESS"), 44);
        printAt(26, 3, F("Limits Saved!"));
        printAt(20, 5, F("Saved to EEPROM"));
        forceFullViewRedraw = false;
      }
      if (now - calStartTime >= 2000) {
        menuState = 8;
        forceFullViewRedraw = true;
      }
    } else if (calState == 3) {
      if (redraw) {
        drawHeader(F("FAILED"), 44);
        printAt(20, 3, F("Cal. Invalid!"));
        printAt(14, 5, F("Knob not moved?"));
        forceFullViewRedraw = false;
      }
      if (now - calStartTime >= 2500) {
        menuState = 8;
        forceFullViewRedraw = true;
      }
    }
    return;
  }

  // ==========================================
  // STATE 11: EXCLUSIVE MODE CONFIGURATION SCREEN
  // ==========================================
  if (menuState == 11) {
    if (redraw || par.values->exclusiveMode != lastPowerState || par.values->exclusiveHysteresis != lastDeadValue || submenuSelect != lastSubmenuSelect || isEditing != lastIsEditing) {
      if (redraw) {
        drawHeader(F("EXCLUSIVE"), 34);
        printFooter();
        forceFullViewRedraw = false;
      }
      printMenuLabel(2, 0, F("MODE: "));
      oled.setInvertMode(submenuSelect == 0 && isEditing);
      oled.print(par.values->exclusiveMode ? F("ON ") : F("OFF"));
      oled.setInvertMode(false);
      printSpaces(2);

      printMenuLabel(3, 1, F("HYST: "));
      oled.setInvertMode(submenuSelect == 1 && isEditing);
      oled.print(par.values->exclusiveHysteresis);
      oled.setInvertMode(false);
      printSpaces(3);

      lastPowerState = par.values->exclusiveMode;
      lastDeadValue = par.values->exclusiveHysteresis;
      lastSubmenuSelect = submenuSelect;
      lastIsEditing = isEditing;
    }
    return;
  }

  // ==========================================
  // STATE 12: LIVE DRIFT OFFSETS & TOGGLE SCREEN
  // ==========================================
  if (menuState == 12) {
    if (redraw || par.values->compEnabled != lastPowerState) {
      if (redraw) {
        drawHeader(F("DRIFT OFFSETS"), 25);
        printAt(18, 2, F("     A  |   B"));
        printAt(4, 7, F("R: TOGGLE   HL: BACK"));
        forceFullViewRedraw = false;
      }
      oled.setCursor(34, 1);
      oled.print(F("DRIFT: "));
      oled.print(par.values->compEnabled ? F("ON ") : F("OFF"));
      lastPowerState = par.values->compEnabled;
    }

    printOffsetLine('N', offsets[4], offsets[5], 3);  
    printOffsetLine('S', offsets[0], offsets[1], 4);  
    printOffsetLine('E', offsets[2], offsets[3], 5);  
    printOffsetLine('W', offsets[6], offsets[7], 6);  
    return;
  }

  // ==========================================
  // STATE 13: FACTORY RESET CONFIRMATION SCREEN
  // ==========================================
  if (menuState == 13) {
    if (redraw) {
      drawHeader(F("FACTORY RESET"), 25);
      printAt(14, 2, F("Reset EEPROM data?"));
      printAt(8, 3, F("All custom settings"));
      printAt(22, 4, F("will be erased"));
      printAt(10, 6, F("Hold R (1s): RESET"));
      printAt(28, 7, F("HOLD L: BACK"));
      forceFullViewRedraw = false;
    }
    return;
  }

  // ==========================================
  // STATE 14: AXIS DIRECTION & INVERSION SUBMENU
  // ==========================================
  if (menuState == 14) {
    if (redraw || submenuSelect != lastSubmenuSelect) {
      if (redraw) {
        drawHeader(F("DIRECTION"), 37);
        printAt(4, 7, F("HOLD R: TOGGLE  HL: BACK"));
        forceFullViewRedraw = false;
      }
      drawDirSubmenu(par, submenuSelect);
      lastSubmenuSelect = submenuSelect;
    }
    return;
  }

  // ==========================================
  // HOME STATE 0: PERFECTLY ALIGNED AXIS VISUALIZER
  // ==========================================
  if (redraw) {
    drawHeader(F("AXIS"), 52);

    printAt(0, 1, F("   TRANS"));
    printAt(72, 1, F("   ROT"));
    drawHorizontalLine(2, 0x02);
    drawVerticalLine(65, 1, 6);

    for (uint8_t i = 0; i < 3; i++) {
      char axisChar = (i == 0) ? 'X' : ((i == 1) ? 'Y' : 'Z');
      oled.setCursor(2, i + 3);
      oled.print(axisChar); oled.print(':');
      oled.setCursor(70, i + 3);
      oled.print(axisChar); oled.print(':');
    }

    drawHorizontalLine(6, 0x40);
    printAt(10, 7, F("HOLD [R] : OPTIONS"));

    for (uint8_t i = 0; i < 3; i++) {
      lastTransBarLen[i] = 127;
      lastRotBarLen[i] = 127;
      lastTransVal[i] = 9999;
      lastRotVal[i] = 9999;
    }
    forceFullViewRedraw = false;
  }

  for (uint8_t i = 0; i < 3; i++) {
    drawBiBar(16, 37, 26, i + 3, velocity[i], lastTransBarLen[i]);
    if (velocity[i] != lastTransVal[i]) {
      oled.setCursor(39, i + 3);
      printSignedVal4(velocity[i]); 
      lastTransVal[i] = velocity[i];
    }
  }

  for (uint8_t i = 0; i < 3; i++) {
    drawBiBar(83, 103, 93, i + 3, velocity[i + 3], lastRotBarLen[i]);
    if (velocity[i + 3] != lastRotVal[i]) {
      oled.setCursor(104, i + 3);
      printSignedVal4(velocity[i + 3]);
      lastRotVal[i] = velocity[i + 3];
    }
  }
}

#endif  // ENABLE_OLED