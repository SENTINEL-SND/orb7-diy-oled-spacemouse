# 🛸 O.R.B.7 (Orbital Rotation Base + OLED & Web Studio v1.6.2)
### DIY 6DOF SpaceMouse Pro Firmware & Printable Chassis (COMING SOON) | Standalone OLED Hardware Debugger & Driverless Web Studio

[![License: MIT](https://img.shields.io/badge/License-MIT-yellow.svg)](https://opensource.org/licenses/MIT)
[![Firmware](https://img.shields.io/badge/Firmware-Alpha_v0.0.8-blue.svg)]()
[![Web Studio](https://img.shields.io/badge/Web_Studio-v1.6.2-00ffcc.svg)]()
[![Hardware](https://img.shields.io/badge/MCU-ATmega32U4-red.svg)]()
[![3D Print](https://img.shields.io/badge/3D_Print-STL_Files-ff007f.svg)]()
[![Web API](https://img.shields.io/badge/API-WebHID-00ffcc.svg)]()

> ⚠️ **Note from the Author:** I am a CAD & maker enthusiast, not a professional software engineer! I'm currently learning programming and built this project using AI tools to help bring my vision to life. If you find bugs, rough code, or non-standard practices—or if you don't even know how to use `git` (just like me!)—you're in good company. Feedback, cleanups, and Pull Requests are always welcome!

---

> [!TIP]
> ### ⚡ OFFICIAL O.R.B.7 PRINTABLE CHASSIS & UNIVERSAL DIY COMPATIBILITY
> 
> **O.R.B.7** includes an official **custom 4-button 3D-printable housing/chassis** (STL files COMING SOON in this repository under `/3D_Files` and Release Assets!).
> 
> **Using a different DIY SpaceMouse model or don't have an OLED screen? No problem!** 
> The O.R.B.7 firmware is **100% backward-compatible** with legacy 8-channel Hall-effect builds (e.g., *TeachingTech* or *ArdunHH* designs, with 0, 2, or 4 buttons). If your build omits the OLED display, simply open `config.h` and set:
> ```cpp
> #define ENABLE_OLED 0
> ```
> The OLED driver, UI code, and graphics are automatically stripped at compile-time, freeing up Flash memory while giving your original hardware a **massive performance upgrade**:
> 
> * 🌐 **Full WebHID 3D Studio Support:** Configure, calibrate, tune Q7/Q8 response curves, and visualize 3D motion live in Chrome/Edge **without installing any drivers or PC software!**
> * 🚀 **Fixed-Point Motion Engine (Q7/Q8):** Integer-only calculations for ultra-low latency 125Hz CAD navigation.
> * 🎛️ **Full 4-Button Customization & Presets:** Map physical buttons to 32 native 3DConnexion CAD shortcuts.
> * 🔥 **Thermal Anti-Drift Engine:** Decoupled sensor tracking stops the cursor from drifting as sensors warm up.
> * ⚡ **Optimized 250 kHz ADC Sampling:** Noise-free 10-bit analog read throughput for snappy, jitter-free input.
> * 🔄 **Remote Hardware Reboot:** Trigger a remote MCU restart via Watchdog Timer directly from the browser.

---

An ultra-optimized, high-performance firmware and hardware ecosystem for DIY 6-DOF (Six Degrees of Freedom) 3D navigation controllers based on the **ATmega32U4** (Arduino Pro Micro). 

Emulating a native **3Dconnexion SpaceMouse Pro** USB HID device, **O.R.B.7** combines a **custom 4-button 3D-printed chassis**, a **standalone OLED hardware assembly debugger**, an integer-only **Q7/Q8 fixed-point math engine**, decoupled real-time drift compensation, and a **driverless single-page WebHID 3D Studio (v1.6.2)** to deliver a smooth, high-precision CAD navigation experience.

--- 

## 📖 The Story Behind O.R.B.7

What started as a simple hobby project to enhance [AndunHH](https://github.com/AndunHH)'s fantastic DIY SpaceMouse quickly turned into an engineering rabbit hole. 

My initial goal was modest: add a small OLED display for a sleek look and easier debugging. However, as I built and assembled the physical hardware, I realized the OLED could do something revolutionary: **turn the controller into a standalone hardware diagnostic station.** 

In **v0.0.8 / Web Studio v1.6.2**, the ecosystem expands into a complete web management suite paired with the official **O.R.B.7 3D-printable 4-button enclosure**. Featuring dynamic hardware telemetry, an interactive 4-button studio, a Three.js 3D motion viewport, Chart.js curve visualizer, a metrologically hardened calibration wizard, and JSON profile management, O.R.B.7 offers a complete end-to-end DIY 3D navigation experience.

> [!IMPORTANT]
> ### 🎯 THE CROWN JEWEL: PC-FREE HARDWARE ASSEMBLY & SENSOR ALIGNMENT
> 
> Building a 3D Hall-effect SpaceMouse usually involves painful trial-and-error—guessing magnet polarities, plugging into a PC, opening Serial Monitors, and reflashing code just to check if structural bolts are unevenly tightened.
> 
> **With O.R.B.7's OLED screen, you can assemble, align, and balance your physical 3D-printed housing directly at your workbench without a computer connected!** The built-in **`Align Sensors`** screen displays real-time differential deltas ($\Delta$) between opposing North, South, East, and West Hall sensor pairs. You can tighten tension screws and rotate magnet plates at your workbench with instant **`OK` / `ADJUST`** visual feedback!

---

### 💡 Why "O.R.B.7"?

* **O.R.B. (Orbital Rotation Base):** Represents the core mechanical behavior of the 3D controller. Just like orbiting a 3D model in CAD software, the knob pivots and rotates freely around a central 3D space.
* **7:** Signifies the breakthrough 7th major iteration of the hardware/firmware architecture, marking the integration of the OLED standalone hardware debugger, WebHID Studio, and real-time telemetry.

---

## ✨ Key Features

### 🖨️ Official O.R.B.7 3D-Printable Enclosure & Universal Support
* **Official O.R.B.7 4-Button Chassis:** Purpose-built 3D-printable (COMING SOON) housing optimized for FDM 3D printing tolerances, featuring an integrated OLED bezel and 4 tactile push-buttons (*STL files COMING SOON in `/3D_Files` and Release assets*).
* **Universal Backward Compatibility:** Fully compatible with legacy DIY SpaceMouse designs (*TeachingTech*, *ArdunHH*, or custom builds) supporting 0, 2, or 4 buttons.

---

### 📺 Standalone OLED Hardware Assembly & Debugging Station
Turn your Pro Micro into a standalone electronics workbench diagnostic tool using a zero-RAM-buffer display (`SSD1306AsciiWire`).
* **🎯 Precision Mechanical Assembly (`Align Sensors` Screen):**
  * Displays live raw readings for all 8 Hall sensors grouped into physical pairs: **North** ($H6 \mid H7$), **South** ($H0 \mid H1$), **East** ($H2 \mid H3$), and **West** ($H8 \mid H9$).
  * Computes mathematical differential deltas ($\Delta = |A - B|$) in real time.
  * Shows instant visual status badges: **`OK`** ($\Delta \le 100$) for perfectly balanced magnets vs **`!`** ($\Delta > 100$) flagging mechanical tilt or loose structural bolts.
* **⚡ On-Device Zeroing & Recalibration (`Re-Zero` Screen):** Execute full hardware rest-coordinate zeroing directly from the screen with a 1-second physical button hold.
* **📏 Dynamic Limits Tester (`Cal. Limits` Screen):** Runs a standalone 20-second dynamic range sampling cycle on the device and saves min/max bounds directly to EEPROM.
* **📊 Real-Time 6DOF Visualizer:** High-refresh mini bar graphs displaying live Translation ($TX, TY, TZ$) and Rotation ($RX, RY, RZ$) velocity outputs with zero screen flicker.
* **⚙️ Complete On-Device Configuration Menu:** Live adjustment of Global Sensitivity (%), Deadzone, Curves (LIN, SQR, Tangent), Axis Inversions (`INV TX..RZ`), Swapping (`SWP XY/YZ`), OLED Auto-Sleep Timers, and CAD Shortcut Button Remapping.

---

### 🌐 Driverless WebHID 3D Studio Suite (v1.6.2)
Manage all internal EEPROM parameters directly from Google Chrome, Edge, or Brave without installing any background services or executable drivers.

* **🩺 Device Health & System Telemetry:** Live monitoring of dynamic Firmware Version, EEPROM XOR Checksum Integrity (`VALID`), ADC Reference Voltage (`2.56V Internal`), Magnetic Balance (`BALANCED`), USB Connection State, and Sampling Frequency (Hz).
* **🎛️ Interactive Hardware Button Studio:**
  * Configure all **4 physical push buttons** dynamically.
  * Mapped to all **32 native 3DConnexion HID shortcuts** (*FIT, TOP, FRONT, ISO1, ESC, SHIFT, CTRL, ALT*, etc.).
  * **1-Click CAD Presets:** Instant configuration for **SolidWorks**, **Blender 3D**, **Modifier Keys**, and **4-View Camera Navigation**.
  * Real-time button press telemetry highlights button cards in the web dashboard instantly.
* **🔄 Remote Hardware Reboot (`Restart Mouse`):** Triggers a remote software restart of the ATmega32U4 MCU via the AVR Watchdog Timer (`wdt_enable(WDTO_60MS)`) with automatic USB disconnect handling.
* **📐 Hardened 20-Second Calibration Wizard:**
  * Executes a proactive auto Re-Zero before starting the boundary test.
  * Includes an **ADC Noise Spike Filter** that rejects electrical glitches ($> 900$ or $<-900$).
  * **Tab Visibility Monitor:** Warns the user if the browser tab loses focus during testing.
  * **Validation Guard:** Prevents saving invalid calibration boundary ranges ($\Delta < 80$) to EEPROM.
* **🧊 Interactive 3D Viewport (Three.js):** Real-time rendering of a 3D SpaceMouse knob that mirrors hand movements in 6DOF.
* **📈 Curve Visualizer (Chart.js):** Plot Q8 math modifiers (Slope A / Slope B) in real-time to fine-tune CAD panning and zooming curves.
* **💾 JSON Profile Management:** Save complete configuration profiles (`.json`) locally on PC and restore them into the Web Studio in 1 click.
* **🌐 Single-File Web Deployment:** Hosted as a single, standalone HTML file (`O.R.B.7 Web Studio v1.6.2.html`) that can be deployed via GitHub Pages or run locally by double-clicking.

---

### 📐 Core Kinematics & Extreme Optimization
* **Fixed-Point Arithmetic (Q7/Q8):** The 6DOF motion vector path is 100% free of runtime floating-point operations. Sensitivities are scaled via fast bit-shifts, drastically reducing cycle latency on 8-bit AVR architecture.
* **Dedicated Translation MicroGate (`gate_trans`):** Low-level noise gate (3 to 10) applied strictly to Translation X and Y (Pan) to eliminate post-matrix residual spring bleed without introducing initial movement resistance.
* **Full Physical Travel Matrix Restoration:** Restored optimal matrix divisors (`/4` and `/8`) to eliminate premature 50% physical travel deadband clipping and restore full stroke resolution.
* **Master Global Sensitivity & Per-Axis Noise Gates:** Symmetrical noise gates across all 6 axes eliminate parasitic axis crosstalk and accidental inputs.
* **Enhanced Exclusive Mode:** Features *Neutral Unlocking on Relaxation* (`EXCL_RELAX_THRESHOLD`), allowing fluid switching between Translation and Rotation dominance mid-movement.

---

### ⚡ ADC Acceleration & Thermal Drift Processing
* **250 kHz ADC Clock:** Hardware prescaler configuration (`0x06`) reduces analog conversion time to **~26 µs** per sample while maintaining clean 10-bit resolution.
* **2x Oversampling & Integer EMA Filtering:** Non-blocking Exponential Moving Average filter ($\alpha = 0.25$) with non-stuck rounding logic for smooth, noise-free motion output.
* **Decoupled Per-Axis Drift Tracking:** Independent tracking channels per sensor pair. Eliminates thermal zero-point drift (caused by ambient temperature changes or spring fatigue) in real time without blocking active movement.

---

### 🛡️ System Integrity & Reliability
* **Memory Corruption Firewall:** EEPROM parameter structure is protected by an 8-bit XOR checksum and rigid mathematical boundaries. Protects the MCU from zero-division crashes even if corrupted data is injected.
* **Clean Watchdog Reset Recovery:** Immediate clearing of Watchdog flags (`MCUSR = 0; wdt_disable();`) at boot prevents infinite boot loops upon software reboot.
* **Non-Blocking Boot Fallback:** 3-attempt zeroing retry on startup with OLED visual warning and lock-bypass fallback if the knob is touched during boot.
* **I2C Bus Recovery Guard:** 3 ms hardware timeout handling automatically clears I2C bus stalls caused by ESD or electrical noise.
* **CAD Viewport Isolation:** Kinematic output and HID button reports are automatically suppressed while navigating OLED menus or saving WebHID settings to prevent accidental wild camera spins.

---

## 🛠️ Hardware Requirements & 3D Printed Chassis

* **3D Printed Chassis:** Official **O.R.B.7 4-Button Enclosure** (*STL files COMING SOON in `/3D_Files`*) OR any standard DIY SpaceMouse housing (*TeachingTech*, *ArdunHH*).
* **Microcontroller:** ATmega32U4 (Arduino Pro Micro 5V / 16 MHz).
* **Sensors:** 8x Ratiometric Linear Hall-Effect Sensors (AH49E or equivalent).
* **Display (Optional):** 0.96" OLED SSD1306 (128x64 pixels, I2C address `0x3C`). Set `#define ENABLE_OLED 0` in `config.h` if omitted.
* **Buttons (Optional):** Up to 4 Tactile Push Buttons (ORB7 native chassis layout) or up to 32 buttons wired with internal `INPUT_PULLUP`.

---

## 🔌 Default Pinout Configuration (Official O.R.B.7 Hardware Layout)

| Component / Function | Sensor / Button | Pro Micro Pin | Notes |
| :--- | :--- | :--- | :--- |
| **OLED SDA** | Hardware I2C | **2** | I2C Data (400 kHz) |
| **OLED SCL** | Hardware I2C | **3** | I2C Clock (400 kHz) |
| **Hall Sensor HES0** | South Pair (A) | **A0** | Analog Input |
| **Hall Sensor HES1** | South Pair (B) | **A1** | Analog Input |
| **Hall Sensor HES2** | East Pair (A) | **A2** | Analog Input |
| **Hall Sensor HES3** | East Pair (B) | **A3** | Analog Input |
| **Hall Sensor HES6** | North Pair (A) | **4 (A6)** | Analog Input |
| **Hall Sensor HES7** | North Pair (B) | **6 (A7)** | Analog Input |
| **Hall Sensor HES8** | West Pair (A) | **8 (A8)** | Analog Input |
| **Hall Sensor HES9** | West Pair (B) | **9 (A9)** | Analog Input |
| **Front Right Button [R]** | `keys[0]` | **5** | Dynamic HID Shortcut |
| **Front Left Button [L]** | `keys[1]` | **0 (RX)** | Dynamic HID Shortcut |
| **Back Left Button [2]** | `keys[2]` | **1 (TX)** | Dynamic HID Shortcut |
| **Back Right Button [1]** | `keys[3]` | **7** | Dynamic HID Shortcut |

*(Note: If building a 2-button or custom layout, buttons can be mapped to any available GPIO pins in `config.h`)*

---

## 📜 License, Credits & Historical Lineage

O.R.B.7 is built upon a rich history of open-source DIY SpaceMouse development and community innovation. We gratefully acknowledge the pioneer creators whose foundational code, kinematics, and hardware concepts made this project possible:

* **[Shiura](https://www.thingiverse.com/thing:5739462):** Creator of the original *Space Mushroom* 3D navigation concept.
* **[jfedor](https://pastebin.com/gQxUrScV) & [BennyBWalker](https://pastebin.com/erhTgRBH):** Pioneer USB HID emulation code for native 3Dconnexion SpaceMouse protocol.
* **[fdmakara](https://www.thingiverse.com/thing:5817728):** 4-joystick/sensor matrix movement logic.
* **[TeachingTech](https://www.youtube.com/@TeachingTech):** Consolidated 4-joystick mixing with USB HID emulation and extensive debugging tools.
* **Daniel_1284580:** Min/Max dynamic calibration bounds and author of the original mathematical **Modifier Function** curve algorithm.
* **LivingTheDream:** Code optimization, written tutorials, and modifier curve enhancements.
* **JoseLuisGZA:** Integrated kill-keys concept and knob encoder wheel logic.
* **[AndunHH](https://github.com/AndunHH):** Hall-effect sensor matrix adaptation, ADC dead-zone integration, thermal drift tracking, and creator of the baseline repository that directly inspired O.R.B.7.

---

Distributed under the **MIT License**. Free to use, modify, and distribute for personal and commercial projects.
