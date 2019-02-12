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

#ifndef HP_DISPLAY_CONFIG_H
#define HP_DISPLAY_CONFIG_H


/* Main software configuration */


/*
 * *** Global defines to change core functionality
 */

/*
 * Default is configuration for Arduino Pro Micro.
 * For Arduino Nano, define ARDUINO_NANO
 */
//#define ARDUINO_NANO

/*
 * *** Enable/disable optional software components
 */

/* LCD, 20x4 HD44780 compatible character display */
//#define LCD_20X4_HD44780

/* OLED, 128x64 SSD1306 pixel graphical display, i2c (hardware) */
//#define OLED_128X64
/* OLED default i2c address is 0x3d. Another pretty common one is 0x3c. */
//#define OLED_I2C_ADDR 0x3d

/* OLED, 128x64 SSD1309 pixel graphical display, SPI (in software) */
//#define OLED_128X64_SSD1309_SW_SPI


/* Other options */

/* 
 * Hack for Arduino Pro Micro with ATmega32U4, probably also useful on
 * Leonardo: Disable pin 17, RX-LED, which we want to use as SPI /SS
 * input, disable output to not put uneccesary stress on the
 * SS_OUT_PIN in case a very low resistor value is used.
 *
 * Do not use on non-Pro Micro/Leonardo boards.
 */
#ifndef ARDUINO_NANO
#define PRO_MICRO_WORKAROUND
#endif


/*
 * The interval, in milliseconds, for printing things on the console.
 * For a 32U4 MCU, which has integrated USB and no speed issues, use
 * 0, or perhaps 50 for more convenient debugging, depending on your
 * terminal.
 *
 * For ATmega328 based MCU:s, choose at least 70 to not risk losing
 * characters when debugging.
 */
#ifndef ARDUINO_NANO
// Pro Micro:
#define CONSOLE_PRINT_INTERVAL_MS 50
#else
// Nano:
#define CONSOLE_PRINT_INTERVAL_MS 70
#endif


/* END of use configurable options */

/* The OLED_128X64_SSD1309_SW_SPI uses the same code as OLED_128X64 */
#ifdef OLED_128X64_SSD1309_SW_SPI
 #ifndef OLED_128X64
  #define OLED_128X64
 #endif
#endif

#endif // HP_DISPLAY_CONFIG_H
