#!/bin/python

"""
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
# Interactively map characters in codes-in.list and append them to codes-mapped.list
#

from __future__ import division, absolute_import, print_function
#, unicode_literals
import os, sys, argparse

# do not write bytecode (.pyc) files
sys.dont_write_bytecode=True


codes_in = []
codes_mapped = {}


#############
# 14 seg display

seg_arr = [
  1, 1, 1, 1, 1, 1, 1,  # AAAAAAA
  6,12, 0,  9,0, 11,2,  # FK I LB
  6, 0,12,  9,11, 0,2,  # F KIL B
  5, 7, 7, 10,8, 8, 3,  # EGG HHC
  5, 0, 14,10,13,0, 3,  # E NJM C
  5, 14, 0,10,0, 13,3,  # EN J MC
  4, 4, 4, 4, 4, 4, 4   # DDDDDDD
]

# map HP bits to segments
def code2segs(code):
    x = 0
    # map bits
    if (code & 0x0080): x |= 0x1  # A
    if (code & 0x0008): x |= 0x2  # B
    if (code & 0x0400): x |= 0x4  # C
    if (code & 0x4000): x |= 0x8  # D
    if (code & 0x8000): x |= 0x10  # E
    if (code & 0x0004): x |= 0x20  # F
    if (code & 0x0002): x |= 0x40  # G
    if (code & 0x0001): x |= 0x80  # H
    if (code & 0x0040): x |= 0x100  # I
    if (code & 0x2000): x |= 0x200  # J
    if (code & 0x0010): x |= 0x400  # K
    if (code & 0x0020): x |= 0x800  # L
    if (code & 0x0800): x |= 0x1000  # M
    if (code & 0x1000): x |= 0x2000  # N
    return x


def print_segs(segs):
    for i in range(len(seg_arr)):
        seg = seg_arr[i]
        if seg != 0 and ((segs >> (seg-1)) & 0x01):
            print('#', end='')
        else:
            print(' ', end='')
        if (((i+1) % 7) == 0):
            print('')


def map_char(code):
    segs = code2segs(code)
    
    print('\n####################')
    print('%04x -> %04x' % (code, segs))
    print_segs(segs)
    
    mapped = False
    while not mapped:
        c = raw_input('map character (x to skip)>')
        if len(c) != 1:
            print('ERROR: Reply with one character')
            continue
        if c == 'x':
            break
        add_mapped(code, c)
        break


def read_in():
    global codes_in
    with open('codes-in.list', 'r') as f:
        for line in f:
            n = int(line.strip(), 16) # hex
            if n not in codes_in:
                codes_in.append(n)


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
            


def add_mapped(code, val):
    if code in codes_mapped.keys():
        print("ERROR: duplicate key 0x%04x for codes-mapped.list!" % code)
        return False
    if val in codes_mapped.values():
        print("ERROR: duplicate val %c for codes-mapped.list!" % val)
        return False
    with open('codes-mapped.list', 'a') as f:
        print('0x%04x \'%c\'' % (code, val), file=f)
    codes_mapped[code] = val
    return True


def main():
    global glog
    parser = argparse.ArgumentParser()
    parser.add_argument('-v', '--verbose', action='store_true', help='verbose')
    args = parser.parse_args()

    read_in()
    read_mapped()

    #print(repr(codes_in))
    #print(repr(codes_mapped))

    for c in codes_in:
        if c not in codes_mapped.keys():
            map_char(c)



if __name__ == '__main__':
    main()
