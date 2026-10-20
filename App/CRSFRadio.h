/*
 * CRSFRadio.h
 *
 *  Created on: Jun 13, 2025
 *      Author: bitstream
 */

#ifndef CRSFRADIO_H_
#define CRSFRADIO_H_


typedef struct{
	void ( *Init )();
	void ( *Loop )();
}CRSFRadio_s;

extern CRSFRadio_s CRSFRadio;




#endif /* CRSFRADIO_H_ */
