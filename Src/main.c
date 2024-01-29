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
#include "adc.h"
#include "dma.h"
#include "gpio.h"
#include "usart.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include "lcu.h"
#include "modbus.h"
#include <stdbool.h>
#include <stdio.h>
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
extern DMA_HandleTypeDef hdma_usart2_rx;
extern DMA_HandleTypeDef hdma_usart3_rx;
volatile uint8_t process_it = false;
uint16_t adc = 0xFFFF;
char tmp_str[64] = {0};
// AI[0] - current light level
// AO[1] - light threshold
// DO[0].0 - mode 1-automatic/0-manual
// DO[1].0 - light 1-on/0-off
MODBUS_registers registers;
UART_message rx1_buf;
UART_message rx2_buf;
UART_message tx_buf;
UART_message *rx_buffer = &rx1_buf;
UART_message *parse_buffer = &rx2_buf;
MODBUS_message rx_msg;
MODBUS_message tx_msg;
/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
/* USER CODE BEGIN PFP */
uint8_t process_buffer(void);
/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */

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
    MX_USART3_UART_Init();
    MX_USART2_UART_Init();
    MX_ADC1_Init();
    /* USER CODE BEGIN 2 */
    HAL_ADCEx_Calibration_Start(&hadc1);

    HAL_UARTEx_ReceiveToIdle_DMA(&huart2, rx_buffer->msg_data, BUFFER_SIZE);
    __HAL_DMA_DISABLE_IT(&hdma_usart2_rx, DMA_IT_HT);

    /* TEST BLOCK START */
    //	  READ_DO = 1,
    //		READ_DI,
    //		READ_AO,
    //		READ_AI,
    uint8_t nData[32] = {0x01, WRITE_AO_MULTI, 0x03, 0x02, 0x00, 0x02, 0x4, 0xaa, 0xbb, 0xcc, 0xdd};
    uint8_t wLength = 11;
    UART_message test_uart_rx_msg = {0};
    UART_message test_uart_tx_msg = {0};
    MODBUS_message test_modbus_msg = {0};
    MODBUS_registers test_registers = {0};
    test_registers.DO_start_address = 0x0100;
    test_registers.DI_start_address = 0x0200;
    test_registers.AO_start_address = 0x0300;
    test_registers.AI_start_address = 0x0400;
    test_registers.DO_count = 8;
    test_registers.DI_count = 8;
    test_registers.AO_count = 8;
    test_registers.AI_count = 8;
    for (uint8_t i = 0; i < 8; i++)
    {
        test_registers.DO[i] = i;
        test_registers.DI[i] = i + 16;
        test_registers.AO[i] = i + 32;
        test_registers.AI[i] = i + 64;
    }
    uint16_t myCRC = MODBUS_CRC16(nData, wLength);
    nData[wLength++] = myCRC & 0xFF;
    nData[wLength++] = myCRC >> 8;
    test_uart_rx_msg.msg_length = wLength;
    for (uint8_t i = 0; i < 32; i++)
    {
        test_uart_rx_msg.msg_data[i] = nData[i];
    }
    uint8_t status = msg_validate(&test_uart_rx_msg);
    msg_parse(&test_uart_rx_msg, &test_modbus_msg);
    status = response_prepare(&test_modbus_msg, &test_registers, &test_uart_tx_msg);
    __NOP();
    /* TEST BLOCK END */
    /* USER CODE END 2 */

    /* Infinite loop */
    /* USER CODE BEGIN WHILE */
    while (1)
    {
        /* USER CODE END WHILE */

        /* USER CODE BEGIN 3 */
        if (process_it)
        {
            process_it = false;
            HAL_UART_Transmit_IT(&huart2, parse_buffer->msg_data, parse_buffer->msg_length);

            uint8_t status = process_buffer();
            if (status == MB_ERR)
            {
                __NOP();
            }
        }
        light_level_update(&registers);
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
    RCC_PeriphCLKInitTypeDef PeriphClkInit = {0};

    /** Initializes the RCC Oscillators according to the specified parameters
     * in the RCC_OscInitTypeDef structure.
     */
    RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSE;
    RCC_OscInitStruct.HSEState = RCC_HSE_ON;
    RCC_OscInitStruct.HSEPredivValue = RCC_HSE_PREDIV_DIV1;
    RCC_OscInitStruct.HSIState = RCC_HSI_ON;
    RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
    RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSE;
    RCC_OscInitStruct.PLL.PLLMUL = RCC_PLL_MUL9;
    if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
    {
        Error_Handler();
    }

    /** Initializes the CPU, AHB and APB buses clocks
     */
    RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK | RCC_CLOCKTYPE_SYSCLK | RCC_CLOCKTYPE_PCLK1 | RCC_CLOCKTYPE_PCLK2;
    RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
    RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
    RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV2;
    RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV1;

    if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_2) != HAL_OK)
    {
        Error_Handler();
    }
    PeriphClkInit.PeriphClockSelection = RCC_PERIPHCLK_ADC;
    PeriphClkInit.AdcClockSelection = RCC_ADCPCLK2_DIV8;
    if (HAL_RCCEx_PeriphCLKConfig(&PeriphClkInit) != HAL_OK)
    {
        Error_Handler();
    }
}

/* USER CODE BEGIN 4 */
uint16_t get_light_level()
{
    HAL_ADC_Start(&hadc1);
    HAL_ADC_PollForConversion(&hadc1, 100);
    uint16_t adc = HAL_ADC_GetValue(&hadc1);
    HAL_ADC_Stop(&hadc1);
#ifdef DEBUG
    snprintf(tmp_str, 63, "ADC: %d\n", adc / 655);
    HAL_UART_Transmit(&huart2, (uint8_t *)tmp_str, strlen(tmp_str), 1000);
#endif // DEBUG
    return adc;
}

uint8_t process_buffer()
{
    modbus_status_t status;

    HAL_UART_Transmit_IT(&huart2, parse_buffer->msg_data, parse_buffer->msg_length);

    if (msg_validate(parse_buffer) == MB_ERR)
    {
        return MB_ERR;
    }
    msg_parse(parse_buffer, &rx_msg);
    status = response_prepare(&rx_msg, &registers, &tx_buf);
    if (status == MB_ERR)
    {
        return MB_ERR;
    }

    return MB_OK;
}

void HAL_UARTEx_RxEventCallback(UART_HandleTypeDef *huart, uint16_t Size)
{
    if (huart->Instance == USART2)
    {
        rx_buffer->msg_length = Size;
        rx_buffer->msg_data[Size] = '\0';
        UART_message *tmp = rx_buffer;
        rx_buffer = parse_buffer;
        parse_buffer = tmp;
        process_it = true;
        HAL_UARTEx_ReceiveToIdle_DMA(&huart2, rx_buffer->msg_data, BUFFER_SIZE);
        __HAL_DMA_DISABLE_IT(&hdma_usart2_rx, DMA_IT_HT);
    }
}
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

#ifdef USE_FULL_ASSERT
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
