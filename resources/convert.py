#!/usr/bin/python
import sys
import argparse
import os
from PIL import Image

# convert serial dump from defining DISPLAY_SERIALIZE
# if the line is 100 hex digits it is treated as a row of "monochrome" data, 2 bits per pixel 0=black, 2=grey, 3=white
# if the line is 50 hex digits it is treated as a row of "red" data, 1 bit per pixel 0=red, 1=N/A
# other lines are ignored
# hex digit pairs are most significant first, most significant bits of first byte are left-most pixel
# assumes 200x200 pixels, no row checking

def HexDigit(ch):
    if '0' <= ch and ch <= '9':
        return ord(ch) - ord('0')
    elif 'A' <= ch and ch <= 'F':
        return ord(ch) - ord('A') + 10
    elif 'a' <= ch and ch <= 'f':
        return ord(ch) - ord('a') + 10
    return 0
        

if len(sys.argv) != 2:
    print 'convert.py <inputfile>'
    sys.exit(2)    
fileName = sys.argv[1]

script = open(fileName, 'r')
bmp = Image.new("RGB", (200,200), "black")
line = script.readline().strip();
monoRow = 0
redRow = 0;
while line:
    if len(line) == 100:
        # mono
        for idx in range(len(line)/2):
            byte = HexDigit(line[2*idx])*16 + HexDigit(line[2*idx + 1])
            for pix in range(4):
                val = (byte & 0b11000000) >> 6;
                rgb = (0, 0, 0)
                if val == 2:
                  rgb = (224, 224, 224) # grey is quite light
                elif val == 3:
                  rgb = (255, 255, 255)
                bmp.putpixel((idx*4 + pix, monoRow), rgb)
                byte = byte << 2
        monoRow += 1
    elif len(line) == 50:
        # red
        for idx in range(len(line)/2):
            byte = HexDigit(line[2*idx])*16 + HexDigit(line[2*idx + 1])
            for pix in range(8):
                val = (byte & 0b10000000) >> 7;
                if val == 0:
                  bmp.putpixel((idx*4 + pix, redRow), (255, 0, 0)) # overwrite red
                byte = byte << 1
        redRow += 1        
    line = script.readline().strip();

bmp.save(os.path.splitext(fileName)[0] + ".png")
script.close()
