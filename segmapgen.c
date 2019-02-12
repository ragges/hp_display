
/*
 * hp_display - program for Arduino for replacing the display on some
 * discontinued HP/Agilent/Keysight instruments.
 * Copyright (C) 2019  Ragnar Sundblad
 * 
 * This file is part of the hp_display program.
 * 
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.

 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.

 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#include <Arduino.h>
#include <stdint.h>
#include <stdlib.h>
#include "hp_msg_parse.h"

uint16_t seg_n = 47;
uint16_t seg_codes[] = {
0xc48c, 
0x2040, 
0xc08b, 
0x4489, 
0x040f, 
0x4487, 
0xc487, 
0x0488, 
0xc48f, 
0x448f, 
0x2043, 
0x0003, 
0x64c9, 
0x9014, 
0xc40b, 
0xa403, 
0x0000, 
0xfcff, 
0x1414, 
0x0810, 
0x1020, 
0x3873, 
0x1010, 
0x4003, 
0x208d, 
0x848f, 
0xc084, 
0x64c8, 
0xc087, 
0x8087, 
0xc485, 
0x840f, 
0x60c0, 
0x8816, 
0xc004, 
0x843c, 
0x8c2c, 
0x808f, 
0xcc8c, 
0x888f, 
0x44a1, 
0x20c0, 
0xc40c, 
0x9c0c, 
0x1830, 
0x2030, 
0x5090, 
};
uint8_t seg_mapped_chars[] = {
'0', 
'1', 
'2', 
'3', 
'4', 
'5', 
'6', 
'7', 
'8', 
'9', 
'+', 
'-', 
'B', 
'V', 
'd', 
'm', 
' ', 
'#', 
'%', 
'(', 
')', 
'*', 
'/', 
'=', 
'?', 
'A', 
'C', 
'D', 
'E', 
'F', 
'G', 
'H', 
'I', 
'K', 
'L', 
'M', 
'N', 
'P', 
'Q', 
'R', 
'S', 
'T', 
'U', 
'W', 
'X', 
'Y', 
'Z', 
};
