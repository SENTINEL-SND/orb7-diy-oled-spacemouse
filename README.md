# 🛸 O.R.B. 7 (Orbital Rotation Base + OLED)
### DIY 6DOF SpaceMouse Pro Firmware | Hall-Effect & OLED Interface

[![License: MIT](https://img.shields.io/badge/License-MIT-yellow.svg)](https://opensource.org/licenses/MIT)
[![Version](https://img.shields.io/badge/Firmware-Alpha_v0.0.1-blue.svg)]()
[![Hardware](https://img.shields.io/badge/MCU-ATmega32U4-red.svg)]()

> ⚠️ **Note from the Author:** I am a CAD & maker enthusiast, not a professional software engineer! I'm currently learning programming and built this project using AI tools to help bring my vision to life. If you find bugs, rough code, or non-standard practices—or if you don't even know how to use `git` (just like me!)—you're in good company. Feedback, cleanups, and Pull Requests are always welcome!

An ultra-optimized, high-performance firmware for DIY 6-DOF (Six Degrees of Freedom) 3D navigation controllers based on the **ATmega32U4** (Arduino Pro Micro). 

Emulating a native **3Dconnexion SpaceMouse Pro** USB HID device, **O.R.B. 7** combines an integer-only **Q7/Q8 fixed-point math engine**, a **zero-RAM-buffer OLED UI**, and **decoupled real-time drift compensation** to deliver a smooth, high-precision CAD navigation experience.

--- 


## 📖 The Story Behind O.R.B. 7

What started as a simple hobby project to enhance the fantastic [ardunnh](https://github.com/AndunHH)'s DIY SpaceMouse quickly turned into a two-month engineering rabbit hole. 

My initial goal was modest: add a small OLED display for a sleek, futuristic look and easier debugging. However, as I spent more time tweaking the system to meet my exact workflow needs, one optimization led to another. Hundreds of modifications later, the project evolved into a fully overhauled, production-ready firmware.

The OLED screen isn't just aesthetic—it fundamentally transforms how you use the device. **With O.R.B. 7, you can calibrate sensors, tweak deadzones, swap axes, and remap CAD shortcuts directly on the device.** You no longer need to dive into code or reflash your Arduino just to fine-tune your controller!

> **Project Status (Alpha v0.0.1):** 
> Development moved so fast that formal documentation is still catching up. Future updates will focus on modularizing the codebase for easier community hacking, but the core firmware is fully feature-complete, rock-solid, and ready for daily CAD work.

---

### 💡 Why "O.R.B. 7"?

* **O.R.B. (Orbital Rotation Base):** Represents the core mechanical behavior of the 3D controller. Just like orbiting a 3D model in CAD software, the knob pivots and rotates freely around a central 3D space.
* **7 (OLED):** Signifies the 7th major iteration of the hardware/firmware, marking the breakthrough integration of the OLED interface, real-time telemetry, and complete on-screen calibration.

---

## ✨ Key Features

### 📐 Core Kinematics & Math Engine
* **Fixed-Point Arithmetic (Q7/Q8):** The 6DOF motion vector path is 100% free of runtime floating-point operations, drastically reducing cycle latency on 8-bit AVR architecture.
* **Cached Trigonometric Curves:** Supports Linear, Squared, and Squared-Tangent response curves with static parameter caching to save thousands of CPU cycles per second.
* **Master Global Sensitivity & Per-Axis Noise Gates:** Symmetrical noise gates across all 6 axes eliminate parasitic axis crosstalk and accidental inputs.
* **Enhanced Exclusive Mode:** Features *Neutral Unlocking on Relaxation* (`RELAX_THRESHOLD`), allowing fluid switching between Translation and Rotation without needing to fully release the knob.

### 📺 Zero-RAM OLED Interface (`SSD1306AsciiWire`)
* **Direct-Write Driver (400 kHz I2C):** Renders graphics directly to the display without allocating a 1 KB SRAM frame buffer, preserving critical RAM on the ATmega32U4.
* **Real-Time 6DOF Visualizer:** High-refresh mini bar graphs displaying live Translation ($TX, TY, TZ$) and Rotation ($RX, RY, RZ$) outputs with zero screen flicker.
* **14-State On-Screen Configuration Menu:**
  * **Sensitivity & Curves:** Live adjustment of Global Sensitivity (%), Deadzone, and Curve parameters.
  * **Direction & Axis Swap:** Toggle axis inversion (`INV TX..RZ`) and axis swapping (`SWP XY`, `SWP YZ`).
  * **Dynamic Shortcut Remapping:** Assign hardware buttons to CAD functions (*FIT, TOP, RIGHT, FRONT, ESC, SHIFT, CTRL*, etc.).
  * **Diagnostics:** Sensor alignment diagnostic (`Align Sensors`), 20-second dynamic limits calibration (`Cal. Limits`), and live drift monitoring.
* **Smart Sleep Timer:** Inactivity timer (*1m, 3m, 5m, OFF*) with instant wake-up on movement or button press.

### ⚡ ADC Acceleration & Signal Processing
* **500 kHz ADC Clock:** Reconfigured prescaler reduces analog conversion time from ~52 µs to **~26 µs** per sample.
* **2x Oversampling & Integer EMA Filtering:** Non-blocking Exponential Moving Average filter ($\alpha = 0.25$) with non-stuck rounding logic for smooth, noise-free motion output.
* **Decoupled Per-Axis Drift Compensation:** Independent tracking channels per sensor pair with adaptive muting. Eliminates thermal zero-point drift in real time without blocking active movement.

### 🛡️ System Integrity & Reliability
* **8-bit XOR Checksum & Auto-Recovery:** EEPROM parameter structure protected by checksum verification; automatically restores safe factory defaults if data corruption is detected.
* **Non-Blocking Boot Fallback:** 3-attempt zeroing retry on startup with OLED visual warning and lock-bypass fallback if the knob is touched during boot.
* **I2C Bus Recovery Guard:** 3 ms hardware timeout handling automatically clears bus stalls caused by ESD or electrical noise.
* **CAD Viewport Isolation:** Kinematic output and HID button reports are automatically suppressed while navigating OLED menus.

---

## 🛠️ Hardware Requirements

* **Microcontroller:** ATmega32U4 (Arduino Pro Micro 5V / 16 MHz).
* **Sensors:** 8x Ratiometric Linear Hall-Effect Sensors (AH49E or equivalent).
* **Display:** 0.96" OLED SSD1306 (128x64 pixels, I2C address `0x3C`).
* **Buttons:** 2 to 4 Tactile Push Buttons (wired with internal `INPUT_PULLUP`).

---

## 🔌 Default Pinout Configuration

| Component | Pin / Channel | Pro Micro Pin | Notes |
| :--- | :--- | :--- | :--- |
| **OLED SDA** | SDA | **2** | Hardware I2C |
| **OLED SCL** | SCL | **3** | Hardware I2C |
| **Hall Sensor HES0** | Signal | **10** | South Pair (A) |
| **Hall Sensor HES1** | Signal | **A1** | South Pair (B) |
| **Hall Sensor HES2** | Signal | **A2** | East Pair (A) |
| **Hall Sensor HES3** | Signal | **A3** | East Pair (B) |
| **Hall Sensor HES6** | Signal | **4 (A6)** | North Pair (A) |
| **Hall Sensor HES7** | Signal | **6 (A7)** | North Pair (B) |
| **Hall Sensor HES8** | Signal | **8 (A8)** | West Pair (A) |
| **Hall Sensor HES9** | Signal | **9 (A9)** | West Pair (B) |
| **Button 1 (L / Back)** | Digital In | **1 (TX)** | Internal Pullup |
| **Button 2 (R / Confirm)**| Digital In | **0 (RX)** | Internal Pullup |
| **Button 3 (Optional)** | Digital In | **5** | Internal Pullup |
| **Button 4 (Optional)** | Digital In | **7** | Internal Pullup |

---

## 🚀 Building & Flashing

### Arduino IDE
1. Install the **Arduino AVR Boards** core.
2. Install the required dependencies via Library Manager:
   * [NicoHood HID Library](https://github.com/NicoHood/HID)
   * [SSD1306Ascii Library](https://github.com/greiman/SSD1306Ascii)
3. Select Board: **Arduino Leonardo** or **Arduino Micro**.
4. *(Optional)* Set `#define ENABLE_SERIAL_DEBUG 0` in `config.h` for maximum memory savings and peak performance.
5. Compile and Upload.

---

## 📊 Resource Usage (ATmega32U4)

* **Flash Memory:** ~21 KB / 28.672 bytes (~73% utilization).
* **Dynamic SRAM:** ~900 B / 2.560 bytes (~35% utilization, **>1.6 KB free RAM**).

---

## 📜 License & Acknowledgments

* Special thanks to **ardunnh**, **TeachingTech**, and the broader open-source 3D SpaceMouse community for the foundational hardware and concept designs that inspired this firmware.
* Distributed under the **MIT License**. Free to use, modify, and distribute for personal and commercial projects.
