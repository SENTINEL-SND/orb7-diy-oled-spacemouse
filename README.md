# O.R.B.7 (Orbital Rotation Base + OLED & Web Studio)

> DIY 6DOF SpaceMouse Pro Firmware & Printable Chassis | Standalone OLED Hardware Debugger & Driverless Web Studio

---

## 📋 Table of Contents

- [Overview](#-overview)
- [Key Features](#-key-features)
  - [3D-Printable Enclosure & Universal Support](#️-3d-printable-enclosure--universal-support)
  - [Standalone OLED Hardware Assembly & Debugging Station](#-standalone-oled-hardware-assembly--debugging-station)
  - [Driverless WebHID 3D Studio Suite](#-driverless-webhid-3d-studio-suite-v180)
  - [Core Kinematics & Motion Processing](#-core-kinematics--motion-processing)
  - [Sensor Acquisition & Fixed-Center Processing](#-sensor-acquisition--fixed-center-processing)
  - [System Integrity & Reliability](#️-system-integrity--reliability)
- [Quick Start](#-quick-start)
- [Hardware Requirements](#️-hardware-requirements)
- [Pinout Configuration](#-default-pinout-configuration)
- [Project Background](#-the-story-behind-orb7)
- [Credits & Historical Lineage](#-license-credits--historical-lineage)
- [License](#-license)

---

## 🔍 Overview

**O.R.B.7** is an open-source firmware and hardware project for DIY **6DOF (Six Degrees of Freedom) 3D navigation controllers** built around the **ATmega32U4**, including the Arduino Pro Micro 5V / 16 MHz.

It behaves as a **3Dconnexion SpaceMouse Pro-compatible USB HID device**, but the goal of the project goes beyond basic SpaceMouse emulation.

O.R.B.7 combines a custom 4-button 3D-printable chassis, an optional OLED display, on-device calibration and diagnostics, a lightweight Q7/Q8 motion engine, and a driverless WebHID Studio that can configure, calibrate, and test the controller directly from a browser.

The OLED is not just there for looks. It turns the controller into a standalone hardware debugging tool: you can inspect sensor alignment, Re-Zero the device, calibrate its physical travel, adjust settings, and troubleshoot the hardware without constantly returning to a serial console or external application.

The current firmware takes a deliberately simpler approach to motion processing.

Sensor readings are oversampled, compared against the neutral position captured during Re-Zero, passed through the 6DOF kinematic matrix, and finally cleaned up using six configurable post-matrix axis gates.

The idea is simple: **preserve small intentional movements while giving the user predictable control over unwanted motion around the neutral position.**

> [!TIP]
> **Using an older DIY SpaceMouse build?**\
> O.R.B.7 does not require the OLED or the official chassis.
>
> If you are using a legacy 8-channel Hall-effect design such as the *TeachingTech* or *AndunHH* builds, set:
>
> ```cpp
> #define ENABLE_OLED 0
> ```
>
> in `config.h`.
>
> The OLED subsystem is removed at compile time, freeing Flash while keeping the motion engine, calibration, USB HID, and WebHID functionality available.

---

## ✨ Key Features

### 🖨️ 3D-Printable Enclosure & Universal Support

O.R.B.7 was designed around a dedicated 4-button enclosure, but the firmware itself is not tied to one specific chassis.

- **Official O.R.B.7 4-Button Chassis:** Purpose-built enclosure designed around normal FDM printing tolerances, with an integrated OLED bezel and four tactile buttons. *STL files are still under final refinement and will be released separately.*
- **Legacy Hardware Support:** Existing DIY SpaceMouse builds can be adapted by assigning the correct sensor and button pins in `config.h`.
- **Flexible Button Layouts:** The firmware can be used with 0, 2, or 4 physical buttons without requiring the official enclosure.
- **Optional OLED:** The entire display subsystem can be disabled when only USB HID functionality is needed.

---

### 📺 Standalone OLED Hardware Assembly & Debugging Station

The optional 0.96" SSD1306 OLED turns O.R.B.7 into a self-contained configuration and diagnostic tool.

The display uses the lightweight `SSD1306AsciiWire` library and does not require a full-screen framebuffer, which is especially useful on the memory-constrained ATmega32U4.

> [!IMPORTANT]
> **PC-Free Mechanical Alignment (`Align Sensors`):**\
> One of the most useful parts of the OLED interface is the ability to physically assemble and tune the controller without relying on a PC.
>
> The Align Sensors screen shows live Hall-effect readings and the differential value (`Δ`) between opposing sensor pairs. As the mechanism is adjusted, the display immediately shows whether each pair is balanced or still needs mechanical correction.

- **Re-Zero:** Capture a fresh neutral position directly from the controller.
- **Dynamic Limits:** Run a 20-second physical movement test and save the actual usable motion range to EEPROM.
- **6DOF Visualizer:** Watch live Translation (`TX`, `TY`, `TZ`) and Rotation (`RX`, `RY`, `RZ`) output directly on the OLED.
- **Sensor Alignment:** Inspect opposing Hall-effect sensor pairs in real time while assembling or servicing the mechanism.
- **On-Device Configuration:** Adjust Global Sensitivity, Unified Deadzone, Modifier Curves, Axis Inversions, XY/YZ swapping, OLED sleep behavior, and button shortcuts without opening the Web Studio.
- **Factory Reset:** Restore the firmware configuration to safe defaults directly from the hardware.
- **Auto-Sleep:** Set the OLED timeout to 1, 3, or 5 minutes, or disable sleep entirely. Movement or a button press wakes it immediately.

The goal is to make the controller useful even when it is sitting alone on a workbench.

---

### 🌐 Driverless WebHID 3D Studio Suite (v1.8.0)

The **O.R.B.7 Web Studio** is the easiest way to configure, calibrate, and troubleshoot the device.

It connects directly to the controller through WebHID, so there is no separate configuration application, background service, or installer to maintain.

A supported Chromium-based browser is enough.

| Dashboard & 3D Motion Studio | Sensitivities & Modifier Curves |
| :---: | :---: |
| ![Dashboard](images/Web-Studio-Dashboard.jpeg)<br>*Live 3D puck visualization, 6DOF output, telemetry health & 4-button studio.* | ![Sensitivities](images/Web-Studio-Sensitivities.jpeg)<br>*Per-axis tuning, axis inversions, unified deadzone & real-time response-curve visualization.* |
| **Hardware Test & Device Health** | **Setup Wizard & Calibration** |
| ![Hardware Test](images/Web-Studio-Gates-Drift.jpeg)<br>*Gates, including the Translation Base Gates, Rotation Base Gates and Exclusive Mode Tuning* | ![Setup Wizard](images/Web-Studio-Debug.jpeg)<br>*Guided sensor alignment, Re-Zero, Dynamic Limits, axis configuration & EEPROM persistence.* |

- **Interactive 3D Viewport:** A Three.js-powered representation of the controller follows live translation and rotation output in real time.
- **Modifier Curve Visualizer:** Chart.js shows how the configured response curves affect motion before those values reach the CAD application.
- **Unified Deadzone Control:** A single 0–100% control adjusts low-level motion suppression across the six post-matrix axis gates.
- **Hardware Button Studio:** Configure all four O.R.B.7 buttons using native 3Dconnexion shortcuts such as `FIT`, `TOP`, `FRONT`, `ISO1`, `ESC`, `SHIFT`, `CTRL`, and `ALT`.
- **Guided Setup Wizard:** Walk through initial device setup step by step, including sensor alignment, Re-Zero, Dynamic Limits calibration, axis configuration, and EEPROM saving.
- **Guided Hardware Test:** Validate Connection, Sensors, 6DOF Movement, Buttons, Safe Commands, and generate a final diagnostic report.
- **JSON Hardware Reports:** Export completed Hardware Test results for troubleshooting, comparison, or sharing.
- **System Telemetry:** Inspect firmware version, EEPROM state, magnetic balance, USB state, sensor activity, and live communication data.
- **Dynamic Limits Calibration:** Run the complete movement-range calibration from the browser with built-in validation.
- **Remote Re-Zero:** Capture a new neutral center without navigating the on-device menus.
- **Remote Restart:** Restart the ATmega32U4 directly from the Studio without physically disconnecting USB.
- **JSON Profiles:** Export and restore complete Web Studio configurations.
- **Synchronization Safety:** If the Studio can no longer determine whether a command completed successfully, it treats the device state as uncertain rather than silently assuming success.
- **Disconnect Recovery:** Device state is cleared after USB disconnect and rebuilt only after a fresh configuration read.
- **Standalone HTML:** The complete Studio can be distributed and opened as a single local HTML file.

---

### 📐 Core Kinematics & Motion Processing

The motion engine is intentionally lightweight.

The ATmega32U4 is an 8-bit microcontroller with limited Flash, SRAM, and processing headroom, so O.R.B.7 avoids doing expensive work where simpler math produces the same practical result.

- **Q7/Q8 Fixed-Point Processing:** Sensitivity and modifier-curve calculations use compact fixed-point arithmetic designed for the AVR platform.
- **True 6DOF Output:** Translation (`TX`, `TY`, `TZ`) and Rotation (`RX`, `RY`, `RZ`) are generated from the Hall-sensor matrix and processed as six independent output axes.
- **Unified Deadzone:** Instead of stacking several separate noise filters, one user-facing deadzone control raises the six independent post-matrix axis gates proportionally.
- **Continuous Gate Rescaling:** Once an axis passes its gate threshold, output is rescaled continuously instead of suddenly jumping from zero to the raw motion value.
- **Full Physical Travel:** Matrix scaling is tuned to preserve useful mechanical stroke and sensor resolution.
- **Configurable Curves:** Linear and nonlinear response curves allow movement feel to be adjusted without changing the physical calibration.
- **Exclusive Mode:** Optional translation/rotation isolation can reduce unwanted cross-axis interaction while still allowing smooth transitions back to neutral.
- **Menu Isolation:** 6DOF reports are suppressed while sensitive configuration screens or save operations are active, preventing the CAD viewport from moving unexpectedly.

#### Why the Motion Pipeline Was Simplified

Earlier O.R.B.7 revisions accumulated several layers of filtering while I was trying to solve small drift, noise, and cross-coupling problems.

Those experiments included dynamic drift compensation, integer EMA filtering, a sensor-level deadzone, Direct Root MicroGate filtering, and an additional post-curve noise gate.

Each layer was originally added for a reason. The problem was that, over time, some of them began compensating for the side effects of other filters rather than improving the actual sensor signal.

That made the pipeline harder to understand and, more importantly, could make very small intentional movements less predictable.

The biggest improvement came from simplifying the system.

Starting with **firmware v0.0.7-alpha**, O.R.B.7 trusts the neutral center captured during Re-Zero instead of continuously moving that reference point during normal use.

The current pipeline is built around three basic ideas:

1. **Arithmetic oversampling** to clean up ADC acquisition.
2. **A fixed neutral center** captured during Re-Zero.
3. **Six post-matrix axis gates** controlled by one unified Deadzone setting.

This removed a large amount of stateful filtering from the signal path.

On the hardware tested during development, the result was easier to tune, more stable around the neutral position, and better at preserving subtle intentional motion.

> [!NOTE]
> These observations are specific to the O.R.B.7 firmware and the hardware configurations tested during its development.
>
> They are not intended as a general criticism of the original firmware or of other DIY SpaceMouse designs, where different filtering strategies may work perfectly well.

---

### ⚡ Sensor Acquisition & Fixed-Center Processing

The eight Hall-effect sensors are the foundation of the controller, so keeping their signal path predictable is more important than simply adding more filtering.

- **Fast ADC Acquisition:** The ATmega32U4 ADC is configured for responsive sampling across all eight Hall-effect channels.
- **Arithmetic Oversampling:** Multiple ADC samples are combined to reduce individual sample noise without maintaining a stateful smoothing filter between frames.
- **Fixed Neutral Reference:** Re-Zero captures the resting position used as the reference for future motion calculations.
- **Independent Sensor Visibility:** All eight Hall-effect channels remain individually observable through the OLED and Web Studio for alignment and troubleshooting.
- **Dynamic Movement Limits:** Calibration measures the real usable travel of the assembled controller instead of assuming every printed mechanism has identical range.
- **No Continuous Neutral Drift:** Firmware v0.0.7-alpha does not continuously move the center position while the controller is being used. Re-Zero explicitly defines where neutral is.

This makes the system easier to reason about: when the controller is at rest, the firmware compares the current sensor position against a known fixed center.

---

### 🛡️ System Integrity & Reliability

A DIY controller should fail predictably.

O.R.B.7 therefore includes several safeguards around EEPROM, USB communication, calibration, watchdog behavior, and the OLED bus.

- **EEPROM Integrity Checking:** Persistent configuration data is protected by bounds validation and an XOR checksum.
- **Versioned EEPROM Layout:** Firmware revisions can invalidate incompatible EEPROM structures instead of trying to interpret old data incorrectly.
- **Safe Defaults:** Invalid or incompatible configuration data is replaced with known-safe values.
- **Watchdog Protection:** AVR watchdog flags are explicitly cleared during startup to prevent accidental reboot loops after software-triggered restarts.
- **I2C Recovery:** OLED communication uses timeout protection to reduce the chance of a stalled I2C transaction locking the display path.
- **Conservative WebHID State Handling:** The Web Studio does not assume that a command succeeded when communication becomes uncertain.
- **CAD Viewport Isolation:** Motion output is paused during sensitive configuration operations.
- **Calibration Validation:** Incomplete or implausible calibration data is rejected instead of being written to EEPROM.

---

## 🚀 Quick Start

> [!WARNING]
> **USB Identity Configuration Required:**\
> Before flashing O.R.B.7, the ATmega32U4 board definition must use the USB Vendor ID (`VID`) and Product ID (`PID`) expected by the SpaceMouse Pro HID configuration used by this project.
>
> This allows the operating system and compatible 3Dconnexion software to recognize the controller using the expected HID identity.

### 1. Download the Latest Release

Download the latest firmware and Web Studio package from the [O.R.B.7 Releases](../../releases) page.

---

### 2. Configure Arduino IDE (`boards.txt`)

Follow AndunHH's original guide:

[Creating a Custom Board for Arduino IDE](https://github.com/AndunHH/spacemouse/wiki/Creating-a-custom-board-for-Arduino-IDE)

1. Locate the Arduino AVR hardware configuration containing `boards.txt`.

   A common Arduino IDE installation path is:

   ```text
   C:\Users\YourName\AppData\Local\Arduino15\packages\arduino\hardware
   ```

2. Add a custom board entry or configure an appropriate **Arduino Leonardo / ATmega32U4** definition using the required USB identity:

   ```ini
   # 3Dconnexion SpaceMouse Pro Emulation Settings
   leonardo.build.vid=0x256F
   leonardo.build.pid=0xC62B
   leonardo.build.usb_product="SpaceMouse Pro"
   ```

3. Save `boards.txt`.

4. Restart Arduino IDE so the board definition is reloaded.

---

### 3. Flash the Firmware

1. Open the downloaded O.R.B.7 firmware in Arduino IDE.

2. Install the required OLED library through Library Manager:

   - **SSD1306Ascii** by Bill Greiman

3. Select the configured ATmega32U4 board and the correct COM port.

4. Review the hardware configuration in `config.h`.

   For an O.R.B.7 build with the OLED installed:

   ```cpp
   #define ENABLE_OLED 1
   ```

   For a build without the OLED:

   ```cpp
   #define ENABLE_OLED 0
   ```

5. Click **Upload**.

---

### 4. Perform Initial Calibration

After the first boot:

1. Leave the controller untouched while the initial neutral position is captured.
2. Open **Align Sensors** and check the balance between opposing Hall-effect sensor pairs.
3. Mechanically adjust the controller if necessary.
4. Run **Re-Zero** after any mechanical adjustment.
5. Run **Dynamic Limits** through either the OLED interface or the Web Studio.
6. Save the resulting configuration to EEPROM.

> [!IMPORTANT]
> Firmware **v0.0.7-alpha** uses a revised EEPROM layout.
>
> When upgrading from an earlier firmware version, the device restores safe defaults on first boot and **Dynamic Limits must be calibrated again**.

---

### 5. Open Web Studio

Use the **O.R.B.7 Web Studio v1.8.0** HTML file included with the release.

1. Open `O.R.B.7 Web Studio v1.8.0.html` in a supported Chromium-based browser.
2. Click **Connect Device**.
3. Select the O.R.B.7 / SpaceMouse device from the WebHID picker.
4. Allow the Studio to read the current firmware configuration.
5. For a new build, run the **Guided Setup Wizard** to walk through the initial configuration and calibration.
6. Optionally run the **Guided Hardware Test** to validate the complete device and generate a diagnostic report.

---

## 🛠️ Hardware Requirements

- **Microcontroller:** ATmega32U4 — Arduino Pro Micro 5V / 16 MHz or compatible board.
- **Sensors:** 8× ratiometric linear Hall-effect sensors such as AH49E or equivalent.
- **Mechanical Assembly:** Four opposing Hall-sensor pairs arranged in a compatible DIY SpaceMouse geometry.
- **3D-Printed Chassis:** Official **O.R.B.7 4-Button Enclosure** (*STL release coming soon*) or a compatible legacy DIY SpaceMouse chassis.
- **Display (Optional):** 0.96" SSD1306 OLED, 128×64, I2C, typically address `0x3C`.
- **Buttons (Optional):** Four tactile push-buttons in the standard O.R.B.7 layout, or another configuration adapted in firmware.

---

## 🔌 Default Pinout Configuration

| Component / Function       | Sensor / Button | Pro Micro Pin  | Description          |
| :------------------------- | :-------------- | :------------- | :------------------- |
| **OLED SDA**               | Hardware I2C    | **Pin 2**      | I2C Data             |
| **OLED SCL**               | Hardware I2C    | **Pin 3**      | I2C Clock            |
| **Hall Sensor HES0**       | South Pair (A)  | **A0**         | Analog Input         |
| **Hall Sensor HES1**       | South Pair (B)  | **A1**         | Analog Input         |
| **Hall Sensor HES2**       | East Pair (A)   | **A2**         | Analog Input         |
| **Hall Sensor HES3**       | East Pair (B)   | **A3**         | Analog Input         |
| **Hall Sensor HES6**       | North Pair (A)  | **Pin 4 (A6)** | Analog Input         |
| **Hall Sensor HES7**       | North Pair (B)  | **Pin 6 (A7)** | Analog Input         |
| **Hall Sensor HES8**       | West Pair (A)   | **Pin 8 (A8)** | Analog Input         |
| **Hall Sensor HES9**       | West Pair (B)   | **Pin 9 (A9)** | Analog Input         |
| **Button Front Right [R]** | `keys[0]`       | **Pin 0**      | Dynamic HID Shortcut |
| **Button Front Left [L]**  | `keys[1]`       | **Pin 1**      | Dynamic HID Shortcut |
| **Button Back Left [2]**   | `keys[2]`       | **Pin 5**      | Dynamic HID Shortcut |
| **Button Back Right [1]**  | `keys[3]`       | **Pin 7**      | Dynamic HID Shortcut |

> [!NOTE]
> These are only the default assignments.
>
> Pins can be changed in `config.h` to support custom hardware.

---

## 📖 The Story Behind O.R.B.7

O.R.B.7 started as a much smaller project than it eventually became.

I originally wanted to modify [AndunHH's DIY SpaceMouse firmware](https://github.com/AndunHH/spacemouse) for one simple reason: I wanted an OLED on the controller.

At first, the idea was mostly cosmetic. A small screen would make the device feel more finished and give me somewhere to display basic status information.

That changed while I was assembling the hardware.

When working with eight Hall-effect sensors, small mechanical differences matter. Magnet position, printed tolerances, preload, sensor spacing, and even how the mechanism is tightened can affect the resting values.

I realized the OLED could show that information directly on the device.

That turned it from a display into a tool.

The **Align Sensors** screen made it possible to physically tune opposing Hall-effect sensor pairs at the workbench. Re-Zero made it possible to establish a fresh neutral position without reflashing anything. Dynamic Limits made it possible to measure the real travel of the assembled mechanism instead of relying on theoretical values.

From there, the project kept growing.

I began revisiting the motion-processing code, experimenting with fixed-point math, calibration behavior, filtering, USB timing, memory usage, button handling, and eventually browser-based configuration through WebHID.

That became the Web Studio.

Some experiments worked well and stayed. Others did not.

One of the more important lessons came from the filtering system. While trying to eliminate tiny drift and noise artifacts, O.R.B.7 gradually accumulated several layers of compensation. Eventually, testing showed that those layers were making the signal path more complicated than it needed to be.

Removing them was one of the biggest improvements.

The current firmware trusts a fixed neutral center captured during Re-Zero, uses simple arithmetic oversampling, and applies filtering only after the 6DOF matrix where it can be controlled predictably.

That process shaped what O.R.B.7 is today: not just another SpaceMouse firmware, but an attempt to make a DIY 6DOF controller easier to **build, understand, tune, diagnose, and use**.

O.R.B.7 is not a clean-room implementation, and it was never intended to hide where it came from.

It exists because previous developers openly shared their mechanical designs, HID research, kinematic models, calibration ideas, and firmware.

The project is an extension of that work.

### Why "O.R.B.7"?

- **O.R.B. — Orbital Rotation Base:** Refers to the controller's ability to translate, pivot, and rotate through 3D space.
- **7:** Represents the seventh major architecture revision reached during development of the project.

---

## 📜 License, Credits & Historical Lineage

O.R.B.7 stands on the work of the open-source DIY SpaceMouse community.

Many of the ideas that now exist together in this project originated in earlier mechanical designs, USB experiments, kinematic implementations, and community firmware.

Credit belongs to the people who made that work available:

- **[Shiura](https://www.thingiverse.com/thing:5739462):** Creator of the original *Space Mushroom* mechanism concept.
- **[jfedor](https://pastebin.com/gQxUrScV) & [BennyBWalker](https://pastebin.com/erhTgRBH):** Early native 3Dconnexion SpaceMouse USB HID protocol emulation work.
- **[fdmakara](https://www.thingiverse.com/thing:5817728):** Four-joystick / sensor-matrix kinematic approach.
- **[TeachingTech](https://www.youtube.com/@TeachingTech):** Brought together joystick mixing, USB HID integration, mechanical implementation, and accessible build documentation for the wider DIY community.
- **Daniel_1284580:** Dynamic calibration range logic and the original Modifier Function curve algorithm.
- **LivingTheDream:** Firmware optimization and mathematical curve refinements.
- **JoseLuisGZA:** Kill-keys implementation and encoder integration logic.
- **[AndunHH](https://github.com/AndunHH):** Hall-effect sensor-matrix adaptation, ADC deadzone and thermal-drift work, and author of the firmware that became the direct starting point for O.R.B.7.

### Direct Firmware Lineage

O.R.B.7 was built directly from **[AndunHH's open-source SpaceMouse firmware](https://github.com/AndunHH/spacemouse)**.

The first O.R.B.7 release kept that foundation while adding the OLED interface, revised motion processing, new calibration and diagnostic tools, memory and USB optimizations, and expanded hardware configuration.

Later versions continued moving further away from the original architecture, including major changes to filtering, EEPROM storage, button handling, WebHID communication, and the complete Web Studio.

Even with those changes, the project's origin remains important and should be clear to anyone reading the source.

---

## 📄 License

Distributed under the **MIT License**.

Open-source software — free to use, modify, and distribute for personal and commercial applications under the terms of the included [LICENSE](./LICENSE) file.
