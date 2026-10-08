/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.c
  * @brief          : Main program body
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2026 STMicroelectronics.
  * All rights reserved.
  *
  * This software is licensed under terms that can be found in the LICENSE file
  * in the root directory of this software component.
  * If no LICENSE file comes with this software, it is provided AS-IS.
  *
  ******************************************************************************
  */
/* USER CODE END Header */
/* Includes ------------------------------------------------------------------*/
#include "main.h"
#include "usart.h"
#include "gpio.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include "stdio.h"
#include "stdbool.h"
/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */

/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */
// 【極性設定】回路に合わせてここを true / false で切り替えます
// スイッチを押したときにピンが「Low」になる回路（プルアップ）なら true
// スイッチを押したときにピンが「High」になる回路（プルダウン）なら false
#define SWITCH_ACTIVE_LOW   false

// ANDゲート互換出力:
// 全正常時 = High（給電許可）、どれか1つでも異常時 = Low（給電遮断）
#define OUTPUT_AND_GATE_COMPATIBLE  true
/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/

/* USER CODE BEGIN PV */
// デバウンス用構造体
typedef struct {
    bool stable_state;    // 確定した論理値 (true / false)
    uint8_t count;        // 連続一致カウンタ
} DebouncePin_t;

// 5ms周期でサンプリングし、4回連続一致 (5ms * 4 = 20ms) で状態確定
#define DEBOUNCE_THRESHOLD 4 

static DebouncePin_t deb_pa4 = {false, 0};
static DebouncePin_t deb_pa5 = {false, 0};
static DebouncePin_t deb_pa6 = {false, 0};

// チャタリング除去関数
bool UpdateDebounce(DebouncePin_t *pin, bool raw_state) {
    if (raw_state != pin->stable_state) {
        pin->count++;
        if (pin->count >= DEBOUNCE_THRESHOLD) {
            pin->stable_state = raw_state; // 閾値を超えたら確定状態を更新
            pin->count = 0;
        }
    } else {
        pin->count = 0; // raw値が確定値と同じならカウンタをリセット
    }
    return pin->stable_state;
}
/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
/* USER CODE BEGIN PFP */

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
  MX_USART2_UART_Init();
  /* USER CODE BEGIN 2 */
  char tx_buf[64];
  uint32_t last_uart_tick = 0;

  deb_pa4.stable_state = (HAL_GPIO_ReadPin(GPIOA, GPIO_PIN_4) == GPIO_PIN_SET);
  deb_pa5.stable_state = (HAL_GPIO_ReadPin(GPIOA, GPIO_PIN_5) == GPIO_PIN_SET);
  deb_pa6.stable_state = (HAL_GPIO_ReadPin(GPIOA, GPIO_PIN_6) == GPIO_PIN_SET);
  /* USER CODE END 2 */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
  while (1)
  {
    // 1. 各ピンのraw電圧を読み取る
    bool raw_pa4 = (HAL_GPIO_ReadPin(GPIOA, GPIO_PIN_4) == GPIO_PIN_SET);
    bool raw_pa5 = (HAL_GPIO_ReadPin(GPIOA, GPIO_PIN_5) == GPIO_PIN_SET);
    bool raw_pa6 = (HAL_GPIO_ReadPin(GPIOA, GPIO_PIN_6) == GPIO_PIN_SET);

    // 2. チャタリング除去
    bool pin_pa4 = UpdateDebounce(&deb_pa4, raw_pa4);
    bool pin_pa5 = UpdateDebounce(&deb_pa5, raw_pa5);
    bool pin_pa6 = UpdateDebounce(&deb_pa6, raw_pa6);

    // 3. スイッチの論理値判定 (true: 正常・投入 / false: 異常・切断)
    bool sw_pa4, sw_pa5, sw_pa6;
#if SWITCH_ACTIVE_LOW
    // 押してGNDに落ちる(Low)回路の場合、Lowのときを判定
    sw_pa4 = !pin_pa4;
    sw_pa5 = !pin_pa5;
    sw_pa6 = !pin_pa6;
#else
    // 押してHighになる回路の場合
    sw_pa4 = pin_pa4;
    sw_pa5 = pin_pa5;
    sw_pa6 = pin_pa6;
#endif

    // 4. 【AND演算】3つすべてのスイッチが正常(true)なら true
    bool and_result = (sw_pa4 && sw_pa5 && sw_pa6);

    // 5. PA1 に出力
    // 外付けANDゲート互換: 全て正常なら High、どれか欠けたら Low
#if OUTPUT_AND_GATE_COMPATIBLE
    HAL_GPIO_WritePin(GPIOA, GPIO_PIN_1, and_result ? GPIO_PIN_SET : GPIO_PIN_RESET);
#else
    // （もし「異常時にHigh」を求める仕様なら反転）
    HAL_GPIO_WritePin(GPIOA, GPIO_PIN_1, and_result ? GPIO_PIN_RESET : GPIO_PIN_SET);
#endif

    // 6. UART送信（100ms周期）
    uint32_t now = HAL_GetTick();
    if ((now - last_uart_tick) >= 100) {
        last_uart_tick = now;
        int len = snprintf(tx_buf, sizeof(tx_buf), "PWR:%d,%d,%d\r\n", 
                           sw_pa4 ? 1 : 0, sw_pa5 ? 1 : 0, sw_pa6 ? 1 : 0);
        HAL_UART_Transmit(&huart2, (uint8_t*)tx_buf, len, 50);
    }

    HAL_Delay(5);
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

  __HAL_FLASH_SET_LATENCY(FLASH_LATENCY_0);

  /** Initializes the RCC Oscillators according to the specified parameters
  * in the RCC_OscInitTypeDef structure.
  */
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSE;
  RCC_OscInitStruct.HSEState = RCC_HSE_ON;
  if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
  {
    Error_Handler();
  }

  /** Initializes the CPU, AHB and APB buses clocks
  */
  RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK|RCC_CLOCKTYPE_SYSCLK
                              |RCC_CLOCKTYPE_PCLK1;
  RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_HSE;
  RCC_ClkInitStruct.SYSCLKDivider = RCC_SYSCLK_DIV1;
  RCC_ClkInitStruct.AHBCLKDivider = RCC_HCLK_DIV1;
  RCC_ClkInitStruct.APB1CLKDivider = RCC_APB1_DIV1;

  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_0) != HAL_OK)
  {
    Error_Handler();
  }
}

/* USER CODE BEGIN 4 */

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
