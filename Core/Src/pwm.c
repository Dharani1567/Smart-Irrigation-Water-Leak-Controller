/**
  ******************************************************************************
  * @file           : pwm.c
  * @brief          : Water Pump PWM Speed Controller using TIM2 Channel 2
  * @project       : Smart Irrigation & Water-Leak Controller
  ******************************************************************************
  */

#include "pwm.h"

static uint8_t g_current_duty_pct = 0;

/**
  * @brief  Initialize PWM driver and start TIM2 Channel 2
  */
void PWM_Pump_Init(void)
{
    g_current_duty_pct = 0;
    __HAL_TIM_SET_COMPARE(&htim2, TIM_CHANNEL_2, 0);
    HAL_TIM_PWM_Start(&htim2, TIM_CHANNEL_2);
}

/**
  * @brief  Set Water Pump PWM Duty Cycle (0 to 100%)
  * @param  duty_pct: Desired PWM percentage
  */
void PWM_Pump_SetDuty(uint8_t duty_pct)
{
    if (duty_pct > 100U) {
        duty_pct = 100U;
    }
    
    g_current_duty_pct = duty_pct;

    // Convert percentage (0-100%) to Timer Compare Value (0 - PWM_TIMER_PERIOD)
    uint32_t compare_value = ((uint32_t)duty_pct * PWM_TIMER_PERIOD) / 100U;
    __HAL_TIM_SET_COMPARE(&htim2, TIM_CHANNEL_2, compare_value);
}

/**
  * @brief  Get active PWM Duty Cycle
  * @retval Duty cycle percentage (0 - 100%)
  */
uint8_t PWM_Pump_GetDuty(void)
{
    return g_current_duty_pct;
}

/**
  * @brief  Immediately Stop PWM Output to Water Pump
  */
void PWM_Pump_Stop(void)
{
    PWM_Pump_SetDuty(0);
}

/**
  * @brief  Calculate proportional PWM Duty Cycle based on Soil Moisture percentage
  *         Requirement: Lower moisture -> Higher PWM duty cycle (faster pump rate)
  * @param  moisture_pct: Current soil moisture (0 - 100%)
  * @retval Proportional PWM Duty Cycle (0 - 100%)
  */
uint8_t PWM_Calculate_DutyFromMoisture(uint8_t moisture_pct)
{
    // If moisture is at or above target threshold (70%), pump should be OFF (0% Duty)
    if (moisture_pct >= MOISTURE_THRESHOLD_TARGET) {
        return 0;
    }
    
    // Proportional mapping: 0% moisture -> 100% duty; 70% moisture -> 30% duty (minimum drive)
    // Formula: Duty = PWM_DUTY_MAX - [ moisture_pct * (PWM_DUTY_MAX - PWM_DUTY_MIN) / TARGET_THRESH ]
    uint32_t duty = PWM_DUTY_MAX - ((uint32_t)moisture_pct * (PWM_DUTY_MAX - PWM_DUTY_MIN) / MOISTURE_THRESHOLD_TARGET);
    
    if (duty > PWM_DUTY_MAX) duty = PWM_DUTY_MAX;
    if (duty < PWM_DUTY_MIN) duty = PWM_DUTY_MIN;
    
    return (uint8_t)duty;
}
