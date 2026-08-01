# 🛸 O.R.B.7 (Orbital Rotation Base + OLED & Web Studio)
### DIY 6DOF SpaceMouse Pro Firmware | Standalone OLED Hardware Debugger & Driverless Web Studio

[![License: MIT](https://img.shields.io/badge/License-MIT-yellow.svg)](https://opensource.org/licenses/MIT)
[![Version](https://img.shields.io/badge/Firmware-Alpha_v0.0.4-blue.svg)]()
[![Hardware](https://img.shields.io/badge/MCU-ATmega32U4-red.svg)]()
[![Web API](https://img.shields.io/badge/API-WebHID-00ffcc.svg)]()

> ⚠️ **Note from the Author:** I am a CAD & maker enthusiast, not a professional software engineer! I'm currently learning programming and built this project using AI tools to help bring my vision to life. If you find bugs, rough code, or non-standard practices—or if you don't even know how to use `git` (just like me!)—you're in good company. Feedback, cleanups, and Pull Requests are always welcome!

An ultra-optimized, high-performance firmware for DIY 6-DOF (Six Degrees of Freedom) 3D navigation controllers based on the **ATmega32U4** (Arduino Pro Micro). 

Emulating a native **3Dconnexion SpaceMouse Pro** USB HID device, **O.R.B.7** combines a **standalone OLED hardware assembly debugger**, an integer-only **Q7/Q8 fixed-point math engine**, decoupled real-time drift compensation, and a **driverless WebHID 3D Studio** to deliver a smooth, high-precision CAD navigation experience.

--- 

## 📖 The Story Behind O.R.B.7

What started as a simple hobby project to enhance the fantastic [ardunnh](https://github.com/AndunHH)'s DIY SpaceMouse quickly turned into an engineering rabbit hole. 

My initial goal was modest: add a small OLED display for a sleek look and easier debugging. However, as I built and assembled the physical hardware, I realized the OLED could do something revolutionary: **turn the controller into a standalone hardware diagnostic station.** 

In **v0.0.4**, I expanded the ecosystem with a driverless **WebHID Browser Studio** (featuring a Three.js 3D motion viewport, Chart.js curve visualizer, and a 20-second limits calibration wizard). Together, the OLED handles on-the-fly workbench assembly, while the Web Studio handles deep visual CAD tuning.

> [!TIP]
> ### 🎯 THE CROWN JEWEL: PC-FREE HARDWARE ASSEMBLY & SENSOR ALIGNMENT
> 
> Building a 3D Hall-effect SpaceMouse usually involves painful trial-and-error—guessing magnet polarities, plugging into a PC, opening Serial Monitors, and reflashing code just to check if structural bolts are unevenly tightened.
> 
> **With O.R.B.7's OLED screen, you can assemble, align, and balance your physical 3D-printed housing directly at your workbench without a computer connected!** The built-in **`Align Sensors`** screen displays real-time differential deltas ($\Delta$) between opposing North, South, East, and West Hall sensor pairs. You can tighten tension screws and rotate magnet plates at your workbench with instant **`OK` / `ADJUST`** visual feedback!

---

### 💡 Why "O.R.B.7"?

* **O.R.B. (Orbital Rotation Base):** Represents the core mechanical behavior of the 3D controller. Just like orbiting a 3D model in CAD software, the knob pivots and rotates freely around a central 3D space.
* **7:** Signifies the breakthrough 7th major iteration of the hardware/firmware, marking the integration of the OLED standalone hardware debugger, WebHID Studio, and real-time telemetry.

---

## ✨ Key Features

### 📺 Standalone OLED Hardware Assembly & Debugging Station
Turn your Pro Micro into a standalone electronics workbench diagnostic tool using a zero-RAM-buffer display (`SSD1306AsciiWire`).
* **🎯 Precision Mechanical Assembly (`Align Sensors` Screen):**
  * Displays live raw readings for all 8 Hall sensors grouped into physical pairs: **North** ($H6 \mid H7$), **South** ($H0 \mid H1$), **East** ($H2 \mid H3$), and **West** ($H8 \mid H9$).
  * Computes mathematical differential deltas ($\Delta = |A - B|$) in real time.
  * Shows instant visual status badges: **`OK`** ($\Delta \le 50$) for perfectly balanced magnets vs **`!`** ($\Delta > 50$) flagging mechanical tilt or loose structural bolts.
* **⚡ On-Device Zeroing & Recalibration (`Re-Zero` Screen):** Execute full hardware rest-coordinate zeroing directly from the screen with a 1-second physical button hold.
* **📏 Dynamic Limits Tester (`Cal. Limits` Screen):** Runs a standalone 20-second dynamic range sampling cycle on the device and saves min/max bounds directly to EEPROM.
* **📊 Real-Time 6DOF Visualizer:** High-refresh mini bar graphs displaying live Translation ($TX, TY, TZ$) and Rotation ($RX, RY, RZ$) velocity outputs with zero screen flicker.
* **⚙️ Complete On-Device Configuration Menu:** Live adjustment of Global Sensitivity (%), Deadzone, Curves (LIN, SQR, Tangent), Axis Inversions (`INV TX..RZ`), Swapping (`SWP XY/YZ`), OLED Auto-Sleep Timers, and CAD Shortcut Button Remapping (*FIT, TOP, RIGHT, FRONT, ESC*, etc.).

---

### 🌐 Driverless WebHID 3D Studio (v0.0.4 Studio Suite)
Manage all 32 internal EEPROM parameters directly from Google Chrome, Edge, or Brave.
* **Zero-Install Configuration:** Uses native browser WebHID APIs via a custom Top-Level Collection (TLC) to bypass OS security blocks. No background services or executable drivers required.
* **Interactive 3D Viewport (Three.js):** Real-time rendering of a 3D SpaceMouse knob that mirrors your exact hand movements in 6DOF with zero latency.
* **Curve Visualizer (Chart.js):** Adjust your Q8 math modifiers (Slope A / Slope B) and instantly see the response curve plotted on a graph to tune your CAD pan/zoom behavior.
* **20-Second Web Calibration Wizard:** Click "Start", move the knob to its mechanical extremes, and let the browser automatically calculate your hardware bounds and flash them to the EEPROM.
* **Live Telemetry Stream:** 33Hz real-time streaming of Raw ADC reads, Centered values, Thermal Drift Offsets, and 6DOF Velocity directly to the browser for deep hardware diagnostics.

---

### 📐 Core Kinematics & Extreme Optimization
* **Fixed-Point Arithmetic (Q7/Q8):** The 6DOF motion vector path is 100% free of runtime floating-point operations. Sensitivities are scaled via fast bit-shifts, drastically reducing cycle latency on 8-bit AVR architecture.
* **Universal Scalable Input:** Native support for mapping anywhere from **0 to 32 tactile hardware buttons** without modifying the core HID descriptors.
* **Master Global Sensitivity & Per-Axis Noise Gates:** Symmetrical noise gates across all 6 axes eliminate parasitic axis crosstalk and accidental inputs.
* **Enhanced Exclusive Mode:** Features *Neutral Unlocking on Relaxation* (`RELAX_THRESHOLD`), allowing fluid switching between Translation and Rotation dominance mid-movement.

---

### ⚡ ADC Acceleration & Thermal Drift Processing
* **500 kHz ADC Clock:** Reconfigured hardware prescaler reduces analog conversion time from ~52 µs to **~26 µs** per sample.
* **2x Oversampling & Integer EMA Filtering:** Non-blocking Exponential Moving Average filter ($\alpha = 0.25$) with non-stuck rounding logic for smooth, noise-free motion output.
* **Decoupled Per-Axis Drift Tracking:** Independent tracking channels per sensor pair. Eliminates thermal zero-point drift (caused by ambient temperature changes or spring fatigue) in real time without blocking active movement.

---

### 🛡️ System Integrity & Reliability
* **Memory Corruption Firewall:** EEPROM parameter structure is protected by an 8-bit XOR checksum and rigid mathematical boundaries. Protects the MCU from zero-division crashes even if corrupted data is injected.
* **Non-Blocking Boot Fallback:** 3-attempt zeroing retry on startup with OLED visual warning and lock-bypass fallback if the knob is touched during boot.
* **I2C Bus Recovery Guard:** 3 ms hardware timeout handling automatically clears I2C bus stalls caused by ESD or electrical noise.
* **CAD Viewport Isolation:** Kinematic output and HID button reports are automatically suppressed while navigating OLED menus or saving WebHID settings to prevent accidental wild camera spins.

---

## 🛠️ Hardware Requirements

* **Microcontroller:** ATmega32U4 (Arduino Pro Micro 5V / 16 MHz).
* **Sensors:** 8x Ratiometric Linear Hall-Effect Sensors (AH49E or equivalent).
* **Display:** 0.96" OLED SSD1306 (128x64 pixels, I2C address `0x3C`).
* **Buttons:** Up to 32 Tactile Push Buttons (wired with internal `INPUT_PULLUP`).

---

## 🔌 Default Pinout Configuration

| Component | Pin / Channel | Pro Micro Pin | Notes |
| :--- | :--- | :--- | :--- |
| **OLED SDA** | SDA | **2** | Hardware I2C |
| **OLED SCL** | SCL | **3** | Hardware I2C |
| **Hall Sensor HES0** | Signal | **A0** | South Pair (A) |
| **Hall Sensor HES1** | Signal | **A1** | South Pair (B) |
| **Hall Sensor HES2** | Signal | **A2** | East Pair (A) |
| **Hall Sensor HES3** | Signal | **A3** | East Pair (B) |
| **Hall Sensor HES6** | Signal | **4 (A6)** | North Pair (A) |
| **Hall Sensor HES7** | Signal | **6 (A7)** | North Pair (B) |
| **Hall Sensor HES8** | Signal | **8 (A8)** | West Pair (A) |
| **Hall Sensor HES9** | Signal | **9 (A9)** | West Pair (B) |
| **Button 1 (L / Back)** | Digital In | **1 (TX)** | Internal Pullup |
| **Button 2 (R / Confirm)**| Digital In | **0 (RX)** | Internal Pullup |

*(Note: Additional buttons can be wired to any remaining free digital pins and mapped easily via `config.h`)*

---

## 📜 License & Acknowledgments

* Special thanks to **[ardunnh](https://github.com/AndunHH)**, **TeachingTech**, and the broader open-source 3D SpaceMouse community for the foundational hardware and concept designs that inspired this firmware.
* Distributed under the **MIT License**. Free to use, modify, and distribute for personal and commercial projects.
