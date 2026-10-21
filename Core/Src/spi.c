/**
  ******************************************************************************
  * @file           : spi.c
  * @brief          : SPI Master Telemetry Transmitter Module
  * @project       : Smart Irrigation & Water-Leak Controller
  ******************************************************************************
  */

#include "spi.h"

/**
  * @brief  Initialize SPI Telemetry driver
  */
void SPI_Telemetry_Init(void)
{
    // CS pin set high (inactive)
    HAL_GPIO_WritePin(SPI1_NSS_PORT, SPI1_NSS_PIN, GPIO_PIN_SET);
}

/**
  * @brief  Format and transmit system telemetry via SPI1 Master
  * @param  moisture_pct: Soil Moisture Percentage (0-100)
  * @param  pump_status: Pump State (PUMP_OFF, PUMP_ON, etc.)
  * @param  pwm_pct: PWM Duty Cycle (0-100)
  * @param  flow_rate_lmin: Flow Rate in Liters per minute
  * @param  leak_status: Leak Fault Status
  * @param  sys_state: Current FSM State
  * @retval HAL Status
  */
HAL_StatusTypeDef SPI_Telemetry_Send(uint8_t moisture_pct, PumpStatus_t pump_status, uint8_t pwm_pct, float flow_rate_lmin, LeakStatus_t leak_status, SystemState_t sys_state)
{
    TelemetryFrame_t frame;
    
    frame.header         = SPI_TELEMETRY_HEADER;
    frame.moisture_pct   = moisture_pct;
    frame.pump_status    = (uint8_t)pump_status;
    frame.pwm_duty_pct   = pwm_pct;
    frame.flow_rate_lmin = (uint8_t)(flow_rate_lmin * 10.0f); // Fixed point 1 decimal place (e.g. 12.5 -> 125)
    frame.leak_status    = (uint8_t)leak_status;
    frame.system_state   = (uint8_t)sys_state;

    // Calculate XOR Checksum over bytes 0 through 6
    uint8_t *ptr = (uint8_t *)&frame;
    uint8_t checksum = 0;
    for (int i = 0; i < 7; i++) {
        checksum ^= ptr[i];
    }
    frame.checksum = checksum;

    // Pull CS Low to select SPI peripheral
    HAL_GPIO_WritePin(SPI1_NSS_PORT, SPI1_NSS_PIN, GPIO_PIN_RESET);

    // Transmit 8-byte frame over SPI1
    HAL_StatusTypeDef status = HAL_SPI_Transmit(&hspi1, (uint8_t *)&frame, sizeof(frame), 100);

    // Release CS High
    HAL_GPIO_WritePin(SPI1_NSS_PORT, SPI1_NSS_PIN, GPIO_PIN_SET);

    return status;
}
