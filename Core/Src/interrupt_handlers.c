/**
  ******************************************************************************
  * @file           : interrupt_handlers.c
  * @brief          : Timer ISR and EXTI GPIO Callback Implementations
  * @project       : Smart Irrigation & Water-Leak Controller
  ******************************************************************************
  */

#include "main.h"
#include "adc.h"
#include "irrigation_controller.h"
#include "leak_detection.h"

/**
  * @brief  Period elapsed callback in non-blocking mode (TIM3 100ms Periodic Tick)
  * @param  htim : TIM handle
  */
void HAL_TIM_PeriodElapsedCallback(TIM_HandleTypeDef *htim)
{
    if (htim->Instance == TIM3)
    {
        // 1. Periodic ADC Sampling of Soil Moisture Sensor (Non-blocking Timer-Driven Sampling)
        ADC_Moisture_UpdateSample();

        // 2. Increment System FSM 100ms Tick
        Irrigation_Timer_100ms_Tick();
    }
}

/**
  * @brief  EXTI GPIO External Interrupt Callback Handler
  * @param  GPIO_Pin: Pin that triggered interrupt
  */
void HAL_GPIO_EXTI_Callback(uint16_t GPIO_Pin)
{
    // Flow Sensor Pulse Input (PA2)
    if (GPIO_Pin == FLOW_SENSOR_PIN)
    {
        Leak_OnPulseReceived();
    }
    // System Reset Button (PA3)
    else if (GPIO_Pin == FAULT_INJECT_PIN)
    {
        Irrigation_ResetSystem();
    }
}
