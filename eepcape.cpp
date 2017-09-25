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

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <ctype.h>
#include <unistd.h>
#include <fstream>

#include "cape_eeprom.h"

#define VERSION "1.0"

#define DEBUG

int main (int argc, char *argv[])
{
    bool print = false, dump = false, nOpt = false;
    int opt;
	unsigned int bn;

    while ((opt = getopt(argc, argv, "pdn:")) != -1) {
        switch (opt) {
        case 'p': print = true; break;
        case 'd': dump = true; break;
		case 'n': nOpt = (sscanf(optarg, "%04d \n", &bn) == 1); break;
        default:
            fprintf(stderr, "Usage: %s [-pd] [-nboard number] [input file] [output file]\n", argv[0]);
            exit(EXIT_FAILURE);
        }
    }
	
	int fnArgCount = 0, i = 1;
	char *fnArg[2];
	while (fnArgCount < 2 && i < argc) {
		if (argv[i][0] != '-') {
			fnArg[fnArgCount] = argv[i];
			fnArgCount ++;
		}
		i++;
	}
	if (!fnArgCount) {
        fprintf(stderr, "Usage: %s [-pd] [-nboard number] [input file] [output file]\n", argv[0]);
        exit(EXIT_FAILURE);
	}
	
	fprintf (stderr, 
		"\n******************************************************************************\n"
		"BeagleBone Cape EEPROM Generator Ver:" VERSION "\n"
		"(c) 2017, Milan Neskovic\n"
		"License GPLv3+: GNU GPL version 3 or later <http://gnu.org/licenses/gpl.html>\n"
		"This is free software: you are free to change and redistribute it.\n"
		"There is NO WARRANTY, to the extent permitted by law.\n"
		"******************************************************************************\n\n");

	if (!std::ifstream(fnArg[0]).good()) {
		fprintf(stderr, "ERROR: Specified input settings file does not exist.\n");
        exit(EXIT_FAILURE);
	}
	
	// Load EEPROM data from file
	CapeEeprom cape(fnArg[0]);
	
	if (nOpt) cape.SetBoardNumber(bn);

	// If settings file entered as input argument,
	// write compiled binary EEPROM data to output file
	if (std::string(fnArg[0]).find(".txt") != std::string::npos) {
		if (fnArgCount > 1) {
			// Use input argument file name for output file if specified
			cape.Write(fnArg[1]);
		} else {
			// If not specified make output file name from settings data
			if (nOpt)
				// Make output file name of board name plus version plus board number if specified
				cape.Write((cape.GetPartNumber()+"-"+cape.GetVersion()+"-"+cape.GetBoardNumber()+".eep").c_str());
			else 
				// Make output file name of board name plus version 
				cape.Write((cape.GetPartNumber()+"-"+cape.GetVersion()+".eep").c_str());
		}
	}

	// Print parsed data to screen
	if (print) cape.Print();

	// Dump parsed data to screen
	if (dump) cape.Dump(); 
}
