# Changelog

## Release Notes - Firmware vALPHA 0.0.4 (The 3D Studio, WebHID Integration & Memory Optimization Build)

This major feature and stabilization update elevates the web configurator to a full 3D interactive studio, expands WebHID bi-directional mapping across all 32 MCU EEPROM parameters, resolves critical memory corruption bugs during WebHID serialization, optimizes Flash footprint for strict ATmega32U4 constraints, and implements synchronized multi-channel telemetry.

### WebHID Studio & Protocol Stabilization

* **[Fixed] EEPROM Calibration Corruption Safeguard (`webHID.cpp`):** Fixed a critical memory corruption bug where WebHID configuration payloads (`WEBHID_CMD_SET_CONFIG`) overwrote `minVals[0..3]` calibration bounds with zeroes. Restricted `memcpy` to `offsetof(ParamStorage, minVals)` (56 bytes), guaranteeing that EEPROM calibration arrays remain untouched during browser parameter saves.
* **[Fixed] Telemetry Stream Pipeline Synchronization (`ORB7_ALPHA_0_0_4.ino`):** Relocated `streamWebHIDRawData()` to the end of the primary kinematic pipeline in `loop()`. Ensures `rawReads`, `centered`, `offsets`, and `velocity` in WebHID packets belong to the exact same frame with 0-frame latency.
* **[Added] Runtime Boundary Sanitization (`webHID.cpp`):** Integrated mandatory boundary checks on Q7 sensitivities, `globalSens`, `deadzone`, and `compNoOfPoints` directly inside the WebHID command processor to prevent division-by-zero exceptions in `kinematics.cpp` and `calibration.cpp`.
* **[Added] Full 32-Parameter WebHID Synchronization:** Expanded browser DataView serialization to read, decode, and write all 32 `ParamStorage` EEPROM fields, including per-axis Q7 fixed-point sensitivities, Q8 curve slopes, noise gates, inversions, swaps, exclusive hysteresis, drift thresholds, OLED sleep timers, and button shortcuts.
* **[Added] Smart "SAVE TO MOUSE" State Machine:** Re-engineered the EEPROM save workflow into a top navbar button (`SAVE TO MOUSE`) featuring dynamic dirty/clean state tracking. The button remains greyed-out and disabled until a parameter is modified, turning active neon cyan, sending WebHID payloads on click, providing temporary "SAVED!" feedback, and auto-disabling.

### Flash Memory & OLED Optimization

* **[Improved] Flash Footprint Reduction (`oledDisplay.cpp`):** Eliminated the heavy 54-iteration boot splash progress bar animation loop, replacing it with a clean static display pause (`delay(800)`).
* **[Improved] Fast 16-Bit Q8 Math (`oledDisplay.cpp`):** Refactored `printQ8Fixed()` to utilize fast 16-bit bitwise shifting and 8-bit integer math, eliminating 32-bit uint32_t arithmetic and software division library dependencies (`/ 100`, `% 100`).
* **[Improved] Unified Calibration Display Helpers (`oledDisplay.cpp`):** Compacted `showCalibrationScreen()`, `showCalibrationWarningScreen()`, and `showCalibrationFailedScreen()` into a unified parametrized helper function (`showCalMsg()`), reducing function prolog/epilog overhead.
* **[Result] Memory Budget Compliance:** Reclaimed ~200 bytes of Flash memory, lowering overall firmware binary size to ~28,520 bytes and securing a safe ~150-byte headroom below the 28,672-byte ATmega32U4 Caterina bootloader limit.

### 3D Motion Viewport & Visualizers

* **[Added] Interactive 3D Motion Viewport (Three.js):** Integrated a real-time 3D puck/knob renderer featuring a ceramic glossy white body, neon cyan front indicator notch, and 8 perimeter knurling ribs for clear 360-degree rotation tracking.
* **[Fixed] Corrected 3D Camera Kinematic Alignment:** Inverted 3D target vectors ($TX, TY, RX, RY, RZ$) in JavaScript to ensure 3D viewport translation and rotation mirror physical hand movements with zero spatial inversion.
* **[Added] Interactive Modifier Curves Visualizer (Chart.js):** Integrated a real-time mathematical plotter rendering Dataset 1 ("Squared") and Dataset 3 ("Squared Tangent") curves simultaneously from $x=0$ to $350$. Updates instantly without lag on slider or dropdown changes.
* **[Fixed] Decimal Slider Formatting:** Resolved a floating-point rounding bug in `bindSlider()` by enforcing explicit `.toFixed(2)` decimal formatting for float parameters (`slSlopeA`, `slSlopeB`, and Q7 sensitivities), preventing step rounding to integers ($1.00, 2.00$).

### Calibration & Diagnostics

* **[Added] Browser-Side Dynamic Limits Calibration Wizard:** Integrated a 20-second boundary test with visual progress bar, real-time $33\text{Hz}$ telemetry min/max tracking across all 8 Hall effect sensors, and an automated post-test reference table displaying measured Minimums, Maximums, Dynamic Range ($\Delta$), and sanity status badges (**OK** / **LOW MOVEMENT**).
* **[Improved] Wizard Boundary Persistence Workflow:** Updated `ORB 7 Dashboard v0.0.4.html` to mark configuration dirty upon successful wizard completion, enabling users to commit updated boundaries directly to MCU EEPROM.

---

## Release Notes - Firmware vALPHA 0.0.3 (The WebHID & Memory Optimization Build)

This milestone update introduces driverless, browser-native 2-way WebHID integration, restructures the USB HID descriptor into dual Top-Level Collections, and executes deep AVR assembly and UI optimizations to fit a full-featured web configurator within the strict 28,672-byte Flash limit of the ATmega32U4.

### WebHID & Browser Integration

* **[Added] Native 2-Way WebHID Protocol Subsystem (`webHID.h` / `webHID.cpp`):** Enables browser-based (Chrome, Edge, Brave) configuration, real-time telemetry streaming, and calibration without installing third-party drivers or background services.
* **[Added] Command Protocol Engine:** Supports remote configuration fetching (`0x01`), instant EEPROM parameter updates with boundary sanitization (`0x02`), 33Hz raw sensor telemetry streaming (`0x03`), remote Re-Zero baseline calibration (`0x04`), and remote Factory Reset (`0x05`).
* **[Added] Unified 64-Byte Telemetry Payload:** Bundles raw sensor reads, centered deltas, thermal drift offsets, calculated 6DOF velocities, and physical button states into a single USB packet.
* **[Added] Single-Page Web Configurator (`index.html`):** Modern dark-themed dashboard featuring a dual-tab architecture (Dashboard for daily controls and 6DOF live bars; Debug Studio for sensor pair alignment, drift tables, Re-Zero, and Factory Reset) with automatic USB connection handshaking and stream auto-start.

### USB HID Subsystem & Descriptor Restructuring

* **[Added] Independent Top-Level Collection (TLC):** Restructured `SpaceMouseReportDescriptor` in `SpaceMouseHID.h` to place Report ID 5 (Usage Page `0xFF00`, Vendor Defined) in a separate TLC, bypassing OS/browser security blocks on standard mouse/keyboard report collections.
* **[Improved] Consolidated USB RX Dispatcher (`receiveHostData()`):** Unified incoming USB packet handling to dynamically route requests to host LED indicators (Report ID 4) or WebHID payloads (Report ID 5).
* **[Fixed] Fixed-Length Report Transmissions:** Enforced strict 64-byte fixed-length report transmissions for Report ID 5, preventing OS HID parsers (Windows/macOS) from silently discarding mismatched input reports.

### Display & User Interface (OLED)

* **[Fixed] Sensor Alignment Auto-Sleep:** Resolved an auto-sleep timeout issue on the "Align Sensors" screen (State 9) by continuously renewing the activity timestamp (`lastActivityTime = now`) during physical sensor/magnet alignment, preventing premature exit to the home screen.
* **[Improved] Refactored `oledDisplay.cpp`:** Flash-optimized helper functions (`printAt()` and `printMenuLabel()`) to eliminate duplicated cursor positioning and string printing instructions.
* **[Improved] Compacted Home Screen Rendering:** Compacted 6DOF axis label rendering on the home screen into a lightweight for loop, replacing static string literal arrays to reclaim Flash memory.

### Kinematics & Memory Optimization

* **[Improved] Refactored Kinematics (`kinematics.cpp`):** Introduced `__attribute__((noinline)) static void processAxis()`, eliminating 6 duplicated code blocks of sensitivity scaling, curve modifier application, and noise gating while keeping floating-point math curves (`powf()`, `tanf()`) 100% intact.
* **[Improved] Consolidated WebHID Dispatch:** Consolidated response dispatch logic in `processWebHIDPacket()` via a single unified `SendReport()` path, saving ~200 bytes of Flash.
* **[Improved] Stack Memory Cleanups:** Eliminated unnecessary local stack array zero-initializations (`= {0}`) across USB RX/TX buffers, removing redundant `memset()` compilation overhead.

---

## Release Notes - Firmware vALPHA 0.0.2 (The Stability & Scalability Build)

This major maintenance update elevates the firmware to a commercial-grade stability level. It introduces universal scalable inputs (up to 32 buttons), strict compile-time assertions, and comprehensive runtime sanitization to guarantee absolute immunity against memory corruption, arithmetic overflows, and zero-division exceptions.

### System Architecture & Memory Protection

* **[Added] EEPROM Boundary Cleansing:** Automated firewall in `getParametersFromEEPROM()` that intercepts, validates, and auto-corrects corrupted runtime parameters, sensitivities, and drift thresholds before they enter the kinematic loop.
* **[Added] Compile-Time Assertions:** Compile-time hardware validation via `calibrationChecks.h` using `static_assert` to strictly validate `ADC_OVERSAMPLES`, `ADC_EMA_SHIFT`, `EXCL_RELAX_THRESHOLD`, `COMP_NR`, and `COMP_WAIT`, halting compilation on unstable configurations.
* **[Fixed] Key Byte Boundary Check:** Resolved a critical Out-Of-Bounds (OOB) SRAM memory write vulnerability in `prepareKeyBytes()` by enforcing a strict bitmask boundary check (`bitNum < 32`), preventing invalid EEPROM shortcut IDs from overflowing `keyData[4]` buffers and crashing the MCU.
* **[Fixed] Drift Accumulator Clamping:** Eliminated a fatal infinite loop vulnerability in `compensateDrifts()` by clamping the `compNoOfPoints` accumulator to `[1, 500]`, preventing `int16_t` wrap-around and system freezes during sensor sampling.
* **[Improved] Centralized Tuning (`config.h`):** Centralized low-level hardware filters and processing thresholds into `config.h` (`ADC_PRESCALER_PRESET`, `ADC_OVERSAMPLES`, `ADC_EMA_SHIFT`, `EXCL_RELAX_THRESHOLD`), making core system tuning accessible without modifying `.cpp` files.

### Scalable Input & USB HID Subsystem

* **[Added] Universal Scalable Input Support:** Redesigned hardware button processing to seamlessly support anywhere from 0 to 32 physical push-buttons natively through the HID descriptor, without requiring core code modifications.
* **[Improved] Dual-Tier Key Mapping Architecture:** Primary keys (`keys[0]` and `keys[1]`) utilize dynamic on-device OLED shortcuts, while any additional expanded keys (`keys[2..31]`) automatically fallback to static `BUTTONLIST` assignments defined in `config.h`.

### Math Engine & Kinematics Shielding

* **[Fixed] Mathematical Domain Clamping:** Implemented absolute mathematical domain clamping in `modifierFunction()` for Curve A Q8 (`[26, 768]`) and Curve B Q8 (`[26, 402]`), preventing trigonometric asymptote explosions (NaN or Infinity) inside `tanf()` and `powf()`.
* **[Fixed] Zero-Division Protection:** Added strict zero-division hardware guards across the kinematic pipeline, protecting `readAllFromJoystick()` (`ADC_OVERSAMPLES < 1`) and `busyZeroing()` (`numIterations == 0`).
* **[Fixed] Serial CLI Overflow Clamping:** Corrected double-to-integer conversion overflows in the Serial CLI (`writeParameter()`), enforcing value clamping for Q7 (`[1, 32767]`) and Q8 (`[26, 768]`) parameters before 16-bit integer casting.
* **[Fixed] Underflow Protection:** Added underflow protection in `exclusiveMode()` to prevent 16-bit integer wrap-around glitches when negative hysteresis values are injected via corrupted EEPROM bytes.
* **[Improved] Dynamic Limit Verification:** Enhanced dynamic limit calibration (`oledDisplay.cpp`) with active deadzone threshold verification (`calMin[i] < -dz` and `calMax[i] > dz`), actively rejecting incomplete user calibration attempts to preserve valid EEPROM tracking.

---

## Release Notes - Firmware vALPHA 0.0.1 (The OLED & Performance Build)

This milestone release introduces a full-featured on-device OLED user interface, transitions motion processing to fixed-point arithmetic, doubles USB HID polling rates, and comprehensively optimizes ADC throughput for ultra-low latency 6DoF navigation.

### Display & User Interface (OLED)

* **[Added] Native SSD1306 Display Driver:** Native 0.96" SSD1306 I2C OLED display driver (`oledDisplay.cpp`) operating via `SSD1306AsciiWire` in direct RAM-less write mode, entirely eliminating the need for a 1KB SRAM frame buffer on the ATmega32U4.
* **[Added] Real-Time AXIS Visualizer:** Real-time AXIS Visualizer home screen featuring bi-directional graphic bar graphs and zero-flicker signed 4-digit formatted velocity readouts (+015, -003) across all 6 degrees of freedom.
* **[Added] On-Device UI Menu Hierarchy:** Allows real-time adjustment of: General Sensitivity, Axis Deadzone, Modifier Curves (LIN/SQR/A-B), Axis Inversions, XY/YZ Swapping, Display Sleep Timers, and Button Shortcut assignments.
* **[Added] Interactive Cal. Limits Screen:** Executes a 20-second dynamic limit sampling cycle with direct EEPROM persistence.
* **[Added] Sensor Alignment Diagnostic Screen:** Align Sensors debug diagnostic screen displaying real-time Hall effect sensor pair outputs (North, South, East, West) and differential deltas to assist physical magnet assembly.
* **[Added] Live Drift Monitor Screen:** Displays real-time per-sensor offsets and an instant toggle to enable/disable active drift compensation.
* **[Added] On-Device Factory Reset:** Provides EEPROM clearing and default parameter set restoration directly from the display.
* **[Added] Configurable Auto-Sleep Timer:** Inactivity auto-sleep timer (1min, 3min, 5min, or OFF) with instant wake-up on physical knob movement or tactile button press.

### HID & USB Protocol

* **[Improved] Polling Rate Upgrade:** Doubled USB HID report update rate from 16ms (~62.5Hz) to 8ms (125Hz polling rate) via `HIDUPDATERATE_MS`, cutting input transmission latency in half.
* **[Improved] Unified 12-Byte Motion Report:** Consolidated 6DoF translational and rotational motion packets into a single 12-byte unified payload per HID report cycle, streamlining USB endpoint packet handling.
* **[Added] USB Clock Resynchronization:** Automatic USB clock resynchronization mechanism inside `IsNewHidReportDue()` to prevent cascading packet delays and USB endpoint saturation during main loop timing jitter.
* **[Fixed] Jiggle Buffer Sizing:** Corrected data buffer sizing in `jiggleValues()` from a 6-byte array to match the full 12-byte 6DoF HID report payload.

### Kinematics & Motion Processing

* **[Improved] Fixed-Point Conversion:** Converted all 6DoF sensitivity calculations from floating-point arithmetic to fixed-point Q7 (`SENS_TX_Q7`) and Q8 (`SLOPE_A_Q8`, `SLOPE_B_Q8`) integer math, eliminating heavy runtime float emulation on the AVR architecture.
* **[Improved] Low-Pass ADC Filtering:** Implemented 2x hardware oversampling combined with an integer Exponential Moving Average (EMA) low-pass filter (`EMA_FILTER_SHIFT = 2`) to eliminate ADC jitter with zero floating-point overhead.
* **[Added] Global Sensitivity Scaling:** Integrated global sensitivity scaling parameter (`GLB_SENS`) providing master output scaling from 10% to 300% across all 6 axes.
* **[Added] Low-Level Noise Gates:** Integrated per-axis noise gates (`GATE_NTZ`, `GATE_RX`, `GATE_RY`, `GATE_RZ`) directly into kinematic processing to aggressively suppress axis cross-coupling and mechanical micro-movements.
* **[Changed] Re-Engineered Exclusive Mode:** Exclusive Mode algorithm with an auto-reset relaxation threshold (`RELAX_THRESHOLD = 35`) and neutral state (`mode = 0`), allowing instant, fluid switching between pure translation and pure rotation upon hand release.
* **[Added] UI Motion Suppression:** Implemented kinematic output suppression and key input blocking while OLED configuration menus are active to prevent unintended 3D canvas navigation during settings adjustment.

### Calibration & Drift Compensation

* **[Improved] Decoupled Drift Tracking:** Overhauled `compensateDrifts()` to execute independent, decoupled drift tracking per individual sensor axis rather than applying global group lockouts.
* **[Added] Adaptive Muting Logic:** Stable drifting axes are dynamically zeroed in real-time, while active movement instantly reverts the axis to the last verified stable baseline offset.
* **[Improved] Non-Blocking Boot Zeroing:** Updated boot-time zeroing (`busyZeroing()`) with a 3-attempt non-blocking retry loop and fallback OLED warning screen, replacing the legacy hardware lockout loop when uncalibrated magnet plates are detected.

### Hardware Acceleration & Memory Management

* **[Improved] ADC Prescaler Acceleration:** Configured ATmega32U4 ADC prescaler to 32 (`ADCSRA = (ADCSRA & 0xF8) | 0x05`), doubling the hardware ADC clock speed to 500 kHz and reducing analog conversion time per channel from 52µs to 26µs.
* **[Added] EEPROM Integrity Checksum:** 8-bit XOR checksum verification across `ParamStorage` in EEPROM to automatically detect memory corruption or magic number mismatches and safely restore factory defaults.
* **[Added] Hardware I2C Watchdog (`Wire.setWireTimeout`):** Auto-recovers the I2C bus in 3ms in case of OLED disconnection or Electrostatic Discharge (ESD) strikes, preventing main loop kernel panics.
* **[Improved] Debug String Tables Optimization:** Enclosed debug description string tables (`ParamDescription`) inside conditional `#if ENABLE_SERIAL_DEBUG` guards, shrinking `sizeof(ParamData)` from 542 bytes down to 2 bytes in production builds to maximize Flash space.
* **[Improved] Global Variable Downsizing:** Promoted global sensor variables (`rawReads`, `centered`, `centerPoints`, `offsets`, `velocity`) to explicit `int16_t` types and downsized key state arrays to `uint8_t` for 32-bit portability and SRAM conservation.

### Buttons & Input Processing

* **[Improved] Instant Press / Delayed Release Debouncing:** Upgraded key debouncing logic (`evalKeys()`) providing immediate zero-latency tactile response on initial button contact while forgiving mechanical release bounces.
* **[Changed] Debounce Timing Tuning:** Reduced default button debounce threshold (`DEBOUNCE_KEYS_MS`) from 200ms to 50ms for snappier tactile registration.
* **[Improved] Debounce Timestamp Memory Optimization:** Converted key timestamp tracking arrays to `uint16_t` timestamps, cutting SRAM consumption for key debouncing in half.
* **[Added] Dynamic OLED Key Mapping:** Allows physical Left (L) and Right (R) buttons to be assigned to standard 3DConnexion shortcuts (`FIT`, `TOP`, `RIGHT`, `FRONT`, `ROLL`, `ESC`, `ALT`, `SHIFT`, `CTRL`, etc.) directly via the OLED menu.