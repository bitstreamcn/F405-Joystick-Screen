/*
 * Application.c
 *
 *  Created on: Jun 13, 2025
 *      Author: bitstream
 */
#include "Application.h"
#include "CRSFRadio.h"
//#include "CRSF.h"
#include "crsf2.h"

#include <stdbool.h>
#include "main.h"

#include "adc.h"
#include "AdcToChannel.h"
#include <stdio.h>
#include <string.h>
#include "timer.h"
#include "../Drivers/Board/Board.h"
#include "app_usbx_host.h"
#include "iwdg.h"
#include <math.h>

#define LOG_MAX_LEN 128

void Init();
void Loop();
void JumpToBootloader(void);
void ui_worker();

Application_s Application = {
		Init,
		Loop
};

bool FirstPowerKeyUp = false;
enum eFlightMode run_mode = MODE_CAR;

uint8_t usb_recv[256];
uint8_t usb_recv_index = 0;
bool    usb_recv_done = false;
bool    init_done = false;

uint16_t roll = 0;
uint16_t pitch = 0;
uint16_t throttle = 0;
uint16_t yaw = 0;
static float vbat = 0;
static int led = 0;
static float freq_hz = 0;
static uint16_t adc_values[ADC_CHANNEL_COUNT] = {0, 0, 0, 0, 0, 0};

void Init()
{
	LogI("app init");

	if (RCC->CSR & RCC_CSR_PORRSTF) //冷启动，上电重启
	{
		//处理开机
		for (int i=0;i<100;i++)
		{
		  DelayMs(5);
		  if (HAL_GPIO_ReadPin(POWER_KEY_GPIO_Port, POWER_KEY_Pin) == GPIO_PIN_SET)
		  {
			  break;
		  }
		}
		if (HAL_GPIO_ReadPin(POWER_KEY_GPIO_Port, POWER_KEY_Pin) == GPIO_PIN_SET)
		{
		  LogE("power key set");
		  return;
		}
	}
	else if (RCC->CSR & RCC_CSR_PINRSTF) {
        // 外部引脚复位 (按下了 RESET 按键)
        // 注意：上电时 PINRSTF 通常也会置位，所以判断 POR 要放在前面
        printf("Reset Cause: Pin Reset (NRST)\n");
    }
    else if (RCC->CSR & RCC_CSR_IWDGRSTF) {
        printf("Reset Cause: Independent Watchdog (IWDG)\n");
    }
    else if (RCC->CSR & RCC_CSR_WWDGRSTF) {
        printf("Reset Cause: Window Watchdog (WWDG)\n");
    }
    else if (RCC->CSR & RCC_CSR_SFTRSTF) {
        printf("Reset Cause: Software Reset\n");
    }
    else if (RCC->CSR & RCC_CSR_BORRSTF) {
        printf("Reset Cause: Brown-out Reset (BOR)\n");
    }

	RCC->CSR |= RCC_CSR_RMVF;

    HAL_GPIO_WritePin(TURN_ON_GPIO_Port, TURN_ON_Pin, GPIO_PIN_SET);
    HAL_GPIO_WritePin(LED_EN_GPIO_Port, LED_EN_Pin, GPIO_PIN_SET);

    LogI("board init");
    board_init();

    DelayMs(100);

    LogI("radio init");
	CRSFRadio.Init();

	HAL_ADC_Start_DMA(&hadc1, (uint32_t*)adc_dma_buffer, ADC_CHANNEL_COUNT);

	HAL_GPIO_WritePin(LED_CAR_GPIO_Port, LED_CAR_Pin, GPIO_PIN_SET);
	HAL_GPIO_WritePin(LED_DRONE_GPIO_Port, LED_DRONE_Pin, GPIO_PIN_RESET);

	for (int i=0; i<100; i++)
	{
		process_adc_input(0, (adc_max - adc_min) / 2);
		process_adc_input(1, (adc_max - adc_min) / 2);
		process_adc_input(2, (adc_max - adc_min) / 2);
		process_adc_input(3, (adc_max - adc_min) / 2);
	}

	LogI("app init done");
	init_done = true;
}

uint16_t reserved(uint16_t in)
{
	return adc_max - in;
}

void Loop()
{
	if (!init_done)
	{
		return;
	}
	  //处理关机
	  if (HAL_GPIO_ReadPin(POWER_KEY_GPIO_Port, POWER_KEY_Pin) == GPIO_PIN_RESET)
	  {
		  //printf("POWER_KEY_Pin down\r\n");
		  if (FirstPowerKeyUp)
		  {
			  for (int i=0;i<100;i++)
			  {
				  DelayMs(10);
				  if (HAL_GPIO_ReadPin(POWER_KEY_GPIO_Port, POWER_KEY_Pin) == GPIO_PIN_SET)
				  {
					  break;
				  }
			  }
			  if (HAL_GPIO_ReadPin(POWER_KEY_GPIO_Port, POWER_KEY_Pin) == GPIO_PIN_RESET)
			  {
				  //正式关机
				  cdc_acm_send((uint8_t*)"shutdown\n", 9);

				  HAL_GPIO_WritePin(TURN_ON_GPIO_Port, TURN_ON_Pin, GPIO_PIN_RESET);
				  HAL_GPIO_WritePin(LED_EN_GPIO_Port, LED_EN_Pin, GPIO_PIN_RESET);
			  }
		  }
	  }
	  else{
		  FirstPowerKeyUp = true;
	  }

	  if (adc_value_flag == 1)
	  {
		  memcpy(adc_values, adc_values_new, sizeof(adc_values_new));
		  adc_value_flag = 0;
	  }

		static uint32_t lastSend;
		uint32_t now = TimerGetCurrentTime();
		if (now - lastSend > 1000)
		{
			//输出遥杆数据
			for (int i=1; i<5; i++)
			{
				//uint16_t channel = adc_values[i]; //process_adc_input(i - 1, adc_values[i]);
				//printf("adc[%d] = %d [%d]\r\n", i, adc_values[i], channel);
			}
			lastSend = now;
		}

		/*
左手 左右 = adc[2]   左小  右大
左手 上下 = adc[1]   下小  上大

右手 左右 = adc[4]   左小  右大
右手 上下 = adc[3]   下小  上大
		 */

		uint16_t validate_channel = 0;
		{
			uint8_t  channel_no = 4;
			uint16_t adcvalue = reserved(adc_values[channel_no]);
			//自动校准///////////////
			static uint32_t cal_total = 0;
			static uint16_t cal_count = 0;
			static int16_t cal_value = 0;
			if (cal_count < 50 && abs(adcvalue - 2048) < 50)
			{
				cal_total += adcvalue;
				cal_count ++;
			}
			else
			{
				if (cal_value == 0 && cal_count >= 50)
				{
					cal_value = 2048 - (int16_t)(cal_total / cal_count);
				}
			}
			int16_t cal_adcvalue = (int16_t)adcvalue + cal_value;
			if (cal_adcvalue < 0)
			{
				cal_adcvalue = 0;
			}
			adcvalue = cal_adcvalue;
			///////////////////////
			uint16_t channel = process_adc_input(channel_no - 1, adcvalue);
			//CrsfSerial_setChannel(1, channel); //Roll横滚
			roll = crsf_map(channel, CRSF_CHANNEL_VALUE_1000, CRSF_CHANNEL_VALUE_2000, 1000, 2000);
			crsf2_setChannel(1, channel);
			//printf("roll = %d\r\n", channel);
			//CrsfSerial_setChannel(11, channel); //对照验证
			crsf2_setChannel(11, channel);
			validate_channel += channel;
		}

		{
			uint8_t  channel_no = 3;
			uint16_t adcvalue = reserved(adc_values[channel_no]);
			//自动校准///////////////
			static uint32_t cal_total = 0;
			static uint16_t cal_count = 0;
			static int16_t cal_value = 0;
			if (cal_count < 50 && abs(adcvalue - 2048) < 50)
			{
				cal_total += adcvalue;
				cal_count ++;
			}
			else
			{
				if (cal_value == 0 && cal_count >= 50)
				{
					cal_value = 2048 - (int16_t)(cal_total / cal_count);
				}
			}
			int16_t cal_adcvalue = (int16_t)adcvalue + cal_value;
			if (cal_adcvalue < 0)
			{
				cal_adcvalue = 0;
			}
			adcvalue = cal_adcvalue;
			///////////////////////
			uint16_t channel = process_adc_input(channel_no - 1, adcvalue);
			//CrsfSerial_setChannel(2, channel); //俯仰（Pitch）
			pitch = crsf_map(channel, CRSF_CHANNEL_VALUE_1000, CRSF_CHANNEL_VALUE_2000, 1000, 2000);
			crsf2_setChannel(2, channel);
			//printf("pitch = %d\r\n", channel);
			//CrsfSerial_setChannel(12, channel); //对照验证
			crsf2_setChannel(12, channel);

			validate_channel += channel;
		}

		{
			static uint16_t prev_value = 0;
			uint8_t  channel_no = 1;
			uint16_t adcvalue = reserved(adc_values[channel_no]);
			//自动校准///////////////
			static uint32_t cal_total = 0;
			static uint16_t cal_count = 0;
			static int16_t cal_value = 0;
			if (cal_count < 50 && abs(adcvalue - 2048) < 50)
			{
				cal_total += adcvalue;
				cal_count ++;
			}
			else
			{
				if (cal_value == 0 && cal_count >= 50)
				{
					cal_value = 2048 - (int16_t)(cal_total / cal_count);
				}
			}
			int16_t cal_adcvalue = (int16_t)adcvalue + cal_value;
			if (cal_adcvalue < 0)
			{
				cal_adcvalue = 0;
			}
			adcvalue = cal_adcvalue;
			///////////////////////
			uint16_t channel = process_adc_input(channel_no - 1, adcvalue);
			//CrsfSerial_setChannel(3, channel); //油门（Throttle）
			throttle = crsf_map(channel, CRSF_CHANNEL_VALUE_1000, CRSF_CHANNEL_VALUE_2000, 1000, 2000);
			crsf2_setChannel(3, channel);
			if (channel < CRSF_CHANNEL_VALUE_MID && prev_value > (CRSF_CHANNEL_VALUE_MID + 20))
			{
				printf("throttle: %d => %d\r\n", prev_value, channel);
			}
			prev_value = channel;
			//CrsfSerial_setChannel(13, channel); //对照验证
			crsf2_setChannel(13, channel);
			validate_channel += channel;
		}

		{
			uint8_t  channel_no = 2;
			uint16_t adcvalue = reserved(adc_values[channel_no]);
			//自动校准///////////////
			static uint32_t cal_total = 0;
			static uint16_t cal_count = 0;
			static int16_t cal_value = 0;
			if (cal_count < 50 && abs(adcvalue - 2048) < 50)
			{
				cal_total += adcvalue;
				cal_count ++;
			}
			else
			{
				if (cal_value == 0 && cal_count >= 50)
				{
					cal_value = 2048 - (int16_t)(cal_total / cal_count);
				}
			}
			int16_t cal_adcvalue = (int16_t)adcvalue + cal_value;
			if (cal_adcvalue < 0)
			{
				cal_adcvalue = 0;
			}
			adcvalue = cal_adcvalue;
			///////////////////////
			uint16_t channel = process_adc_input(channel_no - 1, adcvalue);
			//CrsfSerial_setChannel(4, channel); //偏航（Yaw）
			yaw = crsf_map(channel, CRSF_CHANNEL_VALUE_1000, CRSF_CHANNEL_VALUE_2000, 1000, 2000);
			crsf2_setChannel(4, channel);
			//printf("yaw = %d\r\n", channel);
			//CrsfSerial_setChannel(14, channel); //对照验证
			crsf2_setChannel(14, channel);

			validate_channel += channel;
		}

		//验证通道
		validate_channel = CRSF_CHANNEL_VALUE_1000 + validate_channel % (CRSF_CHANNEL_VALUE_2000 - CRSF_CHANNEL_VALUE_1000);
		//CrsfSerial_setChannel(15, validate_channel);
		crsf2_setChannel(15, validate_channel);

#if 0
		static uint32_t ModeTapTime = 0;
		static enum eFlightMode prev_mode = MODE_NONE;
		static bool btn_change_mode_down = false;
		if (HAL_GPIO_ReadPin(BTN_CHANG_MODE_GPIO_Port, BTN_CHANG_MODE_Pin) == GPIO_PIN_RESET)
		{
			if (ModeTapTime == 0)
			{
				ModeTapTime = now;
			}
			if ((now - ModeTapTime) > 200)
			{
				if (!btn_change_mode_down)
				{
					prev_mode = run_mode;
				}
				if (btn_change_mode_down && prev_mode != run_mode)
				{
					//遥测已经返回切换结果
					//CrsfSerial_setChannel(5, 1000);
					crsf2_setChannel(5, CRSF_CHANNEL_VALUE_1000);
				}
				else
				{
					//CrsfSerial_setChannel(5, 2000); //车/无人机 模式
					crsf2_setChannel(5, CRSF_CHANNEL_VALUE_2000);
				}
				btn_change_mode_down = true;
			}
		}
		else
		{
			btn_change_mode_down = false;
			ModeTapTime = 0;
			//CrsfSerial_setChannel(5, 1000);
			crsf2_setChannel(5, CRSF_CHANNEL_VALUE_1000);
		}
#endif
		if (HAL_GPIO_ReadPin(ARMED_GPIO_Port, ARMED_Pin) == GPIO_PIN_SET)
		{
			//CrsfSerial_setChannel(6, 2000);  // 车灯
			crsf2_setChannel(6, CRSF_CHANNEL_VALUE_2000);
		}
		else
		{
			//CrsfSerial_setChannel(6, 1000);
			crsf2_setChannel(6, CRSF_CHANNEL_VALUE_1000);
		}
#if 0
		static uint32_t HomeTapTime = 0;
		static bool btn_home_down = false;
		if (HAL_GPIO_ReadPin(BTN_HOME_GPIO_Port, BTN_HOME_Pin) == GPIO_PIN_RESET)
		{
			if (HomeTapTime == 0)
			{
				HomeTapTime = now;
			}
			if ((now - HomeTapTime) > 200)
			{
				if (!btn_home_down)
				{
					prev_mode = run_mode;
				}
				if (btn_home_down && prev_mode != run_mode)
				{
					//遥测已经返回切换结果
					//CrsfSerial_setChannel(7, 1000);
					crsf2_setChannel(7, CRSF_CHANNEL_VALUE_1000);
				}
				else
				{
					//CrsfSerial_setChannel(7, 2000); //返航
					crsf2_setChannel(7, CRSF_CHANNEL_VALUE_2000);
				}
				btn_home_down = true;
			}
		}
		else
		{
			btn_home_down = false;
			HomeTapTime = 0;
			//CrsfSerial_setChannel(7, 1000);
			crsf2_setChannel(7, CRSF_CHANNEL_VALUE_1000);
		}
#endif
	CRSFRadio.Loop();

	//统计刷新频率
    static uint32_t last_tick = 0;
    static uint32_t call_count = 0;
    const uint32_t now_tick = HAL_GetTick();
    const uint32_t diff_ms = now_tick - last_tick;
    call_count++;

    if (diff_ms >= 1000) {
        freq_hz = (float)call_count * 1000.0f / diff_ms;
        call_count = 0;
        last_tick = now_tick;
    }

	HAL_IWDG_Refresh(&hiwdg);


	DELAY_MS(1);
}

void JumpToBootloader(void)
{
    // 1. 关闭全局中断，复位 SysTick
    __disable_irq();
    SysTick->CTRL = 0;
    SysTick->LOAD = 0;
    SysTick->VAL = 0;

    // 2. 关闭 USB OTG FS 外设时钟 (F4 专有安全断开方式)
    // 当 OTG 时钟关闭后，内部 PHY 停止工作，D+ 线被释放，电脑会立刻识别为设备拔出
    __HAL_RCC_USB_OTG_FS_CLK_DISABLE();

    // 粗略延时几十毫秒，确保电脑识别到设备断开
    for (volatile uint32_t i = 0; i < 1000000; i++) { __NOP(); }

    // 3. 恢复默认时钟（使用 HSI），关闭所有中断并清除挂起标志
    HAL_RCC_DeInit();

    // 清除所有 NVIC 中断
    for (uint32_t i = 0; i < sizeof(NVIC->ICER) / sizeof(NVIC->ICER[0]); i++)
    {
        NVIC->ICER[i] = 0xFFFFFFFF;
        NVIC->ICPR[i] = 0xFFFFFFFF;
    }

    // 4. 设置重映射到系统 Flash
    __HAL_SYSCFG_REMAPMEMORY_SYSTEMFLASH();

    uint32_t BootAddr = 0x1FFF0000;  // STM32F4 系统存储器基地址
    void (*SysMemBootJump)(void) = (void (*)(void)) (*((uint32_t *)(BootAddr + 4)));

    // 5. 设置主堆栈指针并跳转
    __set_MSP(*(uint32_t *)BootAddr);
    SysMemBootJump();

    while (1) { }
}


void Enter_DFU_With_IWDG(void)
{
    // 1. 解除 IWDG 写保护
    IWDG->KR = 0x5555;

    // 2. 设置最大预分频系数 256
    IWDG->PR = 0x06;

    // 3. 设置最大重载值 4095
    IWDG->RLR = 0x0FFF;

    // 4. 喂一次狗，应用新设置
    IWDG->KR = 0xAAAA;

    // 5. 立刻跳转到 DFU (此时 IWDG 会在 32 秒后超时)
    JumpToBootloader();
}


void cdc_acm_receive(uint8_t ch)
{
	if (usb_recv_done)
	{
		return;
	}
	if (usb_recv_index >= sizeof(usb_recv))
	{
		usb_recv_index = 0;
	}
	usb_recv[usb_recv_index] = ch;
	if (ch == '\n' || ch == '\r')
	{
		usb_recv[usb_recv_index] = '\0';
		usb_recv_done = true;
	}
	usb_recv_index++;
}

static void LogWrite(const char * tag, const char *format, va_list ap)
{
    char buffer[LOG_MAX_LEN];

    int taglen = strlen(tag);

    char * p = buffer;
    snprintf(buffer, LOG_MAX_LEN, "%s", tag);
    p += taglen;

    vsnprintf(p, LOG_MAX_LEN - taglen, format, ap);
    strcat(buffer, "\n");
    uint32_t len = strlen(buffer);

	if (!cdc_acm_send((uint8_t*)buffer, len))
	{
		//串口出问题，亮灯显示错误
		//led.Error();
	}
}


void LogI(const char *format, ...)
{
    va_list ap;
    va_start(ap, format);
    LogWrite("[I] ", format, ap);
    va_end(ap);
}
void LogW(const char *format, ...)
{
    va_list ap;
    va_start(ap, format);
    LogWrite("[W] ", format, ap);
    va_end(ap);
}
void LogE(const char *format, ...)
{
    va_list ap;
    va_start(ap, format);
    LogWrite("[E] ", format, ap);
    va_end(ap);
}

void ui_worker()
{
	static uint32_t times = 0;
	times++;
	uint32_t now = TimerGetCurrentTime();

    //计算电压
    float  vref = adc_values[0];
#define VREFINT_CALIBRATION_VOLTAGE 1.21f // STM32F4 内部基准电压标称值 1.21V
    uint16_t adc_value = adc_values[5];
#define DIV_RATIO 11.0f      // 分压比 (10k + 1k) / 1k = 11
    // 公式: (ADC值 / 4095) * 3.3V * 11(分压比)
    float voltage = ((float)adc_value * VREFINT_CALIBRATION_VOLTAGE) / (float)vref * DIV_RATIO;

    //一阶低通滤波
#define lpf_rate 0.01
    if (vbat == 0)vbat = voltage;
    if (isnan(vbat))vbat = 0;
    vbat = vbat * (1 - lpf_rate) + voltage * lpf_rate;

	//处理手机下发的命令
	bool changemode = false;
	bool return_home = false;

	static int climb_power = 0;//攀爬力度
	if (usb_recv_done)
	{
		//查找=号
		int idx = -1;
		for (int i=0; i<usb_recv_index; i++)
		{
			if (usb_recv[i] == '=')
			{
				idx = i;
				break;
			}
		}
		if (idx > -1)
		{
			if (memcmp(usb_recv, "mode", idx) == 0)
			{
				if (memcmp(&usb_recv[idx+1], "1", 1) == 0)
				{

				}
				else if (memcmp(&usb_recv[idx+1], "2", 1) == 0)
				{

				}
			}
			else if (memcmp(usb_recv, "cmd", idx) == 0)
			{
				if (memcmp(&usb_recv[idx+1], "climb", usb_recv_index - idx - 2) == 0)
				{

				}
				else if (memcmp(&usb_recv[idx+1], "rth", usb_recv_index - idx - 2) == 0)
				{
					return_home = true;
				}
				else if (memcmp(&usb_recv[idx+1], "rth_cancel", usb_recv_index - idx - 2) == 0)
				{

				}
				else if (memcmp(&usb_recv[idx+1], "changemode", usb_recv_index - idx - 2) == 0)
				{
					changemode = true;
				}
				else if (memcmp(&usb_recv[idx+1], "update", usb_recv_index - idx - 2) == 0)
				{
					LogI("update...");
					//闪灯10秒，这时候要按住开机键，否则进入bootloader后会断电导致刷机失败
					bool switchled = false;
					for (int i = 0; i<20; i++)
					{
						HAL_GPIO_WritePin(LED_EN_GPIO_Port, LED_EN_Pin, switchled ? GPIO_PIN_SET : GPIO_PIN_RESET);
						switchled = !switchled;
						DELAY_MS(500);
					}
					HAL_GPIO_WritePin(LED_EN_GPIO_Port, LED_EN_Pin, GPIO_PIN_SET);
					Enter_DFU_With_IWDG();
				}
			}
			else if (memcmp(usb_recv, "light", idx) == 0)
			{
				if (memcmp(&usb_recv[idx+1], "1", 1) == 0)
				{
					led = 1;
					crsf2_setChannel(8, CRSF_CHANNEL_VALUE_2000);
				}
				else if (memcmp(&usb_recv[idx+1], "0", 1) == 0)
				{
					led = 0;
					crsf2_setChannel(8, CRSF_CHANNEL_VALUE_1000);
				}
			}
			else if (memcmp(usb_recv, "climb_power", idx) == 0)
			{
				climb_power = atoi(&usb_recv[idx+1]);
			}

		}
		usb_recv_index = 0;
		usb_recv_done = false;
	}

	static uint32_t ModeTapTime = 0;
	static enum eFlightMode prev_mode = MODE_NONE;
	static bool btn_change_mode_down = false;
	static uint32_t cmtime = 0;
	if (changemode)
	{
		cmtime = now;
	}
	if (cmtime > 0 && (now - cmtime) < 2000)
	{
		if (ModeTapTime == 0)
		{
			ModeTapTime = now;
		}
		if ((now - ModeTapTime) > 200)
		{
			if (!btn_change_mode_down)
			{
				prev_mode = run_mode;
			}
			if (btn_change_mode_down && prev_mode != run_mode)
			{
				//遥测已经返回切换结果
				//CrsfSerial_setChannel(5, 1000);
				crsf2_setChannel(5, CRSF_CHANNEL_VALUE_1000);
				cmtime = 0;
			}
			else
			{
				//CrsfSerial_setChannel(5, 2000); //车/无人机 模式
				crsf2_setChannel(5, CRSF_CHANNEL_VALUE_2000);
			}
			btn_change_mode_down = true;
		}
	}
	else
	{
		btn_change_mode_down = false;
		ModeTapTime = 0;
		crsf2_setChannel(5, CRSF_CHANNEL_VALUE_1000);
		cmtime = 0;
	}

	static uint32_t rthtime = 0;
	if (return_home)
	{
		rthtime = now;
	}
	if (rthtime > 0 && (now - rthtime) < 2000)
	{
		crsf2_setChannel(7, CRSF_CHANNEL_VALUE_2000);
	}
	else
	{
		crsf2_setChannel(7, CRSF_CHANNEL_VALUE_1000);
	}

	//统计刷新频率
    static uint32_t last_tick = 0;
    const uint32_t now_tick = HAL_GetTick();
    const uint32_t diff_ms = now_tick - last_tick;
    if (diff_ms >= 5000) {
        last_tick = now_tick;
        LogI("rate:%0.2f", freq_hz);
    }

	//发送到手机
	//cdc_acm_send(usb_recv, 128); //每次发送128字节的情况下，能达到最高100KB速度
	static uint32_t prev_update_tm = 0;
	uint32_t remain_tm = now - prev_update_tm;
	if (remain_tm > 100)
	{
		prev_update_tm = now;
		LogI("roll:%d, pitch:%d, throttle:%d, yaw:%d, bat:%.2f, mode:%d, led:%d, times:%ld\n",
			roll, pitch, throttle, yaw, vbat, run_mode, led, times);
	}
}

void ui(void)
{
	while(1)
	{
		ui_worker();
		DELAY_MS(10);
	}
}



