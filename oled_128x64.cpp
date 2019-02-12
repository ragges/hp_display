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

/*
 * #defines:
 * OLED_128X64 - enable in hp_display_config.h to enable OLED screen
 * Default: 128x64 OLED with SSD1306 controller on i2c, address 0x3d
 * OLED_128X64_SSD1309_SW_SPI - instead a SSD1309 controller on SPI (software driven, bit banged)
 * OLED_SPEEDUP_TEST - test with only transferring the rows needing update to the oled
 * OLED_MEASURE_SPEED - lots of debug printing of time measurements in the drawing loop
 */ 

/*
 * Wiring, i2c:
 *   +-- i2c OLED display
 *   |      +-- Arduino Pro Micro
 *   |      |      +-- Arduino Nano v3.0
 *   |      |      |
 *  SCL   SCL/3  SCL/A5   # i2c clock
 *  SDA   SDA/2  SDA/A4   # i2c data
 *  VCC    VCC    5V
 *  GND    GND    GND
 *
 * Wiring, SPI (software driven):
 *   +-- SPI OLED display
 *   |      +-- Arduino Pro Micro
 *   |      |      +-- Arduino Nano v3.0
 *   |      |      |
 *  CLK   A3/21  A1/D15  # SPI clock (SCK, SCLK)
 *  SDA   A2/20  A2/D16  # SPI data (MOSI, DATA, SDA, SDIN)
 *  RES     2    A3/D17  # reset (RST)
 *  DC    A1/19  A4/D18  # data/command
 *  CS    A0/18  A5/D19  # chip select (/SS)
 *  VCC    VCC    5V
 *  GND    GND    GND
 */ 

#include <Arduino.h>
#include "hp_display_config.h" // include this before the other local files

#ifdef OLED_128X64

#define USE_MOD_FONT
#define OLED_SPEEDUP_TEST
//#define OLED_MEASURE_SPEED // XXX

#include <U8g2lib.h>

//#include <SPI.h>
#include <Wire.h>
//#include "hp_display_spi.h"
#include "hp_msg_parse.h"
#include "oled_128x64.h"

#ifdef USE_MOD_FONT
#include "u8g2_font_helvB10_mod_tf.h"
#endif

#define DISPLAY_WIDTH 128
#define DISPLAY_HEIGHT 64

#ifndef OLED_I2C_ADDR
#define OLED_I2C_ADDR 0x3d
#endif


#ifdef OLED_128X64_SSD1309_SW_SPI
#ifndef ARDUINO_NANO
// Pro Micro
const int clock=21, data=20, cs=18, dc=19, reset=2;
#else
// Nano
const int clock=15, data=16, cs=19, dc=18, reset=17;
#endif
#endif



uint8_t find_line_break(U8G2 *u8g2, char *str, uint8_t screen_width, uint8_t search_space);


#ifdef OLED_SPEEDUP_TEST
// Not sure what will happen if called with update_tile_rows = 0 // XXX
void u8g2_test_FirstPage(u8g2_t *u8g2, uint8_t update_tile_rows) {
  u8g2_FirstPage(u8g2);

  uint8_t disp_tile_height = u8g2_GetU8x8(u8g2)->display_info->tile_height;
  uint8_t last_buf_tile_row = disp_tile_height - u8g2->tile_buf_height;
  // find first tile row that needs to be updated
  uint8_t row;
  for (uint8_t i = 0; i < 8; i++) {
    if (update_tile_rows & 0x01) {
      row = i;
      break;
    }
    update_tile_rows >>= 1;
  }
  if (row > last_buf_tile_row) {
    row = last_buf_tile_row;
  }
  u8g2_SetBufferCurrTileRow(u8g2, row);
}

uint8_t u8g2_test_NextPage(u8g2_t *u8g2, uint8_t tile_rows) {
  uint8_t disp_tile_height = u8g2_GetU8x8(u8g2)->display_info->tile_height;
  uint8_t tile_buf_height = u8g2->tile_buf_height;
  uint8_t last_buf_tile_row = disp_tile_height - tile_buf_height + 1;
  if (!u8g2_NextPage(u8g2))
    return 0;
  // find next tile row that needs to be updated
  uint8_t row = 255;
  for (uint8_t i = u8g2->tile_curr_row; i < 8; i++) {
    if ((tile_rows >> i) & 0x01) {
      row = i;
      break;
    }
  }
  if (row > last_buf_tile_row) {
    return 0;
  }
  u8g2_SetBufferCurrTileRow(u8g2, row);
  return 1;
}

#ifdef OLED_128X64_SSD1309_SW_SPI
#define U8G2_BASE_CLASS U8G2_SSD1309_128X64_NONAME0_1_4W_SW_SPI
#else
#define U8G2_BASE_CLASS U8G2_SSD1306_128X64_NONAME_1_HW_I2C
#endif

class u8g2_test : public U8G2_BASE_CLASS {
  uint8_t _update_tile_rows;
public: 
#ifdef OLED_128X64_SSD1309_SW_SPI
  u8g2_test(const u8g2_cb_t *rotation, uint8_t clock, uint8_t data, uint8_t cs,
	    uint8_t dc, uint8_t reset = U8X8_PIN_NONE):
    U8G2_BASE_CLASS(rotation, clock, data, cs, dc, reset) { }
#else
  u8g2_test(const u8g2_cb_t *rotation, uint8_t reset = U8X8_PIN_NONE,
	    uint8_t clock = U8X8_PIN_NONE, uint8_t data = U8X8_PIN_NONE):
    U8G2_BASE_CLASS(rotation, reset, clock, data) { }
#endif
  void firstPage(uint8_t tile_rows = 0xff) {
    _update_tile_rows = tile_rows;
    u8g2_test_FirstPage(&u8g2, _update_tile_rows);
  }
  uint8_t nextPage(void) {
    return u8g2_test_NextPage(&u8g2, _update_tile_rows);
  }
};

#ifdef OLED_128X64_SSD1309_SW_SPI
u8g2_test u8g2(U8G2_R0, clock, data, cs, dc, reset);
#else
u8g2_test u8g2(U8G2_R0);
#endif

#else // OLED_SPEEDUP_TEST

#ifdef OLED_128X64_SSD1309_SW_SPI
U8G2_SSD1309_128X64_NONAME0_1_4W_SW_SPI u8g2(U8G2_R0, clock, data, cs, dc, reset);
#else
U8G2_SSD1306_128X64_NONAME_1_HW_I2C u8g2(U8G2_R0);
#endif

#endif // OLED_SPEEDUP_TEST


u8g2_uint_t w_gate = 0; // width of the Gate label in pixels

#define ROW1_Y (2*8 - 2)
#define ROW2_Y (4*8 - 2)
#define ROW3_Y (6*8 - 2)
#define ROW4_Y (8*8 - 2)

void oled_128x64_setup() {
  u8g2.setI2CAddress(OLED_I2C_ADDR * 2);
  u8g2.begin();

#ifdef USE_MOD_FONT
  // u8g2_font_helvB10_mod_tf - modded version with NBSP (0xa0) same width as digits
  u8g2.setFont(u8g2_font_helvB10_mod_tf);
#else
  u8g2.setFont(u8g2_font_helvB10_tf);
#endif

#ifndef OLED_128X64_SSD1309_SW_SPI
  Wire.setClock(400000);
#endif

  w_gate = u8g2.getStrWidth(hp_display_units_gate[4]);

  u8g2.firstPage();
  do {
    u8g2.drawStr(0, ROW1_Y, "...");
  } while ( u8g2.nextPage() );
}

/*
 * Strategy:
 * Make four rows of text.
 * First row: Display digits/characters, including separators
 * Second row: End of row: Units
 * Thirs row: Labels
 * Fourth row: If needed, more labels; end of row: Gate
 */

void oled_128x64_update() {
  static uint8_t labels_split_point = 0;
  static uint8_t labels_split_point_2 = 0;
  uint8_t disp_change_local = disp_change;
#ifdef OLED_MEASURE_SPEED
  unsigned long t0 = 0, t1 = 0, t2 = 0, t3 = 0, t4 = 0, t5 = 0, t6 = 0, t7 = 0, t8 = 0;
#endif
#ifdef OLED_SPEEDUP_TEST
  uint8_t tile_rows = 0;
#endif

#ifdef OLED_SPEEDUP_TEST
  // test to have u8g2 only update the tile rows that we need
  if (disp_change_local & CHANGE_TEXT) {
    tile_rows |= 0x03;
  }
  if (disp_change_local & CHANGE_UNITS) {
    tile_rows |= 0x0c;
  }
  if (disp_change_local & CHANGE_LABELS) {
    tile_rows |= 0xf0;
  }
  if (disp_change_local & CHANGE_GATE) {
    tile_rows |= 0xc0;
  }

  if (tile_rows == 0) {
    return; // Nothing to update
  }
#else
  if (disp_change_local == 0) {
      return;
  }
  disp_change_local = 0xff; // always draw everything
#endif

#ifdef USE_MOD_FONT
  if (disp_change_local & CHANGE_TEXT) {
    // use our modded font to use spaces have same width as digits
    // we actually change in the buffer we got, ugly, but saves memory
    strchrrepl_a(disp_text_combined, ' ', '\xa0'); // replace space with our special non-breaking-space
  }
#endif

#ifdef OLED_MEASURE_SPEED
  t0 = millis();
#endif
#ifdef OLED_SPEEDUP_TEST
  u8g2.firstPage(tile_rows);
#else
  u8g2.firstPage();
#endif
#ifdef OLED_MEASURE_SPEED
  t1 = millis();
#endif
  do {
    if (disp_change_local & CHANGE_TEXT) {
      u8g2.drawStr(0, ROW1_Y, disp_text_combined);
    }
#ifdef OLED_MEASURE_SPEED
    t2 = millis();
#endif
    if (disp_change_local & CHANGE_UNITS) {
      // replace "us" with "[mu]s"
      char *duc = disp_units_combined;
      if (duc[0] == 'u' && duc[1] == 's') {
	duc = "\xb5s"; // micro (mu) seconds
      }
      u8g2_uint_t w = u8g2.getStrWidth(duc);
      u8g2.drawStr(DISPLAY_WIDTH - w, ROW2_Y, duc);
    }
#ifdef OLED_MEASURE_SPEED
    t3 = millis();
#endif
    if (disp_change_local & CHANGE_LABELS) {
      if (disp_labels_combined_len < 30 &&  // if the string is very long, the width may wrap on 8 bits
	  u8g2.getStrWidth(disp_labels_combined) <= DISPLAY_WIDTH) {
	// labels fit on one row
	u8g2.drawStr(0, ROW3_Y, disp_labels_combined);
	if (labels_split_point != 0) { // if previous labels were multi row
#ifdef OLED_SPEEDUP_TEST
	  tile_rows |= 0xc0; // need to also update ROW4 (last two tile rows)
#endif
	  disp_change_local |= CHANGE_GATE; // and tell GATE to update itself
	  labels_split_point = 0;
	}
      } else {
#ifdef OLED_MEASURE_SPEED
    t4 = millis();
#endif
	// need to split labels on two rows
#ifdef OLED_SPEEDUP_TEST
	tile_rows |= 0xc0; // need to also update ROW4 (last two tile rows)
#endif
	disp_change_local |= CHANGE_GATE; // and tell GATE to update itself
	labels_split_point = find_line_break(&u8g2, disp_labels_combined, DISPLAY_WIDTH, true);
	if (labels_split_point == 0) // did not find a space, break on any char
	  labels_split_point = find_line_break(&u8g2, disp_labels_combined, DISPLAY_WIDTH, false);
	//labels_split_point_2 = labels_split_point + find_line_break(&u8g2, disp_labels_combined + labels_split_point, DISPLAY_WIDTH, false); // take as much of line 2 as possible, ignore space breaking
	labels_split_point_2 = labels_split_point + 25; // actually finding the right char takes a lot of time.
	// print first line
	char saved = disp_labels_combined[labels_split_point];
#ifdef OLED_MEASURE_SPEED
	t5 = millis();
#endif
	disp_labels_combined[labels_split_point] = '\0'; // ugly, but saves having a copy
	u8g2.drawStr(0, ROW3_Y, disp_labels_combined);
	disp_labels_combined[labels_split_point] = saved;
      }
    }
#ifdef OLED_MEASURE_SPEED
    t6 = millis();
#endif
    if (disp_change_local & CHANGE_GATE) {
      uint8_t display_gate = disp_units_gate[4] == 0 ? false : true;
      if (labels_split_point) { // two row labels
	char saved = disp_labels_combined[labels_split_point_2];
	if(display_gate)
	  u8g2.setClipWindow(0, 0, DISPLAY_WIDTH - w_gate - 5, 0xFFFF);
	disp_labels_combined[labels_split_point_2] = '\0'; // ugly, but saves having a copy
	u8g2.drawStr(0, ROW4_Y, disp_labels_combined + labels_split_point);
	disp_labels_combined[labels_split_point_2] = saved;
	if(display_gate)
	  u8g2.setMaxClipWindow();
      }
      if(display_gate) { // Gate
	u8g2.drawStr(DISPLAY_WIDTH - w_gate, ROW4_Y, (char *) hp_display_units_gate[4]);
      }
    }
#ifdef OLED_MEASURE_SPEED
    t7 = millis();
#endif
  } while ( u8g2.nextPage() );
#ifdef OLED_MEASURE_SPEED
  t8 = millis();
#endif

#ifdef USE_MOD_FONT
  if (disp_change_local & CHANGE_TEXT) {
    strchrrepl_a(disp_text_combined, '\xa0', ' '); // undo what we did above
  }
#endif

#ifdef OLED_MEASURE_SPEED
  Serial.print(disp_text_combined);
  Serial.print('/');
  Serial.print(disp_units_combined);
  Serial.print('/');
  Serial.print(disp_labels_combined);
  Serial.println('<');
  delay(75);
  Serial.print(disp_change_local, 16);
  Serial.print(" 1:");
  Serial.print(t1 - t0);
  Serial.print(" 2:");
  Serial.print(t2 - t0);
  Serial.print(" 3:");
  Serial.print(t3 - t0);
  Serial.print(" 4:");
  Serial.print(t4 - t0);
  Serial.print(" 5:");
  Serial.print(t5 - t0);
  Serial.print(" 6:");
  Serial.print(t6 - t0);
  Serial.print(" 7:");
  Serial.print(t7 - t0);
  Serial.print(" 8:");
  Serial.println(t8 - t0);
  delay(75);
#endif
}

// WARNING - writes to string do avoit having to buffer
// find index in string to break line to fit on screen
uint8_t find_line_break(U8G2 *u8g2, char *str, uint8_t screen_width, uint8_t search_space) {
  uint8_t split_point = 0;
  for (uint8_t i = 0; str[i] != '\0'; i++) {
    if (search_space)
      for(; str[i] != '\0' && disp_labels_combined[i] != ' '; i++); // find next space
    char saved = str[i];
    str[i] = '\0'; // ugly, but saves having a copy
    u8g2_uint_t w = u8g2->getStrWidth(str);
    str[i] = saved; // restore
    if (w < DISPLAY_WIDTH) {
      split_point = i;
    } else {
      break;
    }
  }

  return split_point;
}

#endif // OLED_128X64
