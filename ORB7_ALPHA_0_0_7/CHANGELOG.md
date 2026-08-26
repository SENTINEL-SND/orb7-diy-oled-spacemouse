# Changelog

All notable changes to the SpaceMouse Pro Emulator (O.R.B.7) project will be documented in this file.

## [Web Studio v1.8.0] - 2026-08-26 (Hardware Test & WebHID Reliability)

### Added
- **Guided Hardware Test (`ORB7_Studio`):** Added staged Connection, Sensors, 6DOF Movement, Buttons, Safe Commands, and Final Report validation with JSON export.
- **Hardware Test state reporting:** Added explicit command-test tracking, separate automatic telemetry tracking, retry/continue handling, and preservation of the original Stream state.

### Removed
- **Dynamic Drift Compensation controls and state:** Removed the legacy adaptive-center/drift workflow from the Studio surface. Sensor diagnostics now describe deviation from the fixed center acquired during zeroing.
- **Legacy EMA and sensor-noise filter controls:** Removed compatibility for the firmware's former stateful EMA, sensor-level deadzone, and redundant fixed noise filters, which no longer exist in firmware `v0.0.7`.
- **Separate legacy gate stages:** Removed the old direct-root translation microgate and post-curve gate model from the Studio configuration surface. Noise suppression is now represented by the unified global deadzone and six independent post-matrix base gates.

### Changed
- **WebHID timeout handling:** Added bounded send waits for Stream, configuration, calibration, SET_CONFIG, Factory Reset, and Restart commands. Unknown device state now requires reconnection instead of being treated as confirmed.
- **Synchronization safeguards:** Late or unsolicited configuration/calibration responses are ignored; uncertain data synchronization blocks new Hardware Tests until reconnection.
- **Hardware Test cancellation:** Cancellation now waits for pending commands, prevents concurrent Stream restoration, preserves completed reports after disconnect, and keeps controls locked until restoration finishes.
- **Stream and Re-Zero reporting:** Stream results require the explicit Stream test; Re-Zero responses are reported as observed but not attributable because the protocol has no request ID.
- **Capture and interaction reliability:** Sensor recapture clears stale values, missing telemetry is reported as `NO DATA`/`NO TELEMETRY`, button results are finalized against the latest state, movement retries lock Continue during capture, and command waits pause while the tab is hidden.
- **Studio version:** Updated the Web Studio version metadata and exported profile/report version to `1.8.0`.

### Fixed
- **Connection and firmware state cleanup:** Firmware version and uncertainty flags are cleared on disconnect and restored after a new configuration read.
- **Hardware Test controls:** Corrected sensor-capture reset state, Stream restoration failures, report finalization concurrency, and stale command/telemetry flags in exported metrics.

### Protocol Limitations
- The existing protocol has no request identifier or dedicated Stream-stop acknowledgment. A received Re-Zero response cannot be attributed to a specific request, and a timed-out command can only be considered safe after reconnection.

## [v0.0.7] - 2026-08-25 (Deterministic Sensor Pipeline)

### Removed
- **Dynamic drift compensation:** Removed adaptive center tracking and its persistent parameters. Sensor deltas now use the fixed center acquired during zeroing.
- **Integer EMA filter:** Removed the stateful exponential moving average from ADC acquisition. The firmware now forwards the configured arithmetic oversampling average directly to the fixed-center pipeline.
- **Redundant noise filters:** Removed the sensor-level deadzone and fixed post-curve gate. Noise suppression is now a single post-matrix layer with one configurable gate for each of the six output axes.
- **Removal rationale:** Although the filters introduced in `v0.0.6` partially addressed problems inherited from legacy configurations, the mouse had become unnecessarily complex, with too many processing layers. Reducing the filter stack exposed thermal drift as a common denominator that, in this implementation, created more problems than it solved. Removing thermal drift compensation and refactoring the pipeline eliminated layers of complexity that had accumulated while pursuing maximum precision. The removal of dynamic drift compensation, the integer EMA filter, and the redundant noise filters therefore simplified the code and delivered better overall mouse performance.

### Added
- **Unified user deadzone:** Added a 0..100% comfort control that proportionally raises the six axis-specific base gates in the existing post-matrix layer. Values outside the threshold are continuously rescaled from zero to avoid an activation jump, while sensitivity and MOD curve mathematics remain unchanged.

### Changed
- **EEPROM layout revision:** Added the persistent deadzone level immediately before calibration limits and advanced the EEPROM magic number. The first boot on this build restores safe defaults; dynamic limits must then be recalibrated.

## [v0.0.6 / Web Studio v1.6.5] - 2026-08-07 (The Absolute Silence, Precision & OLED Layout Patch)

### Fixed & Consolidated
- **Exclusive Mode "Neutral Leak" Resolved (`kinematics.cpp`):** Eliminated cross-coupling mechanical bleed (e.g., micro-translations occurring when starting a rotation). The `exclusiveMode` state machine now strictly suppresses all 6DOF payloads to the USB host during State `0` (dominance evaluation) until the physical gesture deliberately breaks the minimum force threshold (`15`).
- **Post-Curve Micro-Filter (`kinematics.cpp`):** Introduced a secondary tail-end suppression gate (`< 5` LSB) within `processAxis()`. This intercepts and annihilates fractional mathematical noise generated by Q8 modifier curve conversions, restoring rock-solid absolute zero values at rest.
- **Factory Reset Screen Layout & Padding (`oledDisplay.cpp`):** Completely reworked State 13 UI. Centered all message strings (`"Reset EEPROM?"` at `X=25`, `"All custom settings"` at `X=7`, `"will be erased!"` at `X=19`), consolidated the footer onto Line 7 (`"HL: BACK   HR: RESET"`), and added instant short-press Left Button back navigation.
- **OLED Re-Zero Text Centering (`oledDisplay.cpp`):** Mathematically re-centered the instant `RE-ZEROED!` success message (`X=34`) with exact leading/trailing spaces (`"  RE-ZEROED!    "` at `X=22`) to perfectly overwrite `"Calibrating..."` without visual misalignment.
- **OLED Visual Ghosting Artifacts Eliminated (`oledDisplay.cpp`):**
  - Corrected Sensor Alignment (State 9) status strings (`" OK"` vs `" ! "`) to eliminate the trailing `!K` visual ghosting bug.
  - Equalized Drift Compensation toggle strings (`" ON  "` vs `" OFF "`) to prevent partial pixel overwrites.
  - Shortened `msg_toggle_footer` to `"HR: TOGGLE  HL: BACK"` for clean 0-margin alignment across submenus.
- **OLED Code Cleanup & Stack Optimization (`oledDisplay.cpp`):**
  - Purged dead legacy functions (`printSignedVal`, `printOffsetLine`) and unused static variables (`lastBarWidth`).
  - Eliminated `char` stack buffer allocations (`buffer[18]`, `labelBuf[8]`) in PROGMEM menu rendering, streaming strings directly via `__FlashStringHelper*`.
  - Simplified `printPaddedVal()` by removing redundant `abs()` calls on positive-only ADC raw values.

---

## [v0.0.5 / Web Studio v1.6.5] - 2026-08-07 (Consolidated 6DOF Kinematic Baseline, Direct Root MicroGate, Dual Diagnostics & Web Studio v1.6.5)

### Added & Consolidated
- **Direct Root MicroGate Engine (`kinematics.cpp`, `config.h`, `parameterMenu.h`, `webHID.cpp`, `index.html`):** Integrated a dedicated pre-sensitivity noise gate (`gate_trans`, range 0..50) applied directly to Translation X (Pan Left/Right) and Translation Y (Pan Up/Down) prior to Q7 sensitivity division and curve processing. Intercepts and annihilates raw matrix parasitic bleed (1..15 LSB) caused by TPU spring asymmetries at the physical root before Q7 amplification.
- **Pure 6DOF Direct Kinematic Engine (`kinematics.cpp`):** Unified 1:1 tactile 6DOF vector space translations utilizing ultra-fast Q7 fixed-point sensitivities, Q8 slope caching (`MODFUNC 3` Squared Tangent), and hard cutoff noise gates. Delivers crisp, zero-latency CAD viewport navigation in SolidWorks, Fusion 360, Inventor, FreeCAD, and Blender.
- **Full 4-Button Hardware Customization (`SpaceMouseHID.cpp`, `parameterMenu.h`, `index.html`, `js/ui.js`):** Enabled 100% dynamic EEPROM assignment for all 4 physical hardware buttons (Front Right [Pin 5], Front Left [Pin 0/RX], Back Left [Pin 1/TX], Back Right [Pin 7]). Includes 32 native 3DConnexion HID shortcuts and 1-click CAD preset profiles (SolidWorks, Blender, Modifiers, 4-View Camera).
- **Dual Validation Magnetic Diagnostics (`oledDisplay.cpp`, `webHID.cpp`, `index.html`, `js/webhid.js`):** Integrated resting signal range validation ($450 \sim 650$ LSB) and 3-tier differential delta checks ($\Delta \le 30$ PERFECT, $\le 60$ BALANCED, $> 60$ ALIGNMENT NEEDED) across N/S/E/W Hall Effect sensor pairs. Detects missing/far magnets ($>800$ LSB) or inverted/close magnets ($<250$ LSB) with explicit OLED and Web Studio alerts.
- **Hardware Software Reboot Command (`webHID.h`, `webHID.cpp`, `ORB7_ALPHA_0_1_2.ino`):** Added remote MCU restart capabilities (`WEBHID_CMD_RESTART` / `0x08`) via AVR Watchdog Timer (`wdt_enable(WDTO_60MS)`), paired with clean boot-time flag clearing (`MCUSR = 0; wdt_disable();`) in `setup()`.
- **Web Studio v1.6.5 Suite (`index.html`, `css/style.css`, `js/webhid.js`, `js/ui.js`, `js/viewport3d.js`):** Cleaned single-page web configurator featuring a real-time Three.js 3D motion viewport, Chart.js modifier curve plotter, 100 Hz USB telemetry stream, 20-second dynamic calibration wizard, manual limit table, JSON profile import/export, and electric cyan UI styling (`#05ACFF`).
- **RAM-less SSD1306 OLED UI (`oledDisplay.cpp`):** Direct-write display driver operating without SRAM buffer overhead, featuring real-time 6DOF bar graph visualizer, instant Re-Zero progress messaging (`Calibrating...` -> `RE-ZEROED!`), and configurable auto-sleep timers.
- **EEPROM Memory Protection & Struct Packing (`parameterMenu.h`, `parameterMenu.cpp`):** Applied `__attribute__((packed))` to `ParamStorage` (37 parameters), aligning C++ EEPROM memory layout 1:1 with JavaScript DataView byte offsets. Includes 8-bit XOR checksum verification and boundary sanitization firewall.

---

## [v0.0.4] - 2026-07-15 (The 3D Studio, WebHID Integration & Memory Optimization Build)

### WebHID Studio & Protocol
- **EEPROM Calibration Protection (`webHID.cpp`):** Restricted configuration payload copying to `offsetof(ParamStorage, minVals)` (59 bytes), guaranteeing that dynamic calibration arrays remain untouched during browser parameter saves.
- **Telemetry Stream Synchronization (`ORB7_ALPHA_0_1_2.ino`):** Synchronized telemetry streaming at the end of the primary kinematic pipeline for 0-frame latency.
- **Full 32-Parameter WebHID Synchronization:** Expanded DataView serialization across all `ParamStorage` EEPROM fields.
- **Smart "SAVE TO MOUSE" State Machine:** Re-engineered EEPROM save workflow into a top navbar button with dynamic dirty/clean state tracking.

### Flash Memory & OLED
- **Flash Footprint Reduction (`oledDisplay.cpp`):** Reclaimed ~200 bytes of Flash memory by eliminating redundant boot progress animations and consolidating calibration display helpers (`showCalMsg()`).
- **Fast 16-Bit Q8 Math (`oledDisplay.cpp`):** Refactored `printQ8Fixed()` to use fast 16-bit bitwise shifting, removing uint32_t division libraries.

### 3D Motion Viewport & Visualizers
- **Interactive 3D Motion Viewport (Three.js):** Integrated real-time 3D puck renderer with glossy ceramic body, electric cyan accent ring, and perimeter knurling ribs.
- **Interactive Modifier Curves Visualizer (Chart.js):** Real-time mathematical plotter rendering Dataset 1 ("Squared") and Dataset 3 ("Squared Tangent") curves.

---

## [v0.0.3] - 2026-06-30 (The WebHID & Memory Optimization Build)

### WebHID & Browser Integration
- **Native 2-Way WebHID Subsystem (`webHID.h` / `webHID.cpp`):** Enabled driverless browser configuration, real-time telemetry streaming, and remote calibration.
- **Independent Top-Level Collection (TLC):** Restructured USB HID report descriptor to place Report ID 5 (Vendor Defined) in a separate TLC, bypassing OS/browser security blocks.
- **Unified 64-Byte Payload:** Bundled raw sensor reads, centered deltas, drift offsets, 6DOF velocities, and button states into a single USB packet.

---

## [v0.0.2] - 2026-05-15 (The Stability & Scalability Build)

### System Architecture & Memory Protection
- **EEPROM Boundary Cleansing:** Automated firewall in `getParametersFromEEPROM()` validating and auto-correcting corrupted parameters.
- **Compile-Time Assertions (`calibrationChecks.h`):** Integrated compile-time validation for `ADC_OVERSAMPLES`, `ADC_EMA_SHIFT`, `EXCL_RELAX_THRESHOLD`, `COMP_NR`, and `COMP_WAIT`.
- **Universal Scalable Input Support:** Redesigned button processing to support anywhere from 0 to 32 physical push-buttons natively through the HID descriptor.

---

## [v0.0.1] - 2026-04-01 (The OLED & Performance Build)

### Display & Kinematics
- **Native SSD1306 OLED Driver (`oledDisplay.cpp`):** Direct-write RAM-less OLED driver eliminating the 1KB SRAM frame buffer.
- **Fixed-Point Kinematic Engine (`kinematics.cpp`):** Converted 6DOF sensitivity calculations from floats to Q7 (`SENS_TX_Q7`) and Q8 (`SLOPE_A_Q8`, `SLOPE_B_Q8`) fixed-point integer math.
- **125Hz USB Polling Rate:** Doubled USB update rate to 8ms polling intervals (`HIDUPDATERATE_MS`).
- **Decoupled Drift Compensation (`calibration.cpp`):** Independent per-sensor thermal drift tracking and adaptive muting.
- **EEPROM XOR Integrity Checksum:** 8-bit XOR checksum verification protecting persistent parameter storage.
