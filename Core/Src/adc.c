/**
  ******************************************************************************
  * @file           : adc.c
  * @brief          : Soil Moisture ADC Sampling & Percentage Calibration
  * @project       : Smart Irrigation & Water-Leak Controller
  ******************************************************************************
  */

#include "adc.h"

static volatile uint16_t g_adc_raw_value = 0;
static volatile uint8_t  g_moisture_percentage = 0;

/**
  * @brief  Initialize ADC module parameters
  */
void ADC_Moisture_Init(void)
{
    g_adc_raw_value = 0;
    g_moisture_percentage = 0;
}

/**
  * @brief  Perform single conversion read on ADC1 Channel 0 (PA0)
  * @retval 12-bit ADC Raw Conversion value (0 to 4095)
  */
uint16_t ADC_Moisture_ReadRaw(void)
{
    HAL_ADC_Start(&hadc1);
    if (HAL_ADC_PollForConversion(&hadc1, 50) == HAL_OK)
    {
        g_adc_raw_value = (uint16_t)HAL_ADC_GetValue(&hadc1);
    }
    else
    {
        // Direct register read fallback in case simulator bypasses EOC flag
        g_adc_raw_value = (uint16_t)(hadc1.Instance->DR & 0x0FFF);
    }
    return g_adc_raw_value;
}

/**
  * @brief  Convert 12-bit ADC value into 0-100% moisture reading
  * @note   In standard resistive/capacitive soil moisture sensors:
  *         0V (ADC=0) -> 0% Dry, 3.3V (ADC=4095) -> 100% Wet
  * @retval Moisture percentage (0 - 100%)
  */
uint8_t ADC_Moisture_GetPercentage(void)
{
    return g_moisture_percentage;
}

/**
  * @brief  Update global moisture reading (called periodically from Timer ISR)
  */
void ADC_Moisture_UpdateSample(void)
{
    uint16_t raw = ADC_Moisture_ReadRaw();
    
    // Scale ADC range (0..4095) to Percentage (0..100)
    uint32_t pct = ((uint32_t)raw * 100U) / MOISTURE_ADC_MAX_VAL;
    if (pct > 100U) {
        pct = 100U;
    }
    
    g_moisture_percentage = (uint8_t)pct;
}
