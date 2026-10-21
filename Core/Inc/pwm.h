/**
  ******************************************************************************
  * @file           : pwm.h
  * @brief          : Header file for Water Pump PWM Speed Control (TIM2 CH2)
  * @project       : Smart Irrigation & Water-Leak Controller
  ******************************************************************************
  */

#ifndef __PWM_H
#define __PWM_H

#ifdef __cplusplus
extern "C" {
#endif

#include "main.h"

void PWM_Pump_Init(void);
void PWM_Pump_SetDuty(uint8_t duty_pct);
uint8_t PWM_Pump_GetDuty(void);
void PWM_Pump_Stop(void);
uint8_t PWM_Calculate_DutyFromMoisture(uint8_t moisture_pct);

#ifdef __cplusplus
}
#endif

#endif /* __PWM_H */
