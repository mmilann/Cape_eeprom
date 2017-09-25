/* 
Cape_eeprom: BeagleBone Cape EEPROM Generator
Copyright (c) 2017 Milan Neskovic

This file is part of Cape_eeprom

Cape_eeprom is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

Cape_eeprom is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with Cape_eeprom.  If not, see <http://www.gnu.org/licenses/>.
*/

#ifndef CAPE_EEPROM_H
#define CAPE_EEPROM_H

#include <string>

//#define DEBUG	1 

class CapeEeprom
{
public:
	CapeEeprom(const std::string inFile);
	int Write(const char *fname);
	int Print();
	int Dump();
	std::string GetBoardName();
	std::string GetPartNumber();
	std::string GetVersion();
	std::string GetBoardNumber();
	void SetBoardNumber(unsigned int n);
	std::string GetSerialNumber();
private:
	struct SerialNumber {
		char asm_code[5];
		int week_of_production;
		int year_of_production;
		int board_number;
	};
	int _ParseLineData(char* cmd, char* c, SerialNumber &serial);
	int _WriteUint16BE(unsigned char *buffer, int value);
	int _ReadUint16BE(unsigned char *buffer);
	std::string _GetAsciiParam(char *param, int lenth);
	
	unsigned char magic[4];
	unsigned char rev[2];
	char   bname[32];
	char   version[4];
	char   manufacturer[16];
	char   part_number[16];
	unsigned char   n_pins[2];
	char   serial[12];
	unsigned char   pins[148];
	unsigned char   vdd_3v3[2];
	unsigned char   vdd_5v[2];
	unsigned char   sys_5v[2];
	unsigned char   dc[2];
};

#endif