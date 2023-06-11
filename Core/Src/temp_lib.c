/*
 * temp_lib.c
 *
 *  Created on: 8 giu 2023
 *      Author: valeriafraenza
 */
#include "temp_lib.h"
#include "stdio.h"
#include "string.h"
#include "math.h"
#include "ssd1306.h"
#include "ssd1306_fonts.h"
#include "adc.h"
#include "usart.h"

/******************************* UTILITY VARS *******************************/
uint16_t data_out;
float temperature, temp_buf[200], mean_temp, absolute_uncertainty_temp, relative_uncertainty_temp, v_in, steinhart, r_ref;
char temp_string[30], m_temp[30], au_temp[30], uart_string_temp[60];
char empty_row_string[3] = "\r\n";
int temp_index, sum_temp, min_temp, max_temp;
/******************************* UTILITY VARS *******************************/


/******************************* SETUP FUNCTIONS *******************************/
/*
 * This function starts the ADC conversion in DMA mode.
 */
void temp_init(){
	  HAL_ADC_Start_DMA(&hadc1, (uint32_t*) &data_out, 1);
}
/*
 * This function does a virtual setup by clearing flags statuses.
*/
void temp_setup(){
	temp_index = 0;
	ssd1306_Clear();
	ssd1306_print(5, 5, "Put the finger on ");
	ssd1306_print(5, 15, "the temperature ");
	ssd1306_print(5, 25, "sensor");
	HAL_Delay(3000);

	ssd1306_Clear();
	ssd1306_print(5,5, "Wait for 15 ");
	ssd1306_print(5, 15,"seconds");
	strcpy(uart_string_temp, "Finger temperature measure running...\r\n");
	HAL_UART_Transmit(&huart2, (uint8_t*)uart_string_temp, strlen(uart_string_temp), HAL_MAX_DELAY);
}
/******************************* SETUP FUNCTIONS *******************************/


/******************************* UTILITY FUNCTIONS *******************************/
/*
 * This function calls the steinharthart to validate the temperature variable.
 */
void get_temp(){
	temperature = steinharthart(data_out);
}

/*
 * This function converts the deltaV (v_in) read on the sensor to temperature thanks to the Steinhart-Hart equation
 * assisted by the resistance value calculation.
 * Since we use Celsius degrees there is also a small conversion in the end.
 */
float steinharthart(uint16_t raw_value){
	v_in=(float)raw_value*V_REF/LEVELS;
	r_ref = -(SERIESRESISTOR * v_in)/(v_in - V_REF);

	steinhart = r_ref / NOMINAL_RESISTANCE;
	steinhart = log(steinhart);
	steinhart /= BCOEFFICIENT;
	steinhart += 1.0 / (NOMINAL_TEMPERATURE + 273.15);
	steinhart = 1.0 / steinhart;
	steinhart -= 273.15;

	return steinhart;
}

/*
 * This functions runs trough the buffer to calculate:
 * average, min, max, absolute uncertainty and relative uncertainty in percentage.
 * The return value is 2 when the measure fails, 1 when it results in anomalies and 0 when it is ok.
 */
int temp_analysis(){
	sum_temp = 0;
	min_temp = temp_buf[0];
	max_temp = temp_buf[0];
	for(int j=0; j<temp_index;j++){
		sum_temp += temp_buf[j];
		if(temp_buf[j] < min_temp)
		  min_temp = temp_buf[j];
		if(temp_buf[j] > max_temp)
		  max_temp = temp_buf[j];
	}
	mean_temp = sum_temp/(temp_index);
	absolute_uncertainty_temp = (max_temp - min_temp)/2;
	relative_uncertainty_temp = (absolute_uncertainty_temp/mean_temp)*100;
	if(mean_temp <= TEMP_TRESHOLD_LOW){
		return 2;
	}
	if(mean_temp > TEMP_TRESHOLD_HIGH){
		return 1;
	}
	else{
		return 0;
	}
}
/******************************* UTILITY FUNCTIONS *******************************/


/******************************* DATA GATHERING FUNCTIONS *******************************/
/*
 * This function loops for about 15 seconds gathering measurements and placing them into a buffer.
 */
void temp_loop(){
	get_temp();
	HAL_Delay(200);
	temp_buf[temp_index] = temperature;
	temp_index++;
}
/******************************* DATA GATHERING FUNCTIONS *******************************/


/******************************* PRINT FUNCTIONS *******************************/
/*
 * These functions are printing utilities that, based on the state and the routine they're called in, print:
 * values, alerts and indication for the patient.
 * Some of them also handle leds that represent the state.
 */
void print_temp_status(int temp_status){
	sprintf(m_temp, "TEMP: %.2f grad\r\n", mean_temp);
	sprintf(au_temp, "Error: %.2f perc\r\n\r\n", relative_uncertainty_temp);

	if(temp_status == 1){
		HAL_GPIO_WritePin(red_led_GPIO_Port, red_led_Pin, 1);
		HAL_GPIO_WritePin(green_led_GPIO_Port, green_led_Pin, 0);

		ssd1306_Clear();
		ssd1306_print(5, 5, "Finger temp");
		ssd1306_print(5, 15, " too high!!! ");
		ssd1306_print(5, 25, m_temp);
		ssd1306_print(5, 55, "See the doctor.");
		HAL_Delay(3000);
		strcpy(uart_string_temp, "Finger temperature too high!!!\r\n");
		HAL_UART_Transmit(&huart2, (uint8_t*)uart_string_temp, strlen(uart_string_temp), HAL_MAX_DELAY);
		HAL_UART_Transmit(&huart2, (uint8_t*)m_temp, strlen(m_temp), HAL_MAX_DELAY);
		HAL_UART_Transmit(&huart2, (uint8_t*)au_temp, strlen(au_temp), HAL_MAX_DELAY);

		HAL_GPIO_WritePin(red_led_GPIO_Port, red_led_Pin, 0);
	}
	else if(temp_status == 2){
		ssd1306_Clear();
		ssd1306_print(5, 5, "Finger temp ");
		ssd1306_print(5, 15, "measure failed ");
		ssd1306_print(5, 35, "New measure ");
		ssd1306_print(5, 45, "running... ");
		HAL_Delay(3000);
		strcpy(uart_string_temp, "Finger temperature measure failed\r\nNew measure running\r\n");
		HAL_UART_Transmit(&huart2, (uint8_t*)uart_string_temp, strlen(uart_string_temp), HAL_MAX_DELAY);
	}
	else{
		HAL_GPIO_WritePin(red_led_GPIO_Port, red_led_Pin, 0);
		HAL_GPIO_WritePin(green_led_GPIO_Port, green_led_Pin, 1);

		ssd1306_Clear();
		ssd1306_print(5, 5, "Good finger");
		ssd1306_print(5, 15, " temperature!");
		ssd1306_print(5, 35, m_temp);
		HAL_Delay(3000);
		strcpy(uart_string_temp, "Good finger temperature\r\n");
		HAL_UART_Transmit(&huart2, (uint8_t*)uart_string_temp, strlen(uart_string_temp), HAL_MAX_DELAY);
		HAL_UART_Transmit(&huart2, (uint8_t*)m_temp, strlen(m_temp), HAL_MAX_DELAY);
		HAL_UART_Transmit(&huart2, (uint8_t*)au_temp, strlen(au_temp), HAL_MAX_DELAY);

		HAL_GPIO_WritePin(green_led_GPIO_Port, green_led_Pin, 0);
	}
}

void print_temp(){
	sprintf(temp_string, "TEMP: %.2f grad\r\n", temperature);
	ssd1306_print(5, 54, temp_string);
	HAL_UART_Transmit(&huart2, (uint8_t*)temp_string, strlen(temp_string), HAL_MAX_DELAY);
	HAL_UART_Transmit(&huart2, (uint8_t*)empty_row_string, strlen(empty_row_string), HAL_MAX_DELAY);
}
/******************************* PRINT FUNCTIONS *******************************/
