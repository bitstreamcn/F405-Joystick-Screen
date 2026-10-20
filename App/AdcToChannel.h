/*
 * AdcToChannel.h
 *
 *  Created on: Jun 15, 2025
 *      Author: bitstream
 */
#include <stdint.h>

#ifndef ADCTOCHANNEL_H_
#define ADCTOCHANNEL_H_

extern uint16_t adc_min;
extern uint16_t adc_max;

uint16_t process_adc_input(uint8_t channel, uint16_t raw_adc);


#endif /* ADCTOCHANNEL_H_ */
