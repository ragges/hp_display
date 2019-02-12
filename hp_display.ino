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

#include <stdint.h>
#include "hp_display_config.h" // include this before the other local files
#include "hp_display_spi.h"
#include "hp_msg_parse.h"
#include "oled_128x64.h"
#include "lcd_20x4_hd44780.h"


uint8_t debug = 0;
uint8_t debug_loopsps = 0;
uint8_t do_print_in_loop = 1;
uint16_t console_print_interval_ms = CONSOLE_PRINT_INTERVAL_MS;
unsigned long last_print_t = 0;

// some debug stuff
unsigned long loops_t = 0;
uint32_t loops_n = 0;
uint32_t loops_n_last = 0;
uint32_t updates_n = 0;
uint32_t updates_n_last = 0;

// ###############
// Setup

void setup() {
  Serial.begin(115200);

  setup_pins();
  setup_hp_display_spi();

  #ifdef LCD_20X4_HD44780
  lcd_20x4_hd44780_setup();
  #endif

  #ifdef OLED_128X64
  oled_128x64_setup();
  #endif
}

// ###############
// Loop

void loop() {
  static uint8_t last_spi_frames = 0;
  static uint8_t updates_to_print = 0;
  unsigned long now_ms = millis();
  uint8_t do_print = 0;

  // cap how often we print, needed on MCU:s without built in USB
  if ((last_print_t + console_print_interval_ms) < now_ms) {
    do_print = do_print_in_loop;
    last_print_t = now_ms;
  }



  if (true) {
    // parse commands
    command_parser();

    if ((last_spi_frames != spi_frames) || hp_display_spi_timeout()) { // is there a complete new frame?
      last_spi_frames = spi_frames;

      // update displays
      update_disp();
      update_disp_combined();

      updates_to_print = 1;

      #ifdef LCD_20X4_HD44780
      lcd_20x4_hd44780_update();
      #endif

      #ifdef OLED_128X64
      oled_128x64_update();
      #endif

      if (disp_change)
        updates_n++;
    }

    if (do_print && updates_to_print) {
      Serial.println("############");
      print_display_combined();
      updates_to_print = 0;
    }

    if (debug and do_print) {
      print_display_fields();
#if 0
      Serial.println(F("########### last msgs"));
      hp_display_print_last_msgs();
      Serial.println(F("########### last sync lost msgs"));
      hp_display_print_last_sync_lost_msgs();
#endif
#if 0
      Serial.println(F("########### Unknowns characters seen:"));
      print_unknown_seg14s();
      Serial.println(F("########### Unknowns separators seen:"));
      print_unknown_separator();
#endif
#if 0    
      Serial.println(F("###########"));
      char displ[17];
      for(int8_t i = 15; i >= 0; i--) {      
        displ[15-i] = print_spi_msg(i, spi_msgs[i]);
      }
      // print message chars
      displ[16] = '\0';
      Serial.println(displ + 4);
      // print highligt chars
      displ[4] = '\0';
      Serial.println(displ);
#endif

      Serial.println(F("###########"));
      hp_display_spi_print_debug();
    }

    // check how many loops we can run per second
    if (debug_loopsps) {
      {
        unsigned long m_now = millis();
        if (m_now > (loops_t + 1000)) {
          loops_n_last = loops_n;
          loops_n = 0;
          updates_n_last = updates_n;
          updates_n = 0;
          loops_t = m_now;
        }
        loops_n++;
        if (do_print) {
          Serial.print(F("loops/s: "));
          Serial.println(loops_n_last);
          Serial.print(F("updates/s: "));
          Serial.println(updates_n_last);
        }
      }
    }
  }
}


void print_display_fields() {
  static uint8_t text[13];
  static uint8_t seps[13];
  static uint8_t highlights[13];
  static uint8_t labels[13];
  static uint8_t units_gate[6];  

  for(uint8_t i = 0; i < 12; i++) {
    text[11-i] = disp_text[i];
    seps[11-i] = disp_separators[i] ? disp_separators[i] : '_';
    highlights[11-i] = disp_highlights[i] ? 'x' : '_';
    labels[11-i] = disp_labels[i] ? 'x' : '_';
  }
  for(uint8_t i = 0; i < 5; i++) {
    units_gate[i] = disp_units_gate[i] ? 'x' : '_';
  }
  text[12] = seps[12] = labels[12] = units_gate[5] = '\0';
  Serial.print((const char*) text);
  Serial.print(" ");
  Serial.print((const char*) seps);
  Serial.print(" ");
  Serial.print((const char*) highlights);
  Serial.print(" ");
  Serial.print((const char*) labels);
  Serial.print(" ");
  Serial.println((const char*) units_gate);
  Serial.println();
}

void print_display_combined() {
  Serial.print(disp_text_combined);
  Serial.print(" ");
  Serial.println(disp_units_combined);

  Serial.print(disp_labels_combined);
  if(disp_units_gate[4] != 0) { // Gate
    Serial.print("    ");
    Serial.print(hp_display_units_gate[4]);
  }
  Serial.println();
}


// ###############
// Command handler

#define BUFLEN 16
char cmdbuf[BUFLEN + 1];
byte cmdi = 0;
unsigned long last_user_input_t = 0;

void command_parser() {
  unsigned long now = millis();

  if (Serial.available() > 0) {
    int c = Serial.read();

    last_user_input_t = now;

    if (c == '\r' || c == '\n') {
      Serial.println();
      if (cmdi > 0) {
        cmdbuf[cmdi] = '\0';
        handle_command();
      }
      cmdi = 0;
      print_prompt();
      return;
    }

    // handle delete & backspace
    if (c == 0x08 || c == 0x7f) {
      if (cmdi > 0) {
        cmdi--;
        Serial.write("\b \b");
      }
      return;
    }

    if (cmdi >= BUFLEN) {
      return;
    }

    if (!isPrintable(c)) {
      return;
    }

    cmdbuf[cmdi] = c;
    cmdi++;
    //Serial.print(c, HEX);
    Serial.write(c);
  }

  if (last_user_input_t > now) {
    last_user_input_t = 0; // millis wrapped (which it does every 50 days or so), just reset
  }
  if ((last_user_input_t == 0) || (last_user_input_t + 10000) < now) {
    do_print_in_loop = 1;
  } else {
    do_print_in_loop = 0;
  }
}

void handle_command() {
  if (match_command("help")) {
    cmd_help();
  } else if (cmdbuf[0] == 'c') {
    cmd_c();
  } else if (cmdbuf[0] == 'u') {
    cmd_u();
  } else if (match_command("debug")) {
    cmd_debug();
  } else if (match_command("lps")) {
    cmd_lps();
  } else {
    Serial.print(F("unknown command: "));
    Serial.println(cmdbuf);
  }
}

bool match_command(char *cmd) {
  if (strncmp(cmdbuf, cmd, strlen(cmdbuf)) == 0)
    return true;
  return false;
}

void print_prompt() {
  Serial.flush();
  Serial.print(F("> "));
}

void cmd_help() {
  Serial.println(F("commands:"));
  Serial.println(F("help             - show this information"));
  Serial.println(F("continue         - continue printout (without waiting for timeout)"));
  Serial.println(F("unk              - print accumulated unknown characters"));
  Serial.println(F("debug            - toggle debug printouts"));
  Serial.println(F("lps              - toggle loops per second printouts"));
  Serial.println("");
}

void cmd_c() {
  last_user_input_t = 0;
}

void cmd_u() {
  Serial.println(F("########### Unknowns characters seen:"));
  print_unknown_seg14s();
  Serial.println(F("########### Unknowns separators seen:"));
  print_unknown_separator();
}

void cmd_debug() {
  debug = ~debug;
  if (debug) {
    Serial.println(F("debugging turned on."));
  } else {
    Serial.println(F("debugging turned off."));
  }
}

void cmd_lps() {
  debug_loopsps = ~debug_loopsps;
  if (debug_loopsps) {
    Serial.println(F("loops per second printout turned on."));
  } else {
    Serial.println(F("loops per second printout turned off."));
  }
}

// ###############
// Setup stuff

void setup_pins() {
  // unused pins

  const uint8_t unused_d_ports[] = {0, 1, 2, 3, 4, 5, 6, 7,
                                  8, 9, 10 /*, 14, 15, 16*/};

  for(uint8_t i = 0; i < sizeof(unused_d_ports); i++) {
    pinMode(i, INPUT);
    digitalWrite(i, HIGH);
  }

#ifndef ARDUINO_NANO
  // Port F, pins 4..7, analog inputs A0-A3
  const uint8_t pins = 0xf0;
  DDRF &= ~pins; // mask of bit to make it input
  PORTF |= pins; // set reg 1 to enable pull-up resistor
#endif
}

