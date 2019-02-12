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
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#include <Arduino.h>
#include <hd44780.h>                       // main hd44780 header
#include <hd44780ioClass/hd44780_pinIO.h> // Arduino pin i/o class header

#include "hp_display_config.h" // include this before the other local files

#ifdef LCD_20X4_HD44780

#include "hp_msg_parse.h"
#include "lcd_20x4_hd44780.h"

/*
 * 20x4 character display, HD44780 compatible controller, 4 bit
 * parallel mode, wiring:
 *   +-- HD44780 compatible 20x4 display
 *   |     +-- Arduino Pro Micro
 *   |     |     +-- Arduino Nano v3.0
 *  RS     8    D9
 *  R/W    1    D8
 *  E      9    D7
 *  DB4    4    D6
 *  DB5    5    D5
 *  DB6    6    D4
 *  DB7    7    D3
 */

#ifndef ARDUINO_NANO
// Pro Micro:
const int rs=8, rw=1, en=9, db4=4, db5=5, db6=6, db7=7;
#else
// Nano:
const int rs=9, rw=8, en=7, db4=6, db5=5, db6=4, db7=3;
#endif

hd44780_pinIO lcd(rs, rw, en, db4, db5, db6, db7);

// LCD geometry
const int LCD_COLS = 20;
const int LCD_ROWS = 4;

const int LCD_UNITS_FIELD_LEN = 5;
const int LCD_GATE_FIELD_LEN = 5;


void lcd_20x4_hd44780_setup() {
  int err = lcd.begin(LCD_COLS, LCD_ROWS);
  lcd.setCursor(LCD_COLS/2 - 2, 0);
  lcd.print("...");
}


void lcd_20x4_hd44780_update() {
  static char line[LCD_COLS+1];

  if (disp_change & (CHANGE_TEXT_COMB | CHANGE_UNITS_COMB)) {
    // check if we can fit text/numbers and units in one line
    if ((disp_text_combined_len + 1 + disp_units_combined_len) <= LCD_COLS) {
      uint8_t l;
      l = strlgcpy_a(line, disp_text_combined);
      l = strlgspacefilln_a(line, 1, l);
      l = strlgcat_a(line, disp_units_combined, l);
      l = strlgspacefill_a(line, l);
      lcd.setCursor(0, 0);
      lcd.print(line);
      // blank out line 2
      l = 0;
      l = strlgspacefill_a(line, l);
      lcd.setCursor(0, 1);
      lcd.print(line);
    } else {
      uint8_t l;
      l = strlgcpy_a(line, disp_text_combined);
      l = strlgspacefill_a(line, l);
      lcd.setCursor(0, 0);
      lcd.print(line);
      // line 2
      l = 0;
      if (disp_text_combined_len > 16) { // continuation of disp_text_combined
	l = strlgcat(line, disp_text_combined + 16, LCD_COLS - LCD_UNITS_FIELD_LEN, l);
      }
      // pad with space for rest of disp_text_combined and to right adjust units
      l = strlgspacefilln(line, LCD_COLS - l - disp_units_combined_len, LCD_COLS, l);
      // copy units
      l = strlgcat_a(line, disp_units_combined, l);
      lcd.setCursor(0, 1);
      lcd.print(line);
    }
  }

  if (disp_change & CHANGE_GATE) {
    // display Gate
    if(disp_units_gate[4] != 0) { // Gate
      lcd.setCursor(LCD_COLS - LCD_GATE_FIELD_LEN + 1, 3);
      lcd.print((char *) hp_display_units_gate[4]);
    } else {
      lcd.setCursor(LCD_COLS - LCD_GATE_FIELD_LEN, 3);
      lcd.print("     ");
    }
  }

  if (disp_change & CHANGE_LABELS_COMB) {
    // display labels - on one or two rows
    const uint8_t LCD_COLS_LR2 = LCD_COLS - LCD_GATE_FIELD_LEN;
    if ( disp_labels_combined_len <= LCD_COLS ) {
      uint8_t l;
      l = strlgcpy_a(line, disp_labels_combined);
      l = strlgspacefill_a(line, l);
      lcd.setCursor(0, 2);
      lcd.print(line);
      
      l = 0;
      l = strlgspacefill(line, LCD_COLS_LR2, l); // space fill from start of line
      lcd.setCursor(0, 3);
      lcd.print(line);
    } else {
      // need to wrap, find a space backwards up to 8 chars
      uint8_t index = LCD_COLS;
      for (uint8_t i = LCD_COLS; i > LCD_COLS - 8; i--) {
	if (disp_labels_combined[i] == ' ') {
	  index = i;
	  break;
	}
      }
      // copy first part of text
      uint8_t l;
      l = strlgcpy_a(line, disp_labels_combined);
      l = strlgspacefill_a(line, l);
      lcd.setCursor(0, 2);
      lcd.print(line);
      // copy second part of text
      if (disp_labels_combined[index] == ' ')
	index += 1;
      l = strlgcpy(line, disp_labels_combined + index, LCD_COLS_LR2);
      l = strlgspacefill(line, LCD_COLS_LR2, l);
      lcd.setCursor(0, 3);
      lcd.print(line);
    }
  }
}

#endif // LCD_20X4_HD44780
