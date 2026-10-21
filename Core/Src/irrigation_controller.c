/**
  ******************************************************************************
  * @file           : irrigation_controller.c
  * @brief          : Smart Irrigation Controller Finite State Machine (FSM)
  * @project       : Smart Irrigation & Water-Leak Controller
  ******************************************************************************
  */

#include "irrigation_controller.h"
#include "adc.h"
#include "pwm.h"
#include "lcd.h"
#include "spi.h"
#include "leak_detection.h"

static volatile SystemState_t g_system_state = SYSTEM_STATE_INIT;
static volatile uint32_t g_state_timer_sec = 0;
static volatile uint32_t g_ms_tick_counter = 0;
static volatile uint32_t g_sec_tick_flag = 0;

/**
  * @brief  Update LED Indicators according to current FSM State
  */
static void Update_LED_Indicators(SystemState_t state)
{
    switch (state) {
        case SYSTEM_STATE_IDLE_MONITORING:
            HAL_GPIO_WritePin(LED_GREEN_NORMAL_PORT, LED_GREEN_NORMAL_PIN, GPIO_PIN_SET);   // Green ON
            HAL_GPIO_WritePin(LED_YELLOW_IRR_PORT,   LED_YELLOW_IRR_PIN,   GPIO_PIN_RESET); // Yellow OFF
            HAL_GPIO_WritePin(LED_RED_LEAK_PORT,     LED_RED_LEAK_PIN,     GPIO_PIN_RESET); // Red OFF
            HAL_GPIO_WritePin(BUZZER_PORT,            BUZZER_PIN,           GPIO_PIN_RESET); // Buzzer OFF
            break;

        case SYSTEM_STATE_IRRIGATING:
            HAL_GPIO_WritePin(LED_GREEN_NORMAL_PORT, LED_GREEN_NORMAL_PIN, GPIO_PIN_RESET); // Green OFF
            HAL_GPIO_WritePin(LED_YELLOW_IRR_PORT,   LED_YELLOW_IRR_PIN,   GPIO_PIN_SET);   // Yellow ON
            HAL_GPIO_WritePin(LED_RED_LEAK_PORT,     LED_RED_LEAK_PIN,     GPIO_PIN_RESET); // Red OFF
            HAL_GPIO_WritePin(BUZZER_PORT,            BUZZER_PIN,           GPIO_PIN_RESET); // Buzzer OFF
            break;

        case SYSTEM_STATE_LOCKOUT_COOLDOWN:
            HAL_GPIO_WritePin(LED_GREEN_NORMAL_PORT, LED_GREEN_NORMAL_PIN, GPIO_PIN_RESET);
            // Toggle Yellow LED to signal Lockout Cooldown
            HAL_GPIO_TogglePin(LED_YELLOW_IRR_PORT,  LED_YELLOW_IRR_PIN);
            HAL_GPIO_WritePin(LED_RED_LEAK_PORT,     LED_RED_LEAK_PIN,     GPIO_PIN_RESET);
            HAL_GPIO_WritePin(BUZZER_PORT,            BUZZER_PIN,           GPIO_PIN_RESET);
            break;

        case SYSTEM_STATE_FAULT_DRY_RUN:
        case SYSTEM_STATE_FAULT_UNINTENDED_LEAK:
            HAL_GPIO_WritePin(LED_GREEN_NORMAL_PORT, LED_GREEN_NORMAL_PIN, GPIO_PIN_RESET); // Green OFF
            HAL_GPIO_WritePin(LED_YELLOW_IRR_PORT,   LED_YELLOW_IRR_PIN,   GPIO_PIN_RESET); // Yellow OFF
            HAL_GPIO_WritePin(LED_RED_LEAK_PORT,     LED_RED_LEAK_PIN,     GPIO_PIN_SET);   // Red ON
            HAL_GPIO_WritePin(BUZZER_PORT,            BUZZER_PIN,           GPIO_PIN_SET);   // Buzzer ON Alarm
            break;

        default:
            break;
    }
}

/**
  * @brief  Initialize Irrigation Controller Subsystem
  */
void Irrigation_Init(void)
{
    g_system_state = SYSTEM_STATE_IDLE_MONITORING;
    g_state_timer_sec = 0;
    g_ms_tick_counter = 0;
    ADC_Moisture_UpdateSample(); // Initial ADC conversion
    PWM_Pump_Stop();
    Update_LED_Indicators(g_system_state);
}

/**
  * @brief  100ms Periodic Tick invoked from TIM3 Interrupt Handler
  */
void Irrigation_Timer_100ms_Tick(void)
{
    g_ms_tick_counter += 100;
    if (g_ms_tick_counter >= 1000) {
        g_ms_tick_counter = 0;
        g_sec_tick_flag = 1; // Signal 1-second task execution in main loop
    }
}

/**
  * @brief  Main Non-Blocking FSM Process (Called in main while(1) loop)
  */
void Irrigation_FSM_Process(void)
{
    static uint32_t last_1s_tick = 0;
    static uint32_t last_adc_sample = 0;

    // Direct Active-HIGH Button Handling (Pressed when 3.3V / SET)
    uint8_t cur_flow_pin = HAL_GPIO_ReadPin(FLOW_SENSOR_PORT, FLOW_SENSOR_PIN);
    uint8_t cur_fault_pin = HAL_GPIO_ReadPin(FAULT_INJECT_PORT, FAULT_INJECT_PIN);

    // Blue Button Pressed -> Trigger Pipe Leak Alarm immediately!
    if (cur_flow_pin == GPIO_PIN_SET) {
        g_system_state = SYSTEM_STATE_FAULT_UNINTENDED_LEAK;
        Leak_InjectFault(LEAK_STATUS_UNINTENDED_FLOW_ALARM);
        PWM_Pump_Stop();
        Update_LED_Indicators(SYSTEM_STATE_FAULT_UNINTENDED_LEAK);
    }
    // Red Button Pressed -> Reset System back to Normal!
    else if (cur_fault_pin == GPIO_PIN_SET) {
        Irrigation_ResetSystem();
    }

    // Periodic ADC sampling (100ms) to ensure continuous potentiometer responsiveness
    uint32_t now = HAL_GetTick();
    if (now - last_adc_sample >= 100) {
        last_adc_sample = now;
        ADC_Moisture_UpdateSample();
    }

    uint8_t moisture_pct = ADC_Moisture_GetPercentage();
    LeakStatus_t leak_status = Leak_GetStatus();
    uint8_t pump_is_active = (g_system_state == SYSTEM_STATE_IRRIGATING) ? 1 : 0;

    // Instant check: if leak status was set, transition immediately
    if (leak_status == LEAK_STATUS_DRY_RUN_ALARM) {
        g_system_state = SYSTEM_STATE_FAULT_DRY_RUN;
    } else if (leak_status == LEAK_STATUS_UNINTENDED_FLOW_ALARM) {
        g_system_state = SYSTEM_STATE_FAULT_UNINTENDED_LEAK;
    }

    // 1-Second Periodic Updates (SysTick + Timer ISR fallback)
    now = HAL_GetTick();
    if ((now - last_1s_tick >= 1000) || g_sec_tick_flag) {
        last_1s_tick = now;
        g_sec_tick_flag = 0;
        g_state_timer_sec++;

        // Execute 1-second leak detection pulse integration
        Leak_Update_1s_Tick(pump_is_active);
        leak_status = Leak_GetStatus();

        // Check overall Leak Fault conditions
        if (leak_status == LEAK_STATUS_DRY_RUN_ALARM) {
            g_system_state = SYSTEM_STATE_FAULT_DRY_RUN;
        } else if (leak_status == LEAK_STATUS_UNINTENDED_FLOW_ALARM) {
            g_system_state = SYSTEM_STATE_FAULT_UNINTENDED_LEAK;
        }

        // Transmit SPI Telemetry Packet every 1 second
        SPI_Telemetry_Send(moisture_pct, 
                           pump_is_active ? PUMP_ON : ((g_system_state == SYSTEM_STATE_LOCKOUT_COOLDOWN) ? PUMP_LOCKOUT : PUMP_OFF),
                           PWM_Pump_GetDuty(),
                           Leak_GetFlowRateLMin(),
                           leak_status,
                           g_system_state);
    }

    // STATE MACHINE TRANSITION LOGIC
    switch (g_system_state) {

        case SYSTEM_STATE_IDLE_MONITORING:
            PWM_Pump_Stop();
            Update_LED_Indicators(SYSTEM_STATE_IDLE_MONITORING);

            // Check if moisture falls below trigger threshold (50%)
            if (moisture_pct < MOISTURE_THRESHOLD_LOW) {
                g_system_state = SYSTEM_STATE_IRRIGATING;
                g_state_timer_sec = 0; // Reset continuous irrigation timer
            }
            break;


        case SYSTEM_STATE_IRRIGATING:
            Update_LED_Indicators(SYSTEM_STATE_IRRIGATING);

            // Calculate and update proportional PWM Duty Cycle based on moisture deficit
            uint8_t target_duty = PWM_Calculate_DutyFromMoisture(moisture_pct);
            PWM_Pump_SetDuty(target_duty);

            // Condition 1: Moisture reaches target threshold (70%) -> Stop Irrigation
            if (moisture_pct >= MOISTURE_THRESHOLD_TARGET) {
                PWM_Pump_Stop();
                g_system_state = SYSTEM_STATE_IDLE_MONITORING;
                g_state_timer_sec = 0;
            }
            // Condition 2: Continuous Irrigation Lockout Timeout (Safety Over-Watering Prevention)
            else if (g_state_timer_sec >= IRRIGATION_MAX_LOCKOUT_SEC) {
                PWM_Pump_Stop();
                g_system_state = SYSTEM_STATE_LOCKOUT_COOLDOWN;
                g_state_timer_sec = 0;
            }
            break;

        case SYSTEM_STATE_LOCKOUT_COOLDOWN:
            PWM_Pump_Stop();
            Update_LED_Indicators(SYSTEM_STATE_LOCKOUT_COOLDOWN);

            // Wait for cooldown period (15 seconds)
            if (g_state_timer_sec >= IRRIGATION_COOLDOWN_SEC) {
                g_system_state = SYSTEM_STATE_IDLE_MONITORING;
                g_state_timer_sec = 0;
            }
            break;

        case SYSTEM_STATE_FAULT_DRY_RUN:
        case SYSTEM_STATE_FAULT_UNINTENDED_LEAK:
            PWM_Pump_Stop(); // Emergency Shutdown of Water Pump
            Update_LED_Indicators(g_system_state);
            break;

        default:
            g_system_state = SYSTEM_STATE_IDLE_MONITORING;
            break;
    }

    // Refresh 16x2 LCD Dashboard
    LCD_Display_SystemStatus(moisture_pct, pump_is_active, PWM_Pump_GetDuty(), leak_status, g_system_state);
}

/**
  * @brief  Get active FSM State
  */
SystemState_t Irrigation_GetState(void)
{
    return g_system_state;
}

/**
  * @brief  Get remaining lockout/cooldown time in seconds
  */
uint32_t Irrigation_GetLockoutTimeRemaining(void)
{
    if (g_system_state == SYSTEM_STATE_LOCKOUT_COOLDOWN) {
        if (g_state_timer_sec < IRRIGATION_COOLDOWN_SEC) {
            return IRRIGATION_COOLDOWN_SEC - g_state_timer_sec;
        }
    }
    return 0;
}

/**
  * @brief  Reset System from Fault State
  */
void Irrigation_ResetSystem(void)
{
    Leak_ClearAlarm();
    PWM_Pump_Stop();
    g_system_state = SYSTEM_STATE_IDLE_MONITORING;
    g_state_timer_sec = 0;
    Update_LED_Indicators(SYSTEM_STATE_IDLE_MONITORING);
}
