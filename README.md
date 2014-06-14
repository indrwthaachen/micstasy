MICSTASY
========

Library for controlling Micstasy Microphone Preamps through MIDI (using PortMidi)


DEPENDENCIES 
------------
* portmidi 
  * On Linux (Ubuntu, Debian...):  `apt-get install libportmidi-dev libportmidi0`
  * On Windows: download source from http://portmedia.sourceforge.net/portmidi
    and compile with VisualStudio
 
COMPILATION OF MATLAB INTERFACE
-------------------------------

 * Start Matlab
 * Navigate to micstasy/matlab folder
   * If you are running Linux type: `compile_linux`
   * If you are running Windows 
   copy portmidi.dll and portmidi.lib to micstasy/matlab folder 
   and type: `compile_windows`


USAGE OF MATLAB INTERFACE
-------------------------

Run 'help micstasy' from Matlab for further information


COMPILATION OF STANDALONE C LIBRARY (OPTIONAL)
----------------------------------------------

Linux:

         cmake .
         make

Windows: Use cMake GUI and compile with VisualStudio



