/**
  ******************************************************************************
  * @file    stm32f1xx_my_rtc.c
  * @author  AliMoallem (https://github.com/alimoal)
  * @brief   Source file of RTC MY module.
  ******************************************************************************
  
==============================================================================
                  ##### How to use this driver #####
==============================================================================
Add this library beside stm32f1xx_hal_rtc files

*/
#include "stm32f1xx_my_rtc.h"

int Temp_t;
uint32_t days_elapsed_2 = 0U;

const unsigned char month_table[12] = {31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};
uint32_t cal_year = 1970U;
	
static unsigned char check_for_leap_year(unsigned int year)
{
    if(year % 4 == 0)
    {
        if(year % 100 == 0)
        {
           if(year % 400 == 0)
           {
               return 1;
           }
           else 
           {
               return 0;
           }
        }
        else 
        {
           return 1;
        }
    }
	else 
    {
       return 0;
    }
}
/**
  * @brief  Enters the RTC Initialization mode.
  * @param  hrtc   pointer to a RTC_HandleTypeDef structure that contains
  *                the configuration information for RTC.
  * @retval HAL status
  */
static HAL_StatusTypeDef RTC_EnterInitMode(RTC_HandleTypeDef *hrtc)
{
  uint32_t tickstart = 0U;

  tickstart = HAL_GetTick();
  /* Wait till RTC is in INIT state and if Time out is reached exit */
  while ((hrtc->Instance->CRL & RTC_CRL_RTOFF) == (uint32_t)RESET)
  {
    if ((HAL_GetTick() - tickstart) >  RTC_TIMEOUT_VALUE)
    {
      return HAL_TIMEOUT;
    }
  }

  /* Disable the write protection for RTC registers */
  __HAL_RTC_WRITEPROTECTION_DISABLE(hrtc);


  return HAL_OK;
}

/**
  * @brief  Exit the RTC Initialization mode.
  * @param  hrtc   pointer to a RTC_HandleTypeDef structure that contains
  *                the configuration information for RTC.
  * @retval HAL status
  */
static HAL_StatusTypeDef RTC_ExitInitMode(RTC_HandleTypeDef *hrtc)
{
  uint32_t tickstart = 0U;

  /* Disable the write protection for RTC registers */
  __HAL_RTC_WRITEPROTECTION_ENABLE(hrtc);

  tickstart = HAL_GetTick();
  /* Wait till RTC is in INIT state and if Time out is reached exit */
  while ((hrtc->Instance->CRL & RTC_CRL_RTOFF) == (uint32_t)RESET)
  {
    if ((HAL_GetTick() - tickstart) >  RTC_TIMEOUT_VALUE)
    {
      return HAL_TIMEOUT;
    }
  }

  return HAL_OK;
}
/**
  * @brief  Write the time counter in RTC_CNT registers.
  * @param  hrtc   pointer to a RTC_HandleTypeDef structure that contains
  *                the configuration information for RTC.
  * @param  TimeCounter: Counter to write in RTC_CNT registers
  * @retval HAL status
  */
static HAL_StatusTypeDef RTC_WriteTimeCounter(RTC_HandleTypeDef *hrtc, uint32_t TimeCounter)
{
  HAL_StatusTypeDef status = HAL_OK;

  /* Set Initialization mode */
  if (RTC_EnterInitMode(hrtc) != HAL_OK)
  {
    status = HAL_ERROR;
  }
  else
  {
    /* Set RTC COUNTER MSB word */
    WRITE_REG(hrtc->Instance->CNTH, (TimeCounter >> 16U));
    /* Set RTC COUNTER LSB word */
    WRITE_REG(hrtc->Instance->CNTL, (TimeCounter & RTC_CNTL_RTC_CNT));

    /* Wait for synchro */
    if (RTC_ExitInitMode(hrtc) != HAL_OK)
    {
      status = HAL_ERROR;
    }
  }

  return status;
}
/**
  * @brief  Read the time counter available in RTC_CNT registers.
  * @param  hrtc   pointer to a RTC_HandleTypeDef structure that contains
  *                the configuration information for RTC.
  * @retval Time counter
  */
static uint32_t RTC_ReadTimeCounter(RTC_HandleTypeDef *hrtc)
{
  uint16_t high1 = 0U, high2 = 0U, low = 0U;
  uint32_t timecounter = 0U;

  high1 = READ_REG(hrtc->Instance->CNTH & RTC_CNTH_RTC_CNT);
  low   = READ_REG(hrtc->Instance->CNTL & RTC_CNTL_RTC_CNT);
  high2 = READ_REG(hrtc->Instance->CNTH & RTC_CNTH_RTC_CNT);

  if (high1 != high2)
  {
    /* In this case the counter roll over during reading of CNTL and CNTH registers,
       read again CNTL register then return the counter value */
    timecounter = (((uint32_t) high2 << 16U) | READ_REG(hrtc->Instance->CNTL & RTC_CNTL_RTC_CNT));
  }
  else
  {
    /* No counter roll over during reading of CNTL and CNTH registers, counter
       value is equal to first value of CNTL and CNTH */
    timecounter = (((uint32_t) high1 << 16U) | low);
  }

  return timecounter;
}
/**
  * @brief  Converts from 2 digit BCD to Binary.
  * @param  Value: BCD value to be converted
  * @retval Converted word
  */
static uint8_t RTC_Bcd2ToByte(uint8_t Value)
{
  uint32_t tmp = 0U;
  tmp = ((uint8_t)(Value & (uint8_t)0xF0) >> (uint8_t)0x4) * 10U;
  return (tmp + (Value & (uint8_t)0x0F));
}
/**
  * @brief  Determines the week number, the day number and the week day number.
  * @param  nYear   year to check
  * @param  nMonth  Month to check
  * @param  nDay    Day to check
  * @note   Day is calculated with hypothesis that year > 2000
  * @retval Value which can take one of the following parameters:
  *         @arg RTC_WEEKDAY_MONDAY
  *         @arg RTC_WEEKDAY_TUESDAY
  *         @arg RTC_WEEKDAY_WEDNESDAY
  *         @arg RTC_WEEKDAY_THURSDAY
  *         @arg RTC_WEEKDAY_FRIDAY
  *         @arg RTC_WEEKDAY_SATURDAY
  *         @arg RTC_WEEKDAY_SUNDAY
  */
static uint8_t RTC_WeekDayNum(uint32_t nYear, uint8_t nMonth, uint8_t nDay)
{
  uint32_t year = 0U, weekday = 0U;

  year = 2000U + nYear;

  if (nMonth < 3U)
  {
    /*D = { [(23 x month)/9] + day + 4 + year + [(year-1)/4] - [(year-1)/100] + [(year-1)/400] } mod 7*/
    weekday = (((23U * nMonth) / 9U) + nDay + 4U + year + ((year - 1U) / 4U) - ((year - 1U) / 100U) + ((year - 1U) / 400U)) % 7U;
  }
  else
  {
    /*D = { [(23 x month)/9] + day + 4 + year + [year/4] - [year/100] + [year/400] - 2 } mod 7*/
    weekday = (((23U * nMonth) / 9U) + nDay + 4U + year + (year / 4U) - (year / 100U) + (year / 400U) - 2U) % 7U;
  }

  return (uint8_t)weekday;
}


/**
  * @brief  Sets RTC current time.
  * @param  hrtc   pointer to a RTC_HandleTypeDef structure that contains
  *                the configuration information for RTC.
  * @param  sTime: Pointer to Time structure
  * @param  Format: Specifies the format of the entered parameters.
  *          This parameter can be one of the following values:
  *            @arg RTC_FORMAT_BIN: Binary data format 
  *            @arg RTC_FORMAT_BCD: BCD data format
  * @retval HAL status
  */
HAL_StatusTypeDef MY_RTC_SetTime(RTC_HandleTypeDef *hrtc, RTC_TimeTypeDef *sTime, uint32_t Format)
{
  uint32_t counter_time = 0U, Temp = 0U, hours = 0U, days_elapsed = 0U;
	
	Temp = RTC_ReadTimeCounter(hrtc);
	hours = Temp / 3600U;
	if (hours >= 24U)
  {
    days_elapsed = (hours / 24U);
		Temp = (days_elapsed * 24U * 3600U);
	}
  else
	{
		Temp = 0;
	}
  __HAL_LOCK(hrtc);
  hrtc->State = HAL_RTC_STATE_BUSY;
  
  if(Format == RTC_FORMAT_BIN)
  {
    assert_param(IS_RTC_HOUR24(sTime->Hours));
    assert_param(IS_RTC_MINUTES(sTime->Minutes));
    assert_param(IS_RTC_SECONDS(sTime->Seconds));

    counter_time = (uint32_t)(((uint32_t)sTime->Hours * 3600U) + \
                        ((uint32_t)sTime->Minutes * 60U) + \
                        ((uint32_t)sTime->Seconds));  
  }
  else
  {
    assert_param(IS_RTC_HOUR24(RTC_Bcd2ToByte(sTime->Hours)));
    assert_param(IS_RTC_MINUTES(RTC_Bcd2ToByte(sTime->Minutes)));
    assert_param(IS_RTC_SECONDS(RTC_Bcd2ToByte(sTime->Seconds)));

    counter_time = (((uint32_t)(RTC_Bcd2ToByte(sTime->Hours)) * 3600U) + \
              ((uint32_t)(RTC_Bcd2ToByte(sTime->Minutes)) * 60U) + \
              ((uint32_t)(RTC_Bcd2ToByte(sTime->Seconds))));   
  }
	
	counter_time += Temp;
  RTC_WriteTimeCounter(hrtc, counter_time);
  CLEAR_BIT(hrtc->Instance->CRL, (RTC_FLAG_SEC | RTC_FLAG_OW));
 
  hrtc->State = HAL_RTC_STATE_READY;
  __HAL_UNLOCK(hrtc); 
     
  return HAL_OK;
}

/**
  * @brief  Gets RTC current time.
  * @param  hrtc   pointer to a RTC_HandleTypeDef structure that contains
  *                the configuration information for RTC.
  * @param  sTime: Pointer to Time structure
  * @param  Format: Specifies the format of the entered parameters.
  *          This parameter can be one of the following values:
  *            @arg RTC_FORMAT_BIN: Binary data format 
  *            @arg RTC_FORMAT_BCD: BCD data format
  * @retval HAL status
  */
HAL_StatusTypeDef MY_RTC_GetTime(RTC_HandleTypeDef *hrtc, RTC_TimeTypeDef *sTime, uint32_t Format)
{
  uint32_t counter_time = 0U,/* days_elapsed = 0U, */hours = 0U;

  if (__HAL_RTC_OVERFLOW_GET_FLAG(hrtc, RTC_FLAG_OW))
  {
      return HAL_ERROR;
  }
  counter_time = RTC_ReadTimeCounter(hrtc);
  hours = counter_time / 3600U;
  sTime->Minutes  = (uint8_t)((counter_time % 3600U) / 60U);
  sTime->Seconds  = (uint8_t)((counter_time % 3600U) % 60U);
	sTime->Hours = (hours % 24U);

  if (hours >= 24U)
  {
 //   days_elapsed = (hours / 24U);
    sTime->Hours = (hours % 24U);    
  }
  else 
  {
    sTime->Hours = hours;    
  }
  return HAL_OK;
}


/**
  * @brief  Sets RTC current date.
  * @param  hrtc   pointer to a RTC_HandleTypeDef structure that contains
  *                the configuration information for RTC.
  * @param  sDate: Pointer to date structure
  * @param  Format: specifies the format of the entered parameters.
  *          This parameter can be one of the following values:
  *            @arg RTC_FORMAT_BIN: Binary data format 
  *            @arg RTC_FORMAT_BCD: BCD data format
  * @retval HAL status
  */
HAL_StatusTypeDef MY_RTC_SetDate(RTC_HandleTypeDef *hrtc, RTC_DateTypeDef *sDate, uint32_t Format)
{
  uint32_t counter_time = 0U, hours = 0U;
  uint32_t year = 0U, counts = 0U, Hrs = 0U,Min = 0U, Sec = 0U, Month_1 = 0U, Day_1 = 0U;
	const unsigned char month_table[12] = {31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};
	int i;
	
	counter_time = RTC_ReadTimeCounter(hrtc);
	hours = counter_time / 3600U;
  Min  = (uint8_t)((counter_time % 3600U) / 60U);
  Sec  = (uint8_t)((counter_time % 3600U) % 60U);
	Hrs = (hours % 24U);
	
	year = (uint32_t) sDate->Year + 2000U;
	if(year > 2099)
  {
    year = 2099;
  }
  if(year < 1970)
  {
    year = 1970;
  }
	for(i = 1970; i < year; i++)
  {
    if(check_for_leap_year(i) == 1)
    {
      counts += 31622400;
    }
    else
    {
      counts += 31536000;
    }
  }
	
	Month_1 = (uint32_t) sDate->Month;
  if (check_for_leap_year(year) == 1 && Month_1 > 2)
	{
		counts += 86400;
	}
	Month_1 -= 1;
	for(i = 0; i < Month_1; i++)
  {
    counts += (((uint32_t)month_table[i]) * 86400);
  }
	
	Day_1 = (uint32_t) sDate->Date;
	counts += ((uint32_t)(Day_1 - 1) * 86400);
  counts += ((uint32_t)Hrs * 3600);
  counts += ((uint32_t)Min * 60);
  counts += Sec;
	
 __HAL_LOCK(hrtc);
 hrtc->State = HAL_RTC_STATE_BUSY; 
	
 RTC_WriteTimeCounter(hrtc, counts);
	
 hrtc->State = HAL_RTC_STATE_READY ; 
 __HAL_UNLOCK(hrtc);
  
  return HAL_OK;    
}

/**
  * @brief  Gets RTC current date.
  * @param  hrtc   pointer to a RTC_HandleTypeDef structure that contains
  *                the configuration information for RTC.
  * @param  sDate: Pointer to Date structure
  * @param  Format: Specifies the format of the entered parameters.
  *          This parameter can be one of the following values:
  *            @arg RTC_FORMAT_BIN:  Binary data format 
  *            @arg RTC_FORMAT_BCD:  BCD data format
  * @retval HAL status
  */
HAL_StatusTypeDef MY_RTC_GetDate(RTC_HandleTypeDef *hrtc, RTC_DateTypeDef *sDate, uint32_t Format)
{
	unsigned int temp1 = 0, temp2 = 0;
//	static unsigned int day_count;
	unsigned long temp = 0;
  uint32_t counts = 0U;
	counts = RTC_ReadTimeCounter(hrtc);
//  temp = (counts / 86400);
//	if(day_count != temp)
//  {
    day_count = temp;
    temp1 = 1970;
    while(temp >= 365)
    {
      if(check_for_leap_year(temp1) == 1)
      {
        if(temp >= 366)
        {
          temp -= 366;
        }
        else
        {
          break;
        }
      }
      else
      {
        temp -= 365;
      }
      temp1++;
    };
		
		temp2 = temp1;
		sDate->Year = (uint8_t)(temp1 - 2000);	
		temp1 = 0;
		while(temp >= 28)
    {
			if((temp1 == 1) && (check_for_leap_year(temp2) == 1))
      {
        if(temp >= 29)
        {
          temp -= 29;
        }
        else
        {
          break;
        }
      }
      else
      {
        if(temp >= month_table[temp1])
        {
          temp -= ((uint32_t)month_table[temp1]);
        }
        else
        {
          break;
        }
      }
      temp1++;
    };	 
		sDate->Month = (uint8_t)(temp1 + 1);
		sDate->Date = (uint8_t)(temp + 1);
		sDate->WeekDay = RTC_WeekDayNum(sDate->Year, sDate->Month, sDate->Date);
//  }
  return HAL_OK;
}
