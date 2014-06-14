/*==========================================================
 * micstasy.c - MATLAB Interface for Micstasy Microphone Preamp
 *
 * Provides an interface for controlling Micstasy Preamps
 *
 * The calling syntax is:
 *
 *		output = micstasy(cmd, ...)
 *
 * This is a MEX-file for MATLAB.
 *
 * Copyright (c) 2014, 
 * Gerrit Wyen <gerrit.wyen@rwth-aachen.de> 
 * Florian Heese <heese@ind.rwth-aachen.de>
 * Institute of Communication Systems and Data Processing
 * RWTH Aachen University, Germany
 *
 *========================================================*/


#include "mex.h"
#include "../micstasyc.h"
#include <string.h>



static struct micstasy *pMicstasy = NULL;

#define DEBUG 0


mxArray *createStruct(const char **fieldnames, int len)
{
	mxArray *mxary;
	mxary = mxCreateStructMatrix(1,1,len, fieldnames);
	return mxary;
}


void addDoubleToStruct(mxArray *structMatrix, int pos, double value)
{
	mxArray *doubleValue;

	doubleValue  = mxCreateDoubleMatrix(1,1, mxREAL);
	*mxGetPr(doubleValue) = value;
	mxSetFieldByNumber(structMatrix,0,pos, doubleValue);
}


/* The gateway function */
void mexFunction( int nlhs, mxArray *plhs[],  int nrhs, const mxArray *prhs[])
{
	int i, ret;
	double value;
	char *fctName;

	if(nrhs<1) {
		mexErrMsgIdAndTxt("micstasy:pre", "too few parameters");
	}

	if( !mxIsChar(prhs[0]) )
		mexErrMsgIdAndTxt("micstasy:pre", "first argument must be a string");

	fctName = mxArrayToString(prhs[0]);

	if(DEBUG) mexPrintf("calling method: %s\n", fctName);


	if(strcmp(fctName, "list_midiDevices") == 0) 
	{
		char *deviceList = micstasy_list_midiDevices();

		if(deviceList == NULL) mexErrMsgIdAndTxt("micstasy:list_midiDevices", micstasy_errorMessage());

		mexPrintf("%s", deviceList);
		free(deviceList);
	}
	else
	if(strcmp(fctName, "init") == 0) 
	{

		int midiDeviceIn, midiDeviceOut, bankNumber, deviceID;
		bankNumber = 0x7;
		deviceID = 0xF;

		if(nrhs<3) {
			mexErrMsgIdAndTxt("micstasy:init", "too few parameters");
		}

		if( !mxIsNumeric(prhs[1]) ) mexErrMsgIdAndTxt("micstasy:init", "first argument of 'init' must be an int");
		if( !mxIsNumeric(prhs[2]) ) mexErrMsgIdAndTxt("micstasy:init", "second argument of 'init' must be an int");

	
		midiDeviceIn =  (int) mxGetScalar( prhs[1] );
		midiDeviceOut =  (int) mxGetScalar( prhs[2] );


		if(nrhs == 5) {	/* optional parameters */
			if( !mxIsNumeric(prhs[3]) ) mexErrMsgIdAndTxt("micstasy:init", "third argument of 'init' must be an int (optional)");
			if( !mxIsNumeric(prhs[4]) ) mexErrMsgIdAndTxt("micstasy:init", "fourth argument of 'init' must be an int (optional)");
			bankNumber = (int)mxGetScalar( prhs[3]);
			deviceID = (int)mxGetScalar( prhs[4]);
		}

		if(pMicstasy != NULL)
			mexErrMsgIdAndTxt("micstasy:init", "micstasy already initialized");


		if(DEBUG) mexPrintf("calling micstasy_init(0x%x,0x%x,0x%x,0x%x)\n", midiDeviceIn, midiDeviceOut, bankNumber, deviceID);

		pMicstasy = micstasy_init(midiDeviceIn, midiDeviceOut, bankNumber, deviceID);
		/* struct micstasy *micstasy_init(int midiDeviceIn, int midiDeviceOut, int bankNumber, int deviceID) */

		if(pMicstasy == NULL) mexErrMsgIdAndTxt("micstasy:init", micstasy_errorMessage());


	}
	else
	{
		if(pMicstasy == NULL)
			mexErrMsgIdAndTxt("micstasy:pre", "not initialized, please run 'init' first");	


		if(strcmp(fctName, "get_levelMeterData") == 0) {
			struct micstasy_levelMeterData levelMeterData;
			double *dynamicData;

			if(nlhs!=1) mexErrMsgIdAndTxt("micstasy:get_levelMeterData", "one output required");

			ret = micstasy_get_levelMeterData(pMicstasy, &levelMeterData);

			if(ret == -1) mexErrMsgIdAndTxt("micstasy:get_levelMeterData", micstasy_errorMessage());


			dynamicData = mxMalloc(8 * sizeof(double));
			for (i = 0; i < 8; i++ ) {
				dynamicData[i] = levelMeterData.channel[i];
			}

			/* 0x0 mxArray; allocate memory dynamically */
			plhs[0] = mxCreateNumericMatrix(0, 0, mxDOUBLE_CLASS, mxREAL);

			/* put C array into mxArray, define dimensions */
			mxSetPr(plhs[0], dynamicData);
			mxSetM(plhs[0], 1);
			mxSetN(plhs[0], 8);


		}
		else
		if(strcmp(fctName, "set_gain") == 0) {

			if(nrhs<3) mexErrMsgIdAndTxt("micstasy:set_gain", "too few parameters");
			if( !mxIsNumeric(prhs[1]) ) mexErrMsgIdAndTxt("micstasy:set_gain", "second argument must be an int");
			if( !mxIsNumeric(prhs[2]) ) mexErrMsgIdAndTxt("micstasy:set_gain", "third argument must be an int");
			
			ret = micstasy_set_gain(pMicstasy, (int)mxGetScalar(prhs[1]), (double)mxGetScalar(prhs[2]));
			/*int micstasy_set_gain(struct micstasy *cMicstasy, int channel, double dbValue)*/

			if(ret == -1) mexErrMsgIdAndTxt("micstasy:set_gain", micstasy_errorMessage());
            
		}
		else
		if(strcmp(fctName, "set_gainCoarse") == 0) {

			if(nrhs<3) mexErrMsgIdAndTxt("micstasy:set_gainCoarse", "too few parameters");
			if( !mxIsNumeric(prhs[1]) ) mexErrMsgIdAndTxt("micstasy:set_gainCoarse", "second argument must be an int");
			if( !mxIsNumeric(prhs[2]) ) mexErrMsgIdAndTxt("micstasy:set_gainCoarse", "third argument must be an int");
			
			ret = micstasy_set_gainCoarse(pMicstasy, (int)mxGetScalar(prhs[1]), (int)mxGetScalar(prhs[2]));
			/*int micstasy_set_gainCoarse(struct micstasy *cMicstasy, int channel, int dbValue);*/

			if(ret == -1) mexErrMsgIdAndTxt("micstasy:set_gainCoarse", micstasy_errorMessage());
            
		}
		else
		if(strcmp(fctName, "get_gainCoarse") == 0) {
			if(nrhs<2) mexErrMsgIdAndTxt("micstasy:get_gainCoarse", "too few parameters");
			if( !mxIsNumeric(prhs[1]) ) mexErrMsgIdAndTxt("micstasy:set_gainCoarse", "second argument must be an int");

			if(nlhs<1) mexErrMsgIdAndTxt("micstasy:get_gainCoarse", "too few parameters on the left");
			plhs[0] = mxCreateDoubleMatrix(1, 1, mxREAL);

			ret = micstasy_get_gainCoarse(pMicstasy, (int)mxGetScalar(prhs[1])); 
			/*int micstasy_get_gainCoarse(struct micstasy *cMicstasy, int channel);*/
			if(ret == -1) mexErrMsgIdAndTxt("micstasy:get_gainCoarse", micstasy_errorMessage());
			
			*mxGetPr(plhs[0]) = ret;

		}
		else
		if(strcmp(fctName, "get_gain") == 0) {
			if(nrhs<2) mexErrMsgIdAndTxt("micstasy:get_gain", "too few parameters");
			if( !mxIsNumeric(prhs[1]) ) mexErrMsgIdAndTxt("micstasy:set_gain", "second argument must be an int");

			if(nlhs<1) mexErrMsgIdAndTxt("micstasy:get_gain", "too few parameters on the left");
			plhs[0] = mxCreateDoubleMatrix(1, 1, mxREAL);

			ret = micstasy_get_gain(pMicstasy, (int)mxGetScalar(prhs[1]), &value); 
			/*double micstasy_get_gain(struct micstasy *cMicstasy, int channel, double *dbValue)*/
			if(ret == -1) mexErrMsgIdAndTxt("micstasy:get_gain", micstasy_errorMessage());
			
			*mxGetPr(plhs[0]) = value;

		}
		else
		if(strcmp(fctName, "set_parameters") == 0) {

			if(nrhs<6) mexErrMsgIdAndTxt("micstasy:set_parameters", "too few parameters");
			if( !mxIsNumeric(prhs[1]) ) mexErrMsgIdAndTxt("micstasy:set_parameters", "second argument must be an int");
			if( !mxIsNumeric(prhs[2]) ) mexErrMsgIdAndTxt("micstasy:set_parameters", "third argument must be an int");
			if( !mxIsNumeric(prhs[3]) ) mexErrMsgIdAndTxt("micstasy:set_parameters", "fourth argument must be an int");
			if( !mxIsNumeric(prhs[4]) ) mexErrMsgIdAndTxt("micstasy:set_parameters", "fifth argument must be an int");
			if( !mxIsNumeric(prhs[5]) ) mexErrMsgIdAndTxt("micstasy:set_parameters", "sixth argument must be an int");

			ret = micstasy_set_parameters(pMicstasy, (int)mxGetScalar(prhs[1]), (int)mxGetScalar(prhs[2]), (int)mxGetScalar(prhs[3]), (int)mxGetScalar(prhs[4]), (int)mxGetScalar(prhs[5]));
			/* int micstasy_set_parameters(struct micstasy *cMicstasy, int channel, boolean gainFine, boolean displayAutoDark, boolean autoSetLink, boolean digitalOutSelect) */
			if(ret == -1) mexErrMsgIdAndTxt("micstasy:set_parameters", micstasy_errorMessage());

		}
		else
		if(strcmp(fctName, "get_parameters") == 0) {

			const char *fieldnames[] = {"channel", "gainFine", "digitalOutSelect", "autoSetLink", "levelMeter", "displayAutoDark"};
			struct micstasy_parameters parameters;

			if(nrhs<2) mexErrMsgIdAndTxt("micstasy:get_parameters", "too few parameters");
			if( !mxIsNumeric(prhs[1]) ) mexErrMsgIdAndTxt("micstasy:set_parameters", "second argument must be an int");

			if(nlhs<1) mexErrMsgIdAndTxt("micstasy:get_parameters", "too few parameters on the left");

			ret = micstasy_get_parameters(pMicstasy, (int)mxGetScalar(prhs[1]), &parameters); 
			/* int micstasy_get_parameters(struct micstasy *cMicstasy, int channel, struct micstasy_parameters *parameters) */
			if(ret == -1) mexErrMsgIdAndTxt("micstasy:get_parameters", micstasy_errorMessage());


			plhs[0] = createStruct(fieldnames, 6);


			/* 0: channel */
			addDoubleToStruct(plhs[0], 0, parameters.channel);

			/* 1: gainFine */
			addDoubleToStruct(plhs[0], 1, parameters.gainFine);	

			/* 2: digitalOutSelect */
			addDoubleToStruct(plhs[0], 2, parameters.digitalOutSelect);	

			/* 3: autoSetLink */
			addDoubleToStruct(plhs[0], 3, parameters.autoSetLink);	

			/* 4: levelMeter */
			addDoubleToStruct(plhs[0], 4, parameters.levelMeter);	

			/* 5: displayAutoDark */
			addDoubleToStruct(plhs[0], 5, parameters.displayAutoDark);	



		}
		else
		if(strcmp(fctName, "get_settings") == 0) { 


			struct micstasy_settings settings;
			const char *fieldnames[] = {"channel", "input", "HiZ", "autoset", "loCut", "MS", "phase", "p48"};


			if(nrhs<2) mexErrMsgIdAndTxt("micstasy:get_settings", "too few parameters");
			if( !mxIsNumeric(prhs[1]) ) mexErrMsgIdAndTxt("micstasy:set_settings", "second argument must be an int");

			if(nlhs<1) mexErrMsgIdAndTxt("micstasy:get_settings", "too few parameters on the left");

			ret = micstasy_get_settings(pMicstasy, (int)mxGetScalar(prhs[1]), &settings);
			/* int micstasy_get_settings(struct micstasy *cMicstasy, int channel, struct micstasy_settings *settings) */
			if(ret == -1) mexErrMsgIdAndTxt("micstasy:get_settings", micstasy_errorMessage());

			plhs[0] = createStruct(fieldnames, 8);


			/* 0: channel */
			addDoubleToStruct(plhs[0], 0, settings.channel);

			/* 1: input */
			addDoubleToStruct(plhs[0], 1, settings.input);

			/* 2: HiZ */
			addDoubleToStruct(plhs[0], 2, settings.HiZ);

			/* 3: autoset */
			addDoubleToStruct(plhs[0], 3, settings.autoset);

			/* 4: loCut */
			addDoubleToStruct(plhs[0], 4, settings.loCut);
	
			/* 5: MS */
			addDoubleToStruct(plhs[0], 5, settings.MS);
;
			/* 6: phase */
			addDoubleToStruct(plhs[0], 6, settings.phase);

			/* 7: p48 */
			addDoubleToStruct(plhs[0], 7, settings.p48);


		}
		else
		if(strcmp(fctName, "set_settings") == 0) {

			if(nrhs<9) mexErrMsgIdAndTxt("micstasy:set_settings", "too few arguments");
			if( !mxIsNumeric(prhs[1]) ) mexErrMsgIdAndTxt("micstasy:set_settings", "second argument must be a boolean");
			if( !mxIsNumeric(prhs[2]) ) mexErrMsgIdAndTxt("micstasy:set_settings", "third argument must be a boolean");
			if( !mxIsNumeric(prhs[3]) ) mexErrMsgIdAndTxt("micstasy:set_settings", "fourth argument must be a boolean");
			if( !mxIsNumeric(prhs[4]) ) mexErrMsgIdAndTxt("micstasy:set_settings", "fifth argument must be a boolean");
			if( !mxIsNumeric(prhs[5]) ) mexErrMsgIdAndTxt("micstasy:set_settings", "sixth argument must be a boolean");
			if( !mxIsNumeric(prhs[6]) ) mexErrMsgIdAndTxt("micstasy:set_settings", "seventh argument must be a boolean");
			if( !mxIsNumeric(prhs[7]) ) mexErrMsgIdAndTxt("micstasy:set_settings", "eighth argument must be a boolean");
			if( !mxIsNumeric(prhs[8]) ) mexErrMsgIdAndTxt("micstasy:set_settings", "ninth argument must be a boolean");

			ret = micstasy_set_settings(pMicstasy, (int)mxGetScalar(prhs[1]), (int)mxGetScalar(prhs[2]), (int)mxGetScalar(prhs[3]), (int)mxGetScalar(prhs[4])
								, (int)mxGetScalar(prhs[5]), (int)mxGetScalar(prhs[6]), (int)mxGetScalar(prhs[7]), (int)mxGetScalar(prhs[8]));    

			/* channel, input, HiZ, autoset, loCut, MS, phase, p48  */

			if(ret == -1) mexErrMsgIdAndTxt("micstasy:set_settings", micstasy_errorMessage());
			
		}
		else
		if(strcmp(fctName, "get_setup") == 0) {

			struct micstasy_setup setup;
			const char *fieldnames[] = {"intFreq", "clockRange", "clockSelect", "analogOutput", "lockKeys", "peakHold", "followClock", "autosetLimit", "delayCompensation", "autoDevice"};

			if(nlhs<1) mexErrMsgIdAndTxt("micstasy:get_setup", "too few parameters on the left");

		
			ret = micstasy_get_setup(pMicstasy, &setup);
			/* int micstasy_get_setup(struct micstasy *cMicstasy, struct micstasy_setup *setup) */

			if(ret == -1) mexErrMsgIdAndTxt("micstasy:get_setup", micstasy_errorMessage());


			plhs[0] = createStruct(fieldnames, 10);


			/* 0: intFreq */
			addDoubleToStruct(plhs[0], 0, setup.intFreq);

			/* 1: clockRange */
			addDoubleToStruct(plhs[0], 1, setup.clockRange);

			/* 2: clockSelect */
			addDoubleToStruct(plhs[0], 2, setup.clockSelect);

			/* 3: analogOutput */
			addDoubleToStruct(plhs[0], 3, setup.analogOutput);

			/* 4: lockKeys */
			addDoubleToStruct(plhs[0], 4, setup.lockKeys);

			/* 5: peakHold */
			addDoubleToStruct(plhs[0], 5, setup.peakHold);

			/* 6: followClock */
			addDoubleToStruct(plhs[0], 6, setup.followClock);

			/* 7: autosetLimit */
			addDoubleToStruct(plhs[0], 7, setup.autosetLimit);

			/* 8: delayCompensation */
			addDoubleToStruct(plhs[0], 8, setup.delayCompensation);

			/* 9: autoDevice */
			addDoubleToStruct(plhs[0], 9, setup.autoDevice);

		}
		else
		if(strcmp(fctName, "setup") == 0) {

			if(nrhs<9) mexErrMsgIdAndTxt("micstasy:setup", "too few arguments");
			if( !mxIsNumeric(prhs[1]) ) mexErrMsgIdAndTxt("micstasy:setup", "second argument must be a boolean");
			if( !mxIsNumeric(prhs[2]) ) mexErrMsgIdAndTxt("micstasy:setup", "third argument must be a boolean");
			if( !mxIsNumeric(prhs[3]) ) mexErrMsgIdAndTxt("micstasy:setup", "fourth argument must be a boolean");
			if( !mxIsNumeric(prhs[4]) ) mexErrMsgIdAndTxt("micstasy:setup", "fifth argument must be a boolean");
			if( !mxIsNumeric(prhs[5]) ) mexErrMsgIdAndTxt("micstasy:setup", "sixth argument must be a boolean");
			if( !mxIsNumeric(prhs[6]) ) mexErrMsgIdAndTxt("micstasy:setup", "seventh argument must be a boolean");
			if( !mxIsNumeric(prhs[7]) ) mexErrMsgIdAndTxt("micstasy:setup", "eighth argument must be a boolean");
			if( !mxIsNumeric(prhs[8]) ) mexErrMsgIdAndTxt("micstasy:setup", "ninth argument must be a boolean");
			if( !mxIsNumeric(prhs[9]) ) mexErrMsgIdAndTxt("micstasy:setup", "10th argument must be a boolean");
			if( !mxIsNumeric(prhs[10]) ) mexErrMsgIdAndTxt("micstasy:setup", "11th argument must be a boolean");


			ret = micstasy_setup(pMicstasy, (int)mxGetScalar(prhs[1]), (int)mxGetScalar(prhs[2]), (int)mxGetScalar(prhs[3]), (int)mxGetScalar(prhs[4]), (int)mxGetScalar(prhs[5]), 
				(int)mxGetScalar(prhs[6]), (int)mxGetScalar(prhs[7]), (int)mxGetScalar(prhs[8]), (int)mxGetScalar(prhs[9]), (int)mxGetScalar(prhs[10]));
			/*int micstasy_setup(struct micstasy *cMicstasy, boolean intFreq, int clockRange, int clockSelect,
				 int analogOutput, boolean lockKeys, boolean peakHold, boolean followClock, int autosetLimit, boolean delayCompensation, boolean autoDevice);*/

			if(ret == -1) mexErrMsgIdAndTxt("micstasy:setup", micstasy_errorMessage());
		}
		else
		if(strcmp(fctName, "get_locksyncInfo") == 0) {
			struct micstasy_locksyncInfo locksyncInfo;
			const char *fieldnames[] = {"wcOut", "wckSync", "wckLock", "aesSync", "aesLock", "optionSync", "optionLock"};

			if(nlhs<1) mexErrMsgIdAndTxt("micstasy:get_locksyncInfo", "too few parameters on the left");

			ret = micstasy_get_locksyncInfo(pMicstasy, &locksyncInfo);
			/* int micstasy_get_locksyncInfo(struct micstasy *cMicstasy, struct micstasy_locksyncInfo *synclock) */
			if(ret == -1) mexErrMsgIdAndTxt("micstasy:get_locksyncInfo", micstasy_errorMessage());
	

			plhs[0] = createStruct(fieldnames, 7);

			/* 0: wcOut */
			addDoubleToStruct(plhs[0], 0, locksyncInfo.wcOut);

			/* 1: wckSync */
			addDoubleToStruct(plhs[0], 1, locksyncInfo.wckSync);

			/* 2: wckLock */
			addDoubleToStruct(plhs[0], 2, locksyncInfo.wckLock);

			/* 3: aesSync */
			addDoubleToStruct(plhs[0], 3, locksyncInfo.aesSync);

			/* 4: aesLock */
			addDoubleToStruct(plhs[0], 4, locksyncInfo.aesLock);

			/* 5: optionSync */
			addDoubleToStruct(plhs[0], 5, locksyncInfo.optionSync);

			/* 6: optionLock */
			addDoubleToStruct(plhs[0], 6, locksyncInfo.optionLock);


		}
		else
		if(strcmp(fctName, "set_bankdevID") == 0) {

			if(nrhs<3) mexErrMsgIdAndTxt("micstasy:set_bankdevID", "too few arguments");
			if( !mxIsNumeric(prhs[1]) ) mexErrMsgIdAndTxt("micstasy:set_bankdevID", "second argument must be numeric");
			if( !mxIsNumeric(prhs[2]) ) mexErrMsgIdAndTxt("micstasy:set_bankdevID", "third argument must be numeric");

			ret = micstasy_set_bankdevID(pMicstasy, (int)mxGetScalar(prhs[1]), (int)mxGetScalar(prhs[2]));
			/* int micstasy_set_bankdevID(struct micstasy *cMicstasy, int8_t bankID, int8_t devID) */

			if(ret == -1) mexErrMsgIdAndTxt("micstasy:set_bankdevID", micstasy_errorMessage());
	
		}
		else
		if(strcmp(fctName, "set_oscillator") == 0) {

			if(nrhs<2) mexErrMsgIdAndTxt("micstasy:set_oscillator", "too few arguments");
			if( !mxIsNumeric(prhs[1]) ) mexErrMsgIdAndTxt("micstasy:set_oscillator", "second argument must be numeric");

			ret = micstasy_set_oscillator(pMicstasy, (int)mxGetScalar(prhs[1]));
			/* int micstasy_set_oscillator(struct micstasy *cMicstasy, int channel) */

			if(ret == -1) mexErrMsgIdAndTxt("micstasy:set_oscillator", micstasy_errorMessage());
			
		}
		else
		if(strcmp(fctName, "memory_save") == 0) {

			if(nrhs<2) mexErrMsgIdAndTxt("micstasy:memory_save", "too few arguments");
			if( !mxIsNumeric(prhs[1]) ) mexErrMsgIdAndTxt("micstasy:memory_save", "second argument must be numeric");

			ret = micstasy_memory_save(pMicstasy, (int)mxGetScalar(prhs[1]));
			/* int micstasy_memory_save(struct micstasy *cMicstasy, int slot) */

			if(ret == -1) mexErrMsgIdAndTxt("micstasy:memory_save", micstasy_errorMessage());
			
		}
		else
		if(strcmp(fctName, "memory_recall") == 0) {

			if(nrhs<2) mexErrMsgIdAndTxt("micstasy:memory_recall", "too few arguments");
			if( !mxIsNumeric(prhs[1]) ) mexErrMsgIdAndTxt("micstasy:memory_recall", "second argument must be numeric");

			ret = micstasy_memory_recall(pMicstasy, (int)mxGetScalar(prhs[1]));
			/* int micstasy_memory_recall(struct micstasy *cMicstasy, int slot) */

			if(ret == -1) mexErrMsgIdAndTxt("micstasy:memory_recall", micstasy_errorMessage());
			
		}
		else
		if(strcmp(fctName, "store_state") == 0) {

			char *pathStr;

			if(nrhs<2) mexErrMsgIdAndTxt("micstasy:store_state", "too few arguments");
			if( !mxIsChar(prhs[1]) ) mexErrMsgIdAndTxt("micstasy:store_state", "second argument must be a string");

			pathStr = mxArrayToString(prhs[1]);
			ret = micstasy_store_state(pMicstasy, pathStr);
			/* int micstasy_store_state(struct micstasy *cMicstasy, char *filePath) */
			mxFree(pathStr);

			if(ret == -1) mexErrMsgIdAndTxt("micstasy:store_state", micstasy_errorMessage());
		}
		else
		if(strcmp(fctName, "restore_state") == 0) {

			char *pathStr;

			if(nrhs<2) mexErrMsgIdAndTxt("micstasy:restore_state", "too few arguments");
			if( !mxIsChar(prhs[1]) ) mexErrMsgIdAndTxt("micstasy:restore_state", "second argument must be a string");


			pathStr = mxArrayToString(prhs[1]);
			ret = micstasy_restore_state(pMicstasy, pathStr);
			/* int micstasy_restore_state(struct micstasy *cMicstasy, char *filePath) */
			mxFree(pathStr);

			if(ret == -1) mexErrMsgIdAndTxt("micstasy:restore_state", micstasy_errorMessage());
		}
		else
		if(strcmp(fctName, "close") == 0) {
			micstasy_close(pMicstasy);
			/* int micstasy_close(struct micstasy *cMicstasy) */
			pMicstasy = NULL;
		}
		else
		{
			mexErrMsgIdAndTxt("micstasy:pre", "operation not found, see 'help micstasy' for valid operations");
		}

		

	}

	mxFree(fctName);


}






