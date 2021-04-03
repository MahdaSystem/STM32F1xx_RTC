# RTC STM32F1xx
This is a library for using RTC peripheral in stm32f1xx mcu
## Purpose
The HAL Library for RTC in STM32F1xx has a restriction that after each power off the values of *DATE* structure including **YEAR** , **MONTH**, **WEEKDAY** and **DAY** reset; so in this Library this restriction has been fixed.  

## How to use
1. Enable RTC peripheral ( Like in CubeMX )
2. Add this library to your project
3. Use **MY** functions instead of **HAL** ones
