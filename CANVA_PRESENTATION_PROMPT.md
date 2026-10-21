# AI PRESENTATION GENERATOR PROMPT (CANVA / GAMMA / CHATGPT)

> **Instructions**: Copy and paste the text block below directly into Canva Magic Design, Gamma App, or ChatGPT/SlidesAI to automatically generate your complete PowerPoint slide presentation.

***

### START COPYING FROM HERE:

Act as an expert presentation designer and embedded systems professor. Generate a professional 12-slide academic slide deck based on the following project details:

**Visual Style Theme**: Modern Tech Academic  
**Color Palette**: Deep Emerald Green (#0F5132), Tech Blue (#0D6EFD), Slate Gray (#343A40), Clean White (#FFFFFF)  
**Font Pairing**: Montserrat (Headings) + Open Sans (Body Text)  

---

#### SLIDE 1: Title Slide
- **Title**: Smart Irrigation & Water-Leak Controller
- **Subtitle**: An Event-Driven, Non-Blocking Resource Optimization & Pipeline Diagnostic System
- **Context**: 23CSE304 – Embedded Systems Project | Department of CSE | Amrita Vishwa Vidyapeetham
- **Presenters**: K Dharani Kumar (AM.SC.U4CSE24123), K Bhanu Sai Raj (AM.SC.U4CSE24125), M Surya Kiran (AM.SC.U4CSE24130), Ayman Jasim M R (AM.SC.U4CSE24163)
- **Target MCU**: STM32F103C8T6 (ARM Cortex-M3 @ 72 MHz)

---

#### SLIDE 2: Project Working Status
- **Headline**: 100% Completed & Validated in Software Simulation
- **Key Points**:
  - **Embedded C Firmware**: Built using ST HAL drivers with 0 compilation errors and 0 warnings.
  - **Virtual Hardware Circuit**: Full Wokwi schematic created with STM32 Blue Pill, Potentiometer, 16x2 LCD, LEDs, and Buttons.
  - **Hardware-Ready Code**: Generated binary (.elf / .bin) is ready to flash directly onto physical hardware via ST-Link V2.

---

#### SLIDE 3: Problem Statement
- **Headline**: The Crisis of Freshwater Wastage & Infrastructure Failure
- **Key Points**:
  - **Agricultural Wastage**: Traditional open-loop timers water crops regardless of soil moisture, wasting freshwater.
  - **Pump Motor Destruction**: Pumping water when tanks are empty (dry-running) overheats and burns out motor windings.
  - **Undetected Pipe Leaks**: Structural pipe cracks cause massive unmonitored water losses.
  - **Our Solution**: Closed-loop dynamic irrigation with real-time pipeline diagnostics on STM32 ARM Cortex-M3.

---

#### SLIDE 4: Objectives & UN SDG Alignment
- **Headline**: Sustainable Engineering Aligned with UN SDG 12
- **Key Points**:
  - **UN SDG 12 (Responsible Consumption)**: Optimizing water and electrical energy usage.
  - **12-Bit ADC Sampling**: Precision soil moisture acquisition via ADC1_IN0 (PA0).
  - **Proportional PWM Control**: Speed regulation of water pump using TIM2_CH2 (PA1).
  - **30-Second Lockout Protection**: TIM3 hardware timer ISR prevents over-irrigation.
  - **Pipeline Diagnostics**: EXTI pulse counting algorithm detecting dry-run & pipe leaks with SPI1 telemetry.

---

#### SLIDE 5: System Architecture & Block Diagram
- **Headline**: STM32 Blue Pill Core Signal Routing
- **Key Components**:
  - **Analog Input (PA0)**: Soil Moisture Potentiometer (0-3.3V)
  - **PWM Output (PA1)**: Water Pump Motor Speed Drive (1 kHz PWM)
  - **Interrupt Pulses (PA2)**: Hall-Effect Water Flow Sensor (EXTI2)
  - **Fault Button (PA3)**: Virtual Leak Trigger & System Reset (EXTI3)
  - **SPI1 Bus (PA4-PA7)**: Diagnostic Telemetry Transmission
  - **Display & Indicators (PB0-PB15)**: 16x2 LCD, Status LEDs (Green/Yellow/Red), Alarm Buzzer

---

#### SLIDE 6: Pin Mapping & Peripheral Setup
- **Headline**: STM32 Hardware Pin Allocation
- **Content Table**:
  - PA0 -> ADC1_IN0 (Soil Moisture Sensor)
  - PA1 -> TIM2_CH2 (1 kHz Pump PWM Drive)
  - PA2 -> EXTI2 (Flow Sensor Pulses)
  - PA3 -> EXTI3 (Fault Injector / Reset)
  - PA4-PA7 -> SPI1 Master Telemetry Bus
  - PB0, PB1 -> LCD RS & EN
  - PB5 -> Green LED (Normal State >= 50%)
  - PB6 -> Yellow LED (Irrigating State < 50%)
  - PB7 -> Red LED (Emergency Leak Alarm)
  - PB8 -> Audible Buzzer Alarm
  - PB12-PB15 -> LCD 4-Bit Data Lines

---

#### SLIDE 7: Software Engineering & Modular Drivers
- **Headline**: Clean, Layered C Firmware Architecture
- **Key Modules**:
  - `main.c`: SYSCLK 72 MHz configuration and main non-blocking event loop.
  - `adc.c`: 12-bit ADC sampling and linear 0-100% moisture scaling.
  - `pwm.c`: Proportional PWM duty calculation and TIM2 compare register updates.
  - `lcd.c`: HD44780 4-bit GPIO display driver.
  - `spi.c`: Formats 8-byte telemetry packets with XOR checksum.
  - `leak_detection.c`: Flow rate integration and anomaly detection.
  - `irrigation_controller.c`: Core Finite State Machine (FSM) engine.

---

#### SLIDE 8: Event-Driven FSM & Timer ISR Logic
- **Headline**: Non-Blocking Execution Flow
- **Key Points**:
  - **TIM3 100ms Base Timer**: Triggers non-blocking ADC sampling and advances system ticks without blocking delays (`HAL_Delay`).
  - **Idle State**: Moisture >= 50% -> Green LED ON, Pump OFF (0% PWM).
  - **Irrigating State**: Moisture < 50% -> Yellow LED ON, Pump ON (Proportional PWM speed).
  - **Lockout Cooldown**: 30-second max watering limit followed by 15-second cooldown (Yellow LED blinks).

---

#### SLIDE 9: Pipeline Leak Diagnostics Algorithm
- **Headline**: Real-Time Anomaly Detection
- **Key Scenarios**:
  - **Dry-Run Failure**: Pump is ON but Flow = 0 for 3 seconds -> Emergency Pump Shutdown, Red LED ON, Buzzer ON, LCD: `ALARM: DRY RUN!`.
  - **Unintended Pipe Leak**: Pump is OFF but Flow >= 2 pulses/sec -> Red LED ON, Buzzer ON, LCD: `ALARM: PIPE LEAK`.
  - **Manual Reset**: Pressing Red Button (PA3 EXTI3) clears active alerts and resets system.

---

#### SLIDE 10: Expected Outputs & Simulation Results
- **Headline**: Wokwi Simulation Verification
- **Visual Display Layouts**:
  - **Normal State**: `Mst: 75% Pmp:OFF` / `PWM:  0% Flow:OK` (Green LED ON)
  - **Irrigating State**: `Mst: 15% Pmp:ON ` / `PWM: 79% Flow:OK` (Yellow LED ON)
  - **Emergency Fault**: `ALARM: DRY RUN! ` / `PUMP NO WATER   ` (Red LED ON + Buzzer)

---

#### SLIDE 11: Real-World Applications & Impact
- **Headline**: Broad Deployment Potential
- **Use Cases**:
  - **Precision Agriculture**: Autonomous field irrigation saving up to 40% freshwater.
  - **Commercial Greenhouses**: Micro-climate hydration bounded by timer lockdowns.
  - **Municipal Public Parks**: Automated watering of public green spaces with leak alerts.
  - **Smart Home Automation**: Autonomous garden watering with pipe burst flood protection.

---

#### SLIDE 12: References & Technical Standards
- **Headline**: Authoritative References
- **Sources**:
  1. STMicroelectronics, "STM32F103xC/D/E Advanced ARM-based 32-bit MCUs Reference Manual," DocID13902.
  2. United Nations, "Sustainable Development Goal 12: Responsible Consumption and Production."
  3. ARM Limited, "Cortex-M3 Technical Reference Manual," Revision r2p1.
  4. Department of CSE, "23CSE304 – Embedded Systems Project Guidelines," Amrita Vishwa Vidyapeetham, 2026.

***
### END OF PROMPT
