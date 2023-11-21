/*
 * gpio.h
 *
 *  Created on: Nov 21, 2023
 *      Author: ubuntu
 */

#ifndef GPIO_H_
#define GPIO_H_

void gpio_export(char* pin);
void gpio_set(char* pin, char* mode);
void gpio_write(char* pin, char* value);
void gpio_unexport(char* pin);

#endif /* GPIO_H_ */
