/**
  ******************************************************************************
  * @file           : irrigation_controller.h
  * @brief          : Header file for Irrigation Controller State Machine (FSM)
  * @project       : Smart Irrigation & Water-Leak Controller
  ******************************************************************************
  */

#ifndef __IRRIGATION_CONTROLLER_H
#define __IRRIGATION_CONTROLLER_H

#ifdef __cplusplus
extern "C" {
#endif

#include "main.h"

void Irrigation_Init(void);
void Irrigation_FSM_Process(void);
void Irrigation_Timer_100ms_Tick(void);
SystemState_t Irrigation_GetState(void);
uint32_t Irrigation_GetLockoutTimeRemaining(void);
void Irrigation_ResetSystem(void);

#ifdef __cplusplus
}
#endif

#endif /* __IRRIGATION_CONTROLLER_H */
