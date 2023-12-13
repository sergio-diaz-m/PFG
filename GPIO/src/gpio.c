/*
 * gpio.c
 *
 *  Created on: Nov 18, 2023
 *      Author:
 *
 *
 *      diazs
 */

#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

void gpio_export(char* pin)
{

	int fd = open("/sys/class/gpio/export", O_WRONLY);
	if (fd == -1) {
		perror("Unable to open /sys/class/gpio/export");
		exit(1);
	}

	if (write(fd, pin , 2) != 2) {
		perror("Error writing to /sys/class/gpio/export");
		exit(1);
	}
}
void gpio_set(char* pin, char* mode)
{
	char dir[50];
	sprintf(dir,"/sys/class/gpio/gpio%s/direction",pin);
   	int fd = open(dir, O_WRONLY);
	if (fd == -1) {
		perror("Unable to open direction");
		exit(1);
	}

	if (write(fd, mode, 3) != 3) {
		perror("Error writing mode");
		exit(1);
	}

	close(fd);
}
void gpio_write(char* pin, char* value)
{
	char dir[50];
	sprintf(dir,"/sys/class/gpio/gpio%s/value",pin);
   	int fd = open(dir, O_WRONLY);
    if (fd == -1) {
        perror("Unable to open value");
        exit(1);
    }
    if (write(fd, value, 1) != 1) {
        perror("Error writing value");
        exit(1);
    }
    close(fd);
}
void gpio_unexport(char* pin)
{
    int fd = open("/sys/class/gpio/unexport", O_WRONLY);
    if (fd == -1) {
        perror("Unable to open /sys/class/gpio/unexport");
        exit(1);
    }

    if (write(fd, pin, 2) != 2) {
        perror("Error writing to /sys/class/gpio/unexport");
        exit(1);
    }

    close(fd);
}

int main()
{
	gpio_export("21");
	gpio_set("21","out");
	while(1){
		gpio_write("21","1");
		usleep(50000);
		gpio_write("21","0");
		usleep(50000);
	}
	gpio_unexport("21");
    return 0;
}


