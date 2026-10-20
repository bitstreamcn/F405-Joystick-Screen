/*
 * Board.h
 *
 *  Created on: Jun 14, 2025
 *      Author: bitstream
 */

#ifndef BOARD_BOARD_H_
#define BOARD_BOARD_H_

void board_init();
HAL_StatusTypeDef Radio_SPI_ReadWrite(uint8_t data, uint8_t *rx_data);
HAL_StatusTypeDef Radio_Read_Reg(uint8_t reg, uint8_t * data, uint16_t len);
HAL_StatusTypeDef Radio_Write_Reg(uint8_t reg, const uint8_t * data, uint16_t len);


#endif /* BOARD_BOARD_H_ */
