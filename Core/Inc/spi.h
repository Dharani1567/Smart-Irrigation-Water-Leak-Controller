/**
  ******************************************************************************
  * @file           : spi.h
  * @brief          : Header file for SPI Telemetry Master Transmission
  * @project       : Smart Irrigation & Water-Leak Controller
  ******************************************************************************
  */

#ifndef __SPI_H
#define __SPI_H

#ifdef __cplusplus
extern "C" {
#endif

#include "main.h"

void SPI_Telemetry_Init(void);
HAL_StatusTypeDef SPI_Telemetry_Send(uint8_t moisture_pct, PumpStatus_t pump_status, uint8_t pwm_pct, float flow_rate_lmin, LeakStatus_t leak_status, SystemState_t sys_state);

#ifdef __cplusplus
}
#endif

#endif /* __SPI_H */
