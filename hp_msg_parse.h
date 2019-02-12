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
 * map HP gate/anode VFD segment information into characters for
 * displaying using other means
 */

/* in hp_msg_parse.cpp */

/* exported constants */
extern const char* const hp_display_labels[];
extern const char* const hp_display_units_gate[];

/* exported variables, updated by update_disp() */
extern uint8_t disp_text[13]; // display text
extern uint8_t disp_separators[12]; // dot, comma, colon, semicolon, or null
extern uint8_t disp_labels[12]; // 0 or 1 if label should be displayed
extern uint8_t disp_highlights[12]; // 0 or 1 if character should be highlighted
extern uint8_t disp_units_gate[5]; // 0 or 1 if unit or Gate should be displayed
extern uint8_t disp_change; // Bitfield stating what fields changed
extern uint8_t disp_no_display_data; // Currently no display data from instrument
#define disp_units_n 4
/* exported variables, updated by update_disp_combined() (after an update_disp())
 * disp_text_combined can in theory be 23 long, but in reality seems to never exceed 16, except at display test.
 * disp_units_combined is normally max 3 long, except at display test. */
extern char disp_text_combined[24];       // string built from disp_text and disp_separators
extern char disp_highlights_combined[24]; // highlights matching disp_text_combined
extern size_t disp_text_combined_len;     // length of string in disp_text_combined and flags in disp_highlights_combined
extern char disp_units_combined[6];       // Combination of the active units (excluding Gate!)
extern size_t disp_units_combined_len;    // length of string in disp_units_combined
extern char disp_labels_combined[64];     // Combination of the active labels, separated with space
extern size_t disp_labels_combined_len;   // length of string in disp_labels_combined

#define CHANGE_TEXT 0x01 /* also disp_separators and/or disp_highlights */
#define CHANGE_LABELS 0x02
#define CHANGE_UNITS 0x04
#define CHANGE_GATE 0x08
#define CHANGE_ALL 0x0f
#define CHANGE_TEXT_COMB 0x10 /* also disp_highlights_combined */
#define CHANGE_UNITS_COMB 0x20
#define CHANGE_LABELS_COMB 0x40

/* Update disp_* variables */
void update_disp(void);
/* update disp_*_combined variables from disp_* variables - must call update_disp() first! */
void update_disp_combined();

/* map a character segments combination into a character to display, null for unknown */
uint8_t map_seg14_code(uint16_t segs14);
/* map a character segments combination into a character to display, x for unknown */
uint8_t map_seg14_code_x(uint16_t segs14);

void print_unknown_seg14s();
void print_unknown_separator();

/* exported internal variables for debug */
extern uint8_t unknown_dp;
extern int unk_n_chars;
extern uint16_t unk_chars[];

/* internal */
void add_unk_seg14(uint16_t c);

/* Debug only - could really use some cleanup! */
uint8_t print_spi_msg(int8_t i, uint32_t msg);



// strcpy/strcat version that have single byte lengths, and that don't
// need to strlen dest string.
// ALSO - the maxlen, meaning the max string length, is given - the
// data field must be able to fit one more character for the trailing
// null char!

#ifdef __cplusplus
#define restrict
#endif

#define strlgcpy_a(dst, src) strlgcpy(dst, src, (sizeof(dst)-1))
inline uint8_t strlgcpy(char * restrict dst, const char * restrict src, uint8_t maxlen) {
  uint8_t i;
  for (i = 0; i < maxlen && *src != '\0'; i++) {
    dst[i] = *(src++);
  }
  dst[i] = '\0';
  return i;
}

#define strlgcat_a(dst, src, currdstlen) strlgcat(dst, src, (sizeof(dst)-1), currdstlen)
inline uint8_t strlgcat(char * restrict dst, const char * restrict src, uint8_t maxlen, uint8_t currdstlen) {
  uint8_t i;
  for (i = currdstlen; i < maxlen && *src != '\0'; i++) {
    dst[i] = *(src++);
  }
  dst[i] = '\0';
  return i;
}

#define strlgspacefill_a(dst, currdstlen) strlgspacefill(dst, (sizeof(dst)-1), currdstlen)
inline uint8_t strlgspacefill(char * restrict dst, uint8_t maxlen, uint8_t currdstlen) {
  uint8_t i;
  for (i = currdstlen; i < maxlen; i++) {
    dst[i] = ' ';
  }
  dst[i] = '\0';
  return i;
}

#define strlgspacefilln_a(dst, n, currdstlen) strlgspacefilln(dst, n, (sizeof(dst)-1), currdstlen)
inline uint8_t strlgspacefilln(char * restrict dst, uint8_t n, uint8_t maxlen, uint8_t currdstlen) {
  uint8_t i;
  for (i = currdstlen; i < maxlen && n > 0; i++) {
    dst[i] = ' ';
    n--;
  }
  dst[i] = '\0';
  return i;
}

#define memlgcmp_a(dst, src) memlgcmp(dst, src, sizeof(dst))
inline uint8_t memlgcmp(char * restrict dst, const char * restrict src, uint8_t maxlen) {
  uint8_t i, ret = 0;
  for (i = 0; i < maxlen; i++) {
    if (dst[i] != src[i]) {
      ret = 1;
    }
  }
  return ret;
}

#define memlgcpycmp_a(dst, src) memlgcpycmp(dst, src, (sizeof(dst)-1))
inline uint8_t memlgcpycmp(char * restrict dst, const char * restrict src, uint8_t maxlen) {
  uint8_t i, ret = 0;
  for (i = 0; i < maxlen; i++) {
    if (dst[i] != src[i]) {
      dst[i] = src[i];
      ret = 1;
    }
  }
  return ret;
}

// replace all char a with b, stops on null
#define strchrrepl_a(str, a, b) strchrrepl(str, a, b, sizeof(str))
inline void strchrrepl(char * restrict str, char a, char b, uint8_t maxlen) {
  for (uint8_t i = 0; i < maxlen; i++) {
    char c = str[i];
    if (c == a) {
      str[i] = b;
    }
    else if (c == '\0') {
      return;
    }
  }
}

#define memset_a(b, c) memset(b, c, sizeof(b))

#ifdef __cplusplus
#undef restrict
#endif


/* in segmapgen.c */

#ifdef __cplusplus
extern "C" {
#endif

extern uint16_t seg_n;
extern uint16_t seg_codes[];
extern uint8_t seg_mapped_chars[];

#ifdef __cplusplus
} // extern "C"
#endif
