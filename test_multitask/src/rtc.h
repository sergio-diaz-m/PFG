/*
 * rtc.h
 *
 *  Created on: Nov 21, 2023
 *      Author: ubuntu
 */

#ifndef RTC_H_
#define RTC_H_
uint8_t bin2bcd(uint8_t binary);
uint8_t bcd2bin(uint8_t bcd);
int openI2C();
void setupRTC();
uint8_t readRegister(uint8_t reg);
void writeRegister(uint8_t reg, uint8_t data);
void setDateTime(uint8_t year, uint8_t month, uint8_t date, uint8_t hours, uint8_t minutes, uint8_t seconds);
void readDateTime();


#endif /* RTC_H_ */
