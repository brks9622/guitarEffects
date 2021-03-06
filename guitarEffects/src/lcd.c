/*
 * lcd.c
 *
 *  Created on: Sep 19, 2014
 *      Author: Clay, Jonathan
 */
#include "lcd.h"
#include "lcdCodes.h"
#include "effect.h"
#include "math.h"
#include "DSP28x_Project.h"
#include "F28335_example.h"
#define DELAYLCD 10000

//Similar to queue system.  mainDisplay contains what could possibly be printed to screen.
//mainDisplay_show holds whether or not it is printed to screen (on_off[]);
int numInMainDisplay, tunerScreen, currentPreset, presetScreen, reprint;
int noteChart[355];

#pragma CODE_SECTION(controlLCD, "secureRamFuncs")
#pragma CODE_SECTION(printLCD, "secureRamFuncs")
#pragma CODE_SECTION(updateLCD, "secureRamFuncs")
#pragma CODE_SECTION(printTremolo, "secureRamFuncs")
#pragma CODE_SECTION(printReverb, "secureRamFuncs")
#pragma CODE_SECTION(updateLevel, "secureRamFuncs")
#pragma CODE_SECTION(goToMain, "secureRamFuncs")
#pragma CODE_SECTION(addToLCD, "secureRamFuncs")
#pragma CODE_SECTION(toggleLCD, "secureRamFuncs")
#pragma CODE_SECTION(printFreq, "secureRamFuncs")

void printTremolo(){
	controlLCD(CL);
	controlLCD(SECOND);
	printLCD(T);
	printLCD(R+lc);
	printLCD(E+lc);
	printLCD(M+lc);
	printLCD(O+lc);
	printLCD(L+lc);
	printLCD(O+lc);
	controlLCD(HOME); //Return cursor to home;
}
void printReverb(){
	controlLCD(CL);
	controlLCD(SECOND);
	printLCD(R);
	printLCD(E+lc);
	printLCD(V+lc);
	printLCD(E+lc);
	printLCD(R+lc);
	printLCD(B+lc);
	controlLCD(HOME); //Return cursor to home;
}
void printFlanger(){
	controlLCD(CL);
	controlLCD(SECOND);
	printLCD(F);
	printLCD(L+lc);
	printLCD(A+lc);
	printLCD(N+lc);
	printLCD(G+lc);
	printLCD(E+lc);
	printLCD(R+lc);
	controlLCD(HOME); //Return cursor to home;
}
void printPhaser(){
	controlLCD(CL);
	controlLCD(SECOND);
	printLCD(P);
	printLCD(H+lc);
	printLCD(A+lc);
	printLCD(S+lc);
	printLCD(E+lc);
	printLCD(R+lc);
	controlLCD(HOME); //Return cursor to home;
}
void printWah(){
	controlLCD(CL);
	controlLCD(SECOND);
	printLCD(W);
	printLCD(A+lc);
	printLCD(H+lc);
	controlLCD(HOME); //Return cursor to home;
}
void updateLevel(int input, int previousInput){
	int i = 0;
	//decreasing
	if(previousInput >= input){
		//Shift cursor to level, print spaces to oldLevel
		if(reprint){
			controlLCD(HOME);
			for(i = 0; i < previousInput; i++)printLCD(BAR);
			reprint = 0;
		}
		int cursorShift = 0x80 | (input);
		controlLCD(cursorShift);
		for(i = input; i <= previousInput; i++){
			printLCD(SPACE);
		}
	}
	//increasing
	else{
		if(reprint){
			previousInput = 0;
			reprint = 0;
		}
		//Shift cursor to oldlevel, print bars to level
		int cursorShift = 0x80 | (previousInput);
		controlLCD(cursorShift);
		for(i = previousInput; i < input; i++){
			printLCD(BAR);
		}
	}
}

//Eventually parses code and tells the printLCD() what to print
void updateLCD(int* update, int* mainDisplay, int* on_off, int* currentPreset, int* numQueued, int distortion){
	//Clear Screen, called by a reset on the queue. Block input if on tuner screen or preset screen
	if(*update == CLEAR && !tunerScreen && !presetScreen){
		*numQueued = 0;
		controlLCD(CL);
	}
	//Block input if tunerScreen
	else if(*update == PRESETTIMEOUT || *update == MAIN){
		presetScreen = 0;
		goToMain(mainDisplay, on_off, numQueued,distortion);

	}
	else if(*update == TUNER){
		if(tunerScreen){
			tunerScreen = 0;
			goToMain(mainDisplay, on_off, numQueued,distortion);
		}
		else{
			tunerScreen = 1;
			controlLCD(CL);
			printLCD(T);
			printLCD(U);
			printLCD(N);
			printLCD(E);
			printLCD(R);
			printLCD(COLON);
		}
	}
	else if(tunerScreen == 1){
		printFreq(*update);
	}
	else if(*update == PRESETUP){
		if(presetScreen){
			controlLCD(HOME); //Return cursor to home;
			//Shift Cursor over to correct position
			int cursorShift = 0x80 | 8;
			controlLCD(cursorShift);
			printLCD(*currentPreset+0x30);
		}
		else{
			//Clear Screen
			controlLCD(CL);
			controlLCD(HOME); //Return cursor to home;
			presetScreen = 1;
			printLCD(P);	//P
			printLCD(R);	//R
			printLCD(E);	//E
			printLCD(S);	//S
			printLCD(E);	//E
			printLCD(T);	//T
			printLCD(COLON);	//:
			printLCD(SPACE); //_
			printLCD(*currentPreset + 0x30);
		}
	}
	else if(*update == PRESETDOWN){
		if(presetScreen){
			controlLCD(HOME); //Return cursor to home;
			//Shift Cursor over to correct position
			int cursorShift = 0x80 | 8;
			controlLCD(cursorShift);
			printLCD(*currentPreset+0x30);
		}
		else{
			//Clear Screen
			controlLCD(CL);
			controlLCD(HOME); //Return cursor to home;
			presetScreen = 1;
			printLCD(P);	//P
			printLCD(R);	//R
			printLCD(E);	//E
			printLCD(S);	//S
			printLCD(E);	//E
			printLCD(T);	//T
			printLCD(COLON);	//:
			printLCD(SPACE); //_
			printLCD(*currentPreset + 0x30);
		}
	}
	else if(*update == SAVEPRESET && presetScreen){
		//Save presets.  Method of calling needs to be moved to main
		presetScreen = 0;
		goToMain(mainDisplay, on_off, numQueued, distortion);
	}
	else if(*update == LOADPRESET && presetScreen){
		//Load presets.  Method of calling needs to be moved to main
		presetScreen = 0;
		goToMain(mainDisplay, on_off, numQueued, distortion);
	}
	else if(*update == CHANGETREMOLO){
		reprint = 1;
		printTremolo();
	}
	else if(*update == CHANGEREVERB){
		reprint = 1;
		printReverb();
	}
	else if(*update == CHANGEWAH){
		reprint = 1;
		printWah();
	}
	else if(*update == CHANGEFLANGER){
		reprint = 1;
		printFlanger();
	}
	else if(*update == CHANGEPHASER){
		reprint = 1;
		printPhaser();
	}
}

void loadPresetScreen(int* location, int* mainDisplay, int* numQueued){
	//0-10 is location
	int i;
	/*Must loop over entire location array.  If the index value is not -1,
	you know the effect is queued.  Here, the index 'i' corresponds to the queued
	effect, defined in the #define section of lcd.h.  The value of location[i] in
	are location values, '1,2,3' of the effect in the queue.  This corresponds to the
	location in the mainDisplay array.  The on_off array(array offset by 10) is then used to determine
	whether or not the queued effect is currently turned on.*/

	for(i = 0; i < *numQueued; i++){
			mainDisplay[i] = location[i];
	}
}

void goToMain(int* mainDisplay, int* on_off, int* numQueued, int distortion){
	//Reprint main screen.  Usually called after a load/save preset or presettimeout or tuner screen toggle
	controlLCD(CL);
	if(distortion == 1){
		controlLCD(SECOND);
		printLCD(C);
		printLCD(R);
		printLCD(U);
		printLCD(N);
		printLCD(C);
		printLCD(H);
		printLCD(0x20);
		printLCD(0x20);
		printLCD(0x20);
		printLCD(0x20);
	}
	else if(distortion == 2){
			controlLCD(SECOND);
			printLCD(D);
			printLCD(I);
			printLCD(S);
			printLCD(T);
			printLCD(O);
			printLCD(R);
			printLCD(T);
			printLCD(I);
			printLCD(O);
			printLCD(N);
		}
	else{
		controlLCD(SECOND);
		printLCD(C);
		printLCD(L);
		printLCD(E);
		printLCD(A);
		printLCD(N);
		printLCD(0x20);
		printLCD(0x20);
		printLCD(0x20);
		printLCD(0x20);
		printLCD(0x20);
	}
	controlLCD(HOME);
	int i;
	for(i = 0; i < *numQueued; i++){
		if(on_off[i]){
			//printLCD(i+1 + 0x30);
			//printLCD(0x3A);
			addToLCD(mainDisplay[i]);
		}
		else{
			printLCD(SPACE);
			printLCD(SPACE);
			//printLCD(SPACE);
			//printLCD(SPACE);
		}
		printLCD(SPACE);
	}
}

void addToLCD(int effect){
	if(effect == TREMOLO){
		printLCD(T);
		printLCD(R);
	}
	else if(effect == REVERB){
		printLCD(R);
		printLCD(E);
	}
	else if(effect == DISTORTION){
		printLCD(D);
		printLCD(I);
	}
	else if(effect == CRUNCH){
		printLCD(C);
		printLCD(R);
	}
	else if(effect == WAH){
		printLCD(W);
		printLCD(A);
	}
	else if(effect == FLANGER){
		printLCD(F);
		printLCD(L);
	}
	else if(effect == PHASER){
		printLCD(P);
		printLCD(H);
	}
}
void toggleDistortion(int distortion){
	controlLCD(HOME);
	controlLCD(SECOND);
	if(distortion == 1){
		printLCD(C);
		printLCD(R);
		printLCD(U);
		printLCD(N);
		printLCD(C);
		printLCD(H);
		printLCD(0x20);
		printLCD(0x20);
		printLCD(0x20);
		printLCD(0x20);
	}
	else if(distortion == 2){
		printLCD(D);
		printLCD(I);
		printLCD(S);
		printLCD(T);
		printLCD(O);
		printLCD(R);
		printLCD(T);
		printLCD(I);
		printLCD(O);
		printLCD(N);
	}
	else{
		printLCD(C);
		printLCD(L);
		printLCD(E);
		printLCD(A);
		printLCD(N);
		printLCD(0x20);
		printLCD(0x20);
		printLCD(0x20);
		printLCD(0x20);
		printLCD(0x20);
	}
}
void toggleLCD(int effect, int index, int on, int numQueued){
	//controlLCD(0x02); //Return cursor to home;
	//Shift to beggining of section that needs to be changed
	if(effect != CRUNCH && effect != DISTORTION){
		int i;
		//int cursorShift = 0x80 | (index*5);
		int cursorShift = 0x80 | (index*3);
		controlLCD(cursorShift);


		//Went from off to on, reprint effect in proper spot.  Fill in proper vales
		if(on){
			//printLCD(index+1 + 0x30);
			//printLCD(0x3A);
			addToLCD(effect);
		}

		//Went from on to off, remove effect from spot
		else{
			//for(i = 0; i < 4; i++) printLCD(0x20);
			for(i = 0; i < 2; i++) printLCD(0x20);
		}
	//cursorShift = 0x80 | (numQueued*5);
		cursorShift = 0x80 | (numQueued*3);
	controlLCD(cursorShift);
	}

}


void printFreq(int data){
	int frequency = data;
	/*
	controlLCD(SECOND); //Second line;
	int i;
	for(i = 0; i < 3; i++){
		printLCD(SPACE);	//clear previous frequency. theres gotta be a better way
	}
	controlLCD(SECOND); //Second line;

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
	}*/
	findNote(frequency);

	controlLCD(HOME); //Return cursor to home;

}

void findNote(frequency){
	int input = 1;
	int note = noteChart[frequency];
	controlLCD(0x87);
	if(note == A){
		frequency -= 110;
		if(frequency > 50) frequency -= 110;
		printLCD(A);
	}
	else if(note == As){
		frequency -= 117;
		if(frequency > 50) frequency -= 116;
		printLCD(A);
		printLCD(SHARP);
	}
	else if(note == B){
		frequency -= 123;
		if(frequency > 50) frequency -= 124;
		printLCD(B);
	}
	else if(note == C){
		frequency -= 131;
		if(frequency > 50) frequency -= 131;
		printLCD(C);
	}
	else if(note == Cs){
		frequency -= 139;
		if(frequency > 50) frequency -= 138;
		printLCD(C);
		printLCD(SHARP);
	}
	else if(note == D){
		frequency -= 147;
		if(frequency > 50) frequency -= 147;
		printLCD(D);
	}
	else if(note == Ds){
		frequency -= 156;
		if(frequency > 50) frequency -= 155;
		printLCD(D);
		printLCD(SHARP);
	}
	else if(note == E){
		frequency -= 82;
		if(frequency > 200) frequency -= 248;
		else if(frequency > 50) frequency -= 83;
		printLCD(E);
	}
	else if(note == F){
		frequency -= 87;
		if(frequency > 200) frequency -= 262;
		else if(frequency > 50)  frequency -= 88;
		printLCD(F);
	}
	else if(note == Fs){
		frequency -= 92;
		if(frequency > 50) frequency -= 93;
		printLCD(F);
		printLCD(SHARP);
	}
	else if(note == G){
		frequency -= 98;
		if(frequency > 50) frequency -= 98;
		printLCD(G);
	}
	else if(note == Gs){
		frequency -= 104;
		if(frequency > 50) frequency -= 104;
		printLCD(G);
		printLCD(SHARP);
	}
	else{
		input = 0;
	}
	if(input){
		if(note == A || note == B || note == C || note == D || note == E || note == F || note == G) printLCD(SPACE);
		printLCD(SPACE);
		if(frequency >= -1 && frequency <=1){
			printLCD(BAR);
			printLCD(SPACE);
		}
		else if(frequency > 1){
			printLCD(BAR);
			printLCD(LEFT);
		}
		else{
			controlLCD(0x89);
			printLCD(RIGHT);
			printLCD(BAR);
			printLCD(SPACE);
		}
	}
}

void initLCD(){
	int i;
	for(i = 0; i < 355; i++){
		if((i >= 75 && i <= 85) || (i >= 161 && i <= 170) || (i >= 320 && i <= 340)) noteChart[i] = E;
		else if((i >= 86 && i <= 89) || (i >= 171 && i <= 180) || (i >= 341 && i <= 360)) noteChart[i] = F;
		else if((i >= 90 && i <= 95) || (i >= 181 && i <= 190)) noteChart[i] = Fs;
		else if((i >= 96 && i <= 101) ||(i >= 191 && i <= 201)) noteChart[i] = G;
		else if((i >= 102&& i <= 106) ||(i >= 202 && i <= 214)) noteChart[i] = Gs;
		else if((i >= 107&& i <= 113) ||(i >= 215 && i <= 226)) noteChart[i] = A;
		else if((i >= 114&& i <= 120) ||(i >= 227 && i <= 240)) noteChart[i] = As;
		else if((i >= 121&& i <= 127) ||(i >= 241 && i <= 254)) noteChart[i] = B;
		else if((i >= 128&& i <= 135) ||(i >= 255 && i <= 269)) noteChart[i] = C;
		else if((i >= 136&& i <= 143) ||(i >= 270 && i <= 285)) noteChart[i] = Cs;
		else if((i >= 144&& i <= 151) ||(i >= 286 && i <= 304)) noteChart[i] = D;
		else if((i >= 152&& i <= 160) ||(i >= 305 && i <= 319)) noteChart[i] = Ds;
		else noteChart[i] = 0xFFFF;
	}
	tunerScreen = 0;
	currentPreset = 1;
	presetScreen = 0;
	EALLOW;
	GpioCtrlRegs.GPADIR.bit.GPIO8 = 0x1;
	GpioCtrlRegs.GPADIR.bit.GPIO9 = 0x1;
	GpioCtrlRegs.GPADIR.bit.GPIO10 = 0x1;
	GpioCtrlRegs.GPADIR.bit.GPIO11 = 0x1;
	GpioCtrlRegs.GPADIR.bit.GPIO12 = 0x1;
	GpioCtrlRegs.GPADIR.bit.GPIO13 = 0x1;
	GpioCtrlRegs.GPADIR.bit.GPIO14 = 0x1;
	GpioCtrlRegs.GPADIR.bit.GPIO15 = 0x1;
	GpioCtrlRegs.GPADIR.bit.GPIO25 = 0x1;
	GpioCtrlRegs.GPADIR.bit.GPIO21 = 0x1;
	//E = 25
	//RS = 21
	//8 Bit Mode
	//Write 38 for 1st function set
	controlLCD(0x38);
	//Write 38 for 2nd function set
	controlLCD(0x38);
	//Write 0C, D = 1 for display on, C = 0 for cursor off, B = 0 for blinking off
	controlLCD(0x0C);
	//Write 01 to clear display
	controlLCD(CL);
	//Entry mode set for inc and no shift
	controlLCD(0x06);
}

void controlLCD(int data){
	GpioDataRegs.GPADAT.bit.GPIO21 = 0;
	GpioDataRegs.GPADAT.all = data << 8;
	GpioDataRegs.GPADAT.bit.GPIO25 = 1;
	DelayUs(15);
	GpioDataRegs.GPADAT.bit.GPIO25 = 0;
	DelayUs(37);
	if(data == CL || data == HOME){
		DelayUs(1483);
	}
}

void printLCD(int data){
	GpioDataRegs.GPADAT.bit.GPIO21 = 1;
	GpioDataRegs.GPADAT.all = data << 8;
	GpioDataRegs.GPADAT.bit.GPIO25 = 1;
	DelayUs(15);
	GpioDataRegs.GPADAT.bit.GPIO25 = 0;
	DelayUs(48);
}
