/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.h
  * @brief          : Header for main.c file.
  *                   This file contains the common defines of the application.
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

/* Define to prevent recursive inclusion -------------------------------------*/
#ifndef __MAIN_H
#define __MAIN_H

#ifdef __cplusplus
extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/
#include "stm32f4xx_hal.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include "tx_api.h"
#include "ux_api.h"
/* USER CODE END Includes */

/* Exported types ------------------------------------------------------------*/
/* USER CODE BEGIN ET */
enum eFlightMode{
	MODE_NONE,
	MODE_CAR, //车模式
	MODE_DRONE, //无人机模式
	MODE_RETURN_HOME, //返航模式
};
/* USER CODE END ET */

/* Exported constants --------------------------------------------------------*/
/* USER CODE BEGIN EC */

/* USER CODE END EC */

/* Exported macro ------------------------------------------------------------*/
/* USER CODE BEGIN EM */

#define DELAY_MS(ms) \
    do { \
        if (tx_thread_identify() != TX_NULL) { \
            tx_thread_sleep(ms); /* 在线程中，休眠让出CPU */ \
        } else { \
            HAL_Delay(ms);       /* 在中断或裸机初始化中，只能死等 */ \
        } \
    } while(0)

#define DelayMs(x)  DELAY_MS(((x) * (TX_TIMER_TICKS_PER_SECOND) / 1000))
/* USER CODE END EM */

/* Exported functions prototypes ---------------------------------------------*/
void Error_Handler(void);

/* USER CODE BEGIN EFP */
void delay_us(uint32_t us);
/* USER CODE END EFP */

/* Private defines -----------------------------------------------------------*/
#define ADC_BAT_Pin GPIO_PIN_2
#define ADC_BAT_GPIO_Port GPIOC
#define ADC_LEFT_X_Pin GPIO_PIN_0
#define ADC_LEFT_X_GPIO_Port GPIOA
#define ADC_LEFT_Y_Pin GPIO_PIN_1
#define ADC_LEFT_Y_GPIO_Port GPIOA
#define ADC_RIGHT_X_Pin GPIO_PIN_2
#define ADC_RIGHT_X_GPIO_Port GPIOA
#define ADC_RIGHT_Y_Pin GPIO_PIN_3
#define ADC_RIGHT_Y_GPIO_Port GPIOA
#define SPI1_NSS_Pin GPIO_PIN_4
#define SPI1_NSS_GPIO_Port GPIOA
#define RF_RXEN_Pin GPIO_PIN_4
#define RF_RXEN_GPIO_Port GPIOC
#define TURN_ON_Pin GPIO_PIN_5
#define TURN_ON_GPIO_Port GPIOC
#define LED_DRONE_Pin GPIO_PIN_0
#define LED_DRONE_GPIO_Port GPIOB
#define LED2812_Pin GPIO_PIN_1
#define LED2812_GPIO_Port GPIOB
#define POWER_KEY_Pin GPIO_PIN_10
#define POWER_KEY_GPIO_Port GPIOB
#define LED_HOME_Pin GPIO_PIN_11
#define LED_HOME_GPIO_Port GPIOB
#define USB_DETECT_Pin GPIO_PIN_12
#define USB_DETECT_GPIO_Port GPIOB
#define LED_CONNECT_Pin GPIO_PIN_9
#define LED_CONNECT_GPIO_Port GPIOC
#define RF_NRST_Pin GPIO_PIN_8
#define RF_NRST_GPIO_Port GPIOA
#define RF_DIO1_Pin GPIO_PIN_15
#define RF_DIO1_GPIO_Port GPIOA
#define RF_DIO1_EXTI_IRQn EXTI15_10_IRQn
#define RF_BUSY_Pin GPIO_PIN_10
#define RF_BUSY_GPIO_Port GPIOC
#define BEEPER_Pin GPIO_PIN_12
#define BEEPER_GPIO_Port GPIOC
#define ARMED_Pin GPIO_PIN_2
#define ARMED_GPIO_Port GPIOD
#define LED_CAR_Pin GPIO_PIN_8
#define LED_CAR_GPIO_Port GPIOB
#define LED_EN_Pin GPIO_PIN_9
#define LED_EN_GPIO_Port GPIOB

/* USER CODE BEGIN Private defines */

/* USER CODE END Private defines */

#ifdef __cplusplus
}
#endif

#endif /* __MAIN_H */
