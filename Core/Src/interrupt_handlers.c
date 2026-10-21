/**
  ******************************************************************************
  * @file           : interrupt_handlers.c
  * @brief          : Timer ISR and EXTI GPIO Callback Implementations
  * @project       : Smart Irrigation & Water-Leak Controller
  ******************************************************************************
  */

#include "main.h"
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
        // NOTE: ADC sampling is intentionally NOT done here.
        // HAL_ADC_PollForConversion() blocks waiting for SysTick (priority 15),
        // but SysTick cannot preempt this ISR -> deadlock -> MCU freezes.
        // ADC is sampled every 100ms in the main loop via Irrigation_FSM_Process().

        // Increment System FSM 100ms Tick counter only
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
