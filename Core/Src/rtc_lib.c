/*
 * rtc_lib.c
 *
 *  Created on: 8 giu 2023
 *      Author: valeriafraenza
 */

#include "rtc_lib.h"
#include "stdio.h"
#include "string.h"
#include "ds1307rtc.h"
#include "ssd1306.h"
#include "ssd1306_fonts.h"
#include "usart.h"

/******************************* UTILITY VARS *******************************/
date_time_t date_time;
char date_string[50], time_string[50];
/******************************* UTILITY VARS *******************************/


/******************************* SETUP FUNCTIONS *******************************/
/*
 * This function is an utility to synchronize the clock date with the actual date.
 * It is just called once thanks to the clock memory.
 */
void set_clock(){
	date_time.date = 9;
	date_time.month = 6;
	date_time.year = 23;
	date_time.hours = 15;
	date_time.minutes = 15;
	date_time.seconds = 0;
	ds1307rtc_set_date_time(&date_time);
}
/******************************* SETUP FUNCTIONS *******************************/


/******************************* UTILITY FUNCTIONS *******************************/
/*
 * This function calls the library function to validate the date_time structure fields.
 */
void get_clock(){
	ds1307rtc_get_date_time(&date_time);
}
/******************************* UTILITY FUNCTIONS *******************************/


/******************************* PRINT FUNCTIONS *******************************/
/*
 * This function is printing print date and time values.
 */
void print_time(){
	sprintf(date_string, "DATE: %d-%d-%d\r\n", date_time.date, date_time.month, date_time.year);
	sprintf(time_string, "TIME: %d:%d:%d\r\n", date_time.hours, date_time.minutes, date_time.seconds);
	ssd1306_print(5, 0, date_string);
	ssd1306_print(5, 10, time_string);
	HAL_UART_Transmit(&huart2, (uint8_t*)date_string, strlen(date_string), HAL_MAX_DELAY);
	HAL_UART_Transmit(&huart2, (uint8_t*)time_string, strlen(time_string), HAL_MAX_DELAY);
}
/******************************* PRINT FUNCTIONS *******************************/
