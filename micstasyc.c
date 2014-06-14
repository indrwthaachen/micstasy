/*
  Library for controlling Micstasy Microphone Preamps through MIDI (using PortMidi)

  Copyright (c) 2014, Gerrit Wyen <gerrit.wyen@rwth-aachen.de>, Florian Heese <heese@ind.rwth-aachen.de>
  Institute of Communication Systems and Data Processing
  RWTH Aachen University, Germany

*/

#include <stdlib.h>
#include <stdio.h>
#include <math.h>  
#include <string.h>
#include <time.h>

#include "micstasyc.h"



#define DEBUG 0

#define BIT(X) (1<<X)
int levelMeterLookupTable[] = { -70, -60, -50, -42, -36, -30, -24, -18, -12, -6, -3, -1, -0.1, 0 };

static char *errorMessage = NULL;

char *micstasy_list_midiDevices()
{
	int i = 0;
	const PmDeviceInfo *devInfo;
	char *descStrs;
	char name[BUF_SIZE];
	char interf[BUF_SIZE];

	int device_cnt = Pm_CountDevices();
	if(DEBUG) printf("device count is: %d\n", device_cnt);

	descStrs = (char *)malloc((70+2*BUF_SIZE)*device_cnt);
	descStrs[0] = 0;

	for(i=0; i<device_cnt; i++)
	{

		devInfo = Pm_GetDeviceInfo( (PmDeviceID) i);

		if(devInfo == NULL) 	
		{

			if(DEBUG) printf("Device by %d is NULL\n", i);
		}
		else
		{
	
			strncpy(name, devInfo->name, BUF_SIZE);
			strncpy(interf, devInfo->interf, BUF_SIZE);
			name[BUF_SIZE-1] = 0; interf[BUF_SIZE-1] = 0;

			sprintf(descStrs+strlen(descStrs), "ID: %i name: '%s' interf: '%s' (input: %d output: %d opened: %d)\n",
					 i, name, interf, devInfo->input, devInfo->output, devInfo->opened);
		}
		
	}	

	return descStrs;
}	


static void print_sysex(char *msg)
{
	int i = 0;
	printf("sysEx: ");
	
	while(1)
	{
		printf("%x ", msg[i]&0xFF);

		if(msg[i] == EOX)
			break;
		i++;
	}
	printf("\n");
}




static int sysex_message_send(struct micstasy *cMicstasy, int messageType, boolean sendParameterNumber, int8_t parameterNumber, boolean sendDataByte, int8_t dataByte)
{

	int8_t msg[20];
	int i=0;
	PmError ret;
	PmTimestamp when = 0;
	PmEvent event;
	PmError count;

	/* before sending anything clear input buffer */
	do {
		count = Pm_Read(cMicstasy->portMidiStreamIn, &event, 1);
	}
	while(count != 0);
	Sleep(1);

	msg[i++] = SYS_EX_HEADER;
	msg[i++] = MIDI_TEMP_MANUFACTRURER_ID_1;
	msg[i++] = MIDI_TEMP_MANUFACTRURER_ID_2;
	msg[i++] = MIDI_TEMP_MANUFACTRURER_ID_3;
	msg[i++] = MODEL_ID;

	msg[i++] = (cMicstasy->bankNumber<<4) | cMicstasy->deviceID;
	msg[i++] = messageType;


	if(sendParameterNumber) msg[i++] = parameterNumber;
	if(sendDataByte) msg[i++] = dataByte;



	msg[i++] = EOX;


	if(DEBUG)
	{
		printf("Sending: ");
		print_sysex(msg);
	}

	ret = Pm_WriteSysEx(cMicstasy->portMidiStreamOut, when, msg);


	return 1;
}


char *micstasy_errorMessage(void)
{
	return errorMessage;
}


static int error(char *err_msg)
{
    if(errorMessage != NULL)
        free(errorMessage);
    
    errorMessage = strdup(err_msg);
    
    if(DEBUG) printf("%s", err_msg);

	return -1;
}



int8_t *getSysExFromEventBuffer(CircularBuffer *readBuffer, int *length)
{
	int8_t *output = (char *)malloc(readBuffer->size*4);
	int i=0;
	int shift;
	int8_t data;
	PmEvent event;

	*length = 0;

	while(!cbIsEmpty(readBuffer))
	{	

		cbRead(readBuffer, &event);

	        if (is_real_time_msg(Pm_MessageStatus(event.message))) continue;


		for (shift = 0; shift < 32; shift += 8)
		{
			data = (event.message >> shift) & 0xFF;

			if(data == SYS_EX_HEADER)
			{

				output[0] = data;
				i=1;
			}
			else if(i>0)
			{


				/* if this is a status byte that's not MIDI_EOX, the sysex
				message is incomplete and there is no more sysex data */
				if(data & 0x80 && data != EOX)
				{
					i=0;
					break;
				}

				output[i] = data;
				i++;

				if(data == EOX)
				{
					*length = i;
					return output;
				}
			}
		}

		
	}

	free(output);

	return NULL;
}



static int8_t *sysex_message_receive(struct micstasy *cMicstasy, int messageType, int *length)
{
	PmEvent msg;
	int cnt;
	time_t timeout = time(NULL)+4;
	int8_t *data = NULL;

	if(DEBUG) printf("reading\n");

	do
	{

		do {
			cnt = Pm_Read(cMicstasy->portMidiStreamIn, &msg, 1);
			if (cnt != 0)
			{
				if(cbIsFull(&cMicstasy->readBuffer) && DEBUG) printf("WARNING: readBuffer overflow\n");
				cbWrite(&cMicstasy->readBuffer, &msg); 
			}
		} while(cnt != 0);


		data = getSysExFromEventBuffer(&cMicstasy->readBuffer, length);

		if(*length > 0)
		{
			if(DEBUG) printf("got SysEX data of lenght: %d\n", *length);


			if(*length > 6)
			{
				print_sysex(data);

				if(data[6] == messageType)
					return data;
			}
			
			
			free(data);
		}
		else Sleep(1);
	}
	while(timeout > time(NULL));



	*length = 0;


	error("no response from micstasy");

	return NULL;
}



struct micstasy *micstasy_init(int midiDeviceIn, int midiDeviceOut, int bankNumber, int deviceID)
{
	struct micstasy *nMicstasy;
	PmError ret;
	PortMidiStream *stream;
	long bufferSize = 100;

	nMicstasy = (struct micstasy *) malloc(sizeof(struct micstasy));

	nMicstasy->bankNumber = bankNumber;
	nMicstasy->deviceID = deviceID;
	cbInit(&nMicstasy->readBuffer, BUF_SIZE);

	if(DEBUG) printf("connecting to micstasy\n");




	if(DEBUG) printf("opening output device %d\n", midiDeviceOut);
	ret = Pm_OpenOutput(&stream,
		         midiDeviceOut,
		         NULL,
		         bufferSize,
		         NULL,
		         NULL,
		         0);

	if(DEBUG) printf("Returned %d\n", ret);
	if(ret != pmNoError) {
		error((char *)Pm_GetErrorText(ret));
		free(nMicstasy);
		return NULL;
	}

	nMicstasy->portMidiStreamOut = stream;


	if(DEBUG) printf("opening input device %d\n", midiDeviceIn);
	ret = Pm_OpenInput(&stream,
		         midiDeviceIn,
		         NULL,
		         bufferSize,
		         NULL,
		         NULL
		         );


	if(DEBUG) printf("returned: %d\n", ret);
	if(ret != pmNoError) {
		error((char *)Pm_GetErrorText(ret));
		free(nMicstasy);
		return NULL;
	}

	nMicstasy->portMidiStreamIn = stream;

	return nMicstasy;
}


int8_t micstasy_request_value(struct micstasy *cMicstasy, char parameterNumber)
{
	char dataByte=-1;
	char *response;
	int8_t value;
	int length=0;


	sysex_message_send( cMicstasy, MESSAGETYPE_REQUEST_VALUE, 0, parameterNumber, 0, dataByte);

	response = sysex_message_receive(cMicstasy, MESSAGETYPE_RESPONSE_VALUE, &length);


	if(length > 8+parameterNumber*2)
		value = response[8+parameterNumber*2];
	else
		value = -1; /* no error -> MSB: 0 */


	free(response);


	return value;
}


int micstasy_get_levelMeterData(struct micstasy *cMicstasy, struct micstasy_levelMeterData *levelMeterData)
{
	char *response;
	int length;
	int i;

	sysex_message_send( cMicstasy, MESSAGETYPE_REQUEST_LEVELMETER_DATA, 0, 0, 0, 0);

	response = sysex_message_receive(cMicstasy, MESSAGETYPE_RESPONSE_LEVELMETER_DATA, &length);
	/* F0 00 20 0D 68 (bank no. / dev ID) 31 (ch.1) (ch.2) (ch.3) (ch.4) (ch.5) (ch.6) (ch.7) (ch.8) F7 */

	if(length > 15)
		for(i=0; i<8; i++)
			if(response[7+i] >= 0 && response[7+i] < 14)
				levelMeterData->channel[i] = levelMeterLookupTable[response[7+i]];
			else levelMeterData->channel[i] = 0;
	else
		for(i=0; i<8; i++)
			levelMeterData->channel[i] = 0;

	free(response);



	/*
	Request Level Meter Data 

		MSB / 7		0
		6		0
		5		0
		4		0
		3 		MSB / 3 	2
		1		LSB / 0 	/ 2/ 1
		0		LSB / 0 	0
		0
	
		Level: 0 	= < -70dBFS peak
		Level: 1..12 	= < -60 / -50 / -42 / -36 / -30 / -24 /-18 / -12 / -6 / -3 / -1 / -0.1 dBFS
		Level: 13 	= > -0.1 dBFS (over)
	*/

	return levelMeterData->channel[0];
}



int micstasy_set_value(struct micstasy *cMicstasy, char parameterNumber, char dataByte)
{
	int ret;

	ret = sysex_message_send(cMicstasy, MESSAGETYPE_SET_VALUE, 1, parameterNumber, 1, dataByte);

	return ret;
}



int micstasy_set_gainCoarse(struct micstasy *cMicstasy, int channel, int dbValue)
{
	char value, ret;
	int parameterNumber;

	if(channel < 1 || channel > 8){
		error("Error: channel out of range (1..8)");
		return -1;
	}
	
	if(dbValue < -9 || dbValue > 76){
		error("Error: dB Value out of range (-9..76 dB)");
		return -1;
	}

	value = dbValue+9;
	parameterNumber = (channel-1)*3;


	ret = micstasy_set_value(cMicstasy, parameterNumber, value);


	return ret;
}

int micstasy_get_gainCoarse(struct micstasy *cMicstasy, int channel)
{

	int dbValue;
	int parameterNumber;

	if(channel < 1 || channel > 8){
		error("Error: channel out of range (1..8)");
		return -1;
	}

	parameterNumber = (channel-1)*3;

	dbValue = micstasy_request_value(cMicstasy, parameterNumber);

	if(dbValue != -1) dbValue = dbValue-9;
	
	return dbValue;
}


double micstasy_get_gain(struct micstasy *cMicstasy, int channel, double *dbValue)
{
	int ret;
	struct micstasy_parameters parameters;

	ret = micstasy_get_gainCoarse(cMicstasy, channel);
	if(ret == -1) return -1;

	*dbValue = ret;

	ret = micstasy_get_parameters(cMicstasy, channel, &parameters);
	if(ret == -1) return -1;

	if(parameters.gainFine == 1) *dbValue += 0.5;

	return ret;
}


int micstasy_get_parameters(struct micstasy *cMicstasy, int channel, struct micstasy_parameters *parameters)
{
	char value;
	int parameterNumber;

	if(channel < 1 || channel > 8){
		error("Error: channel out of range (1..8)");
		return -1;
	}

	parameterNumber = (channel-1)*3+1;
	memset(parameters, 0, sizeof(*parameters));

	value = micstasy_request_value(cMicstasy, parameterNumber);

	if(value != -1)
	{
		parameters->channel = channel;
		parameters->gainFine = value & BIT(0);
		if(channel == 1) {
			parameters->digitalOutSelect = (value & BIT(1)) >> 1;
			parameters->autoSetLink = -1;
			parameters->displayAutoDark = (value & BIT(6)) >> 6;
		}
		else {
			parameters->autoSetLink = (value & BIT(1)) >> 1;
			parameters->digitalOutSelect = -1;
			parameters->displayAutoDark = -1;
		}

		parameters->levelMeter = (value & (BIT(2) | BIT(3) | BIT(4) | BIT(5))) >> 2;

		if(parameters->levelMeter >= 0 && parameters->levelMeter < 14)
			parameters->levelMeter = levelMeterLookupTable[parameters->levelMeter];
		else parameters->levelMeter = 0;

	}

	return value;
}

int micstasy_get_settings(struct micstasy *cMicstasy, int channel, struct micstasy_settings *settings)
{
	char value;
	int parameterNumber;

	if(channel < 1 || channel > 8){
		error("Error: channel out of range (1..8)");
		return -1;
	}

	memset(settings, -1, sizeof(*settings));

	parameterNumber = (channel-1)*3+2;

	value = micstasy_request_value(cMicstasy, parameterNumber);

	if(value != -1)
	{
		settings->channel = channel;
		settings->input = value & BIT(0);
		settings->HiZ = (value & BIT(1))>>1;
		settings->autoset  = (value & BIT(2))>>2;
		settings->loCut = (value & BIT(3))>>3;

		if( channel != 1 && channel != 3 && channel != 5 && channel != 7 )
			settings->MS = -1;
		else settings->MS = (value & BIT(4))>>4;

		settings->phase = (value & BIT(5))>>5;
		settings->p48 = (value & BIT(6))>>6;
	}

	return value;
}


int micstasy_set_parameters(struct micstasy *cMicstasy, int channel, boolean gainFine, boolean displayAutoDark, boolean autoSetLink, boolean digitalOutSelect)
{
	int parameterNumber;
	char value = 0, ret;

	if(channel < 1 || channel > 8){
		error("Error: channel out of range (1..8)");
		return -1;
	}

	if(displayAutoDark != -1 && channel != 1){
		error("Error: Display auto dark only available on channel 1");
		return -1;
	}
	if(autoSetLink != -1 && channel == 1){
		error("Error: AutoSet link only available on channel 2..8");
		return -1;	
	}
	if(digitalOutSelect != -1 && channel != 1){
		error("Error: digital out selection only available on channel 1");
		return -1;	
	}


	parameterNumber = (channel-1)*3+1;

	if(displayAutoDark == 1)	/* 0=off, 1=on */
		value |= (1<<6);

	if(gainFine == 1)		/* 0=0dB, 1=+0.5db */
		value |= (1<<0);

	/* AutoSet Link: 0 = off, 1 = link to lower channel
	  channel 1: digital out AES/ADAT 0 = analog, 1 = Option */
	if(autoSetLink == 1 || digitalOutSelect == 1) 						
		value |= (1<<1);


	ret = micstasy_set_value(cMicstasy, parameterNumber, value);

	return ret;
}


int micstasy_set_settings(struct micstasy *cMicstasy, int channel,
		 boolean input, boolean HiZ, boolean autoset, boolean loCut, boolean MS, boolean phase, boolean p48)
{

	int ret, parameterNumber;
	char value = 0;

	if(channel < 1 || channel > 8){
		error("Error: channel out of range (1..8)");
		return -1;
	}

	if(MS != -1 && (channel != 1 && channel != 3 && channel != 5 && channel != 7) ){
		error("Error: M/S only available on channel 1,3,5,7");
		return -1;
	}	

	parameterNumber = (channel-1)*3+2;	


	if(input == 1)		/* Input: 0 = rear, 1 = front */
		value |= (1<<0);
	if(HiZ == 1)		/* Hi Z: 0 = off, 1 = on */
		value |= (1<<1);
	if(autoset == 1)	/* Autoset: 0 = off, 1 = on */
		value |= (1<<2);
	if(loCut == 1)		/* Lo Cut: 0 = off, 1 = on */
		value |= (1<<3);
	if(MS == 1)		/* M/S: 0 = off, 1 = on (set only ch. 1, 3, 5,7) */
		value |= (1<<4);
	if(phase == 1)		/* Phase: 0 = normal, 1 = inverted */
		value |= (1<<5);
	if(p48 == 1)		/* P48: 0 = off, 1 = on */
		value |= (1<<6);


	ret = micstasy_set_value(cMicstasy, parameterNumber, value);

	return ret;
}


int micstasy_setup(struct micstasy *cMicstasy, boolean intFreq, int clockRange, int clockSelect,
	 int analogOutput, boolean lockKeys, boolean peakHold, boolean followClock, int autosetLimit, boolean delayCompensation, boolean autoDevice)
{

	int ret, parameterNumber;
	char value;

	if(clockRange < 0 || clockRange > 2){
		error("Error: clock range out of range 0..2");
		return -1;
	}	
	if(clockSelect < 0 || clockSelect > 3){
		error("Error: clock select out of range 0..3");
		return -1;
	}
	if(analogOutput < 0 || analogOutput > 2){
		error("Error: analog output out of range 0..2");
		return -1;
	}	
	if(autosetLimit < 0 || autosetLimit > 3){
		error("Error: autoset limit out of range 0..3");
		return -1;
	}	

	parameterNumber = 0x18; /* setup 1 */
	value = 0;

	if(intFreq == 1)
		value |= (1<<0); /* int. freq.: 0 = 44.1kHz, 1 = 48kHz (don't care for clock sel > 0) */
	
	value |= (clockRange << 1); /* clock range: 0 = single speed, 1 = ds, 2= qs */
	value |= (clockSelect << 3); /* clock select: 0 = int., 1 = Option, 2 = AES, 3 = WCK */
	value |= (analogOutput << 5); /* analog output: 0 = +13dBu, 1 =+19dBu, 2 = +24dBu */

	ret = micstasy_set_value(cMicstasy, parameterNumber, value);
	if(ret == -1) return -1;

	parameterNumber = 0x19; /* setup 2 */
	value = 0;

	if(lockKeys == 1)
		value |= (1<<0); /* Lock Keys: 0 = unlock, 1 = lock */
	if(peakHold == 1)
		value |= (1<<1); /* Peak Hold: 0 = off, 1 = on */
	if(followClock == 1)
		value |= (1<<2); /* Follow Clock: 0 = off, 1 = on */

	value |= (autosetLimit << 3); /* Autoset-Limit: 0 = -1dB, 1 = -3dB, 2 = -6dB, 3 = -12dB */

	if(delayCompensation == 1)
		value |= (1<<5); /* Delay Compensation: 0 = off, 1 = on */
	if(autoDevice == 1)
		value |= (1<<6); /* Auto-Device: 0 = off, 1 = on */

	ret = micstasy_set_value(cMicstasy, parameterNumber, value);


	return ret;
}


int micstasy_get_setup(struct micstasy *cMicstasy, struct micstasy_setup *setup)
{
	int value;
	int parameterNumber;


	parameterNumber = 0x18; /* setup 1 */

	value = micstasy_request_value(cMicstasy, parameterNumber);
	if(value == -1) return -1;

	setup->intFreq = (value & BIT(0));
	setup->clockRange = (value & (BIT(1) | BIT(2))) >> 1;
	setup->clockSelect = (value & (BIT(3) | BIT(4))) >> 3;
	setup->analogOutput = (value & (BIT(5) | BIT(6))) >> 5;

	parameterNumber = 0x19; /* setup 2 */

	value = micstasy_request_value(cMicstasy, parameterNumber);
	if(value == -1) return -1;

	setup->lockKeys = (value & BIT(0));
	setup->peakHold = (value & BIT(1))>>1;
	setup->followClock = (value & BIT(2))>>2;
	setup->autosetLimit  = (value & (BIT(3) | BIT(4)))>>3;
	setup->delayCompensation = (value & BIT(5))>>5;
	setup->autoDevice = (value & BIT(6))>>6;


	return 1;
}



int micstasy_get_locksyncInfo(struct micstasy *cMicstasy, struct micstasy_locksyncInfo *synclock)
{
	char parameterNumber = 0x1A;
	char value;


	value = micstasy_request_value(cMicstasy, parameterNumber);

	memset(synclock, -1, sizeof(*synclock));

	if(value != -1)
	{
		synclock->optionLock = (value & BIT(0));
		synclock->optionSync  = (value & BIT(1))>>1;
		synclock->aesLock   = (value & BIT(2))>>2;
		synclock->aesSync = (value & BIT(3))>>3;
		synclock->wckLock   = (value & BIT(4))>>4;
		synclock->wckSync = (value & BIT(5))>>5;
		synclock->wcOut = (value & BIT(6))>>6;

	}
/*
	MSB / 7		0
	6		WC Out: 0 = Fs, 1 = Single Speed
	5		WCK Sync: 0 = no sync, 1 = sync
	4		WCK Lock: 0 = unlock, 1 = lock
	3		AES Sync: 0 = no sync, 1 = sync
	2		AES Lock: 0 = unlock, 1 = lock
	1		Option Sync: 0 = no sync, 1 = sync
	LSB / 0 	Option Lock: 0 = unlock, 1 = lock 
*/
	

	return value;
}


int micstasy_set_bankdevID(struct micstasy *cMicstasy, int8_t bankID, int8_t devID)
{

	int ret;
	int8_t bankdevID, parameterNumber = 0x1D;

	bankdevID = (bankID<<4) | devID;

	if( (bankdevID < 0x00 || bankdevID > 0x77) && bankdevID != 0x7F){
		error("Error: bankdevID out of range 0x00..0x77, 0x7F");
		return -1;
	}

	ret = micstasy_set_value(cMicstasy, parameterNumber, bankdevID);

	return ret;
}


int micstasy_set_oscillator(struct micstasy *cMicstasy, int channel)
{
	int ret;
	char parameterNumber = 0x1E;

	if(channel < 0 || channel > 8){
		error("Error: channel out of range (0=off,1..8)");
		return -1;
	}

	/* Oscillator: 0 = off, 1..8 = Channel 1..8 */
	ret = micstasy_set_value(cMicstasy, parameterNumber, channel);
	
	return ret;
}

int micstasy_memory_save(struct micstasy *cMicstasy, int slot)
{
	int ret;
	char parameterNumber = 0x1B;

	if(slot < 0 || slot > 8){
		error("Error: slot out of range (0=idle, 1..8)");
		return -1;
	}

	/* 0 = idle, 1..8 save memory 1..8 */
	ret = micstasy_set_value(cMicstasy, parameterNumber, slot);
	
	return ret;
}

int micstasy_memory_recall(struct micstasy *cMicstasy, int slot)
{
	int ret;
	char parameterNumber = 0x1C;

	if(slot < 0 || slot > 8){
		error("Error: slot out of range (0=idle, 1..8)");
		return -1;
	}

	/* 0 = idle, 1..8 recall memory 1..8 */
	ret = micstasy_set_value(cMicstasy, parameterNumber, slot);
	
	return ret;
}


int micstasy_store_state(struct micstasy *cMicstasy, char *filePath)
{
	char gainCoarse;
	struct micstasy_parameters parameters;
	struct micstasy_settings settings;
	struct micstasy_setup setup;
	int channel, ret;
	FILE *stateFile;


	stateFile = fopen(filePath, "w");
	if(stateFile == NULL) {
		error("ERROR: unable to open file");
		return -1;
	}

	for(channel=1; channel <= 8; channel++) {
		gainCoarse = micstasy_get_gainCoarse(cMicstasy, channel);
		micstasy_get_parameters(cMicstasy, channel, &parameters);
		micstasy_get_settings(cMicstasy, channel, &settings);

		fprintf(stateFile, "%d \n", gainCoarse);
		fprintf(stateFile, "%d %d %d %d \n", parameters.gainFine, parameters.digitalOutSelect, parameters.autoSetLink, parameters.displayAutoDark);
		fprintf(stateFile, "%d %d %d %d %d %d \n", settings.input, settings.HiZ, settings.loCut, settings.MS, settings.phase, settings.p48);


		if(DEBUG) {
			printf("channel: %d\n", channel);
			printf("gainCoarse: %d\n\n", gainCoarse);
			printf("gainFine: %d\n digitalOutSelect: %d\nautoSetLink: %d\n displayAutoDark: %d\n\n", parameters.gainFine, parameters.digitalOutSelect, parameters.autoSetLink, parameters.displayAutoDark);
			printf("input: %d\n HiZ: %d\n loCut: %d\n MS: %d\n Phase: %d\n p48: %d\n\n", settings.input, settings.HiZ, settings.loCut, settings.MS, settings.phase, settings.p48);

		}

	}

	ret = micstasy_get_setup(cMicstasy, &setup);

	fprintf(stateFile, "%d %d %d %d %d %d %d %d %d %d \n",
	setup.intFreq, setup.clockRange, setup.clockSelect, setup.analogOutput, setup.lockKeys, setup.peakHold, setup.followClock, setup.autosetLimit, setup.delayCompensation, setup.autoDevice);

	fclose(stateFile);

	return ret;
}

int micstasy_restore_state(struct micstasy *cMicstasy, char *filePath)
{
	int gainCoarse;
	struct micstasy_settings settings;
	struct micstasy_setup setup;
	int ret;
	FILE *stateFile;
	int gainFine, digitalOutSelect, autoSetLink, displayAutoDark;
	int channel, input, HiZ, autoset, loCut, MS, phase, p48;
	int intFreq, clockRange, clockSelect, analogOutput, lockKeys, peakHold, followClock, autosetLimit, delayCompensation, autoDevice;


	stateFile = fopen(filePath, "r");
	if(stateFile == NULL) {
		error("ERROR: unable to open file");
		return -1;
	}

	micstasy_set_oscillator(cMicstasy, 0); /* disable oscillator by default */

	for(channel=1; channel <= 8; channel++) {


		ret = fscanf(stateFile, "%d \n", &gainCoarse);
		
		ret = fscanf(stateFile, "%d %d %d %d \n", &gainFine, &digitalOutSelect, &autoSetLink, &displayAutoDark);

		ret = fscanf(stateFile, "%d %d %d %d %d %d \n", &input, &HiZ, &loCut, &MS, &phase, &p48);

	
		micstasy_set_gainCoarse(cMicstasy, channel, gainCoarse);
		micstasy_set_parameters(cMicstasy, channel, gainFine, displayAutoDark, autoSetLink, digitalOutSelect);
		micstasy_set_settings(cMicstasy, channel, input, HiZ, autoset, loCut, MS, phase, p48);

		if(DEBUG) {
			printf("channel: %d\n", channel);
			printf("gainCoarse: %d\n\n", gainCoarse);
			printf("gainFine: %d\n digitalOutSelect: %d\nautoSetLink: %d\n displayAutoDark: %d\n\n", gainFine, digitalOutSelect, autoSetLink, displayAutoDark);
			printf("input: %d\n HiZ: %d\n loCut: %d\n MS: %d\n Phase: %d\n p48: %d\n\n", input, HiZ, loCut, MS, phase, p48);

		}

	}


	ret = fscanf(stateFile, "%d %d %d %d %d %d %d %d %d %d \n",
	&intFreq, &clockRange, &clockSelect, &analogOutput, &lockKeys, &peakHold, &followClock, &autosetLimit, &delayCompensation, &autoDevice);

	ret = micstasy_setup(cMicstasy, intFreq, clockRange, clockSelect, analogOutput, lockKeys, peakHold, followClock, autosetLimit, delayCompensation, autoDevice);

	fclose(stateFile);

	return ret;
}

int micstasy_set_gain(struct micstasy *cMicstasy, int channel, double dbValue)
{
	boolean gainFine;
	struct micstasy_parameters parameters;	
	int ret;

	if(channel < 1 || channel > 8){
		error("Error: channel out of range (1..8)");
		return -1;
	}

	if( dbValue-(int)dbValue >= 0.75) {
		ret = micstasy_set_gainCoarse(cMicstasy, channel, (int)dbValue+1);
		gainFine = 0;
	}
	else if( dbValue-(int)dbValue < 0.25) {
		ret = micstasy_set_gainCoarse(cMicstasy, channel, (int)dbValue);
		gainFine = 0;
	}
	else {
		
		ret = micstasy_set_gainCoarse(cMicstasy, channel, (int)dbValue);
		gainFine = 1;
	}
	if(ret == -1) return -1;

	ret = micstasy_get_parameters(cMicstasy, channel, &parameters);	
	if(ret == -1) return -1;

	ret = micstasy_set_parameters(cMicstasy, channel, gainFine, parameters.displayAutoDark, parameters.autoSetLink, parameters.digitalOutSelect);	

	return ret;
}


int micstasy_close(struct micstasy *cMicstasy)
{
	Pm_Close(cMicstasy->portMidiStreamIn);
	Pm_Close(cMicstasy->portMidiStreamOut);
	cbFree(&cMicstasy->readBuffer);
	free(cMicstasy);

	return 1;
}



/* circular buffer */
  
void cbInit(CircularBuffer *cb, int size) {
	cb->size  = size + 1; 
	cb->start = 0;
	cb->end   = 0;
	cb->elems = (PmEvent *)calloc(cb->size, sizeof(PmEvent));
}
 
void cbFree(CircularBuffer *cb) 
{
	free(cb->elems); 
}
 
int cbIsFull(CircularBuffer *cb) 
{
	return (cb->end + 1) % cb->size == cb->start;
}
 
int cbIsEmpty(CircularBuffer *cb) 
{
	return cb->end == cb->start;
}
 
void cbWrite(CircularBuffer *cb, PmEvent *elem) 
{
	cb->elems[cb->end] = *elem;
	cb->end = (cb->end + 1) % cb->size;
	if (cb->end == cb->start)
		cb->start = (cb->start + 1) % cb->size; 
}
 
void cbRead(CircularBuffer *cb, PmEvent *elem) 
{
	*elem = cb->elems[cb->start];
	cb->start = (cb->start + 1) % cb->size;
}
 




