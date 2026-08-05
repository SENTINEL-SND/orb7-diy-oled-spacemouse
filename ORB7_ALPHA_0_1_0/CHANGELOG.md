# Changelog

All notable changes to the SpaceMouse Pro Emulator (O.R.B.7) project will be documented in this file.

## [v0.1.0 / Web Studio v1.6.3] - 2026-08-04 (Firmware Audit, Kinematic Overflow Fix, CLI Memory Safety & USB Idle Suppression)

### Fixed
- **Translation Z Physical Direction Inversion Alignment** (`kinematics.cpp`): Relocated software axis inversion flags (`invX`..`invRZ`) to execute after physical sensitivity and noise gate processing (`processAxis`). Prevents standard CAD inversions (`invZ = 1`) from misinterpreting physical Push Down gestures (+Z) as Pull Up (-Z), ensuring TPU spring asymmetry settings (`SENS_PTZ` vs `SENS_NTZ` / `GATE_NTZ`) remain 100% aligned with physical hand gestures regardless of software direction settings.
- **Full 37-Parameter Serial CLI Mapping Realignment** (`parameterMenu.h`, `ORB7_...ino`, `parameterMenu.cpp`): Expanded `NUM_PARAMS` from 34 to 37, updated `par.description` descriptor array, and corrected Q7/Q8 fixed-point index checks in `parameterMenu.cpp`. Restores full Serial CLI access to `oledSleepTimer`, `keyL_shortcut`, and `keyR_shortcut`.
- **OLED Screen Power Immediate Re-awakening** (`oledDisplay.cpp`): Added state transition tracking for `oledSleepTimer` in `updateOledDisplay()`. Re-enabling OLED display power from Web Studio now triggers immediate hardware activation (`SSD1306_DISPLAYON`) and view redraw without requiring physical knob movement.
- **Sensor Pair Alignment Cards Dynamic Raw Value Rendering** (`js/webhid.js`): Fixed a visual UI rendering bug in `updateAlignment()` where `#raw-N`, `#raw-S`, `#raw-E`, and `#raw-W` DOM elements on the alignment cards were not being updated with live raw sensor pair readings (`valA | valB`).
- **Chart.js Canvas Tab-Switch Resizing** (`js/ui.js`): Fixed a Chart.js canvas sizing bug in `switchTab()` where switching to the "Sensitivities" tab from a hidden state collapsed or squished the modifier curve plot. Added explicit `curveChart.resize()` and `curveChart.update('none')` calls upon tab activation.
- **3D Viewport Three.js Window Resize Listener** (`js/viewport3d.js`): Integrated a `window.addEventListener('resize', onWindowResize)` handler in `init3DViewport()` to dynamically update camera aspect ratio and WebGL renderer dimensions when the browser window or container resizes.
- **USB Disconnect Diagnostic Badge Reset & Modal Auto-Close** (`js/webhid.js`): Updated `handleDisconnect()` to reset the Magnetic Balance badge (`#healthBalance`) to `"STANDBY"` (`health-badge bad`) and automatically close the Setup Wizard modal (`closeWizard()`) if open when the device is unplugged or rebooted.
- **Setup Wizard 20s Test Navigation Guard** (`js/ui.js`): Added an active test validation check in `#wizBtnNext` handler (`if (window.isWizardActive)`) preventing users from advancing premature wizard steps while the 20-second boundary sampling test is running.
- **Preset Selector & Wizard Inversion Sync on JSON Profile Import** (`js/ui.js`): Updated `importProfile()` to set `#selBtnPreset.value = "custom"` and automatically synchronize counterpart wizard inversion buttons (`#wizInv${axis}`) in `setToggle()` upon importing a profile.
- **Q7 Kinematic Division Overflow Guard** (`kinematics.cpp`): Added 32-bit `constrain` bounds inside `divideBySensitivity()` prior to 16-bit integer casting. Prevents high-sensitivity Q7 division results ($sens\_q7 \le 1$) from overflowing signed `int16_t` bounds ($> 32,767 \to -20,736$), which previously caused physical peak deflections to invert directions unexpectedly.
- **Serial CLI 8-bit Parameter Type Mapping** (`ORB7_...ino`): Corrected parameter descriptor types for parameters 33 to 37 (`OLED_SLEEP`, `KEYL_SHORT`, `KEYR_SHORT`, `KEY2_SHORT`, `KEY1_SHORT`) in `par.description` array from `PARAM_TYPE_INT` to `PARAM_TYPE_BOOL`. Eliminates 16-bit pointer dereferencing on `int8_t` variables that previously caused CLI parameter edits on `OLED_SLEEP` to corrupt adjacent memory fields in `ParamStorage`.
- **USB Bus Idle Flooding Prevention** (`SpaceMouseHID.cpp`): Added saturation caps (`countTransZeros < 255` and `countRotZeros < 255`) in `send_command()` state machine. Prevents 8-bit zero counters from wrapping around from 255 back to 0 every ~2 seconds during rest, eliminating phantom USB zero-report transmissions.
- **Serial Debug Macro Compilation Fix** (`config.h`): Declared missing `#define DEBUGDELAY 100` macro in `config.h`, resolving compilation errors (`'DEBUGDELAY' was not declared in this scope`) when `#define ENABLE_SERIAL_DEBUG 1` is enabled.
- **OLED Header Function Prototype Syntax Fix** (`oledDisplay.h`): Added missing `void` return type to `showCalibrationWarningScreen()` prototype declaration in `oledDisplay.h`.
- **Debug Drift Plotter Array Boundary Guard** (`calibration.cpp`): Added an array index safety check (`axis < 0 || axis >= 8`) in `debugDriftPlotter()` to prevent out-of-bounds sensor array reads during serial debug plotting.
- **Debounced Kill-Keys Evaluation** (`ORB7_...ino`): Updated `NUMKILLKEYS` evaluation in `loop()` to check debounced logical state (`keyState[KILLROT] == 1`) instead of raw un-debounced pin reads (`keyVals[KILLROT] == LOW`).

### Changed
- **Firmware Release Version Milestone Bump** (`release.h`, `index.html`): Promoted firmware release version to official milestone `v0.1.0`.
- **Translation MicroGate Factory Default Realignment** (`config.h`): Updated `#define GATE_TRANS` default macro from `10` to `6` to align 1:1 with file header documentation (`Default: 6`) and Web Studio initial EEPROM defaults.
- **Code Cleanup** (`kinematics.cpp`): Removed unused `#define sign(x)` macro definition.

## [vALPHA 0.0.9 / Web Studio v1.6.3] - 2026-08-04 (Smooth Deadband Kinematics, Guided Setup Wizard, 100Hz Telemetry & Electric Cyan Theme)

### Added
- **Interactive 5-Step Guided Setup Wizard** (`index.html`, `style.css`, `ui.js`): Integrated an onboarding modal overlay for freshly assembled hardware builds featuring an interactive 5-step guided calibration flow:
  - Step 1: Magnetic Pair Check (Live Differential Deltas $\Delta \le 100$).
  - Step 2: TPU Neutral Re-Zero.
  - Step 3: 20-Second Dynamic Boundaries Test.
  - Step 4: CAD Factory Inversions Verification.
  - Step 5: Complete & EEPROM Persistence.
- **CAD Industry Standard Factory Defaults in Wizard** (`index.html`, `ui.js`): Pre-configured Step 4 orientation toggles to 3DConnexion industry defaults (`TX: OFF, TY: ON, TZ: ON, RX: OFF, RY: ON, RZ: ON`) matching SolidWorks, Fusion 360, Inventor, and Blender, complete with an explanatory guide box detailing camera-relative navigation physics.
- **Comprehensive Parameter Impact Guides & 3-Tier Firewall Overview** (`index.html`): Enhanced all Web Studio guide boxes with explicit **INCREASE [▲] / DECREASE [▼]** parameter impact breakdowns. Filled vacant UI area under Exclusive Mode Tuning with a detailed 3-tier firmware firewall overview (Tier 1: Thermal Drift, Tier 2: Noise Gates/MicroGate, Tier 3: Exclusive Mode).
- **Persistent Floating Footer & Attribution** (`index.html`, `style.css`): Added a fixed-position persistent footer linking to the official open-source GitHub repository (`SENTINEL-SND/orb7-diy-oled-spacemouse`) under the MIT License.
- **Google Fonts Orbitron Typography** (`index.html`, `style.css`): Imported Orbitron font (wght 700/900) for logo branding (`.brand-title h1`) and modal headers (`.modal-header h2`).

### Fixed
- **Smooth Deadband Kinematic Continuous Subtraction** (`kinematics.cpp`): Refactored `processAxis()` from a hard step cutoff (`if (abs(vel) < gate) vel = 0`) to a smooth continuous deadband subtraction (`if (vel > gate) vel -= gate; else if (vel < -gate) vel += gate; else vel = 0;`). Completely eliminates initial motion "jumps" when exiting rest position, delivering buttery-smooth 6DOF acceleration starting from 0.
- **Full-Scale ADC Physical Range Capture in Wizard** (`webhid.js`): Expanded outlier noise filter range inside `parseTelemetryData` from `[-900..900]` to full ADC scale `[-1023..1023]`, preventing valid physical sensor deflections at extreme mechanical stops from being incorrectly discarded as noise glitches.
- **Deadzone-Aware Calibration Limits Validation** (`ui.js`): Fixed manual limits table and wizard validation rules to strictly check `min < -dz` and `max > dz` (using active EEPROM deadzone `slDeadzone`) instead of `min < 0` and `max > 0`. Prevents saving narrow limits that resulted in negative/zero denominators (`denom = -dz - minVals[i]`) in C++ `FilterAnalogReadOuts()`, which previously zeroed out sensor outputs.
- **Re-Zero Hardware Delay Realignment** (`ui.js`): Increased JS async delay from `800ms` to `1200ms` prior to starting the 20-second dynamic limits test, ensuring the MCU hardware `busyZeroing()` sampling loop (1000 iterations ~832ms + OLED/I2C overhead) finishes cleanly before telemetry recording begins.

### Changed
- **Electric Cyan/Blue UI Theme Palette (`#05ACFF`)** (`style.css`, `viewport3d.js`, `ui.js`): Replaced green/cyan accents (`#00ffcc`) with Electric Cyan/Blue `#05ACFF` across CSS variables, Three.js 3D puck waist ring and grid centerlines (`0x05ACFF`), and Chart.js curve plotters.
- **Quadrupled Telemetry Streaming Refresh Rate** (`webHID.cpp`): Reduced streaming interval in `streamWebHIDRawData()` from `30ms` (33 Hz) to `10ms` (100 Hz). Allows Web Studio to capture fast physical motion peaks during dynamic calibration without dropping 75% of MCU samples.
- **Firmware Version Release Bump** (`release.h`): Incremented firmware release patch version to `ALPHA 0.0.9`.

## [vALPHA 0.0.8 / Web Studio v1.6.2] - 2026-08-03 (Dynamic Version Reporting, Hardware Reboot Command & Metrological Hardening)

### Added
- **Dynamic Firmware Version Reporting via WebHID** (`release.h`, `webHID.cpp`, `js/webhid.js`): Embedded structured version macros (`FW_VERSION_MAJOR`, `FW_VERSION_MINOR`, `FW_VERSION_PATCH`) in `release.h` and packed them into trailing free bytes (`txBuffer[60..62]`) of the WebHID configuration response payload (`0x01`). The Web Studio now dynamically reads and displays the active firmware version from the connected device in the `#healthFw` DOM element.
- **Hardware Software Reboot Command (`WEBHID_CMD_RESTART` / `0x08`)** (`webHID.h`, `webHID.cpp`, `ORB7_...ino`, `index.html`, `js/webhid.js`, `js/ui.js`): Implemented a "Restart Mouse" feature allowing remote software reboot of the ATmega32U4 MCU via the AVR Watchdog Timer (`wdt_enable(WDTO_60MS)`). Added a dedicated "Restart Mouse" button (`#btnRestart`) in the Debug Studio panel.
- **Clean Watchdog Reset Recovery** (`ORB7_...ino`): Added `MCUSR = 0; wdt_disable();` at the very beginning of `setup()` to safely reset Watchdog flags and prevent infinite boot-loop cycles upon software reboot.
- **Global USB Disconnect Event Listener** (`js/webhid.js`): Added a `navigator.hid.addEventListener('disconnect', ...)` handler that automatically resets UI connection badges to `OFFLINE`, clears telemetry frequency counters, and resets the connect button to "CONNECT DEVICE" when the mouse is rebooted or unplugged.
- **Proactive Auto Re-Zero for Calibration Wizard** (`js/ui.js`): Updated the 20-second Limits Wizard (`#btnStartCalWizard`) to execute an automatic `sendRezeroCommand()` prior to launching the 20-second boundary test, ensuring clean rest coordinates before physical movement begins.
- **Tab Visibility Monitor during Calibration** (`js/ui.js`): Integrated a `visibilitychange` listener that alerts the user if the browser tab loses focus during the 20-second calibration test, preventing background timer throttling from distorting measurement accuracy.

### Fixed
- **Outlier Rejection Filter in Calibration Wizard** (`js/webhid.js`): Added an ADC noise spike guard inside `parseTelemetryData` during `isWizardActive` that discards improbable single-frame noise glitches (> 900 or < -900), preventing electrical spikes from expanding dynamic limits artificially.
- **Invalid Calibration Limit Save Guard** (`js/webhid.js`): Added a validation check in `saveConfigToDevice()` that verifies all manual calibration rows before writing to EEPROM. If any sensor row displays an `INVALID` status ($\Delta < 80$, $min \ge 0$, or $max \le 0$), the save operation is blocked with an alert.
- **HID Shortcut Label Typo** (`js/ui.js`): Corrected shortcut ID `17` label in the `SHORTCUTS` array from `"BUTTON 10 (21)"` to `"BUTTON 6 (17)"`.
- **Dynamic Metadata in Profile JSON Export** (`js/ui.js`): Updated `exportProfile()` to dynamically fetch the active firmware version from the `#healthFw` DOM element instead of hardcoding a static version string.

### Changed
- **Firmware Version Release Bump** (`release.h`): Incremented firmware release version to `ALPHA 0.0.8`.
- **Watchdog Timeout Window Alignment** (`webHID.cpp`): Adjusted Watchdog timeout from `WDTO_15MS` to `WDTO_60MS` to allow the ATmega32U4 USB endpoint interrupts ample time to flush out the ACK report before hardware reset.

## [vALPHA 0.0.7 / Web Studio v1.6.2] - 2026-08-03 (Interactive Dashboard Button Studio, 32-Button CAD Presets & Falsy Zero Fix)

### Added
- **Interactive Hardware Button Studio (Dashboard Panel)**: Integrated a visual button configuration panel directly into the main Dashboard (`view-landing`) with interactive cards (`btn-card-l`, `btn-card-r`, `btn-card-2`, `btn-card-1`), hardware GPIO pin badges (Pin 0/RX, Pin 5, Pin 1/TX, Pin 7), and live press feedback glow effects.
- **32 Native 3DConnexion HID Shortcuts**: Expanded the shortcut dropdown selection from 15 items to all 32 native 3DConnexion HID button usages (0 to 31), logically grouped into `System`, `View`, `Numbers`, `Modifiers`, and `Control` categories in `js/ui.js`.
- **1-Click CAD Profile Presets**: Added quick CAD button preset profiles (`SolidWorks / Standard CAD`, `Blender 3D Modeling`, `Modifier Keys [Ctrl/Shift/Alt/Esc]`, `4-View Camera Navigation`) with an instant `Apply Preset` handler (`#btnApplyPreset`).
- **Dual-Target Real-Time Button Telemetry**: Updated `parseTelemetryData` in `webhid.js` with a `setButtonUIState()` helper to simultaneously illuminate Debug Studio LEDs (`#key-r`, `#key-l`, etc.) and Dashboard Button Studio cards (`#btn-card-r`, `#btn-card-l`, etc.) upon physical button presses.

### Fixed
- **JavaScript Falsy Zero Parsing Bug**: Resolved a critical data evaluation bug across `js/ui.js` and `js/webhid.js` where `parseInt(val, 10) || fallback` evaluated `0` as falsy, wrongly resetting valid zero values (e.g. min/max calibration inputs, sleep timer, or bitmask options) back to fallback defaults (`-400` / `175`). Created a global `safeParseInt()` helper utilizing explicit `isNaN()` validation to safely preserve zero.
- **Profile Import/Export Zero Preservation**: Updated JSON profile export (`exportProfile`) and import (`importProfile`) routines to safely handle zero inputs across all parameters, sliders, key shortcuts, and dynamic calibration arrays.

### Changed
- **Dashboard Layout Reorganization**: Rebalanced the Dashboard (`view-landing`) layout into an ergonomic 2-column structure:
  - **Left Column**: Interactive 3D Motion (6DOF) Visualizer + Device Health & System Telemetry.
  - **Right Column**: Primary Device Controls + Hardware Button Shortcuts Studio.

## [vALPHA 0.0.7 / Web Studio v1.6.1] - 2026-08-02 (Full 4-Button Customization, Manual Calibration & Protocol Hardening)

### Added
- **Full 4-Button Dynamic EEPROM Customization** (`parameterMenu.h`, `parameterMenu.cpp`, `SpaceMouseHID.cpp`): Extended `ParamStorage` struct to add dynamic shortcut parameters `key2_shortcut` (Back Left / Key 2) and `key1_shortcut` (Back Right / Key 1). All 4 physical hardware push buttons are now 100% customizable dynamically without hardcoded locks.
- **WebHID Device Calibration Query Protocol** (`webHID.h`, `webHID.cpp`, `js/webhid.js`): Introduced dedicated `WEBHID_CMD_GET_CALIBRATION` (`0x07`) command to read active `minVals` and `maxVals` arrays (32 bytes) from EEPROM on connection, populating the Web Studio UI with current hardware boundaries.
- **Interactive Manual Limits Fine-Tuning Area** (`index.html`, `css/style.css`, `js/ui.js`): Integrated an interactive manual calibration table with `input type="number"` fields (`limit-min-${i}`, `limit-max-${i}`) for all 8 Hall Effect sensors under the Debug Studio. Allows real-time manual boundary override and fine-tuning based on live raw signal readings.
- **4-Key UI Control & Telemetry Expansion** (`index.html`, `js/ui.js`, `js/webhid.js`): Added 4 dropdown shortcut selectors (`selKeyL`, `selKeyR`, `selKey2`, `selKey1`) under the "Gates & Drift" panel, and added real-time active status LEDs for all 4 buttons (`#key-r`, `#key-l`, `#key-2`, `#key-1`) in the Debug Studio.
- **Extended JSON Profile Persistence** (`js/ui.js`): Extended JSON profile import and export (`exportProfile` / `importProfile`) to persist `key2_shortcut`, `key1_shortcut`, `minVals`, and `maxVals` arrays locally on PC.

### Fixed
- **WebHID Telemetry Byte Alignment & ROTZ Corruption** (`webHID.cpp`, `js/webhid.js`): Resolved byte collision in `streamWebHIDRawData()` where `keyState[0]` was overwriting `txBuffer[60]` (MSB of ROTZ velocity payload). Compacted all 4 hardware button digital states (`keys[0..3]`) into a 4-bit bitmask stored safely at `txBuffer[61]`.
- **EEPROM USB Blocking Data Drop Guard** (`js/webhid.js`): Fixed a critical USB packet drop during Web Studio saves where consecutive WebHID reports (`0x02` followed immediately by `0x06`) caused the second packet to be lost while MCU was blocked in the `EEPROM.put()` loop (~316ms write time). Added an explicit 400ms async delay (`await new Promise(r => setTimeout(r, 400))`) in `saveConfigToDevice()` before transmitting secondary calibration payloads.
- **Calibration Wizard Auto-Stream Trigger** (`js/ui.js`): Added automatic telemetry stream activation (`toggleStream()`) upon launching the 20-second Limits Wizard to prevent idle test freezes if telemetry streaming was turned off.
- **OLED Horizontal Separator Float Typo** (`oledDisplay.cpp`): Fixed a visual bug in `updateOledDisplay()` where `drawHorizontalLine(6, 0.40)` passed a float literal instead of hex `0x40`, which truncated to 0 and rendered the footer separator line blank.
- **Serial CLI Debug Descriptor Array Bounds** (`ORB7_ALPHA_0_0_8_NEW_DESIGN_BASED.ino`): Fixed an out-of-bounds array access bug in `par.description` table when `ENABLE_SERIAL_DEBUG` is enabled (`1`) by expanding the descriptor array to 34 elements, matching `NUM_PARAMS = 34`.

### Changed
- **Firmware Version & Magic Number Bump** (`release.h`, `parameterMenu.h`, `ORB7_...ino`): Promoted firmware release to `ALPHA 0.0.7`, incremented `NUM_PARAMS` to 34, and updated `MAGIC_NUMBER` to `1234567853L` to enforce clean EEPROM structural re-alignment on first boot.
- **Simplified OLED Option Menu** (`oledDisplay.cpp`): Streamlined OLED Main Options menu down to 4 items: `1. Sensitivity`, `2. Direction`, `3. Display`, `4. Debug`.

### Removed
- **OLED Button Editing Submenu** (`oledDisplay.cpp`): Removed "Buttons" shortcut editing submenu (State 7) and associated PROGMEM string tables from the OLED interface, offloading button shortcut mapping strictly to the Web Studio and reclaiming ~200 bytes of Flash memory.

## [vALPHA 0.0.6 / Web Studio v1.6] - 2026-08-02 (Metrological Precision, Hardware Redesign & WebHID Studio)

### Added
- **Device Health & Telemetry Dashboard Panel** (`view-landing`, `index.html`, `js/webhid.js`, `js/ui.js`): Introduced a real-time diagnostic panel on the Dashboard landing page displaying Firmware Release Version (`ALPHA 0.0.6`), EEPROM XOR Checksum Status (`VALID`), ADC VREF (`2.56V Internal`), Live Magnetic Balance (`BALANCED` vs `ALIGNMENT NEEDED`), USB Connection State (`ONLINE`/`OFFLINE`), and Live Hz Refresh Rate.
- **JSON Profile Import & Export System** (`index.html`, `css/style.css`, `js/ui.js`): Added 1-click Export JSON and Import JSON navbar buttons. Allows saving full mouse configuration profiles (`.json`) locally on PC and restoring them into the Web Studio for instant flashing (`SAVE TO MOUSE`).
- **Modular Web Studio Architecture**: Separated monolithic Web Studio into a clean modular structure (`index.html`, `css/style.css`, `js/webhid.js`, `js/viewport3d.js`, `js/ui.js`) for rapid maintenance and targeted chat reviews.
- **WebHID Secondary Calibration Payload** (`webHID.h`, `webHID.cpp`, `js/webhid.js`): Introduced dedicated `WEBHID_CMD_SET_CALIBRATION` (`0x06`) command to transmit `minVals` and `maxVals` (32 bytes) in a secondary payload. Bypasses the 64-byte USB Interrupt endpoint limitation and enables the Web Studio Calibration Wizard to persist measured boundaries to EEPROM.
- **Expanded 4-Key Telemetry Streaming** (`webHID.cpp`): Updated `streamWebHIDRawData()` to stream real-time digital states for all 4 physical hardware buttons (`keyState[0..3]`) at bytes 60..63 in the 64-byte WebHID payload.

### Fixed
- **Boot Zeroing Sanity Bounds** (`calibration.cpp`): Adjusted `CENTERPOINTWARNINGMIN` (100 LSB) and `CENTERPOINTWARNINGMAX` (1000 LSB) for Hall Effect sensors operating on 2.56V Internal Reference. Eliminates false-positive rest warnings and resolves the 5.5-second boot delay / calibration bypass lockout.
- **Zeroing Noise Gate Tolerance** (`calibration.cpp`): Increased `DEADZONEWARNING` from 10 LSB to 25 LSB to absorb peak-to-peak ADC noise over 1000 oversampled reads during rest zeroing.
- **Kill-Keys USB HID Masking** (`SpaceMouseHID.cpp`): Added `#if (NUMKILLKEYS > 0)` check inside `prepareKeyBytes()` to mask `KILLROT` and `KILLTRANS` key indices from setting HID report bits (Report ID 3). Prevents unwanted HID button presses (`SM_1`/`SM_2`) in CAD applications when holding Kill Keys.
- **WebHID modFunc Boundary Sanitization** (`webHID.cpp`): Added boundary check (`if (par.values->modFunc != 0 && par.values->modFunc != 1 && par.values->modFunc != 3) par.values->modFunc = MODFUNC;`) in `processWebHIDPacket` for `WEBHID_CMD_SET_CONFIG` (`0x02`) to safeguard against corrupt curve functions from browser packets.
- **Full Physical Travel Matrix Restoration** (`kinematics.cpp`): Corrected Hall Effect matrix divisors in `_calculateKinematicSensors()` from `/ 2` to `/ 4` (for 4-sensor sums: TRANSX, TRANSY, ROTX, ROTY) and from `/ 4` to `/ 8` (for 8-sensor sums: TRANSZ, ROTZ). Eliminates premature 50% hardware travel deadband clipping and restores full physical stroke resolution.
- **Z-Axis Gate Decoupling** (`kinematics.cpp`): Refactored `calculateKinematic()` to apply `par.values->gate_neg_transZ` strictly when `velocity[TRANSZ] < 0` (pull-up / Zoom Out). Prevents light push-down movements (+Z / Zoom In) from being accidentally muted by the pull-up noise gate.
- **Dynamic MicroGate Bridge** (`kinematics.cpp`): Replaced hardcoded `GATE_TRANS` macro in `processAxis()` calls with live `par.values->gate_trans` EEPROM parameter, restoring real-time Pan X/Y mute slider control from the Web Studio.
- **USB HID Stack Data Leak** (`webHID.cpp`): Zero-initialized `txBuffer[64] = {0}` in `processWebHIDPacket()` and `streamWebHIDRawData()`, eliminating stack memory garbage data leaks over USB HID.
- **Deadzone Boundary Inclusivity** (`kinematics.cpp`): Updated deadzone condition in `FilterAnalogReadOuts()` to `<= dz` and `>= -dz`, ensuring signals resting exactly on boundary edges are properly muted without wasting limit-mapping CPU cycles.
- **Secondary Hardware Buttons Array Indexing** (`SpaceMouseHID.cpp`): Fixed button array indexing in `prepareKeyBytes()` by applying an `i - 2` offset to `defaultButtonList`, accurately mapping physical keys `keys[2]` and `keys[3]` to static `BUTTONLIST` assignments.
- **OLED Submenu Cursor Glitch** (`oledDisplay.cpp`): Added explicit `submenuSelect = 0;` resets across all OLED menu state transitions and parent return actions, preventing out-of-bounds cursor glitches when navigating between submenus of different lengths.

### Changed
- **ORB7 Hardware Physical Layout & Mapping** (`config.h`, `SpaceMouseHID.cpp`): Redesigned pin assignment `KEYLIST {5, 0, 1, 7}` for the new ORB7 physical chassis:
  - Front Right (Key R / `keys[0]`): Pin 5 (Customizable via OLED / Web Studio).
  - Front Left (Key L / `keys[1]`): Pin 0 / RX (Customizable via OLED / Web Studio).
  - Back Left (Key 2 / `keys[2]`): Pin 1 / TX (Mapped to CAD Button SM_2).
  - Back Right (Key 1 / `keys[3]`): Pin 7 (Mapped to CAD Button SM_1).
  - Updated `BUTTONLIST` in `config.h` to `{SM_2, SM_1}`.
- **Web Studio Tab & Panel Restructuring** (`index.html`): Renamed "Advanced & Drift" tab to "Gates & Drift". Relocated "Noise Gates" panel to top of "Gates & Drift", and moved "Dynamic Limits Calibration Wizard" to the "Debug Studio" tab.
- **ADC Prescaler Optimization** (`config.h`): Adjusted `ADC_PRESCALER_PRESET` from `0x05` (500 kHz) to `0x06` (250 kHz), guaranteeing clean 10-bit analog resolution on ATmega32U4 without ADC clock overclock noise.
- **Symmetric EMA Filter Decay** (`kinematics.cpp`): Replaced bitshift `diff >> ADC_EMA_SHIFT` in `readAllFromJoystick()` with integer division `diff / (1 << ADC_EMA_SHIFT)`, fixing asymmetric decay when sensors return to rest.
- **JavaScript Radix Safety** (`js/ui.js`, `js/webhid.js`): Added explicit radix `10` to all `parseInt()` calls across the Web Studio scripts for strict type safety.

### Removed
- **Orphan HID Code** (`SpaceMouseHID.h`, `SpaceMouseHID.cpp`): Removed unused `jiggleValues()` function, saving Flash memory.
- **Ghost EEPROM Parameter** (`parameterMenu.h`, `config.h`, `ORB7_...ino`): Removed unused `prioZexclusiveMode` field from `ParamStorage`, updated `MAGIC_NUMBER` to `1234567852L`, decremented `NUM_PARAMS` to 32, and removed obsolete Z-priority branching from `_calculateKinematicSensors()`.

### Optimized
- **PROGMEM Literal Deduplication** (`oledDisplay.cpp`): Unified repeated string literals (`msg_keep_rest`, `msg_calibrating`, `msg_rezero`, `msg_success`, `msg_warning`, `msg_toggle_footer`) into PROGMEM pointers, reclaiming ~90 bytes of Flash memory.
- **State 9 Delta Calculation Optimization** (`oledDisplay.cpp`): Refactored `printSensorLine()` to return the calculated delta value directly, eliminating 4 redundant `abs()` calls per frame in the OLED alignment view.
- **Axis Inversion Memory Footprint** (`kinematics.cpp`): Compacted 6 individual inversion `if` branches into a pointer loop over `invX`..`invRZ`, reducing Flash usage by ~20 bytes.
- **OLED Sleep Timeout Calculation** (`oledDisplay.cpp`): Replaced 32-bit multiplication formula in `updateOledDisplay()` with a static lookup array `sleep_ms[4]`, preventing potential underflow risks.

## [vALPHA 0.0.5] - 2026-07-28

### Added
- **Dedicated Translation MicroGate** (`config.h`, `parameterMenu.h`, `kinematics.cpp`): Introduced a dedicated low-level noise gate (`#define GATE_TRANS 6`, range 3..10) applied to Translation X (TX) and Translation Y (TY) in `calculateKinematic()`. Filters out post-matrix velocity bleed (1..5) caused by mechanical 3D-printed gimbal spring asymmetries without introducing initial resistance.
- **Translation Gate Decoupling** (`kinematics.cpp`): Uncoupled Translation X and Translation Y from `gate_neg_transZ` (`GATE_NTZ`), preventing high Zoom-Out noise gate settings on the Web Studio from inadvertently choking/muting Pan X and Pan Y movements below threshold.
- **MicroGate EEPROM Persistence & Sanitization** (`parameterMenu.h`, `parameterMenu.cpp`, `webHID.cpp`): Added `gate_trans` field to `ParamStorage` struct, incremented `NUM_PARAMS` to 33, updated `MAGIC_NUMBER` (`1234567851L`), and added boundary cleansing (0..100) to guarantee full EEPROM persistence across reboots.

### Fixed
- **Web Studio MicroGate Control** (`ORB 7 Dashboard v1.1.html`): Integrated dedicated MicroGate (Pan X/Y Mute) range slider (0..50) under the Noise Gates panel on the Web Studio, paired with an informative Guide Box detailing cross-coupling mitigation.
- **Full 33-Parameter WebHID Synchronization** (`ORB 7 Dashboard v1.1.html`): Expanded browser `DataView` serialization to read, decode, and write all 33 `ParamStorage` EEPROM fields (up to DataView offset 58), perfectly aligned 1:1 with C++ memory layout.
- **Unmapped Physical Centered Deltas in WebHID Telemetry** (`webHID.cpp`): Fixed a metrological telemetry bug where `FilterAnalogReadOuts()` overwritten `centered[8]` with mapped values (-350..+350), blinding the Web Calibration Wizard. Refactored `streamWebHIDRawData()` to compute and stream unmapped physical deltas (`Raw - Center + Offset`) in stack memory.
- **3D Viewport Puck Target Kinematic Alignment** (`ORB 7 Dashboard v1.1.html`): Corrected target vector sign for Translation X (`targetPos.x = (vels[0] / 350.0) * 3.0;`), ensuring physical knob movement to the right translates the 3D Puck Mesh to the right (+X).
- **Massive Flash Footprint Reduction** (`oledDisplay.cpp`): Simplified the OLED "Drift Offsets" screen (`menuState == 12`) to a clean, high-impact `STATE: ON` / `STATE: OFF` toggle display. Reclaimed ~140 bytes of Flash, expanding headroom to >160 bytes below the 28,672-byte Caterina bootloader limit.
- **OLED Display Menu Lock Guard** (`oledDisplay.cpp`): Added safety guard at entry of `processMenuInput()` (`if (par.values->oledSleepTimer < 0) return;`) to block hardware menu state transitions when OLED display is powered OFF via software.
- **OLED Submenu 5 Edit Isolation** (`oledDisplay.cpp`): Fixed a UI editing bug in State 5 ("DISPLAY") where editing Option 1 ("POWER") altered `oledSleepTimer` (Option 0). Isolated sleep timer adjustments strictly to `submenuSelect == 0`.
- **OLED Software Sleep Boot Persistence** (`parameterMenu.cpp`): Fixed a boot reset bug where `oledSleepTimer = -1` (OLED disabled via Web Studio) was incorrectly reset to 2 on boot. Relaxed boundary check to `par.values->oledSleepTimer < -1`.

## [vALPHA 0.0.4] - 2026-07-15

### Added
- **EEPROM Calibration Corruption Safeguard** (`webHID.cpp`): Restricted `memcpy` in `WEBHID_CMD_SET_CONFIG` to `offsetof(ParamStorage, minVals)` (56 bytes), guaranteeing that EEPROM calibration arrays remain untouched during browser parameter saves.
- **Telemetry Stream Pipeline Synchronization** (`ORB7_ALPHA_0_0_4.ino`): Relocated `streamWebHIDRawData()` to the end of the primary kinematic pipeline in `loop()`, ensuring 0-frame latency.
- **Runtime Boundary Sanitization** (`webHID.cpp`): Integrated mandatory boundary checks on Q7 sensitivities, `globalSens`, `deadzone`, and `compNoOfPoints` directly inside the WebHID command processor.
- **Smart "SAVE TO MOUSE" State Machine**: Re-engineered the EEPROM save workflow into a top navbar button featuring dynamic dirty/clean state tracking.

## [vALPHA 0.0.3] - 2026-06-30

### Added
- **Native 2-Way WebHID Protocol Subsystem** (`webHID.h` / `webHID.cpp`): Enabled driverless, browser-native 2-way WebHID integration across Chrome, Edge, and Brave.
- **Top-Level Collection (TLC) Descriptor Architecture**: Placed WebHID Vendor Defined Report ID 5 in a separate TLC, bypassing OS/browser security blocks on standard mouse/keyboard report collections.
- **Single-Page Web Configurator** (`index.html`): Launched modern dark-themed dashboard featuring real-time 3D motion, modifier curve plotting, and sensor alignment diagnostics.

## [vALPHA 0.0.2] - 2026-05-15

### Added
- **EEPROM Boundary Cleansing**: Added automated firewall in `getParametersFromEEPROM()` that validates and auto-corrects corrupted runtime parameters before they enter the kinematic loop.
- **Compile-Time Assertions**: Integrated compile-time hardware validation via `calibrationChecks.h`.
- **Universal Scalable Input Support**: Redesigned hardware button processing to seamlessly support anywhere from 0 to 32 physical push-buttons natively through the HID descriptor.

## [vALPHA 0.0.1] - 2026-04-01

### Added
- **Fixed-Point Kinematics Engine**: Replaced floating-point math with Q7 (sensitivities) and Q8 (slopes) fixed-point integer math.
- **Native SSD1306 RAM-less OLED Driver**: Integrated direct-write OLED interface using `SSD1306AsciiWire`.
- **Decoupled Thermal Drift Engine**: Implemented adaptive per-sensor thermal drift tracking.
- **125Hz USB HID Polling Rate**: Doubled USB update rate from 16ms to 8ms.