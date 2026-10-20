/*
 * AdcToChannel.c
 *
 *  Created on: Jun 15, 2025
 *      Author: bitstream
 */

#include "AdcToChannel.h"
#include "crsf_protocol2.h"
#include <math.h>

#define MAX_DELTA 100  // 最大变化值，限制 ADC 的最大波动
#define DEAD_ZONE 20
#define DEAD_ZONE2 10

#define abs(x) ((x)>=0?(x):-(x))

uint16_t apply_dead_zone(uint16_t prev_val, uint16_t new_val)
{
    if (abs((int)new_val - (int)prev_val) < DEAD_ZONE)
        return prev_val; // 忽略变化
    // 计算当前值与上次值的差异
    int16_t delta = new_val - prev_val;
    // 如果差异超过最大变化值，限制为最大变化值
    if (delta > MAX_DELTA)
    {
    	new_val = prev_val + MAX_DELTA;  // 限制上升幅度
    }
    else if (delta < -MAX_DELTA)
    {
    	new_val = prev_val - MAX_DELTA;  // 限制下降幅度
    }
    // 更新上次 ADC 值
    return new_val;
}

uint16_t apply_dead_zone2(uint16_t prev_val, uint16_t new_val)
{
    if (abs((int)new_val - (int)prev_val) < DEAD_ZONE2)
        return prev_val; // 忽略变化
    // 计算当前值与上次值的差异
    int16_t delta = new_val - prev_val;
    // 如果差异超过最大变化值，限制为最大变化值
    if (delta > MAX_DELTA)
    {
    	new_val = prev_val + MAX_DELTA;  // 限制上升幅度
    }
    else if (delta < -MAX_DELTA)
    {
    	new_val = prev_val - MAX_DELTA;  // 限制下降幅度
    }
    // 更新上次 ADC 值
    return new_val;
}

#define FILTER_DEPTH 20

uint16_t adc_buffer[4][FILTER_DEPTH];
uint8_t filter_index[4] = {0, 0, 0, 0};

uint16_t moving_average_filter(uint8_t channel, uint16_t new_sample)
{
    adc_buffer[channel][filter_index[channel]++] = new_sample;
    if (filter_index[channel] >= FILTER_DEPTH)
        filter_index[channel] = 0;

    uint32_t sum = 0;
    for (int i = 0; i < FILTER_DEPTH; ++i)
        sum += adc_buffer[channel][i];

    return (uint16_t)(sum / FILTER_DEPTH);
}

uint16_t map_adc_to_channel(uint16_t adc_value, uint16_t adc_min, uint16_t adc_max)
{
    if (adc_value < adc_min) adc_value = adc_min;
    if (adc_value > adc_max) adc_value = adc_max;

    float dst_value = adc_value - adc_min;
    float rate = 1.0f * dst_value / (adc_max - adc_min);
    float c_value = (CRSF_CHANNEL_VALUE_2000 - CRSF_CHANNEL_VALUE_1000) * rate;

    return CRSF_CHANNEL_VALUE_1000 + (uint16_t)c_value;
}


// 初始化阶段
uint16_t adc_min = 0;   // 实际量程底端（需要校准）
uint16_t adc_max = 4096;  // 实际量程顶端（需要校准）

uint16_t process_adc_input(uint8_t channel, uint16_t raw_adc)
{
    static uint16_t filtered_adc[4] = {0, 0, 0, 0};
    static uint16_t filtered_adc2[4] = {0, 0, 0, 0};
    uint16_t last_output = 0;

    // 1. 死区（可选）
    uint16_t smooth_adc = apply_dead_zone(filtered_adc[channel], raw_adc);
    filtered_adc[channel] = smooth_adc;

    // 2. 均值滤波
    smooth_adc = moving_average_filter(channel, smooth_adc);

    // 3. ADC映射到CRSF
    uint16_t rc_value = map_adc_to_channel(smooth_adc, adc_min, adc_max);

    uint16_t smooth_adc2 = apply_dead_zone2(filtered_adc2[channel], rc_value);
    filtered_adc2[channel] = smooth_adc2;

    // 输出值
    last_output = smooth_adc2;
    return last_output;
}



