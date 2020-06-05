/*
 * retarget.c
 *
 *  Created on: May 29, 2020
 *      Author: andriitkach
 */

#include "retarget.h"

extern UART_HandleTypeDef huart2;

/**
  * @brief  Retargets the C library printf function to the USART.
  * @param  None
  * @retval None
  */
PUTCHAR_PROTOTYPE
{
  /* Place your implementation of fputc here */
  /* e.g. write a character to the EVAL_COM1 and Loop until the end of transmission */
  HAL_UART_Transmit(&huart2, (uint8_t *)&ch, 1, 0xFFFF);

  return ch;
}
