#!/bin/python

from __future__ import division, absolute_import, print_function
#, unicode_literals
import os, sys, argparse
from operator import itemgetter, attrgetter

g_copyright_notice = """
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
"""

#
# Generate codes (display segments) -> character mapping in segmapgen.c
#

# do not write bytecode (.pyc) files
sys.dont_write_bytecode=True


codes_mapped = {}


def read_mapped():
    global codes_mapped
    with open('codes-mapped.list', 'r') as f:
        for line in f:
            (a, b) = line.strip().split(None, 1)
            n = int(a, 16) # hex
            val = b.strip('\'')
            if n in codes_mapped.keys():
                print("ERROR: duplicate key 0x%04x in codes-mapped.list!" % n)
                continue
            if val in codes_mapped.values():
                print("MAYBE ERROR: duplicate val %c in codes-mapped.list!" % val)
            codes_mapped[n] = val


# currenty just a list - should probably do some hashed thing here instead
def gen_file():
    codes = []
    vals = []

    # maybe we should use some hashing or something, but in lieu of
    # that, sort them, at least making it quicker to find digits
    l = codes_mapped.items()
    def sort_key((k, c)):
        o = ord(c)
        if c >= '0' and c <= '9':
            return o
        if c in ['-', '+', 'm', 'V', 'd', 'B']:
            return o + 1000
        return o+2000
    l = sorted(l, key=sort_key)

    codes = [c for (c, v) in l]
    vals = [v for (c, v) in l]

    #for code in codes_mapped.keys():
    #    codes.append(code)
    #    vals.append(codes_mapped[code])

    with open('segmapgen.c', 'w+') as f:
        print(g_copyright_notice, file=f)
        print('#include <Arduino.h>', file=f)
        print('#include <stdint.h>', file=f)
        print('#include <stdlib.h>', file=f)
        print('#include "hp_msg_parse.h"', file=f)
        print('', file=f)
        print('uint16_t seg_n = %d;' % len(codes), file=f)
        print('uint16_t seg_codes[] = {', file=f)
        for code in codes:
            print('0x%04x, ' % code, file=f)
        print('};', file=f)
        print('uint8_t seg_mapped_chars[] = {', file=f)
        for val in vals:
            print('\'%c\', ' % val, file=f)
        print('};', file=f)


def main():
    global glog
    parser = argparse.ArgumentParser()
    parser.add_argument('-v', '--verbose', action='store_true', help='verbose')
    args = parser.parse_args()

    read_mapped()

    gen_file()



if __name__ == '__main__':
    main()
