/*
 * timer.h
 *
 *  Created on: Jun 5, 2025
 *      Author: bitstream
 */

#ifndef RADIO_SX126X_TIMER_H_
#define RADIO_SX126X_TIMER_H_

#include "tim.h"


void HAL_TIM_PeriodElapsedCallback_X(TIM_HandleTypeDef *htim);
void TimerInit(TIM_HandleTypeDef *obj, void ( *callback )( void ));
uint32_t TimerGetCurrentTime(void);
uint32_t TimerGetElapsedTime( uint32_t eventInTime );
void TimerSetValue( TIM_HandleTypeDef *obj, uint32_t value );
void TimerStop( TIM_HandleTypeDef *obj );
void TimerStart( TIM_HandleTypeDef *obj );

#endif /* RADIO_SX126X_TIMER_H_ */
