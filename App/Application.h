/*
 * Application.h
 *
 *  Created on: Jun 13, 2025
 *      Author: bitstream
 */
#ifndef APPLICATION_H_
#define APPLICATION_H_

#include <stdarg.h>

typedef struct{
	void ( *Init )();
	void ( *Loop )();
}Application_s;


void LogI(const char *format, ...);
void LogW(const char *format, ...);
void LogE(const char *format, ...);

extern Application_s Application;

void ui(void);


#endif /* APPLICATION_H_ */
