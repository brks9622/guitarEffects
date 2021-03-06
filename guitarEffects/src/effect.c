/*
 * effect.h
 *
 *  Created on: Sep 22, 2014
 *      Author: Clay, Jonathan
 */

#include "effect.h"
#include "F28335_example.h"
#include "math.h"
#include "DSP28x_Project.h"
#include "spi.h"
#include "eeprom.h"
#include "effect.h"


#pragma CODE_SECTION(loadPreset, "secureRamFuncs")
#pragma CODE_SECTION(savePreset, "secureRamFuncs")
#pragma CODE_SECTION(eepromWrite, "secureRamFuncs")
#pragma CODE_SECTION(eepromRead, "secureRamFuncs")
#pragma CODE_SECTION(i2c_int1a_isr, "secureRamFuncs")
#pragma CODE_SECTION(savePreset, "secureRamFuncs")
#pragma CODE_SECTION(process, "secureRamFuncs")
#pragma CODE_SECTION(processTremolo, "secureRamFuncs")
#pragma CODE_SECTION(processReverb, "secureRamFuncs")
#pragma CODE_SECTION(processCrunch, "secureRamFuncs")
#pragma CODE_SECTION(processDistortion, "secureRamFuncs")


//For eeprom reading/writing
int readDone, writeDone = 0;
unsigned int adcVal[5];
#define EEPROMREADDELAY 5000
#define EEPROMWRITEDELAY 5000
struct I2CMSG *CurrentMsgPtr;	// Used in interrupts
struct I2CMSG messageOut;
struct I2CMSG messageIn;
#define I2C_SLAVE_ADDR        0x50
#define I2C_NUMBYTES          5
Uint16 Error;
void eepromWrite();
void eepromRead();

int returnArray[26];

#define wahMin 250;
#define wahMax 1700;

#define NZEROS 2
#define NPOLES 2
#define GAIN   5.807622925e+02

static float xv[NZEROS+1], yv[NPOLES+1];

void initEffects(struct params* params){
	//Clear the queue
	clearPipeline();
	//Tremolo
	params->tremoloCounter = 0;
	params->tremoloCount = 0;
	params->tremoloRate = 0;
	params->tremoloLimit = 0;
	//Reverb
	params->reverbCount = 0;
	params->reverbStart = 0;
	//Flanger
	params->flangerLimit = 20000;
	params->flangerCounter = 0;
	params->flangerStart = 0;
	params->flangerCount = 0;
	params->flangerSweep = 0;
	//Wah
	params->wahCount = 0;
	params->wahCounter = wahMin;
	params->wahStart = 0;
	int i;
	for(i = 0; i < 600; i++) params->flangerDelay[i] = 0;
	for(i = 0; i < 3; i++){
		params->phaserx[i] = 0;
		params->phasery[i] = 0;
		params->yh[i] = 0;
		params->yb[i] = 0;
		params->yl[i] = 0;
		params->yn[i] = 0;
	}
	for(i = 0; i < NPOLES; i++){
		xv[i] = 0;
		yv[i] = 0;
	}
	params->phaserCount = 0;
	params->phaserCounter = 500;
}

int process(int sample, int numQueued, int* on_off, FUNC**pipeline, struct params* params, int* counts){
	int index;
	for(index = 0; index < numQueued; index++){
		if(on_off[index]){
			sample = pipeline[index](sample, params, counts);  //pipeline[index] maps to a function, (sample, &params) is the prototype
		}
	}
	return sample;
}

/*
 * Processing functions
 */
int processTremolo(int sample, struct params* p, int* counts){
	//Sets rate at which the effect runs
		int pedal = 16-counts[0];
		p->tremoloLimit = (double)9000*((float)pedal/(float)16)+ 5000;

		//Count up or down, if it hits upper limit then count up else count down
		if(p->tremoloCounter >= p->tremoloLimit) p->tremoloCount = -1;
		else if(p->tremoloCounter == 0) p->tremoloCount = 1;
		p->tremoloCounter+=p->tremoloCount;

		//Calculate new tremolo sample
		double temp = (double)p->tremoloCounter*.7/(double)p->tremoloLimit;//*(double)sample;
		sample = temp*(double)sample;
		return sample;
}
int processWah(int sample, struct params* p, int* counts){
	//Auto wah, LFO with bandpass filter

	double damp = .05;
	int minf = wahMin;
	int maxf = wahMax;
	double Fw = (double)counts[3]/(double)16 *(double)7000+ wahMax;

	double Fs = 44000;
	double delta = Fw/Fs;  //Provides the increase/decrease step of the center frequency

	//LFO, similar to autowah
	if(p->wahCounter >= maxf) p->wahCount = -delta;
	else if(p->wahCounter <= minf) p->wahCount = delta;
	p->wahCounter+=p->wahCount;

	double F1 = 2*sin(((double)PI*(double)p->wahCounter)/Fs); //Calculate new F1
	double Q1 = 2*damp;	//Quality factor
	//If not started, initialize arrays to 0
	if(!p->wahStart){
		p->wahStart = 1;
		p->yh[0] = sample;
		p->yb[0] = F1*p->yh[0];
		p->yl[0] = F1*p->yb[0];
		p->yb[1] = sample;
	}
	//Bandpass IIR implementation
	else{
		p->yh[1] = (double)sample - p->yl[0] - Q1*p->yb[0];
		p->yb[1] = F1*p->yh[1] + p->yb[0];
		p->yl[1] = F1*p->yb[1] + p->yl[0];
	}

	//Shift samples through buffer
	sample = (int)p->yb[1];
	p->yb[0] = p->yb[1];
	p->yl[0] = p->yl[1];

	return sample;
}
int processPhaser(int sample, struct params* p, int* counts){
	double damp = .05;
		int minf = 100;
		int maxf = 3000;
		double Fw = (double)counts[3]/(double)16 *(double)7000+ wahMax;

		double Fs = 44000;
		double delta = Fw/Fs;  //Provides the increase/decrease step of the center frequency

		//LFO, similar to autowah
		if(p->wahCounter >= maxf) p->wahCount = -delta;
		else if(p->wahCounter <= minf) p->wahCount = delta;
		p->wahCounter+=p->wahCount;

		double F1 = 2*sin(((double)PI*(double)p->wahCounter)/Fs); //Calculate new F1
		double Q1 = 2*damp;	//Quality factor
		//If not started, initialize arrays to 0
		if(!p->wahStart){
			p->wahStart = 1;
			p->yh[0] = sample;
			p->yb[0] = F1*p->yh[0];
			p->yl[0] = F1*p->yb[0];
			p->yb[1] = sample;
		}
		//Bandpass IIR implementation
		else{
			p->yh[1] = (double)sample - p->yl[0] - Q1*p->yb[0];
			p->yb[1] = F1*p->yh[1] + p->yb[0];
			p->yl[1] = F1*p->yb[1] + p->yl[0];
		}

		//Shift samples through buffer
		sample =  p->yh[1] + p->yl[1];
		p->yb[0] = p->yb[1];
		p->yl[0] = p->yl[1];

		return sample;
}
int processFlanger(int sample, struct params* p, int* counts){
	//Process sweep
	//Max delay = 15ms
	//Delay sweep at 1 Hz
	//int Fs = 22727;
	//int rate = 1;
	double decay = .6;
		//Once reinitialized, start to process reverb
	if(p->flangerCount == 600){
		p->flangerStart = 1;
		p->flangerCount = 0;
	}

	//p->flangerCounter++;
	//if(p->flangerCounter == 22000)p->flangerCounter = 0;
	//int delay = p->flangerSweep * maxSampleDelay;
	//int delayIndex = delay;//abs(p->flangerCount - delay);
	if(p->flangerStart){

		int rate = counts[2];
		rate >>= 4;
		rate = (double)rate*(double)11000 + (double)20000;

		double toSine = 2.0*(double)PI*(double)p->flangerSweepCount*(double)1/(double)rate;

		p->flangerSweep = sin(toSine);
		if(p->flangerSweep < 0) p->flangerSweep *= (double)-1.0;
		p->flangerSweepCount++;
		if(p->flangerSweepCount == rate) p->flangerSweepCount = 0;
		int sweepDelay = p->flangerSweep * (double)100;
		int delayIndex = p->flangerCount;
		delayIndex = delayIndex - sweepDelay;
		if(delayIndex < 0) delayIndex += 100;
		int temp = p->flangerDelay[delayIndex];
		p->flangerDelay[p->flangerCount] = (double)sample*decay + (double)p->flangerDelay[p->flangerCount]* decay;
		p->flangerCount++;

		sample +=  temp;
	}
	else p->flangerDelay[p->flangerCount] = (double)sample*decay;

	p->flangerCount++;
	return sample;
}
int processReverb(int sample, struct params* p, int* counts){
	//Reinitialize the reverb array on start
	if(p->reverbCount == 1600){
		p->reverbStart = 1;
		p->reverbCount = 0;
	}
	double decay = counts[1];
	decay = ((double)decay / (double)16)*.8;
	//Once reinitialized, start to process reverb
	if(p->reverbStart){
		//Grab sample at current delay index, add in to sample to be returned.  Take current delay index and store in the decayed
		//sample plus the previous decayes samples.
		int temp = p->reverbDelay[p->reverbCount];

		xv[0] = xv[1]; xv[1] = xv[2];
		        xv[2] = temp / GAIN;
		        yv[0] = yv[1]; yv[1] = yv[2];
		        yv[2] =   (xv[0] + xv[2]) + 2 * xv[1]
		                     + ( -0.8861273417 * yv[0]) + (  1.8792398422 * yv[1]);
		        temp = yv[2];

		p->reverbDelay[p->reverbCount] = (double)sample + (double)temp*decay;
		sample += temp;
	}
	//If not started, loop through and initialize buffer to remove garbage values
	else p->reverbDelay[p->reverbCount] = (double)sample*decay;

	p->reverbCount++;
	return sample;
}

void savePreset(int presetNum, int* location, int* on_off, int* counts, int distortion){
	//Calculate addresses based on presetNum
	int i = 0;
	int locationMessage1 = (presetNum-1)*32;
	int on_offMessage1 = locationMessage1 + 4;
	int countsMessage1 = on_offMessage1 + 4;
	int distortionMessage1 = countsMessage1+4;
	int tLocation[4];
	//Inc location array by 1 because I don't want to save -1 in eeprom
	for(i = 0; i < 4; i++){
		int temp = location[i];
		tLocation[i] = ++temp;
	}

	//write first half of location array
	messageOut.MemoryLowAddr = locationMessage1 & 0x00FF;
	messageOut.MemoryHighAddr = (locationMessage1 & 0xFF00)>>8;
	messageOut.MsgStatus = I2C_MSGSTAT_SEND_WITHSTOP;
	messageOut.SlaveAddress = 0x50;
	messageOut.NumOfBytes = 4;
	for(i = 0; i < messageOut.NumOfBytes; i++) messageOut.MsgBuffer[i] = tLocation[i];
	eepromWrite();
	DelayUs(EEPROMWRITEDELAY);
	while(messageOut.MsgStatus != I2C_MSGSTAT_INACTIVE){};

	//write first half of on_off array
	messageOut.MemoryLowAddr = on_offMessage1 & 0x00FF;
	messageOut.MemoryHighAddr = (on_offMessage1 & 0xFF00)>>8;
	messageOut.MsgStatus = I2C_MSGSTAT_SEND_WITHSTOP;
	messageOut.SlaveAddress = 0x50;
	messageOut.NumOfBytes = 4;
	for(i = 0; i < messageOut.NumOfBytes; i++) messageOut.MsgBuffer[i] = on_off[i];
	eepromWrite();
	DelayUs(EEPROMWRITEDELAY);
	while(messageOut.MsgStatus != I2C_MSGSTAT_INACTIVE){};

	//write out counts values
	messageOut.MemoryLowAddr = countsMessage1 & 0x00FF;
	messageOut.MemoryHighAddr = (countsMessage1 & 0xFF00)>>8;
	messageOut.MsgStatus = I2C_MSGSTAT_SEND_WITHSTOP;
	messageOut.SlaveAddress = 0x50;
	messageOut.NumOfBytes = 4;
	for(i = 0; i < messageOut.NumOfBytes; i++) messageOut.MsgBuffer[i] = counts[i];
	eepromWrite();
	DelayUs(EEPROMWRITEDELAY);
	while(messageOut.MsgStatus != I2C_MSGSTAT_INACTIVE){};

	//write out distortion values
	messageOut.MemoryLowAddr = distortionMessage1 & 0x00FF;
	messageOut.MemoryHighAddr = (distortionMessage1 & 0xFF00)>>8;
	messageOut.MsgStatus = I2C_MSGSTAT_SEND_WITHSTOP;
	messageOut.SlaveAddress = 0x50;
	messageOut.NumOfBytes = 1;
	for(i = 0; i < messageOut.NumOfBytes; i++) messageOut.MsgBuffer[i] = distortion;
	eepromWrite();
	DelayUs(EEPROMWRITEDELAY);
	while(messageOut.MsgStatus != I2C_MSGSTAT_INACTIVE){};
}

void loadPreset(int presetNum, FUNC**pipeline, FUNC**list, int* location, int* on_off, int* numQueued, int* counts, int *distortion){
	int i = 0;
	//Calculate addresses based on presetNum
	int locationMessage1 = (presetNum-1)*32;
	int on_offMessage1 = locationMessage1 + 4;
	int countsMessage1 = on_offMessage1 + 4;
	int distortionMessage1 = countsMessage1+4;

	//Read in first half of location array
	messageIn.MemoryLowAddr = locationMessage1 & 0x00FF;
	messageIn.MemoryHighAddr = (locationMessage1 & 0xFF00)>>8;
	messageIn.MsgStatus = I2C_MSGSTAT_SEND_NOSTOP;
	messageIn.SlaveAddress = 0x50;
	messageIn.NumOfBytes = 4;
	eepromRead();
	DelayUs(EEPROMREADDELAY);
	for(i = 0; i < messageIn.NumOfBytes; i++) {
		location[i] = messageIn.MsgBuffer[i];
		returnArray[i] = location[i];
	}
	while(messageIn.MsgStatus != I2C_MSGSTAT_INACTIVE);

	//Read in second half of on_off array
	messageIn.MemoryLowAddr = on_offMessage1 & 0x00FF;
	messageIn.MemoryHighAddr = (on_offMessage1 & 0xFF00)>>8;
	messageIn.MsgStatus = I2C_MSGSTAT_SEND_NOSTOP;
	messageIn.SlaveAddress = 0x50;
	messageIn.NumOfBytes = 4;
	eepromRead();
	DelayUs(EEPROMREADDELAY);
	for(i = 0; i < messageIn.NumOfBytes; i++){
		on_off[i] = messageIn.MsgBuffer[i];
	}
	while(messageIn.MsgStatus != I2C_MSGSTAT_INACTIVE);

	//Read in counts values
	messageIn.MemoryLowAddr = countsMessage1 & 0x00FF;
	messageIn.MemoryHighAddr = (countsMessage1 & 0xFF00)>>8;
	messageIn.MsgStatus = I2C_MSGSTAT_SEND_NOSTOP;
	messageIn.SlaveAddress = 0x50;
	messageIn.NumOfBytes = 4;
	eepromRead();
	DelayUs(EEPROMREADDELAY);
	for(i = 0; i < messageIn.NumOfBytes; i++){
		counts[i] = messageIn.MsgBuffer[i];
	}
	while(messageIn.MsgStatus != I2C_MSGSTAT_INACTIVE);

	//Read in distortion value
	messageIn.MemoryLowAddr = distortionMessage1 & 0x00FF;
	messageIn.MemoryHighAddr = (distortionMessage1 & 0xFF00)>>8;
	messageIn.MsgStatus = I2C_MSGSTAT_SEND_NOSTOP;
	messageIn.SlaveAddress = 0x50;
	messageIn.NumOfBytes = 1;
	eepromRead();
	DelayUs(EEPROMREADDELAY);
	for(i = 0; i < messageIn.NumOfBytes; i++){
		*distortion = messageIn.MsgBuffer[i];
	}
	while(messageIn.MsgStatus != I2C_MSGSTAT_INACTIVE);

	//Dec location array by 1 because I don't want to save -1 in eeprom
	for(i = 0; i < 4; i++){
		int temp = location[i];
		temp = temp - 1;
		location[i] = temp;
	}

	*numQueued = 0;
	for(i = 0; i < 4; i++){
		int temp = location[i];
		if(temp != -1){
			location[i] = temp;
			pipeline[temp] = list[i];
			*numQueued = *numQueued + 1;
		}
	}
}

interrupt void i2c_int1a_isr(void){
   Uint16 IntSource, i;
   // Read interrupt source
   IntSource = I2caRegs.I2CISRC.all;
   // Interrupt source = stop condition detected
   if(IntSource == I2C_SCD_ISRC){
      // If completed message was writing data, reset msg to inactive state
      if (CurrentMsgPtr->MsgStatus == I2C_MSGSTAT_WRITE_BUSY){
         CurrentMsgPtr->MsgStatus = I2C_MSGSTAT_INACTIVE;
      }
      else{
         // If a message receives a NACK during the address setup portion of the
         // EEPROM read, the code further below included in the register access ready
         // interrupt source code will generate a stop condition. After the stop
         // condition is received (here), set the message status to try again.
         // User may want to limit the number of retries before generating an error.
         if(CurrentMsgPtr->MsgStatus == I2C_MSGSTAT_SEND_NOSTOP_BUSY){
            CurrentMsgPtr->MsgStatus = I2C_MSGSTAT_SEND_NOSTOP;
         }
         // If completed message was reading EEPROM data, reset msg to inactive state
         // and read data from FIFO.
         else if (CurrentMsgPtr->MsgStatus == I2C_MSGSTAT_READ_BUSY){
            CurrentMsgPtr->MsgStatus = I2C_MSGSTAT_INACTIVE;
            for(i=0; i < I2C_NUMBYTES; i++){
              CurrentMsgPtr->MsgBuffer[i] = I2caRegs.I2CDRR;
              readDone = 1;
            }
         }
      }
   }  // end of stop condition detected

   // Interrupt source = Register Access Ready
   // This interrupt is used to determine when the EEPROM address setup portion of the
   // read data communication is complete. Since no stop bit is commanded, this flag
   // tells us when the message has been sent instead of the SCD flag. If a NACK is
   // received, clear the NACK bit and command a stop. Otherwise, move on to the read
   // data portion of the communication.
   else if(IntSource == I2C_ARDY_ISRC){
      if(I2caRegs.I2CSTR.bit.NACK == 1){
         I2caRegs.I2CMDR.bit.STP = 1;
         I2caRegs.I2CSTR.all = I2C_CLR_NACK_BIT;
      }
      else if(CurrentMsgPtr->MsgStatus == I2C_MSGSTAT_SEND_NOSTOP_BUSY){
         CurrentMsgPtr->MsgStatus = I2C_MSGSTAT_RESTART;
      }
   }  // end of register access ready

   else{
      // Generate some error due to invalid interrupt source
      asm("   ESTOP0");
   }

   // Enable future I2C (PIE Group 8) interrupts
   PieCtrlRegs.PIEACK.all = PIEACK_GROUP8;
}

void eepromWrite(){
	writeDone = 0;
	while(!writeDone){
		 if(messageOut.MsgStatus == I2C_MSGSTAT_SEND_WITHSTOP){
			Error = I2CA_WriteData(&messageOut);
			if (Error == I2C_SUCCESS){
				writeDone = 1;
				CurrentMsgPtr = &messageOut;
				messageOut.MsgStatus = I2C_MSGSTAT_WRITE_BUSY;
			}
		}  // end of write section
	}
}

void eepromRead(){
	readDone = 0;
	while(!readDone){
	//Check outgoing message status. Bypass read section if status is not inactive.
		  if (messageOut.MsgStatus == I2C_MSGSTAT_INACTIVE){
			 // Check incoming message status.
			 if(messageIn.MsgStatus == I2C_MSGSTAT_SEND_NOSTOP){
				// EEPROM address setup portion
				while(I2CA_ReadData(&messageIn) != I2C_SUCCESS){

				}
				// Update current message pointer and message status
				CurrentMsgPtr = &messageIn;
				messageIn.MsgStatus = I2C_MSGSTAT_SEND_NOSTOP_BUSY;
			 }

			 // Once message has progressed past setting up the internal address
			 // of the EEPROM, send a restart to read the data bytes from the
			 // EEPROM. Complete the communique with a stop bit. MsgStatus is
			 // updated in the interrupt service routine.
			 else if(messageIn.MsgStatus == I2C_MSGSTAT_RESTART){
				// Read data portion
				while(I2CA_ReadData(&messageIn) != I2C_SUCCESS){
				   // Maybe setup an attempt counter to break an infinite while
				   // loop.
				}
				// Update current message pointer and message status
				CurrentMsgPtr = &messageIn;
				messageIn.MsgStatus = I2C_MSGSTAT_READ_BUSY;
			 }
		  }
	}
}
