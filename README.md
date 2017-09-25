# Cape_eeprom
EEPROM binary file generation tool for BeagleBone Cape
--------------------------------------------------------------

Description
-----------
Cape_eeprom Generator is a command line tool to easily generate EEPROM files for BeagleBone Capes. It accepts input settings file and generates output binary file to be written to Capes ID EEPROM.


Input Arguments:
------------------------------
eepcape [-pd] [-nboard number] [input file] [output file]<br>

[input file]         	: Input settings text file path<br>
[output file]        	: Output binary file path<br>
[-p]					: Print parsed data to screen<br>
[-d]					: Dump binary EEPROM data to screen<br>
[-nboard number]		: Board number, overrides board number specified in input file<br>


Usage examples:
------------------------------

Make EEPROM binary file with name constracted of board name and version:<br>
~/ ./eepcape settings.txt<br>

Make EEPROM binary file with custom name and board number 543:<br>
~/ ./eepcape  -p settings.txt eeprom.bin -n543 <br>

Print and dump parsed data from binary file to screen:<br>
~/ ./eepcape  -pd eeprom.bin<br>


Compilation:
-----------
Just type "make". You can cross-compile simply setting CXX environmental variable. Current version has no dependency.
