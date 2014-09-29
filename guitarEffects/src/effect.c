#include "../include/effect.h"
#include "math.h"
#include "DSP28x_Project.h"

//Type definition for process*Effect* method prototypes
typedef int FUNC(int, struct params*);

//Create FUNC variables
FUNC processTremolo,processDistortion,processCrunch,processDelay,processWah,processPhaser,processFlange,processReverb,processChorus,processPitchShift;

//Static list of available effects, GPIO must match this
FUNC *list[10] = {processTremolo,processDistortion,processCrunch,processDelay,processWah,processPhaser,processFlange,processReverb,processChorus,processPitchShift};

/*The indices of this array map  directly to the *list array.  This location array holds the location of the effect in the pipeline array.
 * Index 0 of the location array maps to index 0 of the list array.  But the data at index 0 of the location arary points to
 * where that effect is located in the pipeline array. Increase in mem usage for speed gain, don't have to iterate through *list to find
 * the right effect.
 */
int location[10];

//Array of FUNC's, this is the queue set by the user.
FUNC *pipeline[10];

//State of each queued effect, this allows user to stomp effects on/off without losing location in queue
//Indices of this array map direclty to indices of pipeline
int on_off[10];

//Number of queued effects
int numQueued;

//Struct for all the parameters available to effects, passed into the process functions
static struct params params;

void initEffects(){
	//Clear the queue
	clearPipeline();
	//Tremolo
		params.tremoloCounter = 0;
		params.tremoloCount = 0;
		params.tremoloRate = 0;
		params.tremoloLimit = 0;
}

int toggleOn_Off(int effect){
	//If location[] == -1, then the effect is not in the queue so return 0 to signal
	//that this effect needs to be queued up.  Else just toggle the state.
	if(location[effect] != -1){
		on_off[location[effect]] ^= 1;
		return 1;
	}
	else return 0;
}

void queueEffect(int effect){
	location[effect] = numQueued;		//Set location of the effect in the location array, corresponds to the index in the queue
	pipeline[numQueued] = list[effect];	//queue function pointer into the pipeline
	on_off[numQueued] = 1;				//Turn effect on, makes sense for the user
	numQueued++;						//Increase numQueued
}


void clearPipeline(){
	//Clear location/on_off arrays
	int i;
	for(i=0; i < 10; i++){
		location[i] = -1;
		on_off[i] = 0;
	}
	numQueued = 0;
}

int process(int sample){
	int index;
	//Loop through the entire queue, if its on -> process, else skip
	for(index = 0; index < numQueued; index++){
		if(on_off[index]) sample = pipeline[index](sample, &params);  //pipeline[index] maps to a function, (sample, &params) is the prototype
	}
	return sample;
}

/*
 * Processing functions
 */
int processDelay(int sample, struct params* p){
	return sample;
}
int processDistortion(int sample, struct params* p){
	if(sample > 0xF20) return 0xF20;
	//else if (sample < 0xC000) return 0xC000;
	else return sample;
}
int processCrunch(int sample, struct params* p){
	return sample;
}
int processTremolo(int sample, struct params* p){
	//Sets rate at which the effect runs
	p->tremoloLimit = 7000;

	//Count up or down, if it hits upper limit then count up else count down
	if(p->tremoloCounter >= p->tremoloLimit) p->tremoloCount = -1;
	else if(!p->tremoloCounter) p->tremoloCount = 1;
	p->tremoloCounter+=p->tremoloCount;

	//Calculate new tremolo sample
	double temp = (double)p->tremoloCounter/(double)p->tremoloLimit;//*(double)sample;

	return (int)(temp*(double)sample);
}
int processWah(int sample, struct params* p){
	return sample;
}
int processPhaser(int sample, struct params* p){
	return sample;
}
int processFlange(int sample, struct params* p){
	return sample;
}
int processReverb(int sample, struct params* p){
	return sample;
}
int processChorus(int sample, struct params* p){
	return sample;
}
int processPitchShift(int sample, struct params* p){
	return sample;
}
