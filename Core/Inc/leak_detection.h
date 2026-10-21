/**
  ******************************************************************************
  * @file           : leak_detection.h
  * @brief          : Header file for Flow Pulse counting & Water Leak Detection
  * @project       : Smart Irrigation & Water-Leak Controller
  ******************************************************************************
  */

#ifndef __LEAK_DETECTION_H
#define __LEAK_DETECTION_H

#ifdef __cplusplus
extern "C" {
#endif

#include "main.h"

void Leak_Init(void);
void Leak_OnPulseReceived(void);
void Leak_Update_1s_Tick(uint8_t pump_is_active);
LeakStatus_t Leak_GetStatus(void);
float Leak_GetFlowRateLMin(void);
uint32_t Leak_GetPulseCountSec(void);
void Leak_ClearAlarm(void);
void Leak_InjectFault(LeakStatus_t fault_type);

#ifdef __cplusplus
}
#endif

#endif /* __LEAK_DETECTION_H */
