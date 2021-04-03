/**
  ******************************************************************************
  * @file    stm32f1xx_my_rtc.h
  * @author  AliMoallem
  * @brief   Header file of RTC MY module.
  ******************************************************************************
  */

/* Define to prevent recursive inclusion -------------------------------------*/
#ifndef __STM32F1xx_MY_RTC_H
#define __STM32F1xx_MY_RTC_H

#ifdef __cplusplus
extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/
#include "main.h"

HAL_StatusTypeDef MY_RTC_SetTime(RTC_HandleTypeDef *hrtc, RTC_TimeTypeDef *sTime, uint32_t Format);
HAL_StatusTypeDef MY_RTC_GetTime(RTC_HandleTypeDef *hrtc, RTC_TimeTypeDef *sTime, uint32_t Format);
HAL_StatusTypeDef MY_RTC_SetDate(RTC_HandleTypeDef *hrtc, RTC_DateTypeDef *sDate, uint32_t Format);
HAL_StatusTypeDef MY_RTC_GetDate(RTC_HandleTypeDef *hrtc, RTC_DateTypeDef *sDate, uint32_t Format);

#ifdef __cplusplus
}
#endif

#endif /* __STM32F1xx_MY_RTC_H */

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/
