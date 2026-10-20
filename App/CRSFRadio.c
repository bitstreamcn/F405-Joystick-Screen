/*
 * CRSFRadio.c
 *
 *  Created on: Jun 13, 2025
 *      Author: bitstream
 */
#include "CRSFRadio.h"

#include "main.h"
#include "Application.h"

#include "radio.h"
#include "tim.h"
#include "sx126x/sx126x.h"
#include "stm32f4xx_hal_tim.h"
//#include "CRSF.h"
#include "crsf2.h"
#include <string.h>



#if defined( USE_BAND_868 )

#define RF_FREQUENCY                                868000000 // Hz

#elif defined( USE_BAND_915 )

#define RF_FREQUENCY                                915000000 // Hz

#else
    #error "Please define a frequency band in the compiler options."
#endif

#define TX_OUTPUT_POWER                             1 //14        // dBm 最大22dBm 默认14dBm  射频芯片输出功率不能大于9dBm，否则烧毁

#if defined( USE_MODEM_LORA )

#define LORA_BANDWIDTH                              2//2         // [0: 125 kHz,
                                                              //  1: 250 kHz,
                                                              //  2: 500 kHz,
                                                              //  3: Reserved]
#define LORA_SPREADING_FACTOR                       7         // [SF7..SF12]
#define LORA_CODINGRATE                             1         // [1: 4/5,
                                                              //  2: 4/6,
                                                              //  3: 4/7,
                                                              //  4: 4/8]
#define LORA_PREAMBLE_LENGTH                        6         // Same for Tx and Rx
#define LORA_SYMBOL_TIMEOUT                         0         // Symbols
#define LORA_FIX_LENGTH_PAYLOAD_ON                  false
#define LORA_IQ_INVERSION_ON                        false

#elif defined( USE_MODEM_FSK )

#define FSK_FDEV                                    25e3      // Hz
#define FSK_DATARATE                                50e3      // bps
#define FSK_BANDWIDTH                               100e3     // Hz >> DSB in sx126x
#define FSK_AFC_BANDWIDTH                           100e3     // Hz
#define FSK_PREAMBLE_LENGTH                         8         // Same for Tx and Rx
#define FSK_FIX_LENGTH_PAYLOAD_ON                   false

#else
    #error "Please define a modem in the compiler options."
#endif

typedef enum
{
    LOWPOWER,
    RX,
    RX_TIMEOUT,
    RX_ERROR,
    TX,
    TX_TIMEOUT,
}States_t;

#define RX_TIMEOUT_VALUE                            100
#define BUFFER_SIZE                                 255 // Define the payload size here

const uint8_t PingMsg[] = "PING";
const uint8_t PongMsg[] = "PONG";

uint16_t BufferSize = BUFFER_SIZE;
uint8_t Buffer[BUFFER_SIZE];

States_t State = LOWPOWER;

int8_t RssiValue = 0;
int8_t SnrValue = 0;

bool StartTimer = false;

#define CHANNEL_SEND_INTERVAL_MS    5U

/*!
 * Radio events function pointer
 */
static RadioEvents_t RadioEvents;

/*!
 * \brief Function to be executed on Radio Tx Done event
 */
void OnTxDone( void );

/*!
 * \brief Function to be executed on Radio Rx Done event
 */
void OnRxDone( uint8_t *payload, uint16_t size, int16_t rssi, int8_t snr );

/*!
 * \brief Function executed on Radio Tx Timeout event
 */
void OnTxTimeout( void );

/*!
 * \brief Function executed on Radio Rx Timeout event
 */
void OnRxTimeout( void );

/*!
 * \brief Function executed on Radio Rx Error event
 */
void OnRxError( void );


void OnFlightMode(enum eFlightMode mode);
void OnPacketIn();

static void Init();
static void Loop();

CRSFRadio_s CRSFRadio = {
		Init,
		Loop
};


static void Init()
{
	DELAY_MS(10);

    // Radio initialization
    RadioEvents.TxDone = OnTxDone;
    RadioEvents.RxDone = OnRxDone;
    RadioEvents.TxTimeout = OnTxTimeout;
    RadioEvents.RxTimeout = OnRxTimeout;
    RadioEvents.RxError = OnRxError;

    Radio.Init( &RadioEvents );

    Radio.SetChannel( RF_FREQUENCY );

#if defined( USE_MODEM_LORA )

    Radio.SetTxConfig( MODEM_LORA, TX_OUTPUT_POWER, 0, LORA_BANDWIDTH,
                                   LORA_SPREADING_FACTOR, LORA_CODINGRATE,
                                   LORA_PREAMBLE_LENGTH, LORA_FIX_LENGTH_PAYLOAD_ON,
                                   true, 0, 0, LORA_IQ_INVERSION_ON, 3000 );

    Radio.SetRxConfig( MODEM_LORA, LORA_BANDWIDTH, LORA_SPREADING_FACTOR,
                                   LORA_CODINGRATE, 0, LORA_PREAMBLE_LENGTH,
                                   LORA_SYMBOL_TIMEOUT, LORA_FIX_LENGTH_PAYLOAD_ON,
                                   0, true, 0, 0, LORA_IQ_INVERSION_ON, false );

#elif defined( USE_MODEM_FSK )

    Radio.SetTxConfig( MODEM_FSK, TX_OUTPUT_POWER, FSK_FDEV, 0,
                                  FSK_DATARATE, 0,
                                  FSK_PREAMBLE_LENGTH, FSK_FIX_LENGTH_PAYLOAD_ON,
                                  true, 0, 0, 0, 3000 );

    Radio.SetRxConfig( MODEM_FSK, FSK_BANDWIDTH, FSK_DATARATE,
                                  0, FSK_AFC_BANDWIDTH, FSK_PREAMBLE_LENGTH,
                                  0, FSK_FIX_LENGTH_PAYLOAD_ON, 0, true,
                                  0, 0,false, true );

#else
    #error "Please define a frequency band in the compiler options."
#endif

    RadioStatus_t status = SX126xGetStatus();
    if (status.Value == 0x00 || status.Value == 0xff || status.Value == 0xa8)
    {
    	LogE("radio status error");
    }

	//下面两行是测试模块通信是否正常
	uint16_t s = SX126xGetIrqStatus();
	RadioError_t err = SX126xGetDeviceErrors();
	(void)s;
	(void)err;
	PacketStatus_t pktStatus;
	SX126xGetPacketStatus( &pktStatus );

	SX126xClearIrqStatus( IRQ_RADIO_ALL );
	//HAL_GPIO_WritePin(RF_RXEN_GPIO_Port, RF_RXEN_Pin, GPIO_PIN_SET);
	//Radio.Rx( RX_TIMEOUT_VALUE );
	State = RX_TIMEOUT;
	DELAY_MS(10);

	//CrsfInit(CRSF_BAUDRATE);
	crsf2_begin();
	CrsfSerialEvent2.onPacketFlightMode = OnFlightMode;
	CrsfSerialEvent2.onPacketIn = OnPacketIn;




}

uint32_t recvTime = 0;
bool IsConnect = false;

bool isback = false;
uint32_t backTime = 0;

bool reset_radio = false;
bool is_rx_timeout = false;

uint32_t rx_timeout_count = 0;

static void Loop()
{
	if (reset_radio)
	{
		//LogE("reset radio");
		/*
		HAL_GPIO_WritePin(RF_NRST_GPIO_Port, RF_NRST_Pin, GPIO_PIN_RESET);
		DELAY_MS(10); // 拉低至少 100us
		HAL_GPIO_WritePin(RF_NRST_GPIO_Port, RF_NRST_Pin, GPIO_PIN_SET);
		DELAY_MS(50); // 等待芯片上电稳定
		*/
		Init();
		reset_radio = false;
	}

	uint32_t now = HAL_GetTick();
	if (isback && (now - backTime) > RX_TIMEOUT_VALUE)
	{
		State = RX_TIMEOUT;
		isback = false;
	}
    switch( State )
    {
    case RX_TIMEOUT:
    case TX_TIMEOUT:
  	    //发送超时，继续发送
	case RX_ERROR:
		if ((now - recvTime) > 2000 && IsConnect)
		{
		  //接收不到数据，关闭led
		  HAL_GPIO_WritePin(LED_CONNECT_GPIO_Port, LED_CONNECT_Pin, GPIO_PIN_RESET);
		  IsConnect = false;
		}
	    SX126xClearIrqStatus( IRQ_RADIO_ALL );
    case RX:
    case TX:
  	  //接收完成，继续发送
  	  //读取摇杆数据
		// insert mixer logic here and use crsf.setChannel() to set all the channel values
		// This just increments channel 1/8 value by 1/2us every time, wrapping around
		//mixer(1, 1);
		//mixer(8, 2);
		// End mixer code
    	{
    		if (State == RX)
    		{
    			isback = false;
    		}
    		if (State == TX)
    		{
    			if (isback)
    			{
    				backTime = now;
    				HAL_GPIO_WritePin(RF_RXEN_GPIO_Port, RF_RXEN_Pin, GPIO_PIN_SET);
    				//SX126xSetStandby( STDBY_RC ); // 强制回到待机模式
    				// 可以加一个极短的延时，让 PLL 彻底关闭
    				//DELAY_MS(1);
    				//SX126xClearIrqStatus(IRQ_RADIO_ALL); // 写入 0xFFFF
    				Radio.RxBoosted( RX_TIMEOUT_VALUE );
    				State = LOWPOWER;
    				if (StartTimer)
    				{
    					StartTimer = false;
    					HAL_TIM_Base_Stop_IT(&htim14);
    				}
    				break;
    			}
    		}
			static uint32_t lastSend;
			if (now - lastSend < CHANNEL_SEND_INTERVAL_MS)
			{
				//距离上次发送的时间太短，什么也不做
				break;
			}
			lastSend = now;

			HAL_GPIO_WritePin(RF_RXEN_GPIO_Port, RF_RXEN_Pin, GPIO_PIN_RESET);

			static uint32_t last_back = 0;
			if (last_back == 0)
			{
				last_back = now;
			}

			static uint8_t send_times = 0;

			//1秒请求一次响应
			if ((now - last_back) > 300 || send_times >= 10/* || is_rx_timeout*/)
			{
				send_times = 0;
				isback = true;
				last_back = now;
				backTime = now;
				uint8_t mode = 0;
				crsf2_writePacket(CRSF_ADDRESS_CRSF_TRANSMITTER, CRSF_FRAMETYPE_FLIGHT_MODE, &mode, sizeof(mode));
				is_rx_timeout = false;
			}
			else
			{
				crsf2_sendChannels();
				send_times++;
			}
			State = LOWPOWER;
			if (StartTimer)
			{
				StartTimer = false;
				HAL_TIM_Base_Stop_IT(&htim14);
			}
			break;
    	}
    //case TX:
        //发送完成，看看有没有响应包
    	//HAL_GPIO_WritePin(RF_RXEN_GPIO_Port, RF_RXEN_Pin, GPIO_PIN_SET);
        //Radio.Rx( RX_TIMEOUT_VALUE );
        //State = LOWPOWER;
        //break;
    case LOWPOWER:
  	  if (!StartTimer)
  	  {
  		    //在高频头因为错误不返回事件的时候，超过500MS就开始重置状态
				StartTimer = true;
				// 停止定时器
				HAL_TIM_Base_Stop_IT(&htim14);
				// 清除计数器值（从0开始计数）
				__HAL_TIM_SET_COUNTER(&htim14, 0);
				// 清除更新中断标志（避免立即触发）
				__HAL_TIM_CLEAR_FLAG(&htim14, TIM_FLAG_UPDATE);
				HAL_TIM_Base_Start_IT(&htim14);
  	  }
    default:
        // Set low power
        break;
    }


	  /*
    switch( State )
    {
    case RX:

            //if( BufferSize > 0 )
            {
          	  HAL_GPIO_WritePin(LED_CONNECT_GPIO_Port, LED_CONNECT_Pin, GPIO_PIN_SET);
                //if( strncmp( ( const char* )Buffer, ( const char* )PongMsg, 4 ) == 0 )
                {
                    // Send the next PING frame
                    Buffer[0] = 'P';
                    Buffer[1] = 'I';
                    Buffer[2] = 'N';
                    Buffer[3] = 'G';
                    // We fill the buffer with numbers for the payload
                    for(int i = 4; i < BufferSize; i++ )
                    {
                        Buffer[i] = i - 4;
                    }
                    DelayMs( 1 );
                    Radio.Send( Buffer, BufferSize );
                }

            }

        State = LOWPOWER;
        break;
    case TX:
        // Indicates on a LED that we have sent a PING [Master]
        // Indicates on a LED that we have sent a PONG [Slave]
        //GpioToggle( &Led2 );
        Radio.Rx( RX_TIMEOUT_VALUE );
        State = LOWPOWER;
        break;
    case RX_TIMEOUT:
    case RX_ERROR:
  	  HAL_GPIO_WritePin(LED_CONNECT_GPIO_Port, LED_CONNECT_Pin, GPIO_PIN_RESET);
            // Send the next PING frame
            Buffer[0] = 'P';
            Buffer[1] = 'I';
            Buffer[2] = 'N';
            Buffer[3] = 'G';
            for(int i = 4; i < BufferSize; i++ )
            {
                Buffer[i] = i - 4;
            }
            DelayMs( 1 );
            //HAL_GPIO_WritePin(RF_RXEN_GPIO_Port, RF_RXEN_Pin, GPIO_PIN_RESET);
            Radio.Send( Buffer, BufferSize );

        State = LOWPOWER;
        break;
    case TX_TIMEOUT:
  	  HAL_GPIO_WritePin(LED_CONNECT_GPIO_Port, LED_CONNECT_Pin, GPIO_PIN_RESET);
  	  //HAL_GPIO_WritePin(RF_RXEN_GPIO_Port, RF_RXEN_Pin, GPIO_PIN_SET);
        Radio.Rx( RX_TIMEOUT_VALUE );
        State = LOWPOWER;
        break;
    case LOWPOWER:
  	  if (!StartTimer)
  	  {
				StartTimer = true;
				// 停止定时器
				//HAL_TIM_Base_Stop_IT(&htim14);
				// 清除计数器值（从0开始计数）
				//__HAL_TIM_SET_COUNTER(&htim14, 0);
				// 清除更新中断标志（避免立即触发）
				//__HAL_TIM_CLEAR_FLAG(&htim14, TIM_FLAG_UPDATE);
				//HAL_TIM_Base_Start_IT(&htim14);
  	  }
    default:
        // Set low power
        break;
    }
	   */



    //TimerLowPowerHandler( );
    // Process Radio IRQ
    Radio.IrqProcess( );

    crsf2_update();

}


void OnPacketIn()
{
	recvTime = HAL_GetTick();
	//if (!IsConnect)
	{
		HAL_GPIO_WritePin(LED_CONNECT_GPIO_Port, LED_CONNECT_Pin, GPIO_PIN_SET);
		IsConnect = true;
	}
}

extern enum eFlightMode run_mode;

void OnFlightMode(enum eFlightMode mode)
{
	run_mode = mode;
	if (mode == MODE_CAR)
	{
		HAL_GPIO_WritePin(LED_CAR_GPIO_Port, LED_CAR_Pin, GPIO_PIN_SET);
		HAL_GPIO_WritePin(LED_DRONE_GPIO_Port, LED_DRONE_Pin, GPIO_PIN_RESET);
		HAL_GPIO_WritePin(LED_HOME_GPIO_Port, LED_HOME_Pin, GPIO_PIN_RESET);
	}
	else if (mode == MODE_DRONE)
	{
		HAL_GPIO_WritePin(LED_CAR_GPIO_Port, LED_CAR_Pin, GPIO_PIN_RESET);
		HAL_GPIO_WritePin(LED_DRONE_GPIO_Port, LED_DRONE_Pin, GPIO_PIN_SET);
		HAL_GPIO_WritePin(LED_HOME_GPIO_Port, LED_HOME_Pin, GPIO_PIN_RESET);
	}
	else if (mode == MODE_RETURN_HOME)
	{
		HAL_GPIO_WritePin(LED_CAR_GPIO_Port, LED_CAR_Pin, GPIO_PIN_RESET);
		HAL_GPIO_WritePin(LED_DRONE_GPIO_Port, LED_DRONE_Pin, GPIO_PIN_SET);
		HAL_GPIO_WritePin(LED_HOME_GPIO_Port, LED_HOME_Pin, GPIO_PIN_SET);
	}
}

void OnTimer()
{
	StartTimer = false;
	State = RX_TIMEOUT;
	HAL_TIM_Base_Stop_IT(&htim14);

	//进入这里应该是射频挂了，没有触发外部中断
	reset_radio = true;
}

void OnTxDone( void )
{
    //Radio.Sleep( );
    State = TX;
    StartTimer = false;
    HAL_TIM_Base_Stop_IT(&htim14);
}

void OnRxDone( uint8_t *payload, uint16_t size, int16_t rssi, int8_t snr )
{
	rx_timeout_count = 0;
	//printf("OnRxDone\r\n");
    //Radio.Sleep( );
    //BufferSize = size;
    //memcpy( Buffer, payload, BufferSize );
    RssiValue = rssi;
    SnrValue = snr;
    State = RX;
    StartTimer = false;
    HAL_TIM_Base_Stop_IT(&htim14);
    //处理crsf响应
    //CrsfSerial_loop(payload, size);
    crsf2_handleSerialIn(payload, size);
}

void OnTxTimeout( void )
{
    //Radio.Sleep( );
    State = TX_TIMEOUT;

}

void OnRxTimeout( void )
{
	//printf("OnRxTimeout\r\n");
    //Radio.Sleep( );
    State = RX_TIMEOUT;
    is_rx_timeout = true;
    rx_timeout_count++;
    if (rx_timeout_count > 10)
    {
    	//reset_radio = true;
    	rx_timeout_count = 0;
    }
}

void OnRxError( void )
{
    //Radio.Sleep( );
    State = RX_ERROR;
    //LogE("radio rx error");
}

