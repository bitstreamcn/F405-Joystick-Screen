/*
 * Board.c
 *
 *  Created on: Jun 14, 2025
 *      Author: bitstream
 */

#include <string.h>
#include <stdint.h>
#include <sys/stat.h>
#include <stdlib.h>
#include <errno.h>
#include <stdio.h>
#include <signal.h>
#include <time.h>
#include <sys/time.h>
#include <sys/times.h>
#include <sys/unistd.h>
#include <stdbool.h>
#include <stdarg.h>


#include "main.h"
#include "usart.h"
#include "spi.h"
#include "Application.h"

extern DMA_HandleTypeDef hdma_usart1_tx;

#define SPI_DELAY 1//ms

// 全局传输完成标志
volatile uint8_t dma_tx_complete = 1;
volatile uint8_t dma_tx_index = 0;
volatile uint8_t dma_tx_half_index = 0;
#define BUFF_SIZE 256
uint8_t dma_buffer[2][BUFF_SIZE];




void board_init()
{

}

void Radio_SPI_Callback()
{
    //BaseType_t xHigherPriorityTaskWoken = pdFALSE;
    /* 通知等待任务 */
    //xSemaphoreGiveFromISR(radio_sem, &xHigherPriorityTaskWoken);
    //portYIELD_FROM_ISR(xHigherPriorityTaskWoken);
}

#if 0
// 发送并接收单字节数据
HAL_StatusTypeDef Radio_SPI_ReadWrite(uint8_t data, uint8_t *rx_data) {
	uint8_t temp_data;
	if (NULL == rx_data)
	{
		rx_data = &temp_data;
	}
	HAL_StatusTypeDef ret = HAL_SPI_TransmitReceive(&hspi1, &data, rx_data, 1, 10); // 超时10ms
	//HAL_StatusTypeDef ret = HAL_SPI_TransmitReceive_DMA(&hspi1, &data, rx_data, 1);
	if (ret == HAL_OK)
	{
    	//if(xSemaphoreTake(radio_sem, SPI_DELAY) != pdPASS)
    	{
    		//return HAL_TIMEOUT;
    	}
	}
	else
	{
		rx_data[0] = 0;
	}
  return ret;
}
#endif

// 读取寄存器
HAL_StatusTypeDef Radio_Read_Reg(uint8_t reg, uint8_t * data, uint16_t len) {
  HAL_GPIO_WritePin(SPI1_NSS_GPIO_Port, SPI1_NSS_Pin, GPIO_PIN_RESET); // 拉低片选
  delay_us(5);
  //HAL_StatusTypeDef ret = Radio_SPI_ReadWrite(reg, NULL);
  uint8_t tx_data[len + 1];
  uint8_t rx_data[len + 1];

  tx_data[0] = reg;
  for (uint16_t i = 0; i < len; i++)
  {
	  tx_data[i+1] = 0xFF;
  }

  HAL_StatusTypeDef ret = HAL_SPI_TransmitReceive(&hspi1, tx_data, rx_data, len + 1, 10);
  if (HAL_OK != ret)
  {
	  delay_us(5);
	  HAL_GPIO_WritePin(SPI1_NSS_GPIO_Port, SPI1_NSS_Pin, GPIO_PIN_SET); // 释放片选
	  delay_us(5);
	  return ret;
  }
  memcpy(data, &rx_data[1], len);
  /*
  for (uint16_t i = 0; i < len; i++)
  {
	  ret = Radio_SPI_ReadWrite(0xFF, &data[i]); // 发送哑数据获取返回值
	  if (HAL_OK != ret)
	  {
		  delay_us(5);
		  HAL_GPIO_WritePin(SPI1_NSS_GPIO_Port, SPI1_NSS_Pin, GPIO_PIN_SET); // 释放片选
		  delay_us(5);
		  return ret;
	  }
  }
  */
  delay_us(5);
  HAL_GPIO_WritePin(SPI1_NSS_GPIO_Port, SPI1_NSS_Pin, GPIO_PIN_SET); // 释放片选
  delay_us(5);
  return HAL_OK;
}

// 写入寄存器
HAL_StatusTypeDef Radio_Write_Reg(uint8_t reg, const uint8_t * data, uint16_t len) {
  HAL_GPIO_WritePin(SPI1_NSS_GPIO_Port, SPI1_NSS_Pin, GPIO_PIN_RESET);
  delay_us(5);

  uint8_t tx_data[len + 1];
  uint8_t rx_data[len + 1];
  //HAL_StatusTypeDef ret = Radio_SPI_ReadWrite(reg, NULL);

  tx_data[0] = reg;
  if (NULL != data && len > 0)
  {
	  memcpy(&tx_data[1], data, len);
  }

  HAL_StatusTypeDef ret = HAL_SPI_TransmitReceive(&hspi1, tx_data, rx_data, len + 1, 10);
  if (HAL_OK != ret)
  {
	  delay_us(5);
	  HAL_GPIO_WritePin(SPI1_NSS_GPIO_Port, SPI1_NSS_Pin, GPIO_PIN_SET); // 释放片选
	  delay_us(5);
	  return ret;
  }
  /*
  for (uint16_t i = 0; i < len; i++)
  {
	  ret = Radio_SPI_ReadWrite(data[i], NULL);
	  if (HAL_OK != ret)
	  {
		  delay_us(5);
		  HAL_GPIO_WritePin(SPI1_NSS_GPIO_Port, SPI1_NSS_Pin, GPIO_PIN_SET); // 释放片选
		  delay_us(5);
		  return ret;
	  }
  }
  */
  delay_us(5);
  HAL_GPIO_WritePin(SPI1_NSS_GPIO_Port, SPI1_NSS_Pin, GPIO_PIN_SET);
  delay_us(5);
  return HAL_OK;
}

// DMA传输完成中断回调
void HAL_SPI_TxRxCpltCallback(SPI_HandleTypeDef *hspi) {
	if (hspi->Instance == SPI1)
	{
		Radio_SPI_Callback();
	}
}


void printf_dma(const char *format, ...) {
  va_list args;
  va_start(args, format);
  int len = vsnprintf((char*)dma_buffer, sizeof(dma_buffer), format, args);
  HAL_UART_Transmit_DMA(&huart1, (uint8_t*)dma_buffer, len);
  va_end(args);
}

int _write(int file, char *ptr, int len)
{
	  if (file == STDOUT_FILENO || file == STDERR_FILENO) {
	    //HAL_UART_Transmit(&huart1, (uint8_t*)ptr, len, HAL_MAX_DELAY);
		  /*
		  if (len < 128)
		  {
			  memcpy(dma_buffer, ptr, len);
			  HAL_UART_Transmit_DMA(&huart1, dma_buffer, len);
		  }
		  else
		  {
			  HAL_UART_Transmit(&huart1, (uint8_t*)ptr, len, HAL_MAX_DELAY);
		  }
		  */
		  if(dma_tx_complete || (HAL_UART_GetState(&huart1) == HAL_UART_STATE_READY)) {
			  bool redo = false;
		    if ((dma_tx_index + len) < BUFF_SIZE)
			  {
				  memcpy(&dma_buffer[dma_tx_half_index][dma_tx_index], ptr, len);
				  dma_tx_index += len;
			  }
		    else
		    {
		    	redo = true;
		    }
		    if(hdma_usart1_tx.Lock != HAL_UNLOCKED) {
		      hdma_usart1_tx.Lock = HAL_UNLOCKED;
		    }
		    if (dma_tx_index > 0)
		    {
		    	dma_tx_complete = 0;
		    	HAL_UART_Transmit_DMA(&huart1, dma_buffer[dma_tx_half_index], dma_tx_index);
		    	//切换缓冲区
		    	dma_tx_half_index = (dma_tx_half_index + 1) % 2;
		    	dma_tx_index = 0;
		    }
		    if (redo)
		    {
				  if ((dma_tx_index + len) < BUFF_SIZE)
				  {
					  memcpy(&dma_buffer[dma_tx_half_index][dma_tx_index], ptr, len);
					  dma_tx_index += len;
				  }
				  else
				  {
					  return 0;
				  }
		    }
		  }
		  else
		  {
			  if ((dma_tx_index + len) < BUFF_SIZE)
			  {
				  memcpy(&dma_buffer[dma_tx_half_index][dma_tx_index], ptr, len);
				  dma_tx_index += len;
			  }
			  else
			  {
				  return 0;
			  }
		  }
	  }
	  return len;
}

// 在DMA传输完成回调中清除标志
void HAL_UART_TxCpltCallback(UART_HandleTypeDef *huart) {
  if(huart->Instance == USART1) {
	  dma_tx_complete = 1;  // 置位完成标志
	  __HAL_DMA_CLEAR_FLAG(huart->hdmatx, __HAL_DMA_GET_TC_FLAG_INDEX(huart->hdmatx));
  }
}

