/**
  ******************************************************************************
  * @file           : system_config.h
  * @brief          : System Configuration, Pin Mapping, and Global Definitions
  * @project       : Smart Irrigation & Water-Leak Controller
  * @target         : STM32F103C8T6 (ARM Cortex-M3 "Blue Pill")
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2026 Academic Embedded Engineering Project
  * All rights reserved.
  ******************************************************************************
  */

#ifndef __SYSTEM_CONFIG_H
#define __SYSTEM_CONFIG_H

#ifdef __cplusplus
extern "C" {
#endif

#include "stm32f1xx_hal.h"

/* ============================================================================
 * SYSTEM THRESHOLDS & CONSTANTS
 * ============================================================================ */

// Soil Moisture Thresholds (%)
#define MOISTURE_THRESHOLD_LOW       50U   // Start irrigation when moisture < 50%
#define MOISTURE_THRESHOLD_TARGET    50U   // Stop irrigation when moisture >= 50%
#define MOISTURE_ADC_MIN_VAL         0U    // ADC value for 0% Moisture (Dry Soil Calibration)
#define MOISTURE_ADC_MAX_VAL         4095U // ADC value for 100% Moisture (Wet Soil Calibration)


// Irrigation Lockout Timer (Max safety runtime for irrigation pump in seconds)
#define IRRIGATION_MAX_LOCKOUT_SEC   30U   // Max continuous irrigation duration (safety timeout)
#define IRRIGATION_COOLDOWN_SEC      15U   // Cooldown period before re-evaluating irrigation

// PWM Configuration (TIM2 CH2 on PA1)
#define PWM_TIMER_PERIOD             1000U // Auto-Reload Register (ARR) for 1 kHz PWM at 1 MHz TIM2 counter
#define PWM_DUTY_MIN                 30U   // Minimum active PWM duty cycle (%)
#define PWM_DUTY_MAX                 100U  // Maximum active PWM duty cycle (%)

// Leak Detection Parameters
#define LEAK_DRY_RUN_TIMEOUT_SEC     2U    // Seconds with Pump ON and Flow = 0 before triggering Dry Run Fault
#define LEAK_UNINTENDED_FLOW_MIN_PPS 1U    // Min pulses/sec with Pump OFF to trigger Pipe Burst/Leak Fault
#define FLOW_CALIBRATION_FACTOR      7.5f  // Pulses per second per L/min (Standard YF-S201 sensor: Q = F / 7.5)


// Telemetry SPI Config
#define SPI_TELEMETRY_HEADER         0xAAU // Packet start byte
#define SPI_TELEMETRY_FOOTER         0x55U // Packet stop byte

/* ============================================================================
 * HARDWARE PIN MAPPINGS
 * ============================================================================ */

// ADC Input - Soil Moisture Sensor (Potentiometer)
#define SOIL_MOISTURE_PIN            GPIO_PIN_0
#define SOIL_MOISTURE_PORT           GPIOA
#define SOIL_MOISTURE_ADC_CHANNEL    ADC_CHANNEL_0

// PWM Output - Water Pump Speed Control (TIM2 CH2)
#define PUMP_PWM_PIN                 GPIO_PIN_1
#define PUMP_PWM_PORT                GPIOA

// Flow Sensor Pulse Input (EXTI2)
#define FLOW_SENSOR_PIN              GPIO_PIN_2
#define FLOW_SENSOR_PORT             GPIOA
#define FLOW_SENSOR_IRQn             EXTI2_IRQn

// Virtual Fault Injector Button (EXTI3 for Viva / Testing)
#define FAULT_INJECT_PIN             GPIO_PIN_3
#define FAULT_INJECT_PORT            GPIOA
#define FAULT_INJECT_IRQn            EXTI3_IRQn

// SPI1 Hardware Telemetry Pins
#define SPI1_NSS_PIN                 GPIO_PIN_4
#define SPI1_NSS_PORT                GPIOA
#define SPI1_SCK_PIN                 GPIO_PIN_5
#define SPI1_SCK_PORT                GPIOA
#define SPI1_MISO_PIN                GPIO_PIN_6
#define SPI1_MISO_PORT               GPIOA
#define SPI1_MOSI_PIN                GPIO_PIN_7
#define SPI1_MOSI_PORT               GPIOA

// LED Status Indicators
#define LED_GREEN_NORMAL_PIN         GPIO_PIN_5
#define LED_GREEN_NORMAL_PORT        GPIOB

#define LED_YELLOW_IRR_PIN           GPIO_PIN_6
#define LED_YELLOW_IRR_PORT          GPIOB

#define LED_RED_LEAK_PIN             GPIO_PIN_7
#define LED_RED_LEAK_PORT            GPIOB

// Buzzer Alarm Pin
#define BUZZER_PIN                   GPIO_PIN_8
#define BUZZER_PORT                  GPIOB

// 16x2 HD44780 LCD Parallel 4-Bit Interface Pins
#define LCD_RS_PIN                   GPIO_PIN_0
#define LCD_RS_PORT                  GPIOB

#define LCD_EN_PIN                   GPIO_PIN_1
#define LCD_EN_PORT                  GPIOB

#define LCD_D4_PIN                   GPIO_PIN_12
#define LCD_D4_PORT                  GPIOB

#define LCD_D5_PIN                   GPIO_PIN_13
#define LCD_D5_PORT                  GPIOB

#define LCD_D6_PIN                   GPIO_PIN_14
#define LCD_D6_PORT                  GPIOB

#define LCD_D7_PIN                   GPIO_PIN_15
#define LCD_D7_PORT                  GPIOB

/* ============================================================================
 * ENUMERATIONS & DATA STRUCTURES
 * ============================================================================ */

/**
 * @brief System Operating States for Finite State Machine (FSM)
 */
typedef enum {
    SYSTEM_STATE_INIT = 0,
    SYSTEM_STATE_IDLE_MONITORING,
    SYSTEM_STATE_IRRIGATING,
    SYSTEM_STATE_LOCKOUT_COOLDOWN,
    SYSTEM_STATE_FAULT_DRY_RUN,
    SYSTEM_STATE_FAULT_UNINTENDED_LEAK
} SystemState_t;

/**
 * @brief Pump Operating Status
 */
typedef enum {
    PUMP_OFF = 0,
    PUMP_ON,
    PUMP_LOCKOUT,
    PUMP_FAULT_SHUTDOWN
} PumpStatus_t;

/**
 * @brief Water Leak Fault Types
 */
typedef enum {
    LEAK_STATUS_OK = 0,
    LEAK_STATUS_DRY_RUN_ALARM,       // Pump ON, but no flow detected (pipe blocked / dry tank)
    LEAK_STATUS_UNINTENDED_FLOW_ALARM // Pump OFF, but water flow detected (pipe burst / valve leak)
} LeakStatus_t;

/**
 * @brief Telemetry SPI Frame Structure (Packed 8 Bytes)
 */
typedef struct __attribute__((packed)) {
    uint8_t header;         // 0xAA
    uint8_t moisture_pct;   // 0 - 100%
    uint8_t pump_status;    // PumpStatus_t (0=OFF, 1=ON, 2=LOCKOUT, 3=FAULT)
    uint8_t pwm_duty_pct;   // 0 - 100%
    uint8_t flow_rate_lmin; // Flow rate in L/min * 10 (fixed-point 1 decimal place)
    uint8_t leak_status;    // LeakStatus_t (0=OK, 1=DRY_RUN, 2=BURST)
    uint8_t system_state;   // SystemState_t
    uint8_t checksum;       // XOR checksum of preceding 7 bytes
} TelemetryFrame_t;

#ifdef __cplusplus
}
#endif

#endif /* __SYSTEM_CONFIG_H */
