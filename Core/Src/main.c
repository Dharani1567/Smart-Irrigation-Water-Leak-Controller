/**
  ******************************************************************************
  * @file           : main.c
  * @brief          : Smart Irrigation & Water-Leak Controller Main File
  * @target         : STM32F103C8T6 (ARM Cortex-M3 "Blue Pill")
  * @clock          : 72 MHz SysClk via HSE 8MHz PLL
  * @project       : Smart Irrigation & Water-Leak Controller
  ******************************************************************************
  */

#include "main.h"
#include "system_config.h"
#include "adc.h"
#include "pwm.h"
#include "lcd.h"
#include "spi.h"
#include "leak_detection.h"
#include "irrigation_controller.h"

/* Global Peripheral Handles */
ADC_HandleTypeDef hadc1;
TIM_HandleTypeDef htim2;
TIM_HandleTypeDef htim3;
SPI_HandleTypeDef hspi1;

/* Private Function Prototypes */
static void MX_GPIO_Init(void);
static void MX_ADC1_Init(void);
static void MX_TIM2_Init(void);
static void MX_TIM3_Init(void);
static void MX_SPI1_Init(void);

/**
  * @brief  The application entry point.
  */
int main(void)
{
    /* 1. Reset of all peripherals, Initializes the Flash interface and the Systick. */
    HAL_Init();

    /* 2. Configure the system clock to 72 MHz */
    SystemClock_Config();

    /* 3. Initialize configured peripherals */
    MX_GPIO_Init();
    MX_ADC1_Init();
    MX_TIM2_Init();
    MX_TIM3_Init();
    MX_SPI1_Init();

    /* 4. Initialize Subsystem Drivers */
    LCD_Init();
    LCD_SetCursor(0, 0);
    LCD_PrintString(" SMART IRRIGATION ");
    LCD_SetCursor(1, 0);
    LCD_PrintString("   & LEAK CNTRL   ");
    HAL_Delay(1500);
    LCD_Clear();

    ADC_Moisture_Init();
    PWM_Pump_Init();
    SPI_Telemetry_Init();
    Leak_Init();
    Irrigation_Init();

    /* 5. Start TIM3 100ms Periodic Interrupt Timer */
    HAL_TIM_Base_Start_IT(&htim3);

    /* 6. Infinite Event-Driven Main Loop */
    while (1)
    {
        /* Run non-blocking Finite State Machine */
        Irrigation_FSM_Process();
    }
}

/**
  * @brief System Clock Configuration (72 MHz via HSE 8MHz crystal)
  */
void SystemClock_Config(void)
{
    RCC_OscInitTypeDef RCC_OscInitStruct = {0};
    RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};
    RCC_PeriphCLKInitTypeDef PeriphClkInit = {0};

    /** Initializes the RCC Oscillators according to the specified parameters
    * in the RCC_OscInitTypeDef structure.
    */
    RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSE;
    RCC_OscInitStruct.HSEState = RCC_HSE_ON;
    RCC_OscInitStruct.HSEPredivValue = RCC_HSE_PREDIV_DIV1;
    RCC_OscInitStruct.HSIState = RCC_HSI_ON;
    RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
    RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSE;
    RCC_OscInitStruct.PLL.PLLMUL = RCC_PLL_MUL9; // 8 MHz * 9 = 72 MHz
    if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
    {
        Error_Handler();
    }

    /** Initializes the CPU, AHB and APB buses clocks
    */
    RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK | RCC_CLOCKTYPE_SYSCLK
                                | RCC_CLOCKTYPE_PCLK1 | RCC_CLOCKTYPE_PCLK2;
    RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
    RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
    RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV2; // 36 MHz max
    RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV1; // 72 MHz max

    if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_2) != HAL_OK)
    {
        Error_Handler();
    }

    PeriphClkInit.PeriphClockSelection = RCC_PERIPHCLK_ADC;
    PeriphClkInit.AdcClockSelection = RCC_ADCPCLK2_DIV6; // 72 MHz / 6 = 12 MHz (Max 14 MHz)
    if (HAL_RCCEx_PeriphCLKConfig(&PeriphClkInit) != HAL_OK)
    {
        Error_Handler();
    }
}

/**
  * @brief ADC1 Initialization Function (Soil Moisture Sensor PA0)
  */
static void MX_ADC1_Init(void)
{
    ADC_ChannelConfTypeDef sConfig = {0};

    hadc1.Instance = ADC1;
    hadc1.Init.ScanConvMode = ADC_SCAN_DISABLE;
    hadc1.Init.ContinuousConvMode = DISABLE;
    hadc1.Init.DiscontinuousConvMode = DISABLE;
    hadc1.Init.ExternalTrigConv = ADC_SOFTWARE_START;
    hadc1.Init.DataAlign = ADC_DATAALIGN_RIGHT;
    hadc1.Init.NbrOfConversion = 1;
    if (HAL_ADC_Init(&hadc1) != HAL_OK)
    {
        Error_Handler();
    }

    /** Configure Regular Channel 0 (PA0)
    */
    sConfig.Channel = ADC_CHANNEL_0;
    sConfig.Rank = ADC_REGULAR_RANK_1;
    sConfig.SamplingTime = ADC_SAMPLETIME_55CYCLES_5;
    if (HAL_ADC_ConfigChannel(&hadc1, &sConfig) != HAL_OK)
    {
        Error_Handler();
    }

    /* Calibrate ADC on startup */
    HAL_ADCEx_Calibration_Start(&hadc1);
}

/**
  * @brief TIM2 Initialization Function (PWM Water Pump Speed Control PA1 / CH2)
  */
static void MX_TIM2_Init(void)
{
    TIM_MasterConfigTypeDef sMasterConfig = {0};
    TIM_OC_InitTypeDef sConfigOC = {0};

    htim2.Instance = TIM2;
    htim2.Init.Prescaler = 71; // 72 MHz / (71 + 1) = 1 MHz Timer Counter Frequency
    htim2.Init.CounterMode = TIM_COUNTERMODE_UP;
    htim2.Init.Period = PWM_TIMER_PERIOD; // 1000 -> 1 kHz PWM Frequency
    htim2.Init.ClockDivision = TIM_CLOCKDIVISION_DIV1;
    htim2.Init.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_ENABLE;
    if (HAL_TIM_PWM_Init(&htim2) != HAL_OK)
    {
        Error_Handler();
    }

    sMasterConfig.MasterOutputTrigger = TIM_TRGO_RESET;
    sMasterConfig.MasterSlaveMode = TIM_MASTERSLAVEMODE_DISABLE;
    if (HAL_TIMEx_MasterConfigSynchronization(&htim2, &sMasterConfig) != HAL_OK)
    {
        Error_Handler();
    }

    sConfigOC.OCMode = TIM_OCMODE_PWM1;
    sConfigOC.Pulse = 0; // Initial Duty Cycle 0%
    sConfigOC.OCPolarity = TIM_OCPOLARITY_HIGH;
    sConfigOC.OCFastMode = TIM_OCFAST_DISABLE;
    if (HAL_TIM_PWM_ConfigChannel(&htim2, &sConfigOC, TIM_CHANNEL_2) != HAL_OK)
    {
        Error_Handler();
    }

    HAL_TIM_MspPostInit(&htim2);
}

/**
  * @brief TIM3 Initialization Function (100ms Periodic System Tick Interrupt)
  */
static void MX_TIM3_Init(void)
{
    TIM_ClockConfigTypeDef sClockSourceConfig = {0};
    TIM_MasterConfigTypeDef sMasterConfig = {0};

    htim3.Instance = TIM3;
    htim3.Init.Prescaler = 7199; // 72 MHz / 7200 = 10 kHz Timer Clock (0.1ms tick)
    htim3.Init.CounterMode = TIM_COUNTERMODE_UP;
    htim3.Init.Period = 999;     // 1000 counts * 0.1ms = 100 ms (10 Hz Interrupt)
    htim3.Init.ClockDivision = TIM_CLOCKDIVISION_DIV1;
    htim3.Init.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_ENABLE;
    if (HAL_TIM_Base_Init(&htim3) != HAL_OK)
    {
        Error_Handler();
    }

    sClockSourceConfig.ClockSource = TIM_CLOCKSOURCE_INTERNAL;
    if (HAL_TIM_ConfigClockSource(&htim3, &sClockSourceConfig) != HAL_OK)
    {
        Error_Handler();
    }

    sMasterConfig.MasterOutputTrigger = TIM_TRGO_RESET;
    sMasterConfig.MasterSlaveMode = TIM_MASTERSLAVEMODE_DISABLE;
    if (HAL_TIMEx_MasterConfigSynchronization(&htim3, &sMasterConfig) != HAL_OK)
    {
        Error_Handler();
    }

    // Enable TIM3 NVIC Interrupt
    HAL_NVIC_SetPriority(TIM3_IRQn, 2, 0);
    HAL_NVIC_EnableIRQ(TIM3_IRQn);
}

/**
  * @brief SPI1 Initialization Function (Telemetry Transmission Master)
  */
static void MX_SPI1_Init(void)
{
    hspi1.Instance = SPI1;
    hspi1.Init.Mode = SPI_MODE_MASTER;
    hspi1.Init.Direction = SPI_DIRECTION_2LINES;
    hspi1.Init.DataSize = SPI_DATASIZE_8BIT;
    hspi1.Init.CLKPolarity = SPI_POLARITY_LOW;
    hspi1.Init.CLKPhase = SPI_PHASE_1EDGE;
    hspi1.Init.NSS = SPI_NSS_SOFT;
    hspi1.Init.BaudRatePrescaler = SPI_BAUDRATEPRESCALER_64; // 72MHz / 64 = 1.125 MHz
    hspi1.Init.FirstBit = SPI_FIRSTBIT_MSB;
    hspi1.Init.TIMode = SPI_TIMODE_DISABLE;
    hspi1.Init.CRCCalculation = SPI_CRCCALCULATION_DISABLE;
    hspi1.Init.CRCPolynomial = 10;
    if (HAL_SPI_Init(&hspi1) != HAL_OK)
    {
        Error_Handler();
    }
}

/**
  * @brief GPIO Initialization Function
  */
static void MX_GPIO_Init(void)
{
    GPIO_InitTypeDef GPIO_InitStruct = {0};

    /* GPIO Ports Clock Enable */
    __HAL_RCC_GPIOD_CLK_ENABLE();
    __HAL_RCC_GPIOA_CLK_ENABLE();
    __HAL_RCC_GPIOB_CLK_ENABLE();
    __HAL_RCC_AFIO_CLK_ENABLE();


    /* Configure SPI1 CS Pin (PA4) */
    HAL_GPIO_WritePin(SPI1_NSS_PORT, SPI1_NSS_PIN, GPIO_PIN_SET);
    GPIO_InitStruct.Pin = SPI1_NSS_PIN;
    GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_HIGH;
    HAL_GPIO_Init(SPI1_NSS_PORT, &GPIO_InitStruct);

    /* Configure Flow Sensor EXTI Pin (PA2) */
    GPIO_InitStruct.Pin = FLOW_SENSOR_PIN;
    GPIO_InitStruct.Mode = GPIO_MODE_IT_RISING;
    GPIO_InitStruct.Pull = GPIO_PULLDOWN;
    HAL_GPIO_Init(FLOW_SENSOR_PORT, &GPIO_InitStruct);

    /* Enable EXTI2 Interrupt */
    HAL_NVIC_SetPriority(FLOW_SENSOR_IRQn, 1, 0);
    HAL_NVIC_EnableIRQ(FLOW_SENSOR_IRQn);

    /* Configure Virtual Fault Injector Button EXTI Pin (PA3) */
    GPIO_InitStruct.Pin = FAULT_INJECT_PIN;
    GPIO_InitStruct.Mode = GPIO_MODE_IT_RISING;
    GPIO_InitStruct.Pull = GPIO_PULLDOWN;
    HAL_GPIO_Init(FAULT_INJECT_PORT, &GPIO_InitStruct);

    /* Enable EXTI3 Interrupt */
    HAL_NVIC_SetPriority(FAULT_INJECT_IRQn, 3, 0);
    HAL_NVIC_EnableIRQ(FAULT_INJECT_IRQn);

    /* Configure LED Pins & Buzzer on GPIOB */
    HAL_GPIO_WritePin(GPIOB, LED_GREEN_NORMAL_PIN | LED_YELLOW_IRR_PIN | LED_RED_LEAK_PIN | BUZZER_PIN, GPIO_PIN_RESET);
    GPIO_InitStruct.Pin = LED_GREEN_NORMAL_PIN | LED_YELLOW_IRR_PIN | LED_RED_LEAK_PIN | BUZZER_PIN;
    GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
    HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);

    /* Configure LCD Parallel 4-Bit GPIOs on GPIOB */
    HAL_GPIO_WritePin(GPIOB, LCD_RS_PIN | LCD_EN_PIN | LCD_D4_PIN | LCD_D5_PIN | LCD_D6_PIN | LCD_D7_PIN, GPIO_PIN_RESET);
    GPIO_InitStruct.Pin = LCD_RS_PIN | LCD_EN_PIN | LCD_D4_PIN | LCD_D5_PIN | LCD_D6_PIN | LCD_D7_PIN;
    GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_HIGH;
    HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);
}

/**
  * @brief  This function is executed in case of error occurrence.
  */
void Error_Handler(void)
{
    __disable_irq();
    while (1)
    {
        // Blink Red LED rapidly on critical system error
        HAL_GPIO_TogglePin(LED_RED_LEAK_PORT, LED_RED_LEAK_PIN);
        for(volatile int i=0; i<100000; i++);
    }
}

/**
  * @brief Timer MSP Post Initialization Function
  * @param htim TIM handle
  */
void HAL_TIM_MspPostInit(TIM_HandleTypeDef *htim)
{
    GPIO_InitTypeDef GPIO_InitStruct = {0};
    if (htim->Instance == TIM2)
    {
        __HAL_RCC_GPIOA_CLK_ENABLE();
        /**TIM2 GPIO Configuration
        PA1     ------> TIM2_CH2
        */
        GPIO_InitStruct.Pin = PUMP_PWM_PIN;
        GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
        GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_HIGH;
        HAL_GPIO_Init(PUMP_PWM_PORT, &GPIO_InitStruct);
    }
}

/**
  * @brief ADC MSP Initialization Function
  * @param hadc ADC handle
  */
void HAL_ADC_MspInit(ADC_HandleTypeDef *hadc)
{
    GPIO_InitTypeDef GPIO_InitStruct = {0};
    if (hadc->Instance == ADC1)
    {
        /* Enable ADC1 and GPIOA Peripheral Clocks */
        __HAL_RCC_ADC1_CLK_ENABLE();
        __HAL_RCC_GPIOA_CLK_ENABLE();

        /**ADC1 GPIO Configuration
        PA0     ------> ADC1_IN0 (Soil Moisture Potentiometer)
        */
        GPIO_InitStruct.Pin = SOIL_MOISTURE_PIN;
        GPIO_InitStruct.Mode = GPIO_MODE_ANALOG;
        HAL_GPIO_Init(SOIL_MOISTURE_PORT, &GPIO_InitStruct);
    }
}

/**
  * @brief ADC MSP De-Initialization Function
  * @param hadc ADC handle
  */
void HAL_ADC_MspDeInit(ADC_HandleTypeDef *hadc)
{
    if (hadc->Instance == ADC1)
    {
        __HAL_RCC_ADC1_CLK_DISABLE();
        HAL_GPIO_DeInit(SOIL_MOISTURE_PORT, SOIL_MOISTURE_PIN);
    }
}

