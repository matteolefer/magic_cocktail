/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.c
  * @brief          : Main program body
  ******************************************************************************
  */
/* USER CODE END Header */

#include "main.h"
#include <stdlib.h>
#include <string.h>

UART_HandleTypeDef huart2;

static void SystemClock_Config(void);
static void MX_GPIO_Init(void);
static void MX_USART2_UART_Init(void);
static void ProcessCommand(const char *command);
static void RunPump(GPIO_TypeDef *port, uint16_t pin, uint32_t duration_ms);
static void SendString(const char *message);

int main(void)
{
  HAL_Init();
  SystemClock_Config();
  MX_GPIO_Init();
  MX_USART2_UART_Init();

  uint8_t rx_byte;
  char command_buffer[64];
  size_t idx = 0U;

  SendString("STM32 cocktail controller ready\r\n");

  while (1)
  {
    if (HAL_UART_Receive(&huart2, &rx_byte, 1U, HAL_MAX_DELAY) == HAL_OK)
    {
      if (rx_byte == '\n' || rx_byte == '\r')
      {
        if (idx > 0U)
        {
          command_buffer[idx] = '\0';
          ProcessCommand(command_buffer);
          idx = 0U;
        }
      }
      else
      {
        if (idx < (sizeof(command_buffer) - 1U))
        {
          command_buffer[idx++] = (char)rx_byte;
        }
        else
        {
          idx = 0U; /* Reset on overflow */
          SendString("ERR:command-too-long\r\n");
        }
      }
    }
  }
}

static void ProcessCommand(const char *command)
{
  if (command == NULL || strlen(command) < 3U)
  {
    SendString("ERR:format\r\n");
    return;
  }

  if (command[1] != ':')
  {
    SendString("ERR:format\r\n");
    return;
  }

  char pump_id = command[0];
  char *end_ptr = NULL;
  uint32_t duration_ms = (uint32_t)strtoul(&command[2], &end_ptr, 10);

  if (end_ptr == &command[2] || duration_ms == 0U)
  {
    SendString("ERR:duration\r\n");
    return;
  }

  switch (pump_id)
  {
    case '1':
      RunPump(PUMP1_PORT, PUMP1_PIN, duration_ms);
      break;
    case '2':
      RunPump(PUMP2_PORT, PUMP2_PIN, duration_ms);
      break;
    case '3':
      RunPump(PUMP3_PORT, PUMP3_PIN, duration_ms);
      break;
    default:
      SendString("ERR:pump\r\n");
      return;
  }

  SendString("OK\r\n");
}

static void RunPump(GPIO_TypeDef *port, uint16_t pin, uint32_t duration_ms)
{
  HAL_GPIO_WritePin(STATUS_LED_PORT, STATUS_LED_PIN, GPIO_PIN_SET);
  HAL_GPIO_WritePin(port, pin, GPIO_PIN_SET);
  HAL_Delay(duration_ms);
  HAL_GPIO_WritePin(port, pin, GPIO_PIN_RESET);
  HAL_GPIO_WritePin(STATUS_LED_PORT, STATUS_LED_PIN, GPIO_PIN_RESET);
}

static void SendString(const char *message)
{
  if (message == NULL)
  {
    return;
  }
  HAL_UART_Transmit(&huart2, (uint8_t *)message, strlen(message), HAL_MAX_DELAY);
}

static void SystemClock_Config(void)
{
  RCC_OscInitTypeDef RCC_OscInitStruct = {0};
  RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};

  __HAL_RCC_PWR_CLK_ENABLE();
  __HAL_PWR_VOLTAGESCALING_CONFIG(PWR_REGULATOR_VOLTAGE_SCALE2);

  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSI;
  RCC_OscInitStruct.HSIState = RCC_HSI_ON;
  RCC_OscInitStruct.HSICalibrationValue = RCC_HSICALIBRATION_DEFAULT;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
  RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSI;
  RCC_OscInitStruct.PLL.PLLM = 16;
  RCC_OscInitStruct.PLL.PLLN = 336;
  RCC_OscInitStruct.PLL.PLLP = RCC_PLLP_DIV4;
  RCC_OscInitStruct.PLL.PLLQ = 7;
  if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
  {
    Error_Handler();
  }

  RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK | RCC_CLOCKTYPE_SYSCLK
                              | RCC_CLOCKTYPE_PCLK1 | RCC_CLOCKTYPE_PCLK2;
  RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
  RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
  RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV2;
  RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV1;

  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_2) != HAL_OK)
  {
    Error_Handler();
  }
}

static void MX_USART2_UART_Init(void)
{
  huart2.Instance = USART2;
  huart2.Init.BaudRate = 115200;
  huart2.Init.WordLength = UART_WORDLENGTH_8B;
  huart2.Init.StopBits = UART_STOPBITS_1;
  huart2.Init.Parity = UART_PARITY_NONE;
  huart2.Init.Mode = UART_MODE_TX_RX;
  huart2.Init.HwFlowCtl = UART_HWCONTROL_NONE;
  huart2.Init.OverSampling = UART_OVERSAMPLING_16;
  if (HAL_UART_Init(&huart2) != HAL_OK)
  {
    Error_Handler();
  }
}

static void MX_GPIO_Init(void)
{
  GPIO_InitTypeDef GPIO_InitStruct = {0};

  __HAL_RCC_GPIOA_CLK_ENABLE();

  /* Configure pump pins */
  GPIO_InitStruct.Pin = PUMP1_PIN | PUMP2_PIN | PUMP3_PIN | STATUS_LED_PIN;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

  HAL_GPIO_WritePin(GPIOA, PUMP1_PIN | PUMP2_PIN | PUMP3_PIN | STATUS_LED_PIN, GPIO_PIN_RESET);
}

void HAL_UART_MspInit(UART_HandleTypeDef *uartHandle)
{
  if (uartHandle->Instance == USART2)
  {
    GPIO_InitTypeDef GPIO_InitStruct = {0};

    __HAL_RCC_USART2_CLK_ENABLE();
    __HAL_RCC_GPIOA_CLK_ENABLE();

    GPIO_InitStruct.Pin = GPIO_PIN_2 | GPIO_PIN_3;
    GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
    GPIO_InitStruct.Pull = GPIO_PULLUP;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
    GPIO_InitStruct.Alternate = GPIO_AF7_USART2;
    HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);
  }
}

void Error_Handler(void)
{
  __disable_irq();
  while (1)
  {
    HAL_GPIO_TogglePin(STATUS_LED_PORT, STATUS_LED_PIN);
    HAL_Delay(200);
  }
}

#ifdef  USE_FULL_ASSERT
void assert_failed(uint8_t *file, uint32_t line)
{
  (void)file;
  (void)line;
}
#endif /* USE_FULL_ASSERT */
