/**
  ******************************************************************************
  * @file           : adc.h
  * @brief          : Header file for Soil Moisture ADC sampling
  * @project       : Smart Irrigation & Water-Leak Controller
  ******************************************************************************
  */

#ifndef __ADC_H
#define __ADC_H

#ifdef __cplusplus
extern "C" {
#endif

#include "main.h"

void ADC_Moisture_Init(void);
uint16_t ADC_Moisture_ReadRaw(void);
uint8_t ADC_Moisture_GetPercentage(void);
void ADC_Moisture_UpdateSample(void);

#ifdef __cplusplus
}
#endif

#endif /* __ADC_H */
