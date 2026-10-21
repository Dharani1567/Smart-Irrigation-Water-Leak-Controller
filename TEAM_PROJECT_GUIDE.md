#  TEAM PROJECT STUDY & PRESENTATION GUIDE
## SMART IRRIGATION & WATER-LEAK CONTROLLER
**Course**: 23CSE304 – Embedded Systems  
**Institution**: Amrita Vishwa Vidyapeetham, Department of Computer Science and Engineering  

**Team Members**:
- **K Dharani Kumar** (`AM.SC.U4CSE24123`)
- **K Bhanu Sai Raj** (`AM.SC.U4CSE24125`)
- **M Surya Kiran** (`AM.SC.U4CSE24130`)
- **Ayman Jasim M R** (`AM.SC.U4CSE24163`)

---

## 1. Quick Project Summary

- **Target MCU**: STM32F103C8T6 (ARM Cortex-M3 @ 72 MHz)
- **Firmware**: Written in C using official ST HAL Drivers, compiled with PlatformIO / GCC ARM toolchain (**0 Errors, 0 Warnings**).
- **Simulation**: Wokwi VS Code Simulator (Complete hardware circuit with Blue Pill, 10k potentiometer, 16x2 LCD, flow pulse button, fault button, status LEDs, and buzzer).
- **Project Status**: **100% Completed in Software Simulation Phase**. Ready to flash onto physical hardware via ST-Link V2 without changing a single line of code.

---

## 2. Hardware Pin Mapping Reference Table

| STM32 Pin | Wokwi Pin | Peripheral / Signal | Function / Connected Component |
|---|---|---|---|
| **PA0** | `A0` | `ADC1_IN0` | Soil Moisture Sensor (10k Potentiometer 0-3.3V) |
| **PA1** | `A1` | `TIM2_CH2` | Water Pump Speed Drive (1 kHz PWM, 0-100% Duty Cycle) |
| **PA2** | `A2` | `EXTI2` | Water Flow Sensor Pulses (Blue Pushbutton Interrupt) |
| **PA3** | `A3` | `EXTI3` | Virtual Fault Injector / Alarm Reset Button (Red Pushbutton Interrupt) |
| **PA4–PA7**| `A4–A7` | `SPI1 Master` | Telemetry Data Output (CS, SCK, MISO, MOSI) |
| **PB0** | `B0` | GPIO Output | LCD Register Select (RS) |
| **PB1** | `B1` | GPIO Output | LCD Enable Pulse (EN) |
| **PB5** | `B5` | GPIO Output | **Green LED**: Normal Hydrated State ($\ge 50\%$) |
| **PB6** | `B6` | GPIO Output | **Yellow LED**: Irrigating State ($< 50\%$) / Lockout Cooldown |
| **PB7** | `B7` | GPIO Output | **Red LED**: Emergency Fault Alarm (Dry Run / Pipe Leak) |
| **PB8** | `B8` | GPIO Output | **Audible Buzzer**: Emergency Alarm Sounder |
| **PB12–PB15**| `B12–B15` | GPIO Output | LCD 4-bit Data Lines (D4, D5, D6, D7) |

---

## 3. How to Run the Simulation on Any Teammate's Laptop

1. **Install VS Code Extensions**:
   - Install **PlatformIO IDE** and **Wokwi Simulator** from VS Code Marketplace.
2. **Open Project Folder**:
   - Open the folder `Smart_Irrigation_Leak_Controller` in VS Code.
3. **Start Wokwi Simulation**:
   - Open `diagram.json`.
   - Press **`F1`** $\rightarrow$ select **`Wokwi: Start Simulator`** (or click the green **Play** button on the top left of the Wokwi canvas).

---

## 4. Live Demo 5-Step Action Checklist (What to Click during Viva)

| Step # | Action to Perform | Visual Result | What to Say to Professor |
|---|---|---|---|
| **1. Boot Up** | Start simulation | LCD displays `SMART IRRIGATION`, then `Mst: 50% Pmp:OFF`. **Green LED turns ON**. | *"System boots up, initializes HAL drivers, and enters IDLE_MONITORING state. Moisture is 50%, so pump is OFF."* |
| **2. Dry Soil** | Turn Potentiometer **below 50%** (e.g. 15%) | **Yellow LED turns ON**, Green turns OFF. LCD displays `Mst: 15% Pmp:ON`, `PWM: 78%`. | *"When moisture drops below 50%, FSM triggers pump activation with proportional PWM speed regulation based on moisture deficit."* |
| **3. Wet Soil** | Turn Potentiometer **above 50%** (e.g. 75%) | **Green LED turns ON**, Yellow turns OFF. LCD displays `Mst: 75% Pmp:OFF`, `PWM: 0%`. | *"When target hydration (50%) is restored, the pump automatically turns OFF and returns to monitoring."* |
| **4. Dry Run Fault** | Keep moisture $< 50\%$ (Pump ON) and **do not click Blue Button** for 3 seconds | **Red LED turns ON**, **Buzzer sounds**, LCD displays `ALARM: DRY RUN! PUMP NO WATER`. | *"If the pump is running but no water flow pulses are detected for 3 seconds, a Dry Run fault is raised to prevent motor burnout."* |
| **5. Pipe Leak Fault & Reset** | Click **Red Button** to reset, keep moisture $>50\%$ (Pump OFF), click **Blue Button** 2+ times quickly | **Red LED turns ON**, **Buzzer sounds**, LCD displays `ALARM: PIPE LEAK UNINTENDED FLOW!`. Click Red Button to clear. | *"If flow pulses occur while the pump is OFF, an unintended pipe leak is detected. Clicking the Red Button clears the fault via an EXTI3 interrupt."* |

---

## 5. Top 5 Faculty Viva Questions & Answers

### Q1: Why did you choose STM32F103C8T6 instead of Arduino Uno?
> **Answer**: *"The STM32 Blue Pill has a 32-bit ARM Cortex-M3 core operating at 72 MHz (vs 8-bit 16 MHz AVR), a 12-bit ADC (4096 levels vs 1024), hardware general-purpose timers (TIM2 PWM, TIM3 ISR), NVIC priority controller, and hardware SPI master peripherals."*

### Q2: How does your system avoid delay loops (`HAL_Delay`)?
> **Answer**: *"We configured hardware Timer `TIM3` to overflow every 100ms. Its interrupt service routine (`TIM3_IRQHandler`) calls `HAL_TIM_PeriodElapsedCallback()`, which periodically updates ADC moisture samples and advances a system millisecond tick non-blockingly."*

### Q3: How is PWM duty cycle calculated?
> **Answer**: *"Using proportional mapping in `PWM_Calculate_DutyFromMoisture()`: Duty decreases as moisture approaches 50%. At 0% moisture, PWM duty is 100% (maximum pump speed). At 50% moisture, PWM duty drops to 0% (pump stopped)."*

### Q4: How does leak detection work?
> **Answer**: *"Flow pulses are counted via `EXTI2` rising-edge interrupts on pin `PA2`. Every 1 second, `Leak_Update_1s_Tick()` evaluates flow rate:  
> 1. **Dry-Run Fault**: Pump ON & Flow $= 0$ for $>3\text{s} \rightarrow$ Emergency Shutdown (`LEAK_STATUS_DRY_RUN_ALARM`).  
> 2. **Pipe Leak Fault**: Pump OFF & Flow $\ge 2\text{ pps} \rightarrow$ Emergency Alert (`LEAK_STATUS_UNINTENDED_FLOW_ALARM`)."*

### Q5: Is this software ready for real physical hardware?
> **Answer**: *"Yes, 100%. We wrote official STM32 HAL C code compiled with `arm-none-eabi-gcc`. The generated `.bin` / `.elf` file can be flashed directly onto a physical STM32 Blue Pill board using ST-Link V2 without changing a single line of code."*
