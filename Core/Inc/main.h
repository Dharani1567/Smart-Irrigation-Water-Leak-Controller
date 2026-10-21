/**
  ******************************************************************************
  * @file           : main.h
  * @brief          : Main header file containing peripheral declarations and prototypes
  * @project       : Smart Irrigation & Water-Leak Controller
  ******************************************************************************
  */

#ifndef __MAIN_H
#define __MAIN_H

#ifdef __cplusplus
extern "C" {
#endif

#include "stm32f1xx_hal.h"
#include "system_config.h"

/* Peripheral Handles exported to external modules */
extern ADC_HandleTypeDef hadc1;
extern TIM_HandleTypeDef htim2;
extern TIM_HandleTypeDef htim3;
extern SPI_HandleTypeDef hspi1;

/* Function Prototypes */
void Error_Handler(void);
void SystemClock_Config(void);
void HAL_TIM_MspPostInit(TIM_HandleTypeDef *htim);

#ifdef __cplusplus
}
#endif

#endif /* __MAIN_H */
