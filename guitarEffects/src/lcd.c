/*
 * lcd.c
 *
 *  Created on: Sep 19, 2014
 *      Author: Clay
 */
#include "lcd.h"
#include "math.h"
#include "DSP28x_Project.h"

#define DELAY 11000

int updateLCD(int update){
	return -1;
}

void wait(int temp){
	while(temp != 0) temp--;
}

void initLCD(){
	EALLOW;
	GpioCtrlRegs.GPADIR.bit.GPIO0 = 0x1;
	GpioCtrlRegs.GPADIR.bit.GPIO1 = 0x1;
	GpioCtrlRegs.GPADIR.bit.GPIO2 = 0x1;
	GpioCtrlRegs.GPADIR.bit.GPIO3 = 0x1;
	GpioCtrlRegs.GPADIR.bit.GPIO4 = 0x1;
	GpioCtrlRegs.GPADIR.bit.GPIO5 = 0x1;
	GpioCtrlRegs.GPADIR.bit.GPIO6 = 0x1;
	GpioCtrlRegs.GPADIR.bit.GPIO7 = 0x1;
	GpioCtrlRegs.GPBDIR.bit.GPIO48 = 0x1;
	GpioCtrlRegs.GPBDIR.bit.GPIO49 = 0x1;
	//E = 48
	//RS = 49
	//8 Bit Mode
	//Write 3F for 1st function set
	controlLCD(0x3F);
	//Write 3F for 2nd function set
	controlLCD(0x3F);
	//Write 0F, D = 1 for display on, C = 1 for cursor on, B = 1 for blinking on
	controlLCD(0x0F);
	//Write 01 to clear display
	controlLCD(0x01);
	//Entry mode set for inc and no shift
	controlLCD(0x06);
}

void controlLCD(int data){
	GpioDataRegs.GPBDAT.bit.GPIO49 = 0;
	GpioDataRegs.GPADAT.all = data;
	wait(DELAY);
	GpioDataRegs.GPBDAT.bit.GPIO48 = 1;
	wait(DELAY);
	GpioDataRegs.GPBDAT.bit.GPIO48 = 0;
	wait(DELAY);

}

void printLCD(int data){
	GpioDataRegs.GPBDAT.bit.GPIO49 = 1;
	GpioDataRegs.GPADAT.all = data;
	wait(DELAY);
	GpioDataRegs.GPBDAT.bit.GPIO48 = 1;
	wait(DELAY);
	GpioDataRegs.GPBDAT.bit.GPIO48 = 0;
	wait(DELAY);
}

void printFREQ(int data){
	controlLCD(0x01);

	unsigned int* array = (unsigned int*)0xA000;
	int counter = 0;
	while(data >= 1){
		counter += 1;
		*array = fmod(data, 10);
		data  = data / 10;
		array += 1;
	}
	array -= 1;
	for(; counter > 0; counter--, array--){
		printLCD(*array+0x30);
	}
}
