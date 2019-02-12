
## Choosing a display

It seems that the longest reading string that the display has to
handle is 12 digits plus up to 4 separators (.,:;), with possibly the
units/prefixes M, MHz, Hz, us or s.

A 12 character text string can be wider than a 12 digit number if a
proportional font is used.

In the example display drivers, the units/prefixes are kept on the
same row as the value of possible on the character LCD as long as it
fits, which it seemingly always does, but always on a separate row on
the graphics OLED since it quite often does not fit and having it
jumping around would probably be disturbing.

### LCD

Certain LCD:s suffer from poor viewing angles, being slow to update
and having low contrast. There are now TN/STN/VATN displays and other
techniques also in the low price segment that have almost as good
viewing angles and contrast a OLED:s.

The lifetime of a LCD is often very long, though the backlight LED(s)
may have shorter life, typically a couple of tens of thousands of
hours. Often the LEDs just gets dimmer with time, they generally do
not break.


### OLED

OLEDs have very good viewing angles, contrast and speed.

They have a limited lifetime, though they generally just get dimmer
over time. It seems that generally the yellow ones have the longest
lifetime, white almost as long, and red being the worst. There are
OLED:s specified for a lifetime of 50 000 hours or more.


### Character or graphics display

Updating a character display is almost always much faster than a
graphical display, since the microcontroller only has to send over
character codes, not entire bitmaps. It also doesn't require nearly as
much program memory or RAM.


### Display Interface

#### Parallel

Fast, but take a few more pins.

#### SPI

SPI can be quite fast, up to 10 Mb/s, but we have already used one
hardware SPI interface for getting the data from the instrument. An
SPI master interface can also be made in software using bit banging,
which give some 700 kbps on a 328 style MCU. There are MCU:s with more
than one hardware SPI controller.

The OLED used in the example video is a 1.54 inch display with a
SSD1309 controller connected using a software SPI master. This seems to
work just fine.

#### I2C

The slowest way of communicating with the display, normally up to 400
kbps. Likely not at all usable for a colour (multiple bits per pixel)
graphical display, and not on monochrome graphical displays with a
little higher resolution (e.g. more than about 128*64 pixels).

I2C should be more than enough for character displays.

### Display Size

A 20 x 4 character display well covers most uses for a
53131A/53132A/53181A. You could likely get away with 20 x 3 as well.

If you use a 16 character wide display you will have to put the units
on a separate line, since the reading itself can be 16 characters (12
characters + 4 separators).

A 128 x 64 pixel display works for most things, if you use it as
e.g. 4 rows of >=16 characters.

### Installation in the instrument

A NHD-0420CW-AW3, a compact OLED 20x4 character display, or an
equivalent from another manufacturer, may fit where the original VFD
is mounted, but this has not been tested or verified.

The original VFD has an active area of about 110 x 16 mm.

The unmasked area of the glass of a 53131A is about 143 x 26 mm (but
check yours)!

### Supported displays

The project comes with support for three display types, where the two
OLEDs share almost all code thanks to their similarities and the U8g2
library.

#### Display wiring

- Interface for the parallel character display, see:
  - [../lcd_20x4_hd44780.cpp](../lcd_20x4_hd44780.cpp)
- Interface for the SPI or I2C graphical OLEDs, see:
  - [../oled_128x64.cpp](../oled_128x64.cpp)

#### 20x4 character display, parallel

- HD44780 compatible controller
- parallel 4 bit interface (can easily be changed to others)
- needs the "hd44780" Arduino library (version 1.0.1)

Enable by uncommenting 
`#define LCD_20X4_HD44780`
in hp_display_config.h.

#### 128x64 pixel OLED graphical display with SSD1306, I2C

- SSD1306 OLED controller
- I2C communication (can easily be changed to others)
- needs the "U8g2" Arduino library (version 2.24.3)
- proportional font used

Enable by uncommenting 
`#define OLED_128X64`
in hp_display_config.h.


#### 128x64 pixel OLED graphical display with SSD1309, software SPI

- SSD1309 OLED controller
- SPI communication, software based (can easily be changed to others)
- needs the "U8g2" Arduino library (version 2.24.3)
- proportional font used

Enable by uncommenting 
`#define OLED_128X64_SSD1309_SW_SPI`
in hp_display_config.h.

