/**
  ******************************************************************************
  * @file           : stm32f1xx_it.c
  * @brief          : Interrupt Service Routines
  * @project       : Smart Irrigation & Water-Leak Controller
  ******************************************************************************
  */

#include "main.h"
#include "stm32f1xx_it.h"

/* External Handles */
extern TIM_HandleTypeDef htim3;

/******************************************************************************/
/* Cortex-M3 Processor Exceptions Handlers                                    */
/******************************************************************************/

void NMI_Handler(void)
{
    while (1) {}
}

void HardFault_Handler(void)
{
    while (1) {}
}

void MemManage_Handler(void)
{
    while (1) {}
}

void BusFault_Handler(void)
{
    while (1) {}
}

void UsageFault_Handler(void)
{
    while (1) {}
}

void SVC_Handler(void)
{
}

void DebugMon_Handler(void)
{
}

void PendSV_Handler(void)
{
}

void SysTick_Handler(void)
{
    HAL_IncTick();
}

/******************************************************************************/
/* STM32F1xx Peripheral Interrupt Handlers                                    */
/******************************************************************************/

/**
  * @brief This function handles EXTI line 2 interrupt (Flow Sensor).
  */
void EXTI2_IRQHandler(void)
{
    HAL_GPIO_EXTI_IRQHandler(FLOW_SENSOR_PIN);
}

/**
  * @brief This function handles EXTI line 3 interrupt (Virtual Fault Button).
  */
void EXTI3_IRQHandler(void)
{
    HAL_GPIO_EXTI_IRQHandler(FAULT_INJECT_PIN);
}

/**
  * @brief This function handles TIM3 global interrupt (100ms Periodic Tick).
  */
void TIM3_IRQHandler(void)
{
    HAL_TIM_IRQHandler(&htim3);
}
