/*
 * max_lib.h
 *
 *  Created on: 8 giu 2023
 *      Author: valeriafraenza
 */

#ifndef INC_MAX_LIB_H_
#define INC_MAX_LIB_H_

#ifndef HR_TRESHOLD_HIGH
#define HR_TRESHOLD_HIGH 	100.00 //bpm
#endif

#ifndef HR_TRESHOLD_LOW
#define HR_TRESHOLD_LOW		60.00 //bpm
#endif

#ifndef CONFIDENCE
#define CONFIDENCE			60
#endif

#ifndef OX_TRESHOLD
#define OX_TRESHOLD			95.00 //%
#endif

#ifndef MAX_HR
#define MAX_HR 				200 //bpm
#endif

#ifndef MAX_OX
#define MAX_OX 				100 //%
#endif

#ifndef ZERO_THRESHOLD
#define ZERO_THRESHOLD 		30.00 //%
#endif

// Procedure definitions
void max_init();
void get_max();
void max_setup();
void max_loop();
int hr_analysis();
int ox_analysis();
void print_max();
void print_hr_status(int);
void print_ox_status(int);
void print_breathe();


#endif /* INC_MAX_LIB_H_ */
