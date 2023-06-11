/*
 * max_lib.c
 *
 *  Created on: 8 giu 2023
 *      Author: valeriafraenza
 */
#include "max_lib.h"
#include "stdio.h"
#include "string.h"
#include "math.h"
#include "max32664.h"
#include "ssd1306.h"
#include "ssd1306_fonts.h"

/******************************* UTILITY VARS *******************************/
uint8_t res;
MAX32664 hr_sensor;
int hr_index, zero_counter_hr, sum_hr, min_hr, max_hr, ox_index, zero_counter_ox, sum_ox, min_ox, max_ox;
int hr_buf[500], ox_buf[500], conf_buf[200]; //200 because 150 is the max measurements each 100ms for 15 seconds
float zero_perc_hr, zero_perc_ox, mean, absolute_uncertainty, relative_uncertainty;
char hr_string[50], ox_string[50], conf_string[30], m[50], au[50], uart_string_max[60];
/******************************* UTILITY VARS *******************************/


/******************************* SETUP FUNCTIONS *******************************/
/*
 * This function calls the library functions to begin the sensor I2c communication and setting it into application mode,
 * moreover it also does the configuration steps to select the elaboration algorithm and the mode
 */
void max_init(){
	begin(&hr_sensor, &hi2c1, reset_gpio_GPIO_Port, mfio_gpio_GPIO_Port, reset_gpio_Pin, mfio_gpio_Pin);
	config_sensor(&hr_sensor, ENABLE);
}

/*
 * This function does a virtual setup by clearing flags statuses.
 * It stops the ADC conversion for temperature that would be unused and starts the 15 seconds timer for data gathering
 */
void max_setup(){
	hr_index = 0;
	ox_index = 0;
	zero_counter_hr = 0;
	zero_counter_ox = 0;
	ssd1306_Clear();
	ssd1306_print(5, 5, "Put the finger on ");
	ssd1306_print(5, 15, "the pulse_ox ");
	ssd1306_print(5, 25, "sensor");
	HAL_Delay(3000);

	ssd1306_Clear();
	ssd1306_print(5,5, "Wait for 15 ");
	ssd1306_print(5, 15,"seconds");
	strcpy(uart_string_max, "Heart rate and oxygen percentage measures running...\r\n");
	HAL_UART_Transmit(&huart2, (uint8_t*)uart_string_max, strlen(uart_string_max), HAL_MAX_DELAY);
}
/******************************* SETUP FUNCTIONS *******************************/


/******************************* UTILITY FUNCTIONS *******************************/
/*
 * This function calls the read_sensor to validate the hr_sensor structure fields.
 * The delay is necessary and set to 200 because the max_sensor has inside an analysis time of at least 40ms;
 */
void get_max(){
	res = read_sensor(&hr_sensor);
	HAL_Delay(200);
}

/*
 * This functions checks the zero and null values percentages, if it is too high it returns an error,
 * otherwise it runs trough the buffer without considering the null values to calculate:
 * average, min, max, absolute uncertainty and relative uncertainty in percentage.
 * The return value is 2 when the measure fails, 1 when it results in anomalies and 0 when it is ok
 */
int hr_analysis(){
	sum_hr = 0;
	zero_perc_hr = ((float) zero_counter_hr / (hr_index + zero_counter_hr)) * 100;
	if (zero_perc_hr > ZERO_THRESHOLD){
	  return 2;
	}
	else{
	  min_hr = hr_buf[0];
	  max_hr = hr_buf[0];
	  for(int j=0; j<hr_index;j++){
		  sum_hr += hr_buf[j];
		  if(hr_buf[j] < min_hr)
			  min_hr = hr_buf[j];
		  if(hr_buf[j] > max_hr)
			  max_hr = hr_buf[j];
		}
		mean = sum_hr/(hr_index + zero_counter_hr);
		absolute_uncertainty = (max_hr - min_hr)/2;
		relative_uncertainty = (absolute_uncertainty/mean)*100;
		if (mean < HR_TRESHOLD_LOW){
			return 1;
		}
		else if (mean > HR_TRESHOLD_HIGH){
			return 1;
		}
		else{
			return 0;
		}
	}
}

/*
 * This functions checks the zero and null values percentages, if it is too high it returns an error,
 * otherwise it runs trough the buffer without considering the null values to calculate:
 * average, min, max, absolute uncertainty and relative uncertainty in percentage.
 * The return value is 2 when the measure fails, 1 when it results in anomalies and 0 when it is ok
 */
int ox_analysis(){
	sum_ox = 0;
	zero_perc_ox = ((float) zero_counter_ox / (ox_index + zero_counter_ox)) * 100;
	if (zero_perc_ox > ZERO_THRESHOLD){
	  return 2;
	}
	else{
	  min_ox = ox_buf[0];
	  max_ox = ox_buf[0];
	  int j;
	  for(j=0; j<ox_index;j++){
		  sum_ox += ox_buf[j];
		  if(ox_buf[j] < min_ox)
			  min_ox = ox_buf[j];
		  if(ox_buf[j] > max_ox)
			  max_ox = ox_buf[j];
	  }
	  mean = sum_ox/(ox_index + zero_counter_ox);
	  absolute_uncertainty = (max_ox - min_ox)/2;
	  relative_uncertainty = (absolute_uncertainty/mean)*100;
	  if (mean < OX_TRESHOLD){
		  return 1;
	  }
	  else{
		  return 0;
	  }
	}
}
/******************************* UTILITY FUNCTIONS *******************************/


/******************************* DATA GATHERING FUNCTIONS *******************************/
/*
 * This function loops for about 15 seconds gathering measurements and placing them into a buffer.
 * It also counts the number of zero or null values for both oxygen blood saturation and heart rate.
 * MAX_HR macro is necessary because touching the breakout board results in abnormous values.
 * CONFIDENCE macro is necessary to discard uncertain values from the sensor point of view.
 */
void max_loop(){
	read_sensor(&hr_sensor);
	HAL_Delay(100);
	if (hr_sensor.heart_rate == 0 || hr_sensor.heart_rate >= MAX_HR || hr_sensor.confidence < CONFIDENCE){
	  zero_counter_hr++;
	}
	else{
	  hr_buf[hr_index] = hr_sensor.heart_rate;
	  hr_index++;
	}

	if (hr_sensor.oxygen == 0 || hr_sensor.oxygen > MAX_OX) {
	  zero_counter_ox++;
	}
	else{
		ox_buf[ox_index] = hr_sensor.oxygen;
		ox_index++;
	}
}
/******************************* DATA GATHERING FUNCTIONS *******************************/


/******************************* PRINT FUNCTIONS *******************************/
/*
 * These functions are printing utilities that, based on the state and the routine they're called in, print:
 * values, alerts and indication for the patient.
 * Some of them also handle leds that represent the state.
 */

void print_max(){
	sprintf(hr_string, "HR: %u bpm, ", hr_sensor.heart_rate);
	ssd1306_print(5, 27, hr_string);
	sprintf(conf_string, "Confidence: %u\r\n", hr_sensor.confidence);
	strcat(hr_string, conf_string);
	HAL_UART_Transmit(&huart2, (uint8_t*)hr_string, strlen(hr_string), HAL_MAX_DELAY);

	sprintf(ox_string, "OX: %u perc\r\n", hr_sensor.oxygen);
	ssd1306_print(5, 37, ox_string);
	HAL_UART_Transmit(&huart2, (uint8_t*)ox_string, strlen(ox_string), HAL_MAX_DELAY);
}

void print_hr_status(int hr_status){
	sprintf(m, "HR: %.2f bpm \r\n", mean);
	sprintf(au, "Error: %.2f perc \r\n\r\n", relative_uncertainty);

	if(hr_status == 1){
		HAL_GPIO_WritePin(red_led_GPIO_Port, red_led_Pin, 1);
		HAL_GPIO_WritePin(green_led_GPIO_Port, green_led_Pin, 0);

		ssd1306_Clear();
		ssd1306_print(5, 5, "Heart rate out ");
		ssd1306_print(5, 15, "of range!!!");
		ssd1306_print(5, 35, m);
		HAL_Delay(3000);
		strcpy(uart_string_max, "Heart rate out of range!!!\r\n");
		HAL_UART_Transmit(&huart2, (uint8_t*)uart_string_max, strlen(uart_string_max), HAL_MAX_DELAY);
		HAL_UART_Transmit(&huart2, (uint8_t*)m, strlen(m), HAL_MAX_DELAY);
		HAL_UART_Transmit(&huart2, (uint8_t*)au, strlen(au), HAL_MAX_DELAY);

		HAL_GPIO_WritePin(red_led_GPIO_Port, red_led_Pin, 0);
	}
	else if(hr_status == 2){
		ssd1306_Clear();
		ssd1306_print(5, 15, "Hr measure failed");
		ssd1306_print(5, 25, "New measure ");
		ssd1306_print(5, 35, "running... ");
		HAL_Delay(3000);
		strcpy(uart_string_max, "Heart rate measure failed\r\nNew measure running...\r\n");
		HAL_UART_Transmit(&huart2, (uint8_t*)uart_string_max, strlen(uart_string_max), HAL_MAX_DELAY);
	}
	else{
		HAL_GPIO_WritePin(red_led_GPIO_Port, red_led_Pin, 0);
		HAL_GPIO_WritePin(green_led_GPIO_Port, green_led_Pin, 1);

		ssd1306_Clear();
		ssd1306_print(5, 5, "Good heart rate");
		ssd1306_print(5, 25, m);
		HAL_Delay(3000);
		strcpy(uart_string_max, "Good heart rate\r\n");
		HAL_UART_Transmit(&huart2, (uint8_t*)uart_string_max, strlen(uart_string_max), HAL_MAX_DELAY);
		HAL_UART_Transmit(&huart2, (uint8_t*)m, strlen(m), HAL_MAX_DELAY);
		HAL_UART_Transmit(&huart2, (uint8_t*)au, strlen(au), HAL_MAX_DELAY);

		HAL_GPIO_WritePin(green_led_GPIO_Port, green_led_Pin, 0);
	}
}

void print_ox_status(int ox_status){
	sprintf(m, "OX: %.2f perc \r\n", mean);
	sprintf(au, "Error: %.2f perc \r\n\r\n", relative_uncertainty);
	if(ox_status == 1){
		HAL_GPIO_WritePin(red_led_GPIO_Port, red_led_Pin, 1);
		HAL_GPIO_WritePin(green_led_GPIO_Port, green_led_Pin, 0);

		ssd1306_Clear();
		ssd1306_print(5, 5, "Oxygen % out ");
		ssd1306_print(5, 15, "of range!!!");
		ssd1306_print(5, 35, m);
		HAL_Delay(3000);
		strcpy(uart_string_max, "Oxygen percentage out of range!!!\r\n");
		HAL_UART_Transmit(&huart2, (uint8_t*)uart_string_max, strlen(uart_string_max), HAL_MAX_DELAY);
		HAL_UART_Transmit(&huart2, (uint8_t*)m, strlen(m), HAL_MAX_DELAY);
		HAL_UART_Transmit(&huart2, (uint8_t*)au, strlen(au), HAL_MAX_DELAY);

		HAL_GPIO_WritePin(red_led_GPIO_Port, red_led_Pin, 0);
	}
	else if(ox_status == 2){
		ssd1306_Clear();
		ssd1306_print(5, 15, "Ox measure failed");
		ssd1306_print(5, 25, "New measure ");
		ssd1306_print(5, 35, "running... ");
		HAL_Delay(3000);
		strcpy(uart_string_max, "Ox measure failed\r\nNew measure running...\r\n");
		HAL_UART_Transmit(&huart2, (uint8_t*)uart_string_max, strlen(uart_string_max), HAL_MAX_DELAY);
	}
	else{
		HAL_GPIO_WritePin(red_led_GPIO_Port, red_led_Pin, 0);
		HAL_GPIO_WritePin(green_led_GPIO_Port, green_led_Pin, 1);

		ssd1306_Clear();
		ssd1306_print(5, 5, "Oxygen % good");
		ssd1306_print(5, 25, m);
		HAL_Delay(3000);
		strcpy(uart_string_max, "Good oxygen percentage\r\n");
		HAL_UART_Transmit(&huart2, (uint8_t*)uart_string_max, strlen(uart_string_max), HAL_MAX_DELAY);
		HAL_UART_Transmit(&huart2, (uint8_t*)m, strlen(m), HAL_MAX_DELAY);
		HAL_UART_Transmit(&huart2, (uint8_t*)au, strlen(au), HAL_MAX_DELAY);

		HAL_GPIO_WritePin(green_led_GPIO_Port, green_led_Pin, 0);
	}
}

void print_breathe(){
	ssd1306_Clear();
	ssd1306_print(5, 25, "Breathe following");
	ssd1306_print(5, 35, "the light");
	strcpy(uart_string_max, "Patient in breathing phase\r\n");
	HAL_UART_Transmit(&huart2, (uint8_t*)uart_string_max, strlen(uart_string_max), HAL_MAX_DELAY);
}
/******************************* PRINT FUNCTIONS *******************************/
