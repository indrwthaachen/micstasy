/*
  Library for controlling Micstasy Microphone Preamps through MIDI (using PortMidi)

  Copyright (c) 2014, Gerrit Wyen <gerrit.wyen@rwth-aachen.de>, Florian Heese <heese@ind.rwth-aachen.de>
  Institute of Communication Systems and Data Processing
  RWTH Aachen University, Germany

*/

#ifndef MICSTASYC_H
	#define MICSTASYC_H

	#include <portmidi.h>
	#include <stdint.h>


	#define SYS_EX_HEADER (int8_t)0xF0
	#define MIDI_TEMP_MANUFACTRURER_ID_1 (int8_t)0x00  
	#define MIDI_TEMP_MANUFACTRURER_ID_2 (int8_t)0x20
	#define MIDI_TEMP_MANUFACTRURER_ID_3 (int8_t)0x0D

	#define MODEL_ID (int8_t)0x68
	#define EOX (int8_t)0xF7

	#define MESSAGETYPE_REQUEST_VALUE 		(int8_t)0x10
	#define MESSAGETYPE_REQUEST_LEVELMETER_DATA	(int8_t)0x11
	#define MESSAGETYPE_SET_VALUE			(int8_t)0x20
	#define MESSAGETYPE_RESPONSE_VALUE		(int8_t)0x30
	#define MESSAGETYPE_RESPONSE_LEVELMETER_DATA	(int8_t)0x31

	#define BANK_NUMBER_BROADCAST	(int8_t)0x7E

	#define is_real_time_msg(msg)   ((0xF0 & Pm_MessageStatus(msg)) == 0xF8)

	#define BUF_SIZE 200

	typedef int8_t boolean;


	typedef struct {
		int         size;  
		int         start;
		int         end;
		PmEvent   *elems; 
	} CircularBuffer;


	void cbInit(CircularBuffer *cb, int size);
	void cbFree(CircularBuffer *cb);
	int cbIsFull(CircularBuffer *cb);
	int cbIsEmpty(CircularBuffer *cb);
	void cbWrite(CircularBuffer *cb, PmEvent *elem); 
	void cbRead(CircularBuffer *cb, PmEvent *elem);


	struct micstasy {
		int8_t bankNumber;
		int8_t deviceID;
		PortMidiStream *portMidiStreamIn;
		PortMidiStream *portMidiStreamOut;
		CircularBuffer readBuffer;
	};

	struct micstasy_setup {
		boolean intFreq;
		int8_t clockRange;
		int8_t clockSelect;
		int8_t analogOutput;
		boolean lockKeys;
		boolean peakHold;
		boolean followClock;
		int8_t autosetLimit;
		boolean delayCompensation;
		boolean autoDevice;

	};

	struct micstasy_settings {
		int8_t channel;
		boolean input;
		boolean HiZ;
		boolean autoset;
		boolean loCut;
		boolean MS;
		boolean phase;
		boolean p48;
	};


	struct micstasy_parameters {
		int8_t channel;
		boolean gainFine;
		boolean digitalOutSelect;
		boolean autoSetLink;
		int8_t levelMeter;
		boolean displayAutoDark;
	};

	struct micstasy_locksyncInfo {
		boolean wcOut;
		boolean wckSync;
		boolean wckLock;
		boolean aesSync;
		boolean aesLock;
		boolean optionSync;
		boolean optionLock;
	};

	struct micstasy_levelMeterData {
		int channel[8];
	};


	struct micstasy *micstasy_init(int midiDeviceIn, int midiDeviceOut, int bankNumber, int deviceID);

	char *micstasy_list_midiDevices();
	int micstasy_get_levelMeterData(struct micstasy *cMicstasy, struct micstasy_levelMeterData *levelMeterData);
	int micstasy_set_gainCoarse(struct micstasy *cMicstasy, int channel, int dbValue);
	int micstasy_get_gainCoarse(struct micstasy *cMicstasy, int channel);
	double micstasy_get_gain(struct micstasy *cMicstasy, int channel, double *dbValue);
	int micstasy_get_parameters(struct micstasy *cMicstasy, int channel, struct micstasy_parameters *parameters);
	int micstasy_get_settings(struct micstasy *cMicstasy, int channel, struct micstasy_settings *settings);
	int micstasy_set_parameters(struct micstasy *cMicstasy, int channel, boolean gainFine, boolean displayAutoDark, boolean autoSetLink, boolean digitalOutSelect);
	int micstasy_set_settings(struct micstasy *cMicstasy, int channel,
			 boolean input, boolean HiZ, boolean autoset, boolean loCut, boolean MS, boolean phase, boolean p48);
	int micstasy_setup(struct micstasy *cMicstasy, boolean intFreq, int clockRange, int clockSelect,
			 int analogOutput, boolean lockKeys, boolean peakHold, boolean followClock, int autosetLimit, boolean delayCompensation, boolean autoDevice);
	int micstasy_get_setup(struct micstasy *cMicstasy, struct micstasy_setup *setup);
	int micstasy_get_locksyncInfo(struct micstasy *cMicstasy, struct micstasy_locksyncInfo *synclock);
	int micstasy_set_bankdevID(struct micstasy *cMicstasy, int8_t bankID, int8_t devID);
	int micstasy_set_oscillator(struct micstasy *cMicstasy, int channel);
	int micstasy_memory_save(struct micstasy *cMicstasy, int slot);
	int micstasy_memory_recall(struct micstasy *cMicstasy, int slot);
	int micstasy_store_state(struct micstasy *cMicstasy, char *filePath);
	int micstasy_restore_state(struct micstasy *cMicstasy, char *filePath);
	int micstasy_set_gain(struct micstasy *cMicstasy, int channel, double dbValue);
	int micstasy_close(struct micstasy *cMicstasy);
	char *micstasy_errorMessage(void);


	#ifdef _WIN32
		#include <windows.h>
	#else
		#include <unistd.h>
		#define Sleep(x) usleep((x)*1000)
	#endif

#endif
