/*
 * timer.c
 *
 *  Created on: Jun 5, 2025
 *      Author: bitstream
 */
#include "tim.h"
#include "timer.h"
#include "stm32f4xx_hal_tim.h"

// 检查定时器是否使能
#define TIM_IS_ENABLED(htim) ((htim)->Instance->CR1 & TIM_CR1_CEN)

#define pdTICKS_TO_MS(x) ((uint32_t)((x) * 1000 / configTICK_RATE_HZ))

#define TxTimeoutTimer htim6
#define RxTimeoutTimer htim7

void ( *TxCallBack )( void ) = NULL;
void ( *RxCallBack )( void ) = NULL;

void TimerInit(TIM_HandleTypeDef *obj, void ( *callback )( void ))
{
	if (TxTimeoutTimer.Instance == obj->Instance)
	{
		TxCallBack = callback;
	}
	else if (RxTimeoutTimer.Instance == obj->Instance)
	{
		RxCallBack = callback;
	}
}

uint32_t TimerGetCurrentTime(void) {
    //TickType_t ticks = xTaskGetTickCount();
    // 使用pdTICKS_TO_MS宏（FreeRTOS提供的宏，将tick转换为毫秒）
    //return pdTICKS_TO_MS(ticks);
	return HAL_GetTick();
}

uint32_t TimerGetElapsedTime( uint32_t eventInTime )
{
	uint32_t elapsedTime = 0;

    // Needed at boot, cannot compute with 0 or elapsed time will be equal to current time
    if( eventInTime == 0 )
    {
        return 0;
    }

    elapsedTime = TimerGetCurrentTime();

    if( elapsedTime < eventInTime )
    { // roll over of the counter
        return( elapsedTime + ( 0xFFFFFFFF - eventInTime ) );
    }
    else
    {
        return( elapsedTime - eventInTime );
    }
}

void TimerSetValue( TIM_HandleTypeDef *obj, uint32_t value )
{
	//apb1 timer clock 48mHZ
	uint32_t period = (value * 10 * 2) - 1; // 计数值 = (ms×1) -1 → 100ms=1000次计数
	__HAL_TIM_SET_AUTORELOAD(obj, period);
}

void TimerStop( TIM_HandleTypeDef *obj )
{
	if(TIM_IS_ENABLED(obj) && (obj->State != HAL_TIM_STATE_READY)) {
		HAL_TIM_Base_Stop_IT(obj);
		obj->State = HAL_TIM_STATE_READY;
	}

	__HAL_TIM_SET_COUNTER(obj, 0);
}

void TimerStart( TIM_HandleTypeDef *obj )
{
	// 停止定时器
	HAL_TIM_Base_Stop_IT(obj);
	// 清除计数器值（从0开始计数）
	__HAL_TIM_SET_COUNTER(obj, 0);
	// 清除更新中断标志（避免立即触发）
	__HAL_TIM_CLEAR_FLAG(obj, TIM_FLAG_UPDATE);
	//HAL_TIM_OnePulse_Start_IT(obj, TIM_OPMODE_SINGLE);
	HAL_TIM_Base_Start_IT(obj);
}


// 中断回调函数（定时器溢出时执行），需要在HAL_TIM_PeriodElapsedCallback函数中调用
void HAL_TIM_PeriodElapsedCallback_X(TIM_HandleTypeDef *htim) {
  if(htim->Instance == TxTimeoutTimer.Instance) {
    // 此处执行定时任务
	  if (NULL != TxCallBack)
	  {
		  TxCallBack();
	  }
  }
  else if(htim->Instance == RxTimeoutTimer.Instance) {
	    // 此处执行定时任务
	  if (NULL != RxCallBack)
	  {
		  RxCallBack();
	  }
  }

}

