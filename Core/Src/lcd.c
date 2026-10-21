/**
  ******************************************************************************
  * @file           : lcd.c
  * @brief          : 16x2 HD44780 Text LCD Driver (4-Bit GPIO Mode)
  * @project       : Smart Irrigation & Water-Leak Controller
  ******************************************************************************
  */

#include "lcd.h"
#include <stdio.h>
#include <stdarg.h>
#include <string.h>

/* Private Helper Delay Function */
static void LCD_DelayUs(uint32_t us)
{
    // Simple software delay loop tuned for 72MHz STM32F103 SysClk
    uint32_t count = us * 12;
    while (count--) {
        __NOP();
    }
}

static void LCD_PulseEnable(void)
{
    HAL_GPIO_WritePin(LCD_EN_PORT, LCD_EN_PIN, GPIO_PIN_SET);
    LCD_DelayUs(5);
    HAL_GPIO_WritePin(LCD_EN_PORT, LCD_EN_PIN, GPIO_PIN_RESET);
    LCD_DelayUs(50);
}

static void LCD_Send4Bits(uint8_t nibble)
{
    HAL_GPIO_WritePin(LCD_D4_PORT, LCD_D4_PIN, (nibble & 0x01) ? GPIO_PIN_SET : GPIO_PIN_RESET);
    HAL_GPIO_WritePin(LCD_D5_PORT, LCD_D5_PIN, (nibble & 0x02) ? GPIO_PIN_SET : GPIO_PIN_RESET);
    HAL_GPIO_WritePin(LCD_D6_PORT, LCD_D6_PIN, (nibble & 0x04) ? GPIO_PIN_SET : GPIO_PIN_RESET);
    HAL_GPIO_WritePin(LCD_D7_PORT, LCD_D7_PIN, (nibble & 0x08) ? GPIO_PIN_SET : GPIO_PIN_RESET);
    LCD_PulseEnable();
}

static void LCD_SendCommand(uint8_t cmd)
{
    HAL_GPIO_WritePin(LCD_RS_PORT, LCD_RS_PIN, GPIO_PIN_RESET); // RS = 0 for Command
    LCD_Send4Bits(cmd >> 4);   // Send High Nibble
    LCD_Send4Bits(cmd & 0x0F); // Send Low Nibble
    if (cmd == 0x01 || cmd == 0x02) {
        HAL_Delay(2); // Clear & Home commands require 1.52ms
    }
}

static void LCD_SendData(uint8_t data)
{
    HAL_GPIO_WritePin(LCD_RS_PORT, LCD_RS_PIN, GPIO_PIN_SET); // RS = 1 for Data
    LCD_Send4Bits(data >> 4);   // Send High Nibble
    LCD_Send4Bits(data & 0x0F); // Send Low Nibble
}

/**
  * @brief  Initialize HD44780 LCD in 4-Bit Mode
  */
void LCD_Init(void)
{
    HAL_Delay(50); // Wait > 40ms power-up

    HAL_GPIO_WritePin(LCD_RS_PORT, LCD_RS_PIN, GPIO_PIN_RESET);
    HAL_GPIO_WritePin(LCD_EN_PORT, LCD_EN_PIN, GPIO_PIN_RESET);

    // Initialization sequence for 4-bit mode
    LCD_Send4Bits(0x03);
    HAL_Delay(5);
    LCD_Send4Bits(0x03);
    HAL_Delay(5);
    LCD_Send4Bits(0x03);
    LCD_DelayUs(150);
    LCD_Send4Bits(0x02); // Set to 4-bit mode

    // Function Set: 4-bit mode, 2 lines, 5x8 font
    LCD_SendCommand(0x28);
    // Display Control: Display ON, Cursor OFF, Blink OFF
    LCD_SendCommand(0x0C);
    // Clear Display
    LCD_SendCommand(0x01);
    // Entry Mode Set: Increment automatically, no shift
    LCD_SendCommand(0x06);
}

/**
  * @brief  Clear LCD Screen
  */
void LCD_Clear(void)
{
    LCD_SendCommand(0x01);
}

/**
  * @brief  Set Cursor Position (0-indexed: row 0..1, col 0..15)
  */
void LCD_SetCursor(uint8_t row, uint8_t col)
{
    uint8_t address = (row == 0) ? (0x80 + col) : (0xC0 + col);
    LCD_SendCommand(address);
}

/**
  * @brief  Print String on LCD
  */
void LCD_PrintString(const char *str)
{
    while (*str) {
        LCD_SendData((uint8_t)*str++);
    }
}

/**
  * @brief  Formatted Print on LCD at specific row and column
  */
void LCD_PrintFormat(uint8_t row, uint8_t col, const char *fmt, ...)
{
    char buffer[17]; // 16 chars + null terminator
    va_list args;
    va_start(args, fmt);
    vsnprintf(buffer, sizeof(buffer), fmt, args);
    va_end(args);

    LCD_SetCursor(row, col);
    LCD_PrintString(buffer);
}

/**
  * @brief  Render Complete System Dashboard on 16x2 LCD based on FSM State
  */
void LCD_Display_SystemStatus(uint8_t moisture_pct, uint8_t pump_on, uint8_t pwm_pct, LeakStatus_t leak_status, SystemState_t state)
{
    static char prev_line1[17] = {0};
    static char prev_line2[17] = {0};
    char line1[17];
    char line2[17];

    if (state == SYSTEM_STATE_FAULT_DRY_RUN) {
        snprintf(line1, sizeof(line1), "** DRY  RUN  ** ");
        snprintf(line2, sizeof(line2), "Press RED=Reset ");
    }
    else if (state == SYSTEM_STATE_FAULT_UNINTENDED_LEAK) {
        snprintf(line1, sizeof(line1), "** PIPE LEAK ** ");
        snprintf(line2, sizeof(line2), "Press RED=Reset ");
    }
    else if (state == SYSTEM_STATE_LOCKOUT_COOLDOWN) {
        snprintf(line1, sizeof(line1), "Mst:%2d%% LOCKOUT", moisture_pct);
        snprintf(line2, sizeof(line2), "Cooling Down... ");
    }
    else if (pump_on) {
        // Irrigating: show live moisture and actual PWM duty
        snprintf(line1, sizeof(line1), "Mst:%2d%% Pmp:ON ", moisture_pct);
        snprintf(line2, sizeof(line2), "PWM:%2d%% IRRIG. ", pwm_pct);
    }
    else {
        // Idle/Normal: PWM is 0 when pump off
        snprintf(line1, sizeof(line1), "Mst:%2d%% Pmp:OFF", moisture_pct);
        snprintf(line2, sizeof(line2), "PWM: 0%% NORMAL  ");
    }

    // Refresh Line 1 if changed
    if (strncmp(line1, prev_line1, 16) != 0) {
        LCD_SetCursor(0, 0);
        LCD_PrintString(line1);
        strncpy(prev_line1, line1, 16);
    }

    // Refresh Line 2 if changed
    if (strncmp(line2, prev_line2, 16) != 0) {
        LCD_SetCursor(1, 0);
        LCD_PrintString(line2);
        strncpy(prev_line2, line2, 16);
    }
}
