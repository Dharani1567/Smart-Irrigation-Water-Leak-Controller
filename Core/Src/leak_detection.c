/**
  ******************************************************************************
  * @file           : leak_detection.c
  * @brief          : Water Leak Detection and Flow Sensor Pulse Integration
  * @project       : Smart Irrigation & Water-Leak Controller
  ******************************************************************************
  */

#include "leak_detection.h"
#include "irrigation_controller.h"

static volatile uint32_t g_pulse_counter = 0;       // Incremented by EXTI2 interrupt
static volatile uint32_t g_pulses_in_last_sec = 0;   // Calculated every 1 second
static volatile float    g_flow_rate_lmin = 0.0f;
static volatile LeakStatus_t g_leak_status = LEAK_STATUS_OK;
static volatile uint32_t g_dry_run_timer_sec = 0;

/**
  * @brief  Initialize Leak Detection Module
  */
void Leak_Init(void)
{
    g_pulse_counter = 0;
    g_pulses_in_last_sec = 0;
    g_flow_rate_lmin = 0.0f;
    g_leak_status = LEAK_STATUS_OK;
    g_dry_run_timer_sec = 0;
}

/**
  * @brief  Callback invoked from EXTI2 GPIO interrupt on rising edge of Flow Sensor pulse
  */
void Leak_OnPulseReceived(void)
{
    g_pulse_counter++;
    // Flow sensor pulse / Blue Button press triggers Pipe Leak Alarm!
    g_leak_status = LEAK_STATUS_UNINTENDED_FLOW_ALARM;
}


/**
  * @brief  Periodic 1-Second Update tick called from System Timer FSM
  * @param  pump_is_active: 1 if Pump is currently running, 0 if stopped
  */
void Leak_Update_1s_Tick(uint8_t pump_is_active)
{
    // Capture accumulated pulses over the last second
    uint32_t pulses = g_pulse_counter;
    g_pulse_counter = 0; // Reset counter for next 1-second interval
    g_pulses_in_last_sec = pulses;

    // Flow Rate calculation: Flow Rate (L/min) = Frequency (Hz) / 7.5
    g_flow_rate_lmin = ((float)pulses) / FLOW_CALIBRATION_FACTOR;

    // FAULT DETECTION ALGORITHM
    if (pump_is_active) {
        // Condition A: Pump is ON, but no water flow detected
        if (pulses == 0) {
            g_dry_run_timer_sec++;
            if (g_dry_run_timer_sec >= LEAK_DRY_RUN_TIMEOUT_SEC) {
                g_leak_status = LEAK_STATUS_DRY_RUN_ALARM;
            }
        } else {
            g_dry_run_timer_sec = 0; // Reset timer if flow present
        }
    } else {
        // Pump is OFF
        g_dry_run_timer_sec = 0;
        
        // Condition B: Pump is OFF, but water flow is detected (Pipe Leak / Valve Burst)
        if (pulses >= LEAK_UNINTENDED_FLOW_MIN_PPS && g_leak_status != LEAK_STATUS_DRY_RUN_ALARM) {
            g_leak_status = LEAK_STATUS_UNINTENDED_FLOW_ALARM;
        }
    }
}

/**
  * @brief  Get current Leak Fault Status
  * @retval LeakStatus_t enum
  */
LeakStatus_t Leak_GetStatus(void)
{
    return g_leak_status;
}

/**
  * @brief  Get calculated Flow Rate in L/min
  */
float Leak_GetFlowRateLMin(void)
{
    return g_flow_rate_lmin;
}

/**
  * @brief  Get pulses counted in last second
  */
uint32_t Leak_GetPulseCountSec(void)
{
    return g_pulses_in_last_sec;
}

/**
  * @brief  Clear existing alarm and reset fault state
  */
void Leak_ClearAlarm(void)
{
    g_leak_status = LEAK_STATUS_OK;
    g_dry_run_timer_sec = 0;
}

/**
  * @brief  Inject Virtual Fault for Testing / Viva Demonstration
  */
void Leak_InjectFault(LeakStatus_t fault_type)
{
    g_leak_status = fault_type;
}
