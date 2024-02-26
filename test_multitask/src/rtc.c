#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <linux/i2c-dev.h>


#define I2C_BUS "/dev/i2c-1"  // Change this if your I2C bus is different
#define DS3231_ADDRESS 0x68   // DS3231 I2C address same as DS1307

// IMPORTANT, DS3231 USES SAME REGISTERS ADDRESSES AS DS1307!!
// The rest of the code is an example for DS1307
// DS1307 Register Addresses
#define DS1307_SECONDS_REG 0x00
#define DS1307_MINUTES_REG 0x01
#define DS1307_HOURS_REG 0x02
#define DS1307_DAY_REG 0x03
#define DS1307_DATE_REG 0x04
#define DS1307_MONTH_REG 0x05
#define DS1307_YEAR_REG 0x06
int i2cFile;

// Function to convert a binary number to BCD
uint8_t bin2bcd(uint8_t binary) {
    return ((binary / 10) << 4) | (binary % 10);
}

// Function to convert a BCD number to binary
uint8_t bcd2bin(uint8_t bcd) {
    return ((bcd >> 4) * 10) + (bcd & 0x0F);
}
// Open the I2C bus
int openI2C() {
    int i2cFile = open(I2C_BUS, O_RDWR);
    if (i2cFile < 0) {
        perror("Error opening I2C bus");
        exit(EXIT_FAILURE);
    }
    return i2cFile;
}

// Set up the I2C communication with the RTC at address 0x68
void setupRTC() {
    if (ioctl(i2cFile, I2C_SLAVE, DS3231_ADDRESS) < 0) {
        perror("Error setting up I2C communication with RTC");
        close(i2cFile);
        exit(EXIT_FAILURE);
    }
}

// Read a byte from the specified register address above
uint8_t readRegister(uint8_t reg) {
    uint8_t value;
    if (write(i2cFile, &reg, 1) != 1) {
        perror("Error writing to RTC register");
        close(i2cFile);
        exit(EXIT_FAILURE);
    }
    if (read(i2cFile, &value, 1) != 1) {
        perror("Error reading from RTC register");
        close(i2cFile);
        exit(EXIT_FAILURE);
    }
    return value;
}

// Write a byte to the specified register address (for setting time)
void writeRegister(uint8_t reg, uint8_t data) {
    uint8_t buffer[2] = { reg, data };
    if (write(i2cFile, buffer, 2) != 2) {
        perror("Error writing to RTC register");
        close(i2cFile);
        exit(EXIT_FAILURE);
    }
}

// Set the date and time on the RTC
void setDateTime(uint8_t year, uint8_t month, uint8_t date, uint8_t hours, uint8_t minutes, uint8_t seconds) {
    writeRegister(DS1307_SECONDS_REG, seconds);
    writeRegister(DS1307_MINUTES_REG, minutes);
    writeRegister(DS1307_HOURS_REG, hours);
    writeRegister(DS1307_DAY_REG, 1); // Assuming the day of the week (1-7) is Sunday (1)
    writeRegister(DS1307_DATE_REG, date);
    writeRegister(DS1307_MONTH_REG, month);
    writeRegister(DS1307_YEAR_REG, year);
}

// Read the current date and time from the RTC
void readDateTime() {
    uint8_t seconds = bcd2bin(readRegister(DS1307_SECONDS_REG));
    uint8_t minutes = bcd2bin(readRegister(DS1307_MINUTES_REG));
    uint8_t hours = bcd2bin(readRegister(DS1307_HOURS_REG));
    uint8_t date = bcd2bin(readRegister(DS1307_DATE_REG));
    uint8_t month = bcd2bin(readRegister(DS1307_MONTH_REG));
    uint8_t year = bcd2bin(readRegister(DS1307_YEAR_REG));

    //printf("Date and Time: 20%02d-%02d-%02d %02d:%02d:%02d\n", year, month, date, hours, minutes, seconds);
}
