# MASTER TECHNICAL GUIDE & VIVA DEFENSE MANUAL
## SMART IRRIGATION & WATER-LEAK CONTROLLER
**Target MCU**: STM32F103C8T6 (ARM Cortex-M3, 72 MHz)  
**Toolchain**: STM32 HAL / PlatformIO / GCC ARM (`arm-none-eabi-gcc`)  
**Simulation**: Wokwi VS Code Simulator  

---

# SECTION 1 — PROJECT OVERVIEW

### 1. What Problem This Project Solves
Freshwater mismanagement in agriculture and municipal infrastructure leads to massive water wastage, soil degradation, and high operational costs. Traditional irrigation relies on manual operation or open-loop timers that water crops regardless of actual soil moisture. Furthermore, pipeline leaks and pump dry-running go undetected, leading to motor burnout and structural flooding. This project provides an autonomous, closed-loop resource management system that dynamically irrigates based on real-time soil hydration while continuously verifying pipeline integrity.

### 2. Why Smart Irrigation is Needed
- **Water Conservation**: Agriculture accounts for ~70% of global freshwater withdrawals. Smart irrigation delivers water only when soil moisture drops below critical thresholds.
- **Crop Health Optimization**: Prevents both under-watering (crop wilting) and over-watering (root rot, nutrient leaching, soil hypoxia).
- **Energy Efficiency**: Minimizes pump running time, directly reducing electrical power consumption.
- **UN SDG 12 Alignment**: Contributes directly to United Nations Sustainable Development Goal 12 (Responsible Consumption and Production).

### 3. Why Leak Detection is Important
- **Prevents Catastrophic Water Losses**: A small pipeline crack or burst pipe can waste thousands of liters per hour.
- **Protects Pump Motors**: Operating a water pump when the supply tank is dry (dry-running) overheats and destroys the motor bearings and windings within minutes.
- **Detects Faulty Valves**: Identifies stuck-open solenoid valves or unauthorized water tapping when the pump is turned off.

### 4. Real-World Applications
- **Precision Agriculture & Smart Farming**: Automated field hydration scaled across vast crop sectors.
- **Commercial Greenhouses**: Micro-climate hydration control with strict watering window enforcement.
- **Urban & Municipal Infrastructure**: Automated irrigation for public parks, sports arenas, and roadside greenery.
- **Residential Smart Gardens**: Automated garden watering integrated with main supply line leak protection.

### 5. Benefits Over Traditional Systems

| Feature | Traditional Irrigation Systems | Our STM32 Smart Controller |
|---|---|---|
| **Control Logic** | Fixed timers / manual switches | Closed-loop feedback using 12-bit ADC |
| **Pump Speed** | Fixed ON/OFF (100% speed always) | Proportional PWM speed control (30%..100%) |
| **CPU Efficiency** | Continuous polling / heavy delay loops | Non-blocking hardware timer ISRs (`TIM3` 100ms tick) |
| **Safety Mechanisms** | None (runs indefinitely until manual stop) | 30-second continuous watering lockout timer |
| **Diagnostics** | None | Real-time flow integration for dry-run & pipe leaks |
| **Telemetry** | Local only | Packed 8-byte SPI master diagnostic frame output |

---

# SECTION 2 — SYSTEM ARCHITECTURE

```
                                +-------------------+
Soil Moisture (Pot) ----------> | PA0 (ADC1_IN0)    |
Water Pump PWM      <---------- | PA1 (TIM2_CH2)    |
Flow Sensor Pulses  ----------> | PA2 (EXTI2)       |
Virtual Fault Button ---------> | PA3 (EXTI3)       |
SPI1 Bus (CS/SCK/MOSI) <------ | PA4 - PA7 (SPI1)  | ---> SPI Telemetry
                                |                   |
LCD RS / EN         <---------- | PB0 / PB1         |
Status LEDs (G/Y/R) <---------- | PB5 / PB6 / PB7   |
Alarm Buzzer        <---------- | PB8               |
LCD Data (D4-D7)    <---------- | PB12 - PB15       |
                                +-------------------+
```

### 1. Detailed Peripheral & Module Roles

- **STM32F103C8T6 (ARM Cortex-M3 Core)**: Main processing engine running at 72 MHz (HSE 8 MHz + PLL x9). Orchestrates hardware peripherals, executes the Finite State Machine (FSM), and manages real-time interrupt handling.
- **ADC (12-Bit Resolution)**: `ADC1_IN0` on pin `PA0`. Converts analog sensor voltage (0..3.3V) into a 12-bit integer (0..4095), which is converted linearly into a 0..100% moisture percentage.
- **Soil Moisture Sensor**: Simulated via a 10k potentiometer delivering 0..3.3V to `PA0`.
- **Water Flow Sensor**: YF-S201 Hall-effect flow meter simulated via a push-button generating digital pulses on `PA2`. Each pulse triggers an external interrupt (`EXTI2`) to count fluid volume.
- **PWM Generator (`TIM2_CH2`)**: Timer 2 Channel 2 on pin `PA1` outputs a 1 kHz PWM waveform. Duty cycle is dynamically calculated based on moisture deficit (0..100%).
- **SPI Communication (`SPI1 Master`)**: Pins `PA4` (CS), `PA5` (SCK), `PA6` (MISO), `PA7` (MOSI). Sends an 8-byte diagnostic telemetry frame to an external logger/gateway every 1 second.
- **Hardware Timers & Interrupts**:
  - `TIM3`: Generates a 100ms periodic hardware interrupt tick for non-blocking state updates and lockout timing.
  - `EXTI2`: Handles rising-edge interrupts from flow meter pulses on `PA2`.
  - `EXTI3`: Handles falling-edge interrupts from the manual fault trigger button on `PA3`.
- **GPIO & Display Interfaces**:
  - `PB0, PB1, PB12..PB15`: 4-bit parallel GPIO interface for HD44780 16x2 LCD.
  - `PB5` (Green LED), `PB6` (Yellow LED), `PB7` (Red LED), `PB8` (Buzzer).

---

# SECTION 3 — FILE-BY-FILE EXPLANATION

### 1. `main.c`
- **Purpose**: System entry point, clock configuration (72 MHz), hardware peripheral initializations, and main non-blocking event loop execution.
- **Inputs**: Hardware reset vector, peripheral status flags.
- **Outputs**: Initialized hardware handles (`hadc1`, `htim2`, `htim3`, `hspi1`).
- **Key Functions**:
  - `main()`: Orchestrates startup sequence and executes `Irrigation_FSM_Process()` inside `while(1)`.
  - `SystemClock_Config()`: Configures 8 MHz HSE crystal to 72 MHz PLL core clock.
  - `MX_ADC1_Init()`, `MX_TIM2_Init()`, `MX_TIM3_Init()`, `MX_SPI1_Init()`, `MX_GPIO_Init()`.
  - `HAL_TIM_MspPostInit()`: Configures GPIO `PA1` as Alternate Function Push-Pull for TIM2 PWM output.
- **Module Interactions**: Calls initialization functions of `lcd.c`, `adc.c`, `pwm.c`, `spi.c`, `leak_detection.c`, and `irrigation_controller.c`. Starts `TIM3` base interrupts.

### 2. `adc.c` / `adc.h`
- **Purpose**: Manages 12-bit ADC sampling of the soil moisture potentiometer on `PA0`.
- **Inputs**: Analog voltage on `PA0` (`ADC_CHANNEL_0`).
- **Outputs**: 12-bit raw reading (0..4095), converted moisture percentage (0..100%).
- **Key Functions**:
  - `ADC_Moisture_Init()`: Initializes moisture reading buffers.
  - `ADC_Moisture_ReadRaw()`: Polls `hadc1` for conversion completion.
  - `ADC_Moisture_GetPercentage()`: Returns current calculated moisture %.
  - `ADC_Moisture_UpdateSample()`: Performs linear conversion: `Pct = (Raw * 100) / 4095`.
- **Module Interactions**: Called by `interrupt_handlers.c` during `TIM3` 100ms ISR to sample moisture non-blockingly.

### 3. `pwm.c` / `pwm.h`
- **Purpose**: Controls water pump motor speed via TIM2 Channel 2 PWM on pin `PA1`.
- **Inputs**: Desired duty cycle percentage (0..100%) or soil moisture level.
- **Outputs**: 1 kHz PWM signal with variable duty cycle on `PA1`.
- **Key Functions**:
  - `PWM_Pump_Init()`: Starts `TIM2_CH2` PWM mode.
  - `PWM_Pump_SetDuty(duty_pct)`: Updates `TIM2->CCR2` compare register.
  - `PWM_Pump_Stop()`: Sets PWM duty cycle to 0%.
  - `PWM_Calculate_DutyFromMoisture(moisture_pct)`: Inverse proportional mapping formula.
- **Module Interactions**: Called by `irrigation_controller.c` to update pump speed or force emergency shutdown.

### 4. `spi.c` / `spi.h`
- **Purpose**: Transmits system telemetry frames over hardware SPI1 master interface.
- **Inputs**: Moisture %, Pump status, PWM duty %, Flow rate, Leak status, System FSM state.
- **Outputs**: 8-byte formatted SPI packet sent over MOSI (`PA7`) with CS assertion (`PA4`).
- **Key Functions**:
  - `SPI_Telemetry_Init()`: Sets CS pin high.
  - `SPI_Telemetry_Send(...)`: Packages data into `TelemetryFrame_t`, computes XOR checksum, pulls CS low, calls `HAL_SPI_Transmit()`, and releases CS high.
- **Module Interactions**: Called every 1 second by `irrigation_controller.c`.

### 5. `irrigation_controller.c` / `irrigation_controller.h`
- **Purpose**: Implements the main Finite State Machine (FSM), lockout timer logic, and LCD/LED updates.
- **Inputs**: Soil moisture %, Leak status, 100ms timer ticks.
- **Outputs**: FSM state changes, LED states (`PB5`–`PB7`), Buzzer state (`PB8`), LCD dashboard updates.
- **Key Functions**:
  - `Irrigation_Init()`: Sets initial state to `SYSTEM_STATE_IDLE_MONITORING`.
  - `Irrigation_Timer_100ms_Tick()`: Increments millisecond counter and sets 1-second flag.
  - `Irrigation_FSM_Process()`: Main non-blocking FSM evaluator executed in `while(1)`.
  - `Update_LED_Indicators()`: Toggles Green, Yellow, and Red LEDs based on active FSM state.
- **Module Interactions**: Central orchestrator connecting `adc.c`, `pwm.c`, `lcd.c`, `spi.c`, and `leak_detection.c`.

### 6. `leak_detection.c` / `leak_detection.h`
- **Purpose**: Calculates water flow rate and detects dry-run and pipe leak anomalies.
- **Inputs**: Flow pulses from `EXTI2` interrupt (`PA2`), pump active flag from PWM driver.
- **Outputs**: Calculated flow rate (L/min), `LeakStatus_t` enum (`LEAK_STATUS_OK`, `DRY_RUN_ALARM`, `UNINTENDED_FLOW_ALARM`).
- **Key Functions**:
  - `Leak_OnPulseReceived()`: Increments global pulse counter on `EXTI2` interrupt.
  - `Leak_Update_1s_Tick()`: Evaluates pulse frequency (`Q = Pulses / 7.5`) and evaluates fault conditions.
  - `Leak_GetStatus()`, `Leak_InjectFault()`, `Leak_ClearAlarm()`.
- **Module Interactions**: Receives pulse callbacks from `interrupt_handlers.c` and reports fault state to `irrigation_controller.c`.

### 7. `interrupt_handlers.c`
- **Purpose**: Application-level callbacks for hardware timer elapsed interrupts and GPIO EXTI interrupts.
- **Inputs**: `TIM3` interrupt events, `EXTI2` (PA2) and `EXTI3` (PA3) edge triggers.
- **Outputs**: Non-blocking updates to ADC sampling, timer ticks, pulse counters, and fault flags.
- **Key Functions**:
  - `HAL_TIM_PeriodElapsedCallback()`: Triggered every 100ms by `TIM3`; calls `ADC_Moisture_UpdateSample()` and `Irrigation_Timer_100ms_Tick()`.
  - `HAL_GPIO_EXTI_Callback()`: Routes `PA2` rising edge to `Leak_OnPulseReceived()` and `PA3` falling edge to manual fault trigger/reset.
- **Module Interactions**: Connects STM32 HAL hardware vector handlers (`stm32f1xx_it.c`) to module application code.

### 8. `stm32f1xx_it.c` / `stm32f1xx_it.h`
- **Purpose**: Contains physical interrupt vector handler routines generated by STM32Cube.
- **Inputs**: Hardware interrupt vectors (`SysTick`, `TIM3_IRQn`, `EXTI2_IRQn`, `EXTI3_IRQn`).
- **Outputs**: Passes interrupt events to ST HAL drivers (`HAL_TIM_IRQHandler`, `HAL_GPIO_EXTI_IRQHandler`).
- **Key Functions**:
  - `SysTick_Handler()`: Increments HAL timebase tick.
  - `TIM3_IRQHandler()`: Handles Timer 3 interrupt requests.
  - `EXTI2_IRQHandler()`, `EXTI3_IRQHandler()`: Handles GPIO line 2 & 3 interrupts.
- **Module Interactions**: Interfaces directly with Cortex-M3 NVIC vector table.

---

# SECTION 4 — CODE WALKTHROUGH (POWER-ON TRACE)

1. **Power-On & MCU Startup**:
   - Power applied to STM32F103C8T6. Cortex-M3 executes `Reset_Handler` from Flash memory.
   - Stack Pointer (SP) initialized, `.data` section copied to SRAM, `.bss` section zero-initialized.
   - `main()` is invoked.

2. **System Initialization**:
   - `HAL_Init()` resets peripherals and configures 1 ms SysTick.
   - `SystemClock_Config()` sets HSE 8 MHz crystal through PLL x9 multiplier to achieve 72 MHz SYSCLK.
   - Hardware initializations execute: `MX_GPIO_Init()`, `MX_ADC1_Init()`, `MX_TIM2_Init()`, `MX_TIM3_Init()`, `MX_SPI1_Init()`.
   - Driver initializations execute: `LCD_Init()`, `ADC_Moisture_Init()`, `PWM_Pump_Init()`, `SPI_Telemetry_Init()`, `Leak_Init()`, `Irrigation_Init()`.
   - `LCD` displays splash text `SMART IRRIGATION & LEAK CNTRL` for 1.5 seconds.
   - `HAL_TIM_Base_Start_IT(&htim3)` starts Timer 3 periodic interrupts at 100ms intervals.

3. **Sensor Reading (100ms Periodic Cycle)**:
   - Every 100ms, `TIM3` hardware counter overflows, triggering `TIM3_IRQHandler` -> `HAL_TIM_PeriodElapsedCallback()`.
   - Callback calls `ADC_Moisture_UpdateSample()`, triggering a 12-bit ADC conversion on `PA0`. The converted value updates global moisture percentage.
   - Callback calls `Irrigation_Timer_100ms_Tick()`, advancing the system millisecond tick. Every 10 ticks (1 second), `g_sec_tick_flag` is set.

4. **Decision Making & State Machine Execution**:
   - Main loop repeatedly invokes `Irrigation_FSM_Process()`.
   - Read current moisture % and leak status.
   - **If Moisture < 50%**: State transitions to `SYSTEM_STATE_IRRIGATING`. **Yellow LED turns ON**, Green LED turns OFF.
   - **If Moisture >= 50%**: State transitions to `SYSTEM_STATE_IDLE_MONITORING`. **Green LED turns ON**, Yellow LED turns OFF.

5. **Motor & Pump Control**:
   - In `SYSTEM_STATE_IRRIGATING`: Calculates proportional duty cycle via `PWM_Calculate_DutyFromMoisture()`.
   - `PWM_Pump_SetDuty(target_duty)` updates `TIM2->CCR2` compare register, adjusting 1 kHz PWM signal output on `PA1`.
   - In `SYSTEM_STATE_IDLE_MONITORING`: `PWM_Pump_Stop()` sets duty cycle to 0%.

6. **Leak Detection & Pulse Integration**:
   - When water flows through flow meter, rising edges on `PA2` trigger `EXTI2_IRQHandler` -> `Leak_OnPulseReceived()`, incrementing `g_pulse_counter`.
   - When `g_sec_tick_flag` is set (every 1 sec), `Leak_Update_1s_Tick()` executes:
     - Calculates flow rate in L/min (`Q = Pulses / 7.5`).
     - **Dry Run Check**: If Pump is ON but Pulses = 0 for >3s -> set `LEAK_STATUS_DRY_RUN_ALARM`.
     - **Pipe Leak Check**: If Pump is OFF but Pulses >= 2 pps -> set `LEAK_STATUS_UNINTENDED_FLOW_ALARM`.

7. **Alarm Generation & Emergency Shutdown**:
   - If leak fault is detected, FSM transitions to `SYSTEM_STATE_FAULT_DRY_RUN` or `SYSTEM_STATE_FAULT_UNINTENDED_LEAK`.
   - `PWM_Pump_Stop()` executes immediately (emergency pump shutdown).
   - **Red LED turns ON (`PB7`)**, **Buzzer turns ON (`PB8`)**, LCD displays emergency alarm string.
   - Pressing Red Button (`PA3`) triggers `EXTI3_IRQHandler`, clearing alarm and resetting state to `SYSTEM_STATE_IDLE_MONITORING`.

8. **Loop Repetition & Telemetry Transmission**:
   - Every 1 second, `SPI_Telemetry_Send()` constructs an 8-byte packed telemetry packet and transmits it over SPI1 (`PA4`–`PA7`).
   - `LCD_Display_SystemStatus()` updates 16x2 LCD display.
   - Event loop repeats continuously.

---

# SECTION 5 — PRESENTATION SCRIPTS

### 2-Minute Presentation Script (Quick Elevator Pitch)
> *"Respected Faculty, our project is a **Smart Irrigation and Water-Leak Controller** implemented on the **STM32F103C8T6 ARM Cortex-M3** microcontroller using STM32 HAL drivers and validated in full hardware simulation on Wokwi.
> 
> Conventional irrigation systems waste water by using fixed timer schedules or continuous delay polling loops that overheat pump motors. Our project solves this by introducing a closed-loop, non-blocking Finite State Machine driven by hardware timer interrupts (`TIM3` 100ms tick).
> 
> The system reads soil moisture dynamically using the MCU's internal 12-bit ADC (`PA0`). When moisture falls below 50%, the state machine activates the pump and regulates its speed proportionally using 1 kHz PWM (`TIM2_CH2`). To prevent soil over-saturation, a 30-second watering lockout safety timer is enforced.
> 
> Concurrently, a flow sensor pulse integration algorithm on pin `PA2` (`EXTI2`) detects dry-run pump failures and structural pipe bursts, immediately triggering an emergency pump shutdown, illuminating a Red LED, sounding a buzzer, and transmitting an 8-byte diagnostic telemetry frame over SPI master (`SPI1`). This provides an end-to-end resource optimization solution aligned with UN Sustainable Development Goal 12."*

---

### 5-Minute Presentation Script (Standard Viva Presentation)
> *"Good morning Respected Faculty. Today we present our project: **Smart Irrigation and Water-Leak Controller**, developed by K Dharani Kumar, K Bhanu Sai Raj, M Surya Kiran, and Ayman Jasim M R under the Embedded Systems course 23CSE304.
> 
> **Problem & Motivation**: Freshwater mismanagement in agriculture leads to massive resource wastage and crop damage. Furthermore, pumps running without water experience motor burnout, while pipeline bursts often go unnoticed. Traditional systems use crude delay polling or fixed open-loop schedules.
> 
> **System Architecture**: Our solution utilizes the STM32F103C8T6 ARM Cortex-M3 core operating at 72 MHz. We implemented a completely non-blocking architecture where no `HAL_Delay` calls are used in the main loop. Instead, `TIM3` generates 100ms hardware interrupts to trigger ADC sampling and state updates.
> 
> **Key Peripherals & Methodology**:
> 1. **ADC Moisture Sampling**: Analog soil moisture is sampled via `ADC1_IN0` on pin `PA0`. The 12-bit raw value (0..4095) is mapped to 0..100% moisture.
> 2. **PWM Pump Control**: When moisture drops below 50%, the system enters `SYSTEM_STATE_IRRIGATING`, lighting a **Yellow LED** and driving a 1 kHz PWM signal on `PA1` (`TIM2_CH2`). The duty cycle is inversely proportional to moisture deficit—drier soil results in faster pumping.
> 3. **Lockout Protection**: If irrigation runs continuously for 30 seconds, a hardware lockout timer stops the pump and initiates a 15-second cooldown period, preventing flooding or root hypoxia.
> 4. **Leak Diagnostics**: We monitor flow meter pulses on pin `PA2` via `EXTI2` rising-edge interrupts. If the pump is ON but zero flow is detected for 3 seconds, a **Dry Run Fault** is declared. If the pump is OFF but flow is detected, a **Pipe Leak Fault** is declared. Both faults immediately shut down the pump, light a **Red LED**, sound a buzzer, and display an alert on the 16x2 LCD.
> 5. **SPI Telemetry**: Every second, an 8-byte diagnostic packet containing moisture, pump state, PWM duty, flow rate, and XOR checksum is transmitted over `SPI1` (`PA4`–`PA7`).
> 
> We have fully compiled this project with 0 errors using PlatformIO and validated all component responses live in Wokwi simulator. Thank you, and we welcome your questions."*

---

### 10-Minute Presentation Script (In-Depth Technical Defense)
*(Use the 5-minute script above for background, then append the detailed Code Trace from Section 4 and walk the faculty through the live Wokwi demo scenarios in Section 8 step-by-step).*

---

# SECTION 6 — FACULTY VIVA QUESTIONS & ANSWERS

### Basic Questions

**Q1: Why did you choose the STM32F103C8T6 over an Arduino Uno (ATmega328P)?**
> **Answer**: *"The STM32F103C8T6 offers a 32-bit ARM Cortex-M3 core operating at 72 MHz compared to the 8-bit 16 MHz AVR core on Arduino. It features a 12-bit ADC (4096 levels vs 1024 levels), multiple hardware general-purpose timers (TIM2, TIM3), nested vectored interrupt controller (NVIC), and hardware SPI master peripherals, enabling high-speed, non-blocking real-time processing."*

**Q2: What is the purpose of the 12-bit ADC in your project?**
> **Answer**: *"The 12-bit ADC (`ADC1_IN0` on PA0) samples the analog voltage output from the soil moisture sensor. It provides 4096 discrete quantization steps (0..3.3V), allowing high precision when converting raw voltage readings into a 0..100% soil hydration percentage."*

**Q3: What do the three LED colors represent?**
> **Answer**: 
> - **Green LED (`PB5`)**: Normal State (Moisture >= 50%, Pump OFF, system healthy).
> - **Yellow LED (`PB6`)**: Active Irrigation (Moisture < 50%, Pump running under PWM control) or blinking during Lockout Cooldown.
> - **Red LED (`PB7`)**: Emergency Fault Alarm (Dry Run Pump Failure or Unintended Pipe Burst detected).

---

### Intermediate Questions

**Q4: How does your code avoid using blocking delays like `HAL_Delay()`?**
> **Answer**: *"We configured hardware Timer `TIM3` to overflow every 100ms. Its interrupt service routine (`TIM3_IRQHandler`) calls `HAL_TIM_PeriodElapsedCallback()`, which periodically updates ADC moisture samples and advances a system millisecond counter. The main loop executes a non-blocking Finite State Machine (`Irrigation_FSM_Process()`) that checks time flags instead of stalling CPU execution."*

**Q5: How is the PWM duty cycle calculated relative to soil moisture?**
> **Answer**: *"We use an inverse proportional formula in `PWM_Calculate_DutyFromMoisture()`:  
> `Duty = PWM_DUTY_MAX - [ Moisture% * (PWM_DUTY_MAX - PWM_DUTY_MIN) / THRESHOLD ]`  
> When moisture is 0%, duty cycle is 100% (maximum pump speed). As moisture approaches 50%, duty cycle decreases toward 30% before stopping at 0% when moisture reaches 50%."*

**Q6: What algorithm is used to detect a Dry-Run fault versus a Pipe Burst fault?**
> **Answer**: *"Flow pulses are counted via `EXTI2` rising-edge interrupts on pin `PA2`. Every 1 second, `Leak_Update_1s_Tick()` evaluates flow rate:  
> 1. **Dry-Run Fault**: If Pump is ON (PWM > 0) AND Flow Pulses = 0 for 3 consecutive seconds -> Trigger `LEAK_STATUS_DRY_RUN_ALARM`.  
> 2. **Pipe Burst Fault**: If Pump is OFF (PWM = 0) AND Flow Pulses >= 2 pps -> Trigger `LEAK_STATUS_UNINTENDED_FLOW_ALARM`."*

---

### Advanced Questions

**Q7: How does the SPI telemetry packet structure guarantee data integrity?**
> **Answer**: *"We transmit a packed 8-byte structure (`TelemetryFrame_t`) over hardware `SPI1`. The first byte is a fixed sync header (`0xAA`). Bytes 1 through 6 contain Moisture %, Pump Status, PWM Duty %, Flow Rate, Leak Status, and System State ID. Byte 7 is a calculated XOR checksum over the preceding 7 bytes. The receiving node calculates the XOR sum of received bytes and compares it against byte 7; any mismatch indicates bus corruption."*

**Q8: What happens if an EXTI interrupt occurs during SPI transmission?**
> **Answer**: *"In our NVIC priority configuration, `FLOW_SENSOR_IRQn` (EXTI2) is assigned Preemption Priority 1, `TIM3_IRQn` is Priority 2, and `FAULT_INJECT_IRQn` (EXTI3) is Priority 3. Because flow pulse counting takes only 2 CPU cycles (incrementing a volatile integer), the EXTI ISR executes instantly without corrupting the SPI DMA/polling byte stream."*

**Q9: Why did you configure the ADC clock prescaler to `/6` in `SystemClock_Config()`?**
> **Answer**: *"The maximum allowable ADC clock frequency according to the STM32F103 datasheet is 14 MHz. With SYSCLK at 72 MHz and APB2 at 72 MHz, selecting `RCC_ADCPCLK2_DIV6` yields 72 MHz / 6 = 12 MHz, which is safely within the 14 MHz specification."*

---

# SECTION 7 — WOKWI SIMULATION & SCHEMATIC CONFIGURATION

### 1. Components & Hardware Pin Mapping Table

| Wokwi Part Type | Wokwi ID | Wokwi Pin | STM32 Blue Pill Pin | Peripheral / Function |
|---|---|---|---|---|
| `board-stm32-bluepill` | `mcu` | `A0` | `PA0` | ADC1_IN0 Soil Moisture Input |
| `board-stm32-bluepill` | `mcu` | `A1` | `PA1` | TIM2_CH2 PWM Pump Output |
| `board-stm32-bluepill` | `mcu` | `A2` | `PA2` | EXTI2 Flow Sensor Pulse Input |
| `board-stm32-bluepill` | `mcu` | `A3` | `PA3` | EXTI3 Fault Injector / Reset Button |
| `board-stm32-bluepill` | `mcu` | `A4` | `PA4` | SPI1_NSS Telemetry Chip Select |
| `board-stm32-bluepill` | `mcu` | `A5` | `PA5` | SPI1_SCK Telemetry Clock |
| `board-stm32-bluepill` | `mcu` | `A6` | `PA6` | SPI1_MISO Telemetry Data In |
| `board-stm32-bluepill` | `mcu` | `A7` | `PA7` | SPI1_MOSI Telemetry Data Out |
| `board-stm32-bluepill` | `mcu` | `B0` | `PB0` | LCD Register Select (RS) |
| `board-stm32-bluepill` | `mcu` | `B1` | `PB1` | LCD Enable Pulse (EN) |
| `board-stm32-bluepill` | `mcu` | `B5` | `PB5` | Green LED (Normal State) |
| `board-stm32-bluepill` | `mcu` | `B6` | `PB6` | Yellow LED (Irrigation / Lockout) |
| `board-stm32-bluepill` | `mcu` | `B7` | `PB7` | Red LED (Leak Fault Alarm) |
| `board-stm32-bluepill` | `mcu` | `B8` | `PB8` | Audible Alarm Buzzer |
| `board-stm32-bluepill` | `mcu` | `B12..B15`| `PB12..PB15`| LCD 4-bit Data Lines (D4..D7) |

### 2. Wokwi `diagram.json` File

```json
{
  "version": 1,
  "author": "Academic STM32 Embedded Systems Team",
  "editor": "wokwi",
  "parts": [
    {
      "type": "board-stm32-bluepill",
      "id": "mcu",
      "top": 0,
      "left": 0,
      "attrs": {}
    },
    {
      "type": "wokwi-potentiometer",
      "id": "pot_moisture",
      "top": -120,
      "left": -180,
      "attrs": { "label": "Soil Moisture Sensor (0-100%)" }
    },
    {
      "type": "wokwi-lcd1602",
      "id": "lcd",
      "top": -180,
      "left": 120,
      "attrs": { "pins": "4bit" }
    },
    {
      "type": "wokwi-pushbutton",
      "id": "btn_flow",
      "top": 150,
      "left": -180,
      "attrs": { "color": "blue", "label": "Flow Sensor Pulses" }
    },
    {
      "type": "wokwi-pushbutton",
      "id": "btn_fault",
      "top": 240,
      "left": -180,
      "attrs": { "color": "red", "label": "Virtual Fault / Reset" }
    },
    {
      "type": "wokwi-led",
      "id": "led_green",
      "top": 150,
      "left": 120,
      "attrs": { "color": "green", "label": "Normal OK" }
    },
    {
      "type": "wokwi-led",
      "id": "led_yellow",
      "top": 150,
      "left": 180,
      "attrs": { "color": "yellow", "label": "Irrigating" }
    },
    {
      "type": "wokwi-led",
      "id": "led_red",
      "top": 150,
      "left": 240,
      "attrs": { "color": "red", "label": "Leak Alarm" }
    },
    {
      "type": "wokwi-buzzer",
      "id": "buzzer",
      "top": 240,
      "left": 200,
      "attrs": { "label": "Alarm Buzzer" }
    }
  ],
  "connections": [
    [ "mcu:3V3.1", "pot_moisture:VCC", "red", [ "v0" ] ],
    [ "mcu:GND.1", "pot_moisture:GND", "black", [ "v0" ] ],
    [ "pot_moisture:SIG", "mcu:A0", "green", [ "v0" ] ],

    [ "mcu:GND.1", "lcd:VSS", "black", [ "v0" ] ],
    [ "mcu:3V3.1", "lcd:VDD", "red", [ "v0" ] ],
    [ "mcu:GND.1", "lcd:RW", "black", [ "v0" ] ],
    [ "mcu:B0", "lcd:RS", "blue", [ "v0" ] ],
    [ "mcu:B1", "lcd:E", "blue", [ "v0" ] ],
    [ "mcu:B12", "lcd:D4", "purple", [ "v0" ] ],
    [ "mcu:B13", "lcd:D5", "purple", [ "v0" ] ],
    [ "mcu:B14", "lcd:D6", "purple", [ "v0" ] ],
    [ "mcu:B15", "lcd:D7", "purple", [ "v0" ] ],

    [ "mcu:3V3.1", "btn_flow:1.A", "red", [ "v0" ] ],
    [ "btn_flow:2.A", "mcu:A2", "orange", [ "v0" ] ],

    [ "mcu:GND.1", "btn_fault:2.A", "black", [ "v0" ] ],
    [ "btn_fault:1.A", "mcu:A3", "grey", [ "v0" ] ],

    [ "mcu:B5", "led_green:A", "green", [ "v0" ] ],
    [ "mcu:GND.1", "led_green:C", "black", [ "v0" ] ],

    [ "mcu:B6", "led_yellow:A", "yellow", [ "v0" ] ],
    [ "mcu:GND.1", "led_yellow:C", "black", [ "v0" ] ],

    [ "mcu:B7", "led_red:A", "red", [ "v0" ] ],
    [ "mcu:GND.1", "led_red:C", "black", [ "v0" ] ],

    [ "mcu:B8", "buzzer:1", "red", [ "v0" ] ],
    [ "mcu:GND.1", "buzzer:2", "black", [ "v0" ] ]
  ],
  "surroundings": { "color": "gray" }
}
```

---

# SECTION 8 — DEMO SCENARIOS

### Scenario 1: Power-On & Normal Hydrated Soil
- **Inputs**: Potentiometer at 75% (Voltage ~2.47V).
- **STM32 Response**: FSM enters `SYSTEM_STATE_IDLE_MONITORING`. ADC reads raw 3071 -> 75%. `PWM_Pump_Stop()` executes (0% PWM).
- **Expected Outputs**:
  - LCD Line 1: `Mst: 75% Pmp:OFF`
  - LCD Line 2: `PWM:  0% Flow:OK`
  - LEDs: **Green LED ON**, Yellow LED OFF, Red LED OFF. Buzzer OFF.

### Scenario 2: Dry Soil (Irrigation Triggered)
- **Inputs**: Potentiometer turned down to 15% (Voltage ~0.49V).
- **STM32 Response**: Moisture < 50%. FSM transitions to `SYSTEM_STATE_IRRIGATING`. Computes proportional PWM: `Duty = 100 - (15 * 70 / 50) = 79%`. TIM2_CH2 outputs 79% PWM.
- **Expected Outputs**:
  - LCD Line 1: `Mst: 15% Pmp:ON `
  - LCD Line 2: `PWM: 79% Flow:OK`
  - LEDs: **Yellow LED ON**, Green LED OFF, Red LED OFF. Buzzer OFF.

### Scenario 3: Target Moisture Reached (Auto-Stop)
- **Inputs**: Potentiometer turned up to 60%.
- **STM32 Response**: Moisture >= 50%. FSM transitions to `SYSTEM_STATE_IDLE_MONITORING`. `PWM_Pump_Stop()` sets 0% PWM.
- **Expected Outputs**:
  - LCD Line 1: `Mst: 60% Pmp:OFF`
  - LCD Line 2: `PWM:  0% Flow:OK`
  - LEDs: **Green LED ON**, Yellow LED OFF, Red LED OFF.

### Scenario 4: Unintended Pipe Leak / Valve Burst
- **Inputs**: Moisture at 60% (Pump OFF). User clicks Blue Flow Button 3 times in 1 second.
- **STM32 Response**: EXTI2 triggers 3 pulse interrupts. `Leak_Update_1s_Tick()` detects Pump OFF and Pulses >= 2 pps. Sets `LEAK_STATUS_UNINTENDED_FLOW_ALARM`. FSM transitions to `SYSTEM_STATE_FAULT_UNINTENDED_LEAK`.
- **Expected Outputs**:
  - LCD Line 1: `ALARM: PIPE LEAK`
  - LCD Line 2: `UNINTENDED FLOW!`
  - LEDs: **Red LED ON**, Buzzer ON, Green LED OFF, Yellow LED OFF.

### Scenario 5: Dry Run Pump Failure / Blocked Pipe
- **Inputs**: Moisture at 15% (Pump ON). User does NOT press Blue Flow Button for 3 seconds.
- **STM32 Response**: Pump ON but Pulses = 0 for >3s. Sets `LEAK_STATUS_DRY_RUN_ALARM`. FSM transitions to `SYSTEM_STATE_FAULT_DRY_RUN`. `PWM_Pump_Stop()` shuts down pump.
- **Expected Outputs**:
  - LCD Line 1: `ALARM: DRY RUN! `
  - LCD Line 2: `PUMP NO WATER   `
  - LEDs: **Red LED ON**, Buzzer ON, Pump forced OFF (0% PWM).

---

# SECTION 9 — DEBUGGING & TROUBLESHOOTING GUIDE

| Problem / Error | Cause | Solution |
|---|---|---|
| **Linker Error**: `undefined reference to 'main'` | PlatformIO standard `src_dir` defaults to `src/`, ignoring `Core/Src/`. | Add `[platformio]` section to `platformio.ini`: <br/>`src_dir = Core/Src`<br/>`include_dir = Core/Inc`. |
| **Linker Error**: `undefined reference to 'HAL_TIM_MspPostInit'` | Function prototype declared in CubeMX code but not implemented. | Add `HAL_TIM_MspPostInit(TIM_HandleTypeDef *htim)` implementation in `main.c`. |
| **Wokwi Error**: `Board not found board-stm32-f103c8` | Incorrect Wokwi part identifier in `diagram.json`. | Change part type string to **`board-stm32-bluepill`**. |
| **Wokwi LCD Blank / LEDs Unlit** | Pin names in `diagram.json` used `PA0`, `PB0` instead of Wokwi Blue Pill syntax. | Update connection strings in `diagram.json` to **`A0`..`A15`** and **`B0`..`B15`**. |
| **LCD Truncation Warning** | `snprintf()` output string length (17 bytes) exceeded buffer size. | Set format string to exactly 16 chars: `"PWM:%2d%% Flow:OK"`. |
| **Hardware LCD Glitching** | HD44780 4-bit timing violated during high CPU load. | Ensure software delay `LCD_DelayUs(50)` pulses enable pin properly. |

---

# SECTION 10 — FINAL DEMO DAY CHECKLIST

- [x] **Project Build Check**: Run `pio run` in VS Code terminal. Verify output says `========================= [SUCCESS] =========================` with 0 errors and 0 warnings.
- [x] **Wokwi License Check**: Verify `wokwi.lic` is present in project root and user home folder (`C:\Users\darsh\wokwi.lic`).
- [x] **Schematic Pin Check**: Verify `diagram.json` uses `board-stm32-bluepill` and pins `A0`, `A2`, `A3`, `B0`, `B1`, `B5`..`B8`, `B12`..`B15`.
- [x] **Boot Up Verification**: Start Wokwi simulator (`F1` -> `Wokwi: Start Simulator`). Verify splash screen appears for 1.5s, followed by `Mst: 50% Pmp:OFF` and Green LED ON.
- [x] **Potentiometer Reaction Check**: Move potentiometer below 50% -> Yellow LED turns ON, LCD displays `Pmp: ON`. Move above 50% -> Green LED turns ON, LCD displays `Pmp: OFF`.
- [x] **Alarm & Reset Check**: Click Blue Button while pump is OFF -> Red LED ON & Buzzer ON (`ALARM: PIPE LEAK`). Click Red Button -> Alarm clears, Green LED turns ON.
- [x] **Presentation Deck Ready**: Open slide deck, review 2-minute / 5-minute presentation script, and prepare for faculty Q&A!
