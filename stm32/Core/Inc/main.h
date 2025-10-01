/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.h
  * @brief          : Header for main.c file.
  ******************************************************************************
  */
/* USER CODE END Header */

#ifndef __MAIN_H
#define __MAIN_H

#ifdef __cplusplus
extern "C" {
#endif

#include "stm32f4xx_hal.h"

void Error_Handler(void);

/* Pump GPIO definitions */
#define PUMP1_PIN GPIO_PIN_1
#define PUMP1_PORT GPIOA
#define PUMP2_PIN GPIO_PIN_4
#define PUMP2_PORT GPIOA
#define PUMP3_PIN GPIO_PIN_5
#define PUMP3_PORT GPIOA
#define STATUS_LED_PIN GPIO_PIN_9
#define STATUS_LED_PORT GPIOA

#ifdef __cplusplus
}
#endif

#endif /* __MAIN_H */
