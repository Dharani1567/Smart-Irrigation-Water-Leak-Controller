/**
  ******************************************************************************
  * @file           : lcd.h
  * @brief          : Header file for 16x2 HD44780 LCD driver (4-Bit GPIO Mode)
  * @project       : Smart Irrigation & Water-Leak Controller
  ******************************************************************************
  */

#ifndef __LCD_H
#define __LCD_H

#ifdef __cplusplus
extern "C" {
#endif

#include "main.h"

void LCD_Init(void);
void LCD_Clear(void);
void LCD_SetCursor(uint8_t row, uint8_t col);
void LCD_PrintString(const char *str);
void LCD_PrintFormat(uint8_t row, uint8_t col, const char *fmt, ...);
void LCD_Display_SystemStatus(uint8_t moisture_pct, uint8_t pump_on, uint8_t pwm_pct, LeakStatus_t leak_status, SystemState_t state);

#ifdef __cplusplus
}
#endif

#endif /* __LCD_H */
