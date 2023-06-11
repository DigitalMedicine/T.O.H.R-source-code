/*
 * temp_lib.h
 *
 *  Created on: 8 giu 2023
 *      Author: valeriafraenza
 */
#include <stdint.h>

#ifndef INC_TEMP_LIB_H_
#define INC_TEMP_LIB_H_

#ifndef V_REF
#define V_REF					3300 //mV
#endif

#ifndef LEVELS
#define LEVELS					4096 //2^12
#endif

#ifndef V25
#define	V25						760 //mV
#endif

#ifndef SLOPE
#define	SLOPE					2.5	// mV/°C
#endif

#ifndef SERIESRESISTOR
#define SERIESRESISTOR 			10000 //Ohm
#endif

#ifndef NOMINAL_RESISTANCE
#define NOMINAL_RESISTANCE 		10000 //Ohm
#endif

#ifndef NOMINAL_TEMPERATURE
#define NOMINAL_TEMPERATURE 	25 //°C
#endif

#ifndef BCOEFFICIENT
#define BCOEFFICIENT 			3950
#endif

#ifndef TEMP_TRESHOLD_HIGH
#define TEMP_TRESHOLD_HIGH 		31 //°C
#endif

#ifndef TEMP_TRESHOLD_LOW
#define TEMP_TRESHOLD_LOW 		21 //°C
#endif

void temp_init();
float steinharthart(uint16_t raw_value);
void get_temp();
void temp_setup();
void temp_loop();
int temp_analysis();
void print_temp();
void print_temp_status(int);

#endif /* INC_TEMP_LIB_H_ */
