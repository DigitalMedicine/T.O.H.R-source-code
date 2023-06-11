/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.c
  * @brief          : Main program body
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2023 STMicroelectronics.
  * All rights reserved.
  *
  * This software is licensed under terms that can be found in the LICENSE file
  * in the root directory of this software component.
  * If no LICENSE file comes with this software, it is provided AS-IS.
  *
  ******************************************************************************
  */
/* USER CODE END Header */
/* Includes ------------------------------------------------------------------*/
#include "main.h"
#include "adc.h"
#include "dma.h"
#include "i2c.h"
#include "tim.h"
#include "usart.h"
#include "gpio.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include "stdio.h"
#include "string.h"
#include "math.h"
#include "ds1307rtc.h"
#include "ssd1306.h"
#include "ssd1306_fonts.h"
#include "max32664.h"
#include "max_lib.h"
#include "temp_lib.h"
#include "rtc_lib.h"

/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */

/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */
/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */


/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/

/* USER CODE BEGIN PV */

/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
/* USER CODE BEGIN PFP */

/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */


/******************************* FLAGS *******************************/
int steinharthart_flag = 0;
int max_flag = 0;
int temp_flag = 0;
int monitoring_flag = 0;
int time_elapsed_flag = 0;
int breathe_flag = 0;
uint8_t pwm_status_flag = 1;
/******************************* FLAGS *******************************/


/******************************* UTILITY VARS *******************************/
int count = 0;
int temp_status = 0;
int hr_status = 0;
int ox_status = 0;
uint16_t pulse = 0;
/******************************* UTILITY VARS *******************************/



/* USER CODE END 0 */

/**
  * @brief  The application entry point.
  * @retval int
  */
int main(void)
{
  /* USER CODE BEGIN 1 */

  /* USER CODE END 1 */

  /* MCU Configuration--------------------------------------------------------*/

  /* Reset of all peripherals, Initializes the Flash interface and the Systick. */
  HAL_Init();

  /* USER CODE BEGIN Init */


  /* USER CODE END Init */

  /* Configure the system clock */
  SystemClock_Config();

  /* USER CODE BEGIN SysInit */

  /* USER CODE END SysInit */

  /* Initialize all configured peripherals */
  MX_GPIO_Init();
  MX_DMA_Init();
  MX_USART2_UART_Init();
  MX_ADC1_Init();
  MX_TIM2_Init();
  MX_I2C1_Init();
  MX_TIM10_Init();
  MX_TIM3_Init();
  MX_TIM11_Init();
  /* USER CODE BEGIN 2 */


  /******************************* INIT *******************************/
  ssd1306_Init();
  ds1307rtc_init();
  max_init();
  HAL_TIM_Base_Start(&htim2);
  temp_init();
  //set_clock(); executed one time to configure the clock
  /******************************* INIT *******************************/


  /* USER CODE END 2 */
  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
  while (1)
  {
	  if(steinharthart_flag == 1){
		  ssd1306_Clear();
		  get_max();
		  get_clock();
		  get_temp();
		  print_time();
		  print_max();
		  print_temp();
		  steinharthart_flag = 0;
	  }

	  if(max_flag == 1){
		  HAL_TIM_Base_Stop(&htim2);
		if(monitoring_flag == 0 && time_elapsed_flag == 0){
			max_setup();
			HAL_TIM_Base_Start_IT(&htim10);
			monitoring_flag = 1;
		}
		else if (monitoring_flag == 1 && time_elapsed_flag == 0){
			max_loop();
		}
		else if (time_elapsed_flag == 1){
			HAL_TIM_Base_Stop_IT(&htim10);
			HAL_TIM_Base_Start(&htim2);
			hr_status = hr_analysis();
			print_hr_status(hr_status);
			ox_status = ox_analysis();
			print_ox_status(ox_status);

			if(hr_status == 1 || ox_status == 1){
				print_breathe();
				HAL_TIM_Base_Stop(&htim2);
				time_elapsed_flag = 0;
				breathe_flag = 1;
				HAL_TIM_Base_Start_IT(&htim10);
				HAL_TIM_Base_Start_IT(&htim11);
				HAL_TIM_PWM_Start(&htim3, TIM_CHANNEL_1);
				max_flag = 0;
			}
			if(hr_status == 2 || ox_status == 2){
				max_flag = 1;
				monitoring_flag = 0;
				time_elapsed_flag = 0;
			}
			else{
				max_flag = 0;
			}
		}
	  }

	  if(temp_flag == 1) {

		  if(monitoring_flag == 0 && time_elapsed_flag == 0){
			  temp_setup();
			  HAL_TIM_Base_Start_IT(&htim10);
			  monitoring_flag = 1;
		  }
		  else if (monitoring_flag == 1 && time_elapsed_flag == 0){
			  temp_loop();
		  }
		  else if (time_elapsed_flag == 1){
			  HAL_TIM_Base_Stop_IT(&htim10);
			  temp_status = temp_analysis();
			  print_temp_status(temp_status);
			  if(temp_status == 2){
					monitoring_flag = 0;
					time_elapsed_flag = 0;
					temp_flag = 1;
			  }
			  else{
					temp_flag = 0;
			  }
		  }
	  }
	  if(breathe_flag == 1){
		  if(time_elapsed_flag == 1){
				HAL_TIM_Base_Stop_IT(&htim10);
				HAL_TIM_Base_Stop_IT(&htim11);
				HAL_TIM_PWM_Stop(&htim3, TIM_CHANNEL_1);
				max_flag = 1;
				monitoring_flag = 0;
				time_elapsed_flag = 0;
				breathe_flag = 0;
				HAL_TIM_Base_Start(&htim2);
		  }
	  }
  }
    /* USER CODE END WHILE */

    /* USER CODE BEGIN 3 */

  /* USER CODE END 3 */
}

/**
  * @brief System Clock Configuration
  * @retval None
  */
void SystemClock_Config(void)
{
  RCC_OscInitTypeDef RCC_OscInitStruct = {0};
  RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};

  /** Configure the main internal regulator output voltage
  */
  __HAL_RCC_PWR_CLK_ENABLE();
  __HAL_PWR_VOLTAGESCALING_CONFIG(PWR_REGULATOR_VOLTAGE_SCALE2);

  /** Initializes the RCC Oscillators according to the specified parameters
  * in the RCC_OscInitTypeDef structure.
  */
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSI;
  RCC_OscInitStruct.HSIState = RCC_HSI_ON;
  RCC_OscInitStruct.HSICalibrationValue = RCC_HSICALIBRATION_DEFAULT;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
  RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSI;
  RCC_OscInitStruct.PLL.PLLM = 8;
  RCC_OscInitStruct.PLL.PLLN = 64;
  RCC_OscInitStruct.PLL.PLLP = RCC_PLLP_DIV2;
  RCC_OscInitStruct.PLL.PLLQ = 7;
  if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
  {
    Error_Handler();
  }

  /** Initializes the CPU, AHB and APB buses clocks
  */
  RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK|RCC_CLOCKTYPE_SYSCLK
                              |RCC_CLOCKTYPE_PCLK1|RCC_CLOCKTYPE_PCLK2;
  RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
  RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV4;
  RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV1;
  RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV1;

  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_0) != HAL_OK)
  {
    Error_Handler();
  }
}

/* USER CODE BEGIN 4 */

/******************************* CONVERSION DONE ISR *******************************/
void HAL_ADC_ConvCpltCallback(ADC_HandleTypeDef* hadc){
		if(!max_flag && !temp_flag)
			steinharthart_flag = 1;
}
/******************************* CONVERSION DONE ISR *******************************/


/******************************* BUTTON ISR *******************************/
void HAL_GPIO_EXTI_Callback(uint16_t GPIO_Pin){
    if(GPIO_Pin == max_button_Pin){
		steinharthart_flag = 0;
		temp_flag = 0;
		max_flag = 1;
		time_elapsed_flag = 0;
		monitoring_flag = 0;
	  }
    if(GPIO_Pin == temp_button_Pin){
		steinharthart_flag = 0;
		temp_flag = 1;
		max_flag = 0;
		time_elapsed_flag = 0;
		monitoring_flag = 0;
    }
}
/******************************* BUTTON ISR *******************************/


/******************************* TIMER ELAPSED ISR *******************************/
void HAL_TIM_PeriodElapsedCallback(TIM_HandleTypeDef* htim){
	if(htim == &htim10){
		if(count == 1){
			time_elapsed_flag = 1;
			count = 0;
		}
		else{
			count = 1;
		}
	}
	if(htim == &htim11){
		if (pwm_status_flag == 1){
			pulse = (pulse + 4);
		}else{
			pulse = (pulse - 2);
		}
		__HAL_TIM_SET_COMPARE(&htim3, TIM_CHANNEL_1, pulse);
		if ( (pulse <= 0) || (pulse >= 1000))
			pwm_status_flag = !pwm_status_flag;
	}
}
/******************************* TIMER ELAPSED ISR *******************************/



/* USER CODE END 4 */

/**
  * @brief  This function is executed in case of error occurrence.
  * @retval None
  */
void Error_Handler(void)
{
  /* USER CODE BEGIN Error_Handler_Debug */
  /* User can add his own implementation to report the HAL error return state */
  __disable_irq();
  while (1)
  {
  }
  /* USER CODE END Error_Handler_Debug */
}

#ifdef  USE_FULL_ASSERT
/**
  * @brief  Reports the name of the source file and the source line number
  *         where the assert_param error has occurred.
  * @param  file: pointer to the source file name
  * @param  line: assert_param error line source number
  * @retval None
  */
void assert_failed(uint8_t *file, uint32_t line)
{
  /* USER CODE BEGIN 6 */
  /* User can add his own implementation to report the file name and line number,
     ex: printf("Wrong parameters value: file %s on line %d\r\n", file, line) */
  /* USER CODE END 6 */
}
#endif /* USE_FULL_ASSERT */
