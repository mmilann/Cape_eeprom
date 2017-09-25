# Cape_eeprom: BeagleBone Cape EEPROM Generator
# Copyright (c) 2017 Milan Neskovic
# This file is part of Cape_eeprom
#
# Cape_eeprom is free software: you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation, either version 3 of the License, or
# (at your option) any later version.
#
# Cape_eeprom is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with Cape_eeprom.  If not, see <http://www.gnu.org/licenses/>.
#

SRC=eepcape.cpp cape_eeprom.cpp
HEADERS=cape_eeprom.h
eepcape: ${SRC} ${HEADERS}
	${CXX} -g -o $@ ${SRC}

.phony:
clean:
	rm eepcape
