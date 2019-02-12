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


#ifndef EXTERN
  #define EXTERN extern
#endif

extern uint32_t spi_msgs[];
extern uint8_t spi_frames; // incremented when we have got a complete new frame
extern unsigned long spi_msg_last_t; // time of last received spi msg

void setup_hp_display_spi();
// returns true if we have not got any SPI data the last seconds
uint8_t hp_display_spi_timeout();

uint8_t hp_display_spi_msg2gateno(uint8_t *spi_msg);
inline uint32_t hp_display_msg(uint8_t msg_index) {
  noInterrupts();
  uint32_t msg = spi_msgs[msg_index];
  interrupts();
  return msg;
}

/* debugging */

uint32_t hp_display_spi_read();
void hp_display_spi_print_debug();
void hp_display_print_last_msgs();
void hp_display_print_last_sync_lost_msgs();

void hp_display_copy_last_spi_msgs(uint32_t *dest_arr); // may want to disable interrupts to get consistent copy


/* internal variables, also exported for debugging inspection */
extern uint16_t spi_ivr_loops;
extern uint8_t spi_n_bytes;
extern uint8_t spi_bytes[];
extern uint32_t last_spi_msg; // debug
extern uint32_t spi_msgs_incom;
extern uint32_t spi_msgs_ok;
extern uint32_t spi_sync_loss;
extern uint32_t last_spi_msgs[];
extern uint8_t last_spi_msgs_i;
