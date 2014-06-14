function output = micstasy(cmd,..)
%MICSTASY provides a Matlab interface for controlling Micstasy Microphone preamps through MIDI
%
% output = micstasy(cmd,..)
%
% Syntax:
%   initalization:
%  		micstasy('init', midiDeviceIn, midiDeviceOut, { bankNumber, deviceID } )
%   			- default: bankNumber=0x7, deviceID=0xF (broadcast)
%			- midiDeviceIn, midiDeviceOut are IDs of MIDI ports 
%				- can be looked up by calling:  
%					micstasy('list_midiDevices')
%   'get' operations:
%		array = micstasy('get_levelMeterData')
%		gain = micstasy('get_gain', channel)		
%		gain = micstasy('get_gainCoarse', channel)
%		parametersStruct = micstasy('get_parameters', channel)	
%		settingsStruct = micstasy('get_settings', channel)
%		setupStruct = micstasy('get_setup')	
%		locksyncInfoStruct = micstasy('get_locksyncInfo')
%
%   'set' operations:
%		micstasy('set_gain', channel, gain_dB)	
%		micstasy('set_gainCoarse', channel, gain_dB)	
%		micstasy('set_parameters', channel, gainFine, displayAutoDark, autoSetLink, digitalOutSelect)	
%		micstasy('set_settings', channel, input, HiZ, autoset, loCut, MS, phase, p48)
%		micstasy('set_bankdevID', bankID, devID)
%		micstasy('set_oscillator', channel)
%		micstasy('setup', intFreq, clockRange, clockSelect, analogOutput, lockKeys, peakHold, followClock, autosetLimit, delayCompensation, autoDevice)
%
%    store operations:
%		micstasy('memory_save', slot)
%		micstasy('memory_recall', slot)
%		micstasy('store_state', filePath)
%		micstasy('restore_state', filePath)
%
%   termination:
%		micstasy('close')
