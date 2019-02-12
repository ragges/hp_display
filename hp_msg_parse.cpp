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
#include "hp_display_config.h" // include this before the other local files
#include "hp_display_spi.h"
#include "hp_msg_parse.h"


/* exported variables, updated by update_disp() */
uint8_t disp_text[13]; // display text
uint8_t disp_separators[12]; // dot, comma, colon, semicolon, or null
uint8_t disp_labels[12]; // 0 or 1 if label should be displayed
uint8_t disp_highlights[12]; // 0 or 1 if character should be highlighted
uint8_t disp_units_gate[5]; // 0 or 1 if unit should be displayed
uint8_t disp_change; // Bitfield stating what fields changed
uint8_t disp_no_display_data; // Currently no display data from instrument
/* exported variables, updated by update_disp_combined() (after an update_disp()) */
char disp_text_combined[24];       // string built from disp_text and disp_separators
char disp_highlights_combined[24]; // highlights matching disp_text_combined
size_t disp_text_combined_len = 0; // length of string in disp_text_combined and flags in disp_highlights_combined
char disp_units_combined[6];       // Combination of the active units (excluding Gate!)
size_t disp_units_combined_len = 0;// length of string in disp_units_combined
char disp_labels_combined[64];     // Combination of the active labels, separated with space
size_t disp_labels_combined_len = 0;// length of string in disp_labels_combined

/* internal variables, updated by update_disp() */
uint8_t disp_text_prev[13] = {0,0,0,0,0,0,0,0,0,0,0,0,0};
uint8_t disp_separators_prev[12] = {0,0,0,0,0,0,0,0,0,0,0,0};
uint8_t disp_labels_prev[12] = {0,0,0,0,0,0,0,0,0,0,0,0};
uint8_t disp_highlights_prev[12] = {0,0,0,0,0,0,0,0,0,0,0,0};
uint8_t disp_units_gate_prev[5] = {0,0,0,0,0};


/* exported constants */
/* Text labels on display for HP 53131A/53132A/53181A/58503. */
const char* const hp_display_labels[16] = {"Period", "Freq", "+Wid", "-Wid", "Rise", "Fall", "Time", "Ch1",
                 "Ch2", "Ch3", "Limit", "ExtRef"};
const char* const hp_display_units_gate[5] = {"M", "Hz", "u", "s", "Gate"};
/* Text labels on display for HP 34401A. NOTE - NOT TESTED! */
/*
const char* const hp_display_labels_34401A[16] = {"*", "Adrs", "Rmt", "Man", "Trig", "Hold", "Mem", "Ratio",
                 "Math", "ERROR", "Rear", "Shift", "", "", "", ""};
// NOT TESTED/VERIFIED! Maybe "4W" is one label (4-wire)
const char* const hp_display_units_gate_34401A[5] = {"4", "W", "[Cont]", "[???]", "[Diode]"};
*/

/* interal debug variables */
uint8_t unknown_dp;
// unknown characters we have seen
int unk_n_chars = 0;
uint16_t unk_chars[4];


inline uint8_t myisalpha(uint8_t c) {
  return (c >= 'A' && c <= 'Z');
}


/* Update disp_* variables */
void update_disp(void) {
  for (uint8_t i = 0; i < 12; i++) {
    uint32_t m = __builtin_bswap32(hp_display_msg(i));
    //uint16_t gates = m >> 20;
    
    uint16_t segs14 = m & 0x0000fcff;
    disp_text[i] = map_seg14_code_x(segs14);
    uint8_t segs_dp = (m & 0x00070000) >> 16;
    if (i == 0) { 
      disp_units_gate[2] = (segs_dp & 0x01) ? 1 : 0; // "u"
      disp_units_gate[3] = (segs_dp & 0x02) ? 1 : 0; // "s"
      disp_units_gate[4] = (segs_dp & 0x04) ? 1 : 0; // "Gate"
      uint8_t segs_o = (m & 0x00000300) >> 8;
      disp_units_gate[0] = (segs_o & 0x01) ? 1 : 0; // "M"
      disp_units_gate[1] = (segs_o & 0x02) ? 1 : 0; // "Hz"
   } else {
      uint8_t c = '\0';
      if (segs_dp == 0x02) c = '.';
      else if (segs_dp == 0x03) c = ':';
      else if (segs_dp == 0x06) c = ',';
      else if (segs_dp == 0x07) c = ';';
      else unknown_dp = segs_dp;
      disp_separators[i] = c;
    }
    
    uint8_t segs_label = (m & 0x00080000) >> 16;
    disp_labels[i] = segs_label ? 1 : 0;
  }

  // update highlighting
  for (uint8_t i = 0; i < 12; i++) {
    disp_highlights[i] = 0;
  }
  for (uint8_t i = 12; i < 16; i++) {
    uint32_t msg = hp_display_msg(i);
    if (msg == 0x00000080) // quick shortcut - little endian
      continue;
    uint8_t gateno = hp_display_spi_msg2gateno((uint8_t *) &msg);
    uint32_t m = __builtin_bswap32(hp_display_msg(i)); // big endian
    if (m & 0x000FFFFF)
      disp_highlights[gateno] = 1;
  }

  // Zero and O (the letter) are ambiguous, try to decide using character before it
  // Can not in general use character to the right to the decide, since that can be a unit ("V", "dB", ...)
  for (int8_t i = 11; i >= 0; i--) {
    if (disp_text[i] == '0') {
      if ( ( (i < 11 && myisalpha(disp_text[i+1])) || // not leftmost and char to the left is alpha
	     (i == 11 && myisalpha(disp_text[i-1]))) || // leftmost and char to the right is alpha
	   ( i < 10 && disp_text[i+1] == ' ' && // space to the left, and
	     (disp_text[i-1] == 'N') || // "N" to the right -> "ON"
	     (i > 1 && disp_text[i-1] == 'F' && disp_text[i-2] == 'F' ))) { // "FF" to the right -> OFF
	disp_text[i] = 'O';
      }
    }
  }

  uint8_t ch = 0;
  if (memlgcpycmp_a(disp_text_prev, disp_text) | // use bitwise or instead of logical or to always evaluate all
      memlgcpycmp_a(disp_separators_prev, disp_separators) |
      memlgcpycmp_a(disp_highlights_prev, disp_highlights))
    ch |= CHANGE_TEXT;
  if (memlgcpycmp_a(disp_labels_prev, disp_labels))
    ch |= CHANGE_LABELS;
  if (memlgcpycmp(disp_units_gate_prev, disp_units_gate, disp_units_n))
    ch |= CHANGE_UNITS;
  if (memlgcpycmp(disp_units_gate_prev+disp_units_n, disp_units_gate+disp_units_n, 1)) {
    ch |= CHANGE_GATE;
  }

  // handle no new data
  uint8_t new_no_disp = new_no_disp = hp_display_spi_timeout();
  if (disp_no_display_data != new_no_disp) {
    disp_no_display_data = new_no_disp;
    ch |= CHANGE_ALL;
  }
  if (new_no_disp) {
    const char no_disp_str[] PROGMEM = "(NO DISPLAY)";
    for (uint8_t i = 0; i < 12; i++)
      disp_text[11-i] = no_disp_str[i];
    disp_text[12] = '\0';
    memset_a(disp_separators, 0);
    memset_a(disp_labels, 0);
    memset_a(disp_highlights, 0);
    memset_a(disp_units_gate, 0);
  }

  disp_change = ch;
}


/* update disp_*_combined variables from disp_* variables - must call update_disp() first! */
void update_disp_combined() {
  int8_t j = 0;
  
  // update disp_text_combined and disp_highlights_combined
  if (disp_change & CHANGE_TEXT) {
    memset(disp_highlights_combined, 0, sizeof(disp_highlights_combined));
    for (int8_t i = 11; i >= 0 && j < (sizeof(disp_text_combined) - 1); i--) {
      disp_text_combined[j++] = disp_text[i];
      if (disp_highlights[i]) {
	disp_highlights_combined[j-1] = 1;
      }
      if (disp_separators[i]) {
	disp_text_combined[j++] = disp_separators[i];
      }
    }
    disp_text_combined[j] = '\0';
    disp_text_combined_len = j;
    disp_change |= CHANGE_TEXT_COMB;
  }

  // update disp_units_combined
  if (disp_change & CHANGE_UNITS) {
    j = 0;
    disp_units_combined[0] = '\0';
    for(int8_t i = 0; i < disp_units_n && j < sizeof(disp_units_combined); i++) {
      if (disp_units_gate[i] != 0) {
	uint8_t *from = hp_display_units_gate[i];
	j = strlgcat_a(disp_units_combined, from, j);
      }
    }
    disp_units_combined_len = j;
    disp_change |= CHANGE_UNITS_COMB;
  }

  // update disp_labels_combined
  if (disp_change & CHANGE_LABELS) {
    j = 0;
    disp_labels_combined[0] = '\0';
    for(int8_t i = 11; i >= 0 && j < (sizeof(disp_labels_combined) - 1); i--) {
      if (disp_labels[i] != 0) {
	if (j > 0) {
	  j = strlgspacefilln_a(disp_labels_combined, 1, j);
	}
	uint8_t *from = hp_display_labels[11-i];
	j = strlgcat_a(disp_labels_combined, from, j);
      }
    }
    disp_labels_combined[j] = '\0';
    disp_labels_combined_len = j;
    disp_change |= CHANGE_LABELS_COMB;
  }
}

/* map a character segments combination into a character to display, null for unknown */
uint8_t map_seg14_code(uint16_t segs14) {
  for (int i = 0; i < seg_n; i++) {
    if (seg_codes[i] == segs14) {
      return seg_mapped_chars[i];
    }
  }
  add_unk_seg14(segs14);
  return '\0';
}

/* map a character segments combination into a character to display, x for unknown */
uint8_t map_seg14_code_x(uint16_t segs14) {
  for (int i = 0; i < seg_n; i++) {
    if (seg_codes[i] == segs14) {
      return seg_mapped_chars[i];
    }
  }
  add_unk_seg14(segs14);
  return 'x';
}

/* add an unmapped character to the unknowns array */
void add_unk_seg14(uint16_t c) {
  if (unk_n_chars >= (sizeof(unk_chars) / 2))
    return;
  for (int i = 0; i < unk_n_chars; i++) {
    if (unk_chars[i] == c)
      return; // already registered
  }
  unk_chars[unk_n_chars++] = c;
}

/* print unknown characters */
void print_unknown_seg14s() {
  for (int i = 0; i < unk_n_chars; i++) {
    Serial.print("0x");
    Serial.println(unk_chars[i], 16);
  }
}

void print_unknown_separator() {
  if(unknown_dp) {
    Serial.print(F("Unknown separator: 0x"));
    Serial.println(unknown_dp, 16);
  }
}

/* Debug only - could really use some cleanup! */
uint8_t print_spi_msg(int8_t i, uint32_t msg) {
  uint32_t m = __builtin_bswap32(msg); // make big endian  
  uint16_t gates = m >> 20;
  uint32_t segs14 = m & 0x0000fcff;
  uint32_t segs_dp = m & 0x00070000;
  uint32_t segs_o = m & 0x00080300;
//  Serial.print(" gates: ");
//  Serial.print(gates, HEX);

  uint8_t c = map_seg14_code_x(segs14);
  char str[] = {c, 0};
  Serial.print(str);

  if (i != 0) {
    if (segs_dp == 0x00000) Serial.print(" ");
    else if (segs_dp == 0x20000) Serial.print(".");
    else if (segs_dp == 0x60000) Serial.print(",");
    else if (segs_dp == 0x30000) Serial.print(":");
    else if (segs_dp == 0x70000) Serial.print(";");
    else Serial.print("x"); // some more decoding here, perhaps...
  } else {
    if (segs_dp == 0) Serial.print(" ");
    else {
      if (segs_dp & 0x10000) Serial.print("u");
      if (segs_dp & 0x20000) Serial.print("s");
      if (segs_dp & 0x40000) Serial.print(" Gate ");
    }
  }

  Serial.print(" - ");
  if (segs_o & 0x00080000) {
    if (i >= 12) {
      Serial.print(F("BAD_I!"));
    } else {
      Serial.print(hp_display_labels[11 - i]);
    }
  } else {
    Serial.print("    ");
  }

  if (segs_o & 0x200) Serial.print("M");
  if (segs_o & 0x100) Serial.print("Hz");
  
  Serial.print(F(" - "));
  Serial.print(F(" segs14: "));
  Serial.print(segs14, HEX);
  Serial.print(F(" segs_dp: "));
  Serial.print(segs_dp, HEX);
  Serial.print(F(" segs_o: "));
  Serial.print(segs_o, HEX);
  Serial.print(F(" gate: "));
  Serial.print(hp_display_spi_msg2gateno((uint8_t*) &msg));
  Serial.print(F(" msg: "));
  Serial.println(m, 16);

  return c;
}
