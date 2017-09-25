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

#include "cape_eeprom.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <ctype.h>
#include <time.h>
#include <fstream>
#include <sstream>

#if defined(_WIN32)
 
typedef __int8              int8_t;
typedef __int16             int16_t;
typedef __int32             int32_t;
typedef __int64             int64_t;
typedef unsigned __int8     uint8_t;
typedef unsigned __int16    uint16_t;
typedef unsigned __int32    uint32_t;
typedef unsigned __int64    uint64_t;
 
#endif

#define PIN_UNUSED			(0x0000 << 15)
#define PIN_USED			(0x0001 << 15)
#define PINDIR_INPUT		(0x0001 << 13)
#define PINDIR_OUTPUT		(0x0002 << 13)
#define PINDIR_BDIR			(0x0003 << 13)
#define PINSLEW_SLOW		(0x0001 << 6)
#define PINSLEW_FAST		(0x0000 << 6)
#define PINRX_DISABLE		(0x0000 << 5)
#define PINRX_ENABLE		(0x0001 << 5)
#define PINPULL_DOWN		(0x0000 << 4)
#define PINPULL_UP			(0x0001 << 4)
#define PINPULL_DISABLE		(0x0001 << 3)
#define PINPULL_ENABLE		(0x0000 << 3)

static const uint8_t bb_pins[][2] = {
	{9,22}, {9,21}, {9,18}, {9,17}, {9,42}, {8,35}, {8,33}, {8,31}, {8,32}, {9,19},
	{9,20}, {9,26}, {9,24}, {9,41}, {8,19}, {8,13}, {8,14}, {8,17}, {9,11}, {9,13},
	{8,25}, {8,24}, {8,5}, {8,6}, {8,23}, {8,22}, {8,3}, {8,4},{8,12},{8,11},
	{8,16},{8,15},{9,15},{9,23},{9,14},{9,16},{9,12},{8,26},{8,21},{8,20},
	{8,18},{8,7},{8,9},{8,10},{8,8},{8,45},{8,46},{8,43},{8,44},{8,41},
	{8,42},{8,39},{8,40},{8,37},{8,38},{8,36},{8,34},{8,27},{8,29},{8,28},
	{8,30},{9,29},{9,30},{9,28},{9,27},{9,31},{9,25},{9,39},{9,40},{9,37},
	{9,38},{9,33},{9,36},{9,35}
};

static int8_t pin_order[128];
			
template <typename T>
std::string NumberToString ( T Number )
{
	char snum[20];
	sprintf(snum, "%d", Number);
	return std::string(snum);
}
  
int GetWeek
	(
	struct tm* date
	)
{
	if (NULL == date)
	{
		return 0; // or -1 or throw exception
	}
	if (::mktime(date) < 0) // Make sure _USE_32BIT_TIME_T is NOT defined.
	{ 
		return 0; // or -1 or throw exception
	}
	// The basic calculation:
	// {Day of Year (1 to 366) + 10 - Day of Week (Mon = 1 to Sun = 7)} / 7
	int monToSun = (date->tm_wday == 0) ? 7 : date->tm_wday; // Adjust zero indexed week day
	int week = ((date->tm_yday + 11 - monToSun) / 7); // Add 11 because yday is 0 to 365.
 
	// Now deal with special cases:
	// A) If calculated week is zero, then it is part of the last week of the previous year.
	if (week == 0)
	{
		// We need to find out if there are 53 weeks in previous year.
		// Unfortunately to do so we have to call mktime again to get the information we require.
		// Here we can use a slight cheat - reuse this function!
		// (This won't end up in a loop, because there's no way week will be zero again with these values).
		tm lastDay = { 0 };
		lastDay.tm_mday = 31;
		lastDay.tm_mon = 11;
		lastDay.tm_year = date->tm_year - 1;
		// We set time to sometime during the day (midday seems to make sense)
		// so that we don't get problems with daylight saving time.
		lastDay.tm_hour = 12;
		week = GetWeek(&lastDay);
	}
	// B) If calculated week is 53, then we need to determine if there really are 53 weeks in current year
	//    or if this is actually week one of the next year.
	else if (week == 53)
	{
		// We need to find out if there really are 53 weeks in this year,
		// There must be 53 weeks in the year if:
		// a) it ends on Thurs (year also starts on Thurs, or Wed on leap year).
		// b) it ends on Friday and starts on Thurs (a leap year).
		// In order not to call mktime again, we can work this out from what we already know!
		int lastDay = date->tm_wday + 31 - date->tm_mday;
		if (lastDay == 5) // Last day of the year is Friday
		{
			// How many days in the year?
			int daysInYear = date->tm_yday + 32 - date->tm_mday; // add 32 because yday is 0 to 365
			if (daysInYear < 366)
			{
				// If 365 days in year, then the year started on Friday
				// so there are only 52 weeks, and this is week one of next year.
				week = 1;
			}
		}
		else if (lastDay != 4) // Last day is NOT Thursday
		{
			// This must be the first week of next year
			week = 1;
		}
		// Otherwise we really have 53 weeks!
	}
	return week;
}

int CapeEeprom::Write(const char *fname)
{
  FILE *f;

  if ((f = fopen (fname, "wb")) == NULL)
    {
      fprintf (stderr, "Cannot open file: %s\n", fname);
      exit (1);
    }
  fwrite (this, sizeof(CapeEeprom), 1, f);
  fclose (f);

  return 0;
}

int CapeEeprom::Print()
{
	printf ("#####################################################\n");
	printf ("Cape Name         : %.32s\n", _GetAsciiParam(bname, 32).c_str());
	printf ("Cape Version      : %s\n", GetVersion().c_str());
	printf ("Cape Manufacturer : %.16s\n", _GetAsciiParam(manufacturer, 16).c_str());
	printf ("Part Number       : %.16s\n", GetPartNumber().c_str());
	printf ("Serial Number     : %.12s\n", _GetAsciiParam(serial, 12).c_str()); 
	printf ("Pins Used         : %d\n", _ReadUint16BE(n_pins));
	printf ("VDD_3V3B Current  : %d mA\n", _ReadUint16BE(vdd_3v3));
	printf ("VDD_5V Current    : %d mA\n", _ReadUint16BE(vdd_5v));
	printf ("SYS_5V Current    : %d mA\n", _ReadUint16BE(sys_5v));
	printf ("Supplied Current  : %d mA\n", _ReadUint16BE(dc));
	printf ("#####################################################\n");
	printf ("Cape pins: \n");
	//printf("%-7s%s  %-7s%-8s%-11s%s\n\n", "PIN", "MODE", "SLEW", "DIRECTION", "PULL", "RX" );
	for (int i = 0; i < 74; i++) {
		uint16_t pinconfig = _ReadUint16BE(&pins[i*2]);
		if (pinconfig & PIN_USED) {
			std::string pin = "P" + NumberToString(bb_pins[i][0]) +"_" + NumberToString(bb_pins[i][1]);
			int mode = pinconfig & 0x07;
			std::string slew = pinconfig & PINSLEW_SLOW ? "SLOW" : "FAST";
			std::string dir;
			switch (pinconfig & (0x0003<<13)) {
				case PINDIR_BDIR: dir = "BDIR"; break;
				case PINDIR_OUTPUT: dir = "OUTPUT"; break;
				default: dir = "INPUT";
			}
			std::string pull = pinconfig & PINPULL_UP ? "PULL_UP" : "PULL_DOWN";
			std::string rx = pinconfig & PINRX_ENABLE ? "RX_ENABLE" : "RX_DISABLE";
		    printf("%-7s%d  %-7s%-8s%-11s%s\n", pin.c_str(), mode, slew.c_str(), dir.c_str(), pull.c_str(), rx.c_str());
	  }
  }
  printf ("\n");
  return 0;
}

std::string CapeEeprom::_GetAsciiParam(char *param, int lenth) {
	static char temp[50];
	strncpy(temp, param, lenth);
	temp[lenth] = '\0';
	return temp;
}

std::string CapeEeprom::GetBoardName() {
	return _GetAsciiParam(bname, 32);
}

std::string CapeEeprom::GetPartNumber() {
	return _GetAsciiParam(part_number, 16);
}

std::string CapeEeprom::GetVersion() {
	return _GetAsciiParam(version, 4);
}

std::string CapeEeprom::GetBoardNumber() {
	return _GetAsciiParam(serial+8, 4);
}

void CapeEeprom::SetBoardNumber(unsigned int n) {
	char temp[10];
	sprintf(temp, "%04d", n);
	memcpy(serial+8, temp, 4);
}

std::string CapeEeprom::GetSerialNumber() {
	return _GetAsciiParam(serial, 12);
}

int CapeEeprom::Dump()
{
	int            i,j;
	char           c;
	unsigned char *p = (unsigned char*) this;
	printf ("");

	for (i = 0; i < sizeof(CapeEeprom); i+=16)
	{
		if (i % 256 == 0)
			printf ("     00 01 02 03 04 05 06 07 - 08 09 0a 0b 0c 0d 0e 0f\n");
			printf ("%04x ", i);
		for (j = 0; j < 16; j++)
		{
			if ((i+j)<sizeof(CapeEeprom)) {
				printf ("%02x ", (int)*(p + i + j));
				if (j == 7) printf ( "- ");
			} else {
				printf ("   ");
				if (j == 7) printf ( "  ");
			}
		}
		printf (" | ");
		for (j = 0; j < 16 &&(i+j)<sizeof(CapeEeprom); j++)
		{
			c = *(p + i + j);
			printf ("%c", c < 32 || c > 127 ? '.' : c);
			if (j == 7) printf ( " ");
		}
		printf ("\n");
	}
	printf ("\n");
	return 0;
}

int CapeEeprom::_WriteUint16BE(unsigned char *buffer, int value)
{
	buffer[0] = value >> 8;
	buffer[1] = value;
	return 0;
}

int CapeEeprom::_ReadUint16BE(unsigned char *buffer)
{
	int value = buffer[0] << 8;
	value |= buffer[1];
	return value;
}

int CapeEeprom::_ParseLineData(char* cmd, char* c, SerialNumber &serialNumber) {
	int val;
	uint32_t high1, high2;
	char pin[20], modes[20], pull[20], slew[20], rx[20], dir[20];
	uint8_t mode;
	uint16_t pinconfig;
	bool valid;
	int paramValue = 0;
	
	if (strcmp(cmd, "board_name")==0) {
		sscanf(c, "%100s \"%32[^\"]\"", cmd, bname);
	} else if (strcmp(cmd, "version")==0) {
		sscanf(c, "%100s \"%4[^\"]\"", cmd, version);
	} else if (strcmp(cmd, "manufacturer")==0) {
		sscanf(c, "%100s \"%16[^\"]\"", cmd, manufacturer);
	} else if (strcmp(cmd, "part_number")==0) {
		sscanf(c, "%100s \"%16[^\"]\"", cmd, part_number);
	} else if (strcmp(cmd, "number_of_pins")==0) {
		sscanf(c, "%100s %2d", cmd, &paramValue);
		_WriteUint16BE(n_pins, paramValue);
	} else if (strcmp(cmd, "assembly_code")==0) {
		sscanf(c, "%100s \"%16[^\"]\"", cmd, &serialNumber.asm_code);
	} else if (strcmp(cmd, "week_of_production")==0) {
		serialNumber.week_of_production = 0;
		sscanf(c, "%100s %2d", cmd, &serialNumber.week_of_production);
	} else if (strcmp(cmd, "year_of_production")==0) {
		serialNumber.year_of_production = -1;
		sscanf(c, "%100s %2d", cmd, &serialNumber.year_of_production);
	} else if (strcmp(cmd, "board_number")==0) {
		serialNumber.board_number = -1;
		sscanf(c, "%100s %4d", cmd, &serialNumber.board_number);
	} else if (strcmp(cmd, "vdd_3V3b_current")==0) {
		sscanf(c, "%100s %4d", cmd, &paramValue);
		_WriteUint16BE(vdd_3v3, paramValue);
	} else if (strcmp(cmd, "vdd_5v_current")==0) {
		sscanf(c, "%100s %4d", cmd, &paramValue);
		_WriteUint16BE(vdd_5v, paramValue); 
	} else if (strcmp(cmd, "sys_5v_current")==0) {
		sscanf(c, "%100s %4d", cmd, &paramValue);
		_WriteUint16BE(sys_5v, paramValue);
	} else if (strcmp(cmd, "dc_supplied")==0) {
		sscanf(c, "%100s %4d", cmd, &paramValue);
		_WriteUint16BE(dc, paramValue);
	} else if (strcmp(cmd, "pinconfig")==0) {
		#ifdef linux
		sscanf(c, "%20s %19s %d %19s %19s %19s %19s", cmd, pin, &mode, slew, dir, pull, rx);
		#else
		sscanf(c, "%50s %19s %19s %19s %19s %19s %19s", cmd, pin, modes, slew, dir, pull, rx);
		sscanf(modes, "%d", &mode);
		#endif
		int i = 0,j = 0;
		sscanf(pin, "%*[Pp]%d%*[_]%d", &i, &j);
		//printf("%s, %s, %d, %s, %s, %s, %s %d %d\n", cmd, pin, mode, slew, dir, pull, rx, i, j);
		
		int k = (i-8)*64+j;
		if (i<8 || i>9 || k > 128 || pin_order[k] == -1) {
			 printf("Error at pin config: pin not recognised for %s\n", pin);
			 return -1;
		} else {
			valid = true;
			pinconfig = PIN_UNUSED;
			
			if (mode <= 7) {
				pinconfig |= mode;
			} else {
				printf("Error at pin config: mode not recognised for %s\n", pin);
				pinconfig |= 7;
				valid=false;
			}
			
			if (strcmp(dir, "INPUT")==0) {
				pinconfig |= PINDIR_INPUT;
			} else if (strcmp(dir, "OUTPUT")==0) {
				pinconfig |= PINDIR_OUTPUT;
			} else if (strcmp(dir, "BDIR")==0) {
				pinconfig |= PINDIR_BDIR;
			} else {
				pinconfig |= PINDIR_INPUT;
				printf("Error at pin config: direction not recognised for %s\n", pin);
				valid=false;
			}
			
			if (strcmp(pull, "PULL_DOWN")==0) {
				pinconfig |= PINPULL_DOWN | PINPULL_ENABLE;
			} else if (strcmp(pull, "PULL_UP")==0) {
				pinconfig |= PINPULL_UP | PINPULL_ENABLE;
			} else if (strcmp(pull, "PULL_NONE")==0) {
				pinconfig |= PINPULL_DOWN | PINPULL_DISABLE;
			} else {
				pinconfig |= PINPULL_DOWN | PINPULL_ENABLE;
				printf("Error at pin config: pull type not recognised for %s\n", pin);
				valid=false;
			}
			
			if (strcmp(slew, "SLOW")==0) {
				pinconfig |= PINSLEW_SLOW;
			} else if (strcmp(slew, "FAST")==0) {
				pinconfig |= PINSLEW_FAST;
			} else {
				pinconfig |= PINSLEW_SLOW;
				printf("Error at pin config: slew rate not recognised for %s\n", pin);
				valid=false;
			}
			
			if (strcmp(rx, "RX_ENABLE")==0) {
				pinconfig |= PINRX_ENABLE;
			} else if (strcmp(rx, "RX_DISABLE")==0) {
				pinconfig |= PINRX_DISABLE;
			} else {
				pinconfig |= PINRX_DISABLE;
				printf("Error at pin config: rx %s, not recognised for %s\n", rx, pin);
				valid=false;
			}
			
			if (valid) {
				pinconfig |= PIN_USED;
				_WriteUint16BE((unsigned char*)(pins + pin_order[k] * (int)2), pinconfig);
			}
		}
	} 
}

CapeEeprom::CapeEeprom(const std::string inFile) {
	FILE * fp;
	char * line = NULL;
	char * c = NULL;
	size_t len = 0;
	int/*ssize_t*/ read;
	char *comment = NULL;
	int atomct = 2;
	int linect = 0;
	char * command = (char*) malloc (101);
	int i;
	
	memset(pin_order, -1, 128);
	for (int i = 0; i < 74; i++) {
		pin_order[(bb_pins[i][0]-8)*64+bb_pins[i][1]] = i;
	}
	
	memset(this, 0, sizeof(CapeEeprom));
	
	if (inFile.find(".txt") == std::string::npos) {
		// Read file as eeprom image
		std::ifstream eepFile(inFile.c_str(), std::ios::in | std::ios::binary);
		eepFile.read ((char*)this, sizeof(CapeEeprom));
		eepFile.close();
		return;
	}

	SerialNumber serialNumber = {
		"0000",
		-1,
		-1,
		-1
	};
	
	magic[0] = 0xAA;
	magic[1] = 0x55;
	magic[2] = 0x33;
	magic[3] = 0xEE;
	rev[0] = 'A';
	rev[1] = '1';
	
	std::ifstream settingsFile(inFile.c_str());
	if (!settingsFile) {
		printf("Error opening input file\n");
		return;
	}

	std::string sline;
	while ( std::getline (settingsFile, sline) )
    {
		char line[1000];
		memcpy(line, sline.c_str(), sline.length() + 1);
		linect++;
		c = line;
		
		for (i=0; i<sline.length(); i++) if (c[i]=='#') c[i]='\0';
		
		while (isspace(*c)) ++c;
		
		if (*c=='\0' || *c=='\n' || *c=='\r') {
			//empty line, do nothing
		} else if (isalnum (*c)) {
			sscanf(c, "%100s", command);
			
#ifdef DEBUG
			printf("Processing line %u: %s", linect, c);
			if ((*(c+strlen(c)-1))!='\n') printf("\n");
#endif
			
			_ParseLineData(command, c, serialNumber);
		} else printf("Can't parse line %u: %s", linect, c);
	}
	
	if ( serialNumber.week_of_production < 1 || serialNumber.year_of_production < 0 ) {
		time_t t = time(NULL);
		struct tm tm = *gmtime(&t);
		if ( serialNumber.week_of_production < 1 ) serialNumber.week_of_production = GetWeek(&tm);
		if ( serialNumber.year_of_production < 0 ) serialNumber.year_of_production = tm.tm_year - 100;
	}
	
	if (serialNumber.board_number < 0) {
		serialNumber.board_number = 0;
		#ifdef linux
		// if not defined in settings file generate random board number
		uint32_t sn;
		std::ifstream random_file("/dev/urandom", std::ios::in | std::ios::binary);
		if (random_file) {
			random_file.read((char*)(&sn), 4);
			random_file.close();
			serialNumber.board_number = sn % 10000;
		}
		#endif
	}

	char serialStr[13];
	sprintf(serialStr, "%02d%02d%4s%04d", serialNumber.week_of_production, serialNumber.year_of_production, serialNumber.asm_code, serialNumber.board_number);
	memcpy(serial, serialStr, 12);

    settingsFile.close();
}