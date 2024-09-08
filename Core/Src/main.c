/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.c
  * @brief          : Main program body
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2024 STMicroelectronics.
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

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */

/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */

/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */
#define STARTUP		0
#define OPERATION	1
#define	CHARGING	2
#define ERROR		3

/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/
ADC_HandleTypeDef hadc1;

/* USER CODE BEGIN PV */

/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
static void MX_GPIO_Init(void);
static void MX_ADC1_Init(void);
/* USER CODE BEGIN PFP */

/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */

void error_handler(void){
	HAL_GPIO_WritePin(MCU_OK_GPIO_Port, MCU_OK_Pin, GPIO_PIN_RESET);
	// should also close all relays
	// comms over canbus of what the error was?
	while(1){ //freeze everything off
		HAL_Delay(500);
	}
}

void discharge_handler(void){
	// turn off both contactors
	HAL_GPIO_WritePin(HVCP_EN_GPIO_Port, HVCP_EN_Pin, GPIO_PIN_RESET);
	HAL_GPIO_WritePin(HVCN_EN_GPIO_Port, HVCN_EN_Pin, GPIO_PIN_RESET);

	/*
	 Insert code for checking that aux opened
	*/

	// actually discharge the board
	HAL_GPIO_WritePin(MCU_OK_GPIO_Port, MCU_OK_Pin, GPIO_PIN_RESET);
	int i = 0;
	while (i < 5){ // replace w/ vsense code (break? on undervoltage)
		i++;
		HAL_Delay(500);
	}
	HAL_GPIO_WritePin(MCU_OK_GPIO_Port, MCU_OK_Pin, GPIO_PIN_SET);

}

void toggle_precharge(void){
	// Maybe check that HVCP is open?

	HAL_GPIO_WritePin(HVCN_EN_GPIO_Port, HVCN_EN_Pin, GPIO_PIN_SET);
	HAL_GPIO_WritePin(PRECHRG_EN_GPIO_Port, PRECHRG_EN_Pin, GPIO_PIN_SET);
	/*
	 Insert code for checking that "precharge mode" auxs closed
	*/


	int i = 0;
	while (i < 5){ // replace w/ vsense code (break? on undervoltage)
		i++;
		HAL_Delay(500);
		if (i == 4){
			HAL_GPIO_WritePin(HVCP_EN_GPIO_Port, HVCP_EN_Pin, GPIO_PIN_SET);
			HAL_Delay(500);
			// check aux closed...
			HAL_GPIO_WritePin(PRECHRG_EN_GPIO_Port, PRECHRG_EN_Pin, GPIO_PIN_RESET);
			return;
		}
	}
	// will only reach here if vsense fails after tries above
	error_handler();
}

void toggle_charging(void){
	HAL_GPIO_WritePin(CHRGP_EN_GPIO_Port, CHRGP_EN_Pin, GPIO_PIN_SET);
	HAL_GPIO_WritePin(CHRGN_EN_GPIO_Port, CHRGN_EN_Pin, GPIO_PIN_SET);
}

void untoggle_charging(void){
	HAL_GPIO_WritePin(CHRGP_EN_GPIO_Port, CHRGP_EN_Pin, GPIO_PIN_RESET);
	HAL_GPIO_WritePin(CHRGN_EN_GPIO_Port, CHRGN_EN_Pin, GPIO_PIN_RESET);
}

uint8_t get_switch_status(uint8_t){
	if (HAL_GPIO_ReadPin(IGNITION_SW_GPIO_Port, IGNITION_SW_Pin) == GPIO_PIN_SET){
		if (HAL_GPIO_ReadPin(CHARGE_SW_GPIO_Port, CHARGE_SW_Pin) == GPIO_PIN_SET){
			return ERROR;
		} else {
			return OPERATION;
		}
	} else {
		if (HAL_GPIO_ReadPin(CHARGE_SW_GPIO_Port, CHARGE_SW_Pin) == GPIO_PIN_SET){

		} else {
			return STARTUP;
		}
	}
	return ERROR;
}

void aux_check(uint8_t){
	HAL_Delay(500);
	//placeholder
}



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
  MX_ADC1_Init();
  /* USER CODE BEGIN 2 */

  // Setup
  uint8_t status = STARTUP;
  uint8_t new_status;
  HAL_Delay(500);
  HAL_GPIO_WritePin(MCU_OK_GPIO_Port, MCU_OK_Pin, GPIO_PIN_SET);

  /* USER CODE END 2 */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
  while (1)
  {
	  new_status = get_switch_status(status);
	  if (new_status == ERROR){
		  error_handler();
	  }

	  if(new_status != status){
		  if (status == STARTUP){ // where can you go from startup:
			  if (new_status == OPERATION){
				  toggle_precharge();
				  status = OPERATION;
			  } else if (new_status == OPERATION){
				  toggle_charging();
				  status = CHARGING;
			  } else {
				  error_handler(); // should never reach here
			  }
		  }
		  if (status == OPERATION){ // where can you go from operation:
			  if (new_status == STARTUP){
				  discharge_handler();
				  status = STARTUP;
			  } else {
				  error_handler(); // should never reach here
			  }
		  }
		  if (status == CHARGING){ // where can you go from operation:
			  if (new_status == STARTUP){
				  untoggle_charging();
				  status = STARTUP;
			  } else {
				  error_handler(); // should never reach here
			  }
		  }
	  }

	  aux_check(status);
    /* USER CODE END WHILE */

    /* USER CODE BEGIN 3 */
  }
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

  /** Initializes the RCC Oscillators according to the specified parameters
  * in the RCC_OscInitTypeDef structure.
  */
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSI;
  RCC_OscInitStruct.HSIState = RCC_HSI_ON;
  RCC_OscInitStruct.HSICalibrationValue = RCC_HSICALIBRATION_DEFAULT;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_NONE;
  if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
  {
    Error_Handler();
  }

  /** Initializes the CPU, AHB and APB buses clocks
  */
  RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK|RCC_CLOCKTYPE_SYSCLK
                              |RCC_CLOCKTYPE_PCLK1|RCC_CLOCKTYPE_PCLK2;
  RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_HSI;
  RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
  RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV1;
  RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV1;

  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_0) != HAL_OK)
  {
    Error_Handler();
  }
}

/**
  * @brief ADC1 Initialization Function
  * @param None
  * @retval None
  */
static void MX_ADC1_Init(void)
{

  /* USER CODE BEGIN ADC1_Init 0 */

  /* USER CODE END ADC1_Init 0 */

  ADC_ChannelConfTypeDef sConfig = {0};

  /* USER CODE BEGIN ADC1_Init 1 */

  /* USER CODE END ADC1_Init 1 */

  /** Configure the global features of the ADC (Clock, Resolution, Data Alignment and number of conversion)
  */
  hadc1.Instance = ADC1;
  hadc1.Init.ClockPrescaler = ADC_CLOCK_SYNC_PCLK_DIV2;
  hadc1.Init.Resolution = ADC_RESOLUTION_12B;
  hadc1.Init.ScanConvMode = DISABLE;
  hadc1.Init.ContinuousConvMode = DISABLE;
  hadc1.Init.DiscontinuousConvMode = DISABLE;
  hadc1.Init.ExternalTrigConvEdge = ADC_EXTERNALTRIGCONVEDGE_NONE;
  hadc1.Init.ExternalTrigConv = ADC_SOFTWARE_START;
  hadc1.Init.DataAlign = ADC_DATAALIGN_RIGHT;
  hadc1.Init.NbrOfConversion = 1;
  hadc1.Init.DMAContinuousRequests = DISABLE;
  hadc1.Init.EOCSelection = ADC_EOC_SINGLE_CONV;
  if (HAL_ADC_Init(&hadc1) != HAL_OK)
  {
    Error_Handler();
  }

  /** Configure for the selected ADC regular channel its corresponding rank in the sequencer and its sample time.
  */
  sConfig.Channel = ADC_CHANNEL_0;
  sConfig.Rank = 1;
  sConfig.SamplingTime = ADC_SAMPLETIME_3CYCLES;
  if (HAL_ADC_ConfigChannel(&hadc1, &sConfig) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN ADC1_Init 2 */

  /* USER CODE END ADC1_Init 2 */

}

/**
  * @brief GPIO Initialization Function
  * @param None
  * @retval None
  */
static void MX_GPIO_Init(void)
{
  GPIO_InitTypeDef GPIO_InitStruct = {0};
/* USER CODE BEGIN MX_GPIO_Init_1 */
/* USER CODE END MX_GPIO_Init_1 */

  /* GPIO Ports Clock Enable */
  __HAL_RCC_GPIOC_CLK_ENABLE();
  __HAL_RCC_GPIOA_CLK_ENABLE();
  __HAL_RCC_GPIOB_CLK_ENABLE();

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(GPIOC, HVCN_EN_Pin|HVCP_EN_Pin|CHRGP_EN_Pin|CHRGN_EN_Pin
                          |PRECHRG_EN_Pin|PUMP_EN_Pin, GPIO_PIN_RESET);

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(GPIOB, DEBUG1_Pin|DEBUG2_Pin, GPIO_PIN_RESET);

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(MCU_OK_GPIO_Port, MCU_OK_Pin, GPIO_PIN_RESET);

  /*Configure GPIO pins : HVCN_EN_Pin HVCP_EN_Pin CHRGP_EN_Pin CHRGN_EN_Pin
                           PRECHRG_EN_Pin PUMP_EN_Pin */
  GPIO_InitStruct.Pin = HVCN_EN_Pin|HVCP_EN_Pin|CHRGP_EN_Pin|CHRGN_EN_Pin
                          |PRECHRG_EN_Pin|PUMP_EN_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(GPIOC, &GPIO_InitStruct);

  /*Configure GPIO pins : BMS_GPIO1_Pin BMS_GPIO2_Pin BMS_GPIO3_Pin */
  GPIO_InitStruct.Pin = BMS_GPIO1_Pin|BMS_GPIO2_Pin|BMS_GPIO3_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

  /*Configure GPIO pins : IGNITION_SW_Pin SAFETY_LOOP_STATUS_Pin */
  GPIO_InitStruct.Pin = IGNITION_SW_Pin|SAFETY_LOOP_STATUS_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  HAL_GPIO_Init(GPIOC, &GPIO_InitStruct);

  /*Configure GPIO pins : CHARGE_SW_Pin HVCP_AUX_Pin HVCN_AUX_Pin PRECHRG_AUX_Pin */
  GPIO_InitStruct.Pin = CHARGE_SW_Pin|HVCP_AUX_Pin|HVCN_AUX_Pin|PRECHRG_AUX_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);

  /*Configure GPIO pins : DEBUG1_Pin DEBUG2_Pin */
  GPIO_InitStruct.Pin = DEBUG1_Pin|DEBUG2_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);

  /*Configure GPIO pin : MCU_OK_Pin */
  GPIO_InitStruct.Pin = MCU_OK_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(MCU_OK_GPIO_Port, &GPIO_InitStruct);

/* USER CODE BEGIN MX_GPIO_Init_2 */
/* USER CODE END MX_GPIO_Init_2 */
}

/* USER CODE BEGIN 4 */

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
