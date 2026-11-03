/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.c
  * @brief          : Drink Reminder for NUCLEO-G474RE
  ******************************************************************************
  * @attention
  *
  * Hardware Pinout:
  * - PC13: Button (Check-in, Alarm off, Reset timer) -> EXTI13
  * - PC9 : Button (Cycle 30s / 45s / 60s)            -> EXTI9
  * - PC8 : LED PWM (Breathe idle, Rapid blink alarm) -> TIM3_CH3
  * - PC6 : Buzzer PWM (1~3 beeps, confirm, alarm)   -> TIM8_CH1
  *
  * Copyright (c) 2026 STMicroelectronics.
  * All rights reserved.
  ******************************************************************************
  */
/* USER CODE END Header */
/* Includes ------------------------------------------------------------------*/
#include "main.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include "stm32g4xx_hal_tim.h"
/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */
typedef enum {
    STATE_IDLE_COUNTDOWN,
    STATE_ALARM_TRIGGERED
} SystemState_t;

typedef enum {
    BEEP_IDLE,
    BEEP_SHORT_TONES,  // 切換檔位：嗶 1~3 聲
    BEEP_LONG_CONFIRM, // 打卡：長嗶 500ms
    BEEP_ALARM_RAPID   // 警報：急促連響
} BeepMode_t;
/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */
#define DEBOUNCE_DELAY_MS 200
/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/
TIM_HandleTypeDef htim3;
TIM_HandleTypeDef htim8;

/* USER CODE BEGIN PV */
// 倒數檔位設定 (秒)
const uint32_t INTERVAL_SETTINGS[3] = {30, 45, 60};
volatile uint8_t current_interval_idx = 0; // 0: 30s, 1: 45s, 2: 60s

volatile SystemState_t sys_state = STATE_IDLE_COUNTDOWN;
volatile uint32_t target_countdown_ms = 30000;
volatile uint32_t countdown_start_tick = 0;

// 去彈跳時間戳記
volatile uint32_t last_pc13_press_tick = 0;
volatile uint32_t last_pc9_press_tick = 0;

// 蜂鳴器狀態控制變數
volatile BeepMode_t beep_mode = BEEP_IDLE;
volatile uint8_t beep_repeat_count = 0;
uint32_t beep_step_tick = 0;
uint8_t beep_state_step = 0;
/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
static void MX_GPIO_Init(void);
static void MX_TIM3_Init(void);
static void MX_TIM8_Init(void);
/* USER CODE BEGIN PFP */
void Buzzer_Set(uint8_t on);
void Update_Buzzer(uint32_t now);
void Update_LED(uint32_t now);
/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */

/* USER CODE END 0 */

/**
  * @brief  The application entry point.
  * @retval int
  */
int main(void)
{

  /* USER CODE BEGIN 1 */

  /* USER CODE END 1 */

  /* MCU Configuration--------------------------------------------------------*/

  /* Reset of all peripherals, Initializes the Flash interface and the Systick. */
  HAL_Init();

  /* USER CODE BEGIN Init */

  /* USER CODE END Init */

  /* Configure the system clock */
  SystemClock_Config();

  /* USER CODE BEGIN SysInit */

  /* USER CODE END SysInit */

  /* Initialize all configured peripherals */
  MX_GPIO_Init();
  MX_TIM3_Init();
  MX_TIM8_Init();
  /* USER CODE BEGIN 2 */
  // 1. 動態修正 TIM3 (LED PWM)：Prescaler 169, Period 999 產生 1 kHz PWM
  __HAL_TIM_SET_PRESCALER(&htim3, 169);
  __HAL_TIM_SET_AUTORELOAD(&htim3, 999);

  // 2. 啟動 PWM 輸出
  HAL_TIM_PWM_Start(&htim3, TIM_CHANNEL_3); // PC8 (LED)
  HAL_TIM_PWM_Start(&htim8, TIM_CHANNEL_1); // PC6 (Buzzer)

  // 3. 啟用 TIM8 高級定時器主輸出 (MOE)，使 PC6 順利輸出訊號
  __HAL_TIM_MOE_ENABLE(&htim8);
  Buzzer_Set(0);

  // 4. 動態修正 PC9 為 EXTI 下降緣觸發中斷並開啟 NVIC
  GPIO_InitTypeDef GPIO_InitStruct = {0};
  GPIO_InitStruct.Pin = GPIO_PIN_9;
  GPIO_InitStruct.Mode = GPIO_MODE_IT_FALLING;
  GPIO_InitStruct.Pull = GPIO_PULLUP;
  HAL_GPIO_Init(GPIOC, &GPIO_InitStruct);

  HAL_NVIC_SetPriority(EXTI9_5_IRQn, 2, 0);
  HAL_NVIC_EnableIRQ(EXTI9_5_IRQn);

  // 5. 初始化倒數計時時間點
  countdown_start_tick = HAL_GetTick();
  /* USER CODE END 2 */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
  while (1)
  {
    uint32_t current_tick = HAL_GetTick();

    // 1. 倒數計時判斷
    if (sys_state == STATE_IDLE_COUNTDOWN)
    {
      if (current_tick - countdown_start_tick >= target_countdown_ms)
      {
        sys_state = STATE_ALARM_TRIGGERED;
        beep_mode = BEEP_ALARM_RAPID; // 切入急促連響
      }
    }

    // 2. 非阻塞更新輸出狀態
    Update_LED(current_tick);
    Update_Buzzer(current_tick);

    /* USER CODE END WHILE */

    /* USER CODE BEGIN 3 */
  }
  /* USER CODE END 3 */
}

/**
  * @brief System Clock Configuration
  * @retval None
  */
void SystemClock_Config(void)
{
  RCC_OscInitTypeDef RCC_OscInitStruct = {0};
  RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};

  /** Configure the main internal regulator output voltage
  */
  HAL_PWREx_ControlVoltageScaling(PWR_REGULATOR_VOLTAGE_SCALE1_BOOST);

  /** Initializes the RCC Oscillators according to the specified parameters
  * in the RCC_OscInitTypeDef structure.
  */
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSI;
  RCC_OscInitStruct.HSIState = RCC_HSI_ON;
  RCC_OscInitStruct.HSICalibrationValue = RCC_HSICALIBRATION_DEFAULT;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
  RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSI;
  RCC_OscInitStruct.PLL.PLLM = RCC_PLLM_DIV4;
  RCC_OscInitStruct.PLL.PLLN = 85;
  RCC_OscInitStruct.PLL.PLLP = RCC_PLLP_DIV2;
  RCC_OscInitStruct.PLL.PLLQ = RCC_PLLQ_DIV2;
  RCC_OscInitStruct.PLL.PLLR = RCC_PLLR_DIV2;
  if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
  {
    Error_Handler();
  }

  /** Initializes the CPU, AHB and APB buses clocks
  */
  RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK|RCC_CLOCKTYPE_SYSCLK
                              |RCC_CLOCKTYPE_PCLK1|RCC_CLOCKTYPE_PCLK2;
  RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
  RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
  RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV1;
  RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV1;

  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_4) != HAL_OK)
  {
    Error_Handler();
  }
}

/**
  * @brief TIM3 Initialization Function
  * @param None
  * @retval None
  */
static void MX_TIM3_Init(void)
{

  /* USER CODE BEGIN TIM3_Init 0 */

  /* USER CODE END TIM3_Init 0 */

  TIM_ClockConfigTypeDef sClockSourceConfig = {0};
  TIM_MasterConfigTypeDef sMasterConfig = {0};
  TIM_OC_InitTypeDef sConfigOC = {0};

  /* USER CODE BEGIN TIM3_Init 1 */

  /* USER CODE END TIM3_Init 1 */
  htim3.Instance = TIM3;
  htim3.Init.Prescaler = 0;
  htim3.Init.CounterMode = TIM_COUNTERMODE_UP;
  htim3.Init.Period = 65535;
  htim3.Init.ClockDivision = TIM_CLOCKDIVISION_DIV1;
  htim3.Init.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_DISABLE;
  if (HAL_TIM_Base_Init(&htim3) != HAL_OK)
  {
    Error_Handler();
  }
  sClockSourceConfig.ClockSource = TIM_CLOCKSOURCE_INTERNAL;
  if (HAL_TIM_ConfigClockSource(&htim3, &sClockSourceConfig) != HAL_OK)
  {
    Error_Handler();
  }
  if (HAL_TIM_PWM_Init(&htim3) != HAL_OK)
  {
    Error_Handler();
  }
  sMasterConfig.MasterOutputTrigger = TIM_TRGO_RESET;
  sMasterConfig.MasterSlaveMode = TIM_MASTERSLAVEMODE_DISABLE;
  if (HAL_TIMEx_MasterConfigSynchronization(&htim3, &sMasterConfig) != HAL_OK)
  {
    Error_Handler();
  }
  sConfigOC.OCMode = TIM_OCMODE_PWM1;
  sConfigOC.Pulse = 0;
  sConfigOC.OCPolarity = TIM_OCPOLARITY_HIGH;
  sConfigOC.OCFastMode = TIM_OCFAST_DISABLE;
  if (HAL_TIM_PWM_ConfigChannel(&htim3, &sConfigOC, TIM_CHANNEL_3) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN TIM3_Init 2 */

  /* USER CODE END TIM3_Init 2 */
  HAL_TIM_MspPostInit(&htim3);

}

/**
  * @brief TIM8 Initialization Function
  * @param None
  * @retval None
  */
static void MX_TIM8_Init(void)
{

  /* USER CODE BEGIN TIM8_Init 0 */

  /* USER CODE END TIM8_Init 0 */

  TIM_ClockConfigTypeDef sClockSourceConfig = {0};
  TIM_MasterConfigTypeDef sMasterConfig = {0};
  TIM_OC_InitTypeDef sConfigOC = {0};
  TIM_BreakDeadTimeConfigTypeDef sBreakDeadTimeConfig = {0};

  /* USER CODE BEGIN TIM8_Init 1 */

  /* USER CODE END TIM8_Init 1 */
  htim8.Instance = TIM8;
  htim8.Init.Prescaler = 169;
  htim8.Init.CounterMode = TIM_COUNTERMODE_UP;
  htim8.Init.Period = 999;
  htim8.Init.ClockDivision = TIM_CLOCKDIVISION_DIV1;
  htim8.Init.RepetitionCounter = 0;
  htim8.Init.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_DISABLE;
  if (HAL_TIM_Base_Init(&htim8) != HAL_OK)
  {
    Error_Handler();
  }
  sClockSourceConfig.ClockSource = TIM_CLOCKSOURCE_INTERNAL;
  if (HAL_TIM_ConfigClockSource(&htim8, &sClockSourceConfig) != HAL_OK)
  {
    Error_Handler();
  }
  if (HAL_TIM_PWM_Init(&htim8) != HAL_OK)
  {
    Error_Handler();
  }
  sMasterConfig.MasterOutputTrigger = TIM_TRGO_RESET;
  sMasterConfig.MasterOutputTrigger2 = TIM_TRGO2_RESET;
  sMasterConfig.MasterSlaveMode = TIM_MASTERSLAVEMODE_DISABLE;
  if (HAL_TIMEx_MasterConfigSynchronization(&htim8, &sMasterConfig) != HAL_OK)
  {
    Error_Handler();
  }
  sConfigOC.OCMode = TIM_OCMODE_PWM1;
  sConfigOC.Pulse = 0;
  sConfigOC.OCPolarity = TIM_OCPOLARITY_HIGH;
  sConfigOC.OCNPolarity = TIM_OCNPOLARITY_HIGH;
  sConfigOC.OCFastMode = TIM_OCFAST_DISABLE;
  sConfigOC.OCIdleState = TIM_OCIDLESTATE_RESET;
  sConfigOC.OCNIdleState = TIM_OCNIDLESTATE_RESET;
  if (HAL_TIM_PWM_ConfigChannel(&htim8, &sConfigOC, TIM_CHANNEL_1) != HAL_OK)
  {
    Error_Handler();
  }
  sBreakDeadTimeConfig.OffStateRunMode = TIM_OSSR_DISABLE;
  sBreakDeadTimeConfig.OffStateIDLEMode = TIM_OSSI_DISABLE;
  sBreakDeadTimeConfig.LockLevel = TIM_LOCKLEVEL_OFF;
  sBreakDeadTimeConfig.DeadTime = 0;
  sBreakDeadTimeConfig.BreakState = TIM_BREAK_DISABLE;
  sBreakDeadTimeConfig.BreakPolarity = TIM_BREAKPOLARITY_HIGH;
  sBreakDeadTimeConfig.BreakFilter = 0;
  sBreakDeadTimeConfig.BreakAFMode = TIM_BREAK_AFMODE_INPUT;
  sBreakDeadTimeConfig.Break2State = TIM_BREAK2_DISABLE;
  sBreakDeadTimeConfig.Break2Polarity = TIM_BREAK2POLARITY_HIGH;
  sBreakDeadTimeConfig.Break2Filter = 0;
  sBreakDeadTimeConfig.Break2AFMode = TIM_BREAK_AFMODE_INPUT;
  sBreakDeadTimeConfig.AutomaticOutput = TIM_AUTOMATICOUTPUT_DISABLE;
  if (HAL_TIMEx_ConfigBreakDeadTime(&htim8, &sBreakDeadTimeConfig) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN TIM8_Init 2 */

  /* USER CODE END TIM8_Init 2 */
  HAL_TIM_MspPostInit(&htim8);

}

/**
  * @brief GPIO Initialization Function
  * @param None
  * @retval None
  */
static void MX_GPIO_Init(void)
{
  GPIO_InitTypeDef GPIO_InitStruct = {0};
  /* USER CODE BEGIN MX_GPIO_Init_1 */

  /* USER CODE END MX_GPIO_Init_1 */

  /* GPIO Ports Clock Enable */
  __HAL_RCC_GPIOC_CLK_ENABLE();
  __HAL_RCC_GPIOF_CLK_ENABLE();
  __HAL_RCC_GPIOA_CLK_ENABLE();
  __HAL_RCC_GPIOB_CLK_ENABLE();

  /*Configure GPIO pins : PC13 PC9 */
  GPIO_InitStruct.Pin = GPIO_PIN_13|GPIO_PIN_9;
  GPIO_InitStruct.Mode = GPIO_MODE_IT_RISING;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  HAL_GPIO_Init(GPIOC, &GPIO_InitStruct);

  /*Configure GPIO pins : LPUART1_TX_Pin LPUART1_RX_Pin */
  GPIO_InitStruct.Pin = LPUART1_TX_Pin|LPUART1_RX_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  GPIO_InitStruct.Alternate = GPIO_AF12_LPUART1;
  HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

  /* EXTI interrupt init*/
  HAL_NVIC_SetPriority(EXTI9_5_IRQn, 0, 0);
  HAL_NVIC_EnableIRQ(EXTI9_5_IRQn);

  HAL_NVIC_SetPriority(EXTI15_10_IRQn, 1, 0);
  HAL_NVIC_EnableIRQ(EXTI15_10_IRQn);

  /* USER CODE BEGIN MX_GPIO_Init_2 */

  /* USER CODE END MX_GPIO_Init_2 */
}

/* USER CODE BEGIN 4 */

/**
  * @brief EXTI 外部中斷回呼函式（含軟體時間戳去彈跳）
  */
void HAL_GPIO_EXTI_Callback(uint16_t GPIO_Pin)
{
  uint32_t now = HAL_GetTick();

  // PC9: 切換倒數時間 (30s -> 45s -> 60s -> 30s)
  if (GPIO_Pin == GPIO_PIN_9)
  {
    if (now - last_pc9_press_tick > DEBOUNCE_DELAY_MS)
    {
      last_pc9_press_tick = now;

      // 換檔並更新目標時間
      current_interval_idx = (current_interval_idx + 1) % 3;
      target_countdown_ms = INTERVAL_SETTINGS[current_interval_idx] * 1000;
      countdown_start_tick = now; // 換檔後重新倒數

      // 觸發對應聲響 (檔位 0 -> 1 聲, 檔位 1 -> 2 聲, 檔位 2 -> 3 聲)
      beep_mode = BEEP_SHORT_TONES;
      beep_repeat_count = current_interval_idx + 1;
      beep_state_step = 0;
      beep_step_tick = now;

      sys_state = STATE_IDLE_COUNTDOWN;
    }
  }
  // PC13: 確認打卡 / 關閉警報 / 重設倒數
  else if (GPIO_Pin == GPIO_PIN_13)
  {
    if (now - last_pc13_press_tick > DEBOUNCE_DELAY_MS)
    {
      last_pc13_press_tick = now;

      // 恢復倒數狀態並重設計時器
      countdown_start_tick = now;
      sys_state = STATE_IDLE_COUNTDOWN;

      // 觸發打卡長音 (500ms)
      beep_mode = BEEP_LONG_CONFIRM;
      beep_state_step = 0;
      beep_step_tick = now;
    }
  }
}

/**
  * @brief 蜂鳴器 PWM 啟閉控制
  */
void Buzzer_Set(uint8_t on)
{
  if (on) {
    __HAL_TIM_SET_COMPARE(&htim8, TIM_CHANNEL_1, 500); // 50% 佔空比 (ARR=999)
  } else {
    __HAL_TIM_SET_COMPARE(&htim8, TIM_CHANNEL_1, 0);   // 靜音
  }
}

/**
  * @brief 蜂鳴器非阻塞狀態機處理
  */
void Update_Buzzer(uint32_t now)
{
  switch (beep_mode)
  {
    case BEEP_IDLE:
      Buzzer_Set(0);
      break;

    case BEEP_SHORT_TONES:
      // 嗶 80ms，靜音 80ms
      if (beep_state_step % 2 == 0) {
        Buzzer_Set(1);
        if (now - beep_step_tick >= 80) {
          beep_step_tick = now;
          beep_state_step++;
        }
      } else {
        Buzzer_Set(0);
        if (now - beep_step_tick >= 80) {
          beep_step_tick = now;
          beep_state_step++;
          if ((beep_state_step / 2) >= beep_repeat_count) {
            beep_mode = BEEP_IDLE;
          }
        }
      }
      break;

    case BEEP_LONG_CONFIRM:
      // 長嗶 500ms
      if (beep_state_step == 0) {
        Buzzer_Set(1);
        if (now - beep_step_tick >= 500) {
          beep_step_tick = now;
          beep_state_step = 1;
        }
      } else {
        Buzzer_Set(0);
        beep_mode = BEEP_IDLE;
      }
      break;

    case BEEP_ALARM_RAPID:
      // 急促警報：150ms 響 / 150ms 停
      if ((now / 150) % 2 == 0) {
        Buzzer_Set(1);
      } else {
        Buzzer_Set(0);
      }
      break;
  }
}

/**
  * @brief LED 燈效更新處理 (PC8 PWM 柔和呼吸 / 急促快閃)
  */
void Update_LED(uint32_t now)
{
  if (sys_state == STATE_IDLE_COUNTDOWN)
  {
    // 平常狀態：柔和呼吸燈 (週期 2000ms：1000ms 漸亮，1000ms 漸暗)
    uint32_t phase = now % 2000;
    uint32_t duty = 0;

    if (phase < 1000) {
      duty = (phase * 999) / 1000;
    } else {
      duty = ((2000 - phase) * 999) / 1000;
    }
    __HAL_TIM_SET_COMPARE(&htim3, TIM_CHANNEL_3, duty);
  }
  else if (sys_state == STATE_ALARM_TRIGGERED)
  {
    // 警報狀態：100ms 亮 / 100ms 暗急促閃爍
    if ((now / 100) % 2 == 0) {
      __HAL_TIM_SET_COMPARE(&htim3, TIM_CHANNEL_3, 999);
    } else {
      __HAL_TIM_SET_COMPARE(&htim3, TIM_CHANNEL_3, 0);
    }
  }
}

/* USER CODE END 4 */

/**
  * @brief  This function is executed in case of error occurrence.
  * @retval None
  */
void Error_Handler(void)
{
  /* USER CODE BEGIN Error_Handler_Debug */
  /* User can add his own implementation to report the HAL error return state */
  __disable_irq();
  while (1)
  {
  }
  /* USER CODE END Error_Handler_Debug */
}
#ifdef USE_FULL_ASSERT
/**
  * @brief  Reports the name of the source file and the source line number
  *         where the assert_param error has occurred.
  * @param  file: pointer to the source file name
  * @param  line: assert_param error line source number
  * @retval None
  */
void assert_failed(uint8_t *file, uint32_t line)
{
  /* USER CODE BEGIN 6 */
  /* User can add his own implementation to report the file name and line number,
     ex: printf("Wrong parameters value: file %s on line %d\r\n", file, line) */
  /* USER CODE END 6 */
}
#endif /* USE_FULL_ASSERT */
