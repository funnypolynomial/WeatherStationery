#!/usr/bin/python
import sys
import os
from PIL import Image

# Encode multiple regions from a PNG (eg 7-seg)
# Monochrome, max 127x127 pixels
# x0, y0,  // origin of region
# w, {h, dx, dy,} // bulk rect fill if w non-zero
# 
# if b7 of w is clear
# w, offs  // hz line at current row, <w> pixels and offset <offs>. If b7 of offs is set, start a new row first
# else
# w  // skip w-128 rows
# else
# 0 // end

encodeBulk = False # no point in bulk encoding on e-Paper

def ByteStr(b):
    return "0x" + hex(256 + b)[3:].upper()
    
    
def Encode(block, bulk_Colour, detail_Colour):
    global bytes
    bytes = 0
    # find origin of block
    x0 = y0 = 9999
    for y in range(bmp.height):
        for x in range(bmp.width):
            if bmp.getpixel((x, y)) == bulk_Colour or bmp.getpixel((x, y)) == detail_Colour:
                x0 = min(x0, x)
                y0 = min(y0, y)
    if y0 == 9999:
        return
    sys.stdout.write("static const uint8_t " + name + str(block) + "[] PROGMEM =\n")
    sys.stdout.write('{\n')
    # write it                
    sys.stdout.write("  " + ByteStr(x0))
    sys.stdout.write(", ")
    sys.stdout.write(ByteStr(y0))
    sys.stdout.write(", \n")
    bytes += 2

    # find any bulk fill
    xMin = yMin = +9999
    xMax = yMax = -9999
    if encodeBulk:
        for y in range(bmp.height):
            for x in range(bmp.width):
                if bmp.getpixel((x, y)) == bulk_Colour:
                    xMin = min(xMin, x)
                    yMin = min(yMin, y)
                    xMax = max(xMax, x)
                    yMax = max(yMax, y)
    # write it   
    if xMin == +9999:
        if encodeBulk:
            sys.stdout.write("  0x00,") # no bulk
            bytes += 1
    else:
        # bulk w, h, dx, dy, 
        sys.stdout.write("  " + ByteStr(xMax - xMin + 1))
        sys.stdout.write(", ")
        sys.stdout.write(ByteStr(yMax - yMin + 1))
        sys.stdout.write(", ")
        sys.stdout.write(ByteStr(xMin - x0))
        sys.stdout.write(", ")
        sys.stdout.write(ByteStr(yMin - y0))
        sys.stdout.write(", \n")
        bytes += 4

    # find detail lines
    skipRows = 0
    y = y0;
    firstRow = True
    while y < bmp.height:
        x = x0;
        skippedRow = True
        firstOnRow = True;
        while x < bmp.width:
            # skip to non-background
            while x < bmp.width and bmp.getpixel((x, y)) != detail_Colour and (encodeBulk or bmp.getpixel((x, y)) != bulk_Colour):
                x += 1
            if x < bmp.width: # found a pixel
                if skipRows:
                    sys.stdout.write("  " + ByteStr(skipRows+128))
                    sys.stdout.write(", \n")
                    skipRows = 0
                    bytes += 1
                start = x
                runLength = 1
                x += 1
                while x < bmp.width and (bmp.getpixel((x, y)) == detail_Colour or (not encodeBulk and bmp.getpixel((x, y)) == bulk_Colour)):
                    x += 1
                    runLength += 1
                sys.stdout.write("  " + ByteStr(runLength))
                sys.stdout.write(", ")
                if firstOnRow and not firstRow:
                    sys.stdout.write(ByteStr(128+start - x0))
                else:
                    sys.stdout.write(ByteStr(start - x0))
                sys.stdout.write(", ")
                firstRow = firstOnRow = skippedRow = False
                bytes += 2
        y += 1
        if skippedRow:
            skipRows += 1
        else:
            sys.stdout.write("\n")
    
    sys.stdout.write("  0x00\n")
    bytes += 1
    sys.stdout.write("};  // ")
    sys.stdout.write(str(bytes))
    sys.stdout.write(" bytes \n\n")

if len(sys.argv) != 3:
    sys.stdout.write("parameters: <segments name> <input image file>\n")
    exit()
name = sys.argv[1]
bmp = Image.open(sys.argv[2])
blockColours  = (
   # bulk           detail
  ((255, 100, 100), (255, 0, 0)),       #  0 A
  ((100, 255, 100), (0, 255, 0)),       #  1 B
  ((100, 100, 255), (0, 0, 255)),       #  2 C
                                           
  ((200, 100, 100), (200, 0, 0)),       #  3 D
  ((100, 200, 100), (0, 200, 0)),       #  4 E
  ((100, 100, 200), (0, 0, 200)),       #  5 F
                                           
  ((150, 100, 100), (150, 0, 0)),       #  6 G G1
  ((100, 150, 100), (0, 150, 0)),       #  7   G2
  ((100, 100, 150), (0, 0, 150)),       #  8   H
                                           
  ((255, 255, 100), (255, 255,   0)),   #  9   I
  ((100, 255, 255), (0,   255, 255)),   # 10   J
  ((255, 100, 255), (255,  0,  255)),   # 11   K
  
  ((200, 200, 100), (200, 200,   0)),   # 12   L
  ((100, 200, 200), (0,   200, 200)),   # 13   M
  ((200, 100, 200), (200,   0, 200)),   # 14
  
  ((175, 100, 100), (175,   0,   0)),   # 15
  
  )
bytes = 0
total_bytes = 0
sys.stdout.write("// ---------------------------------\n")
sys.stdout.write("#define " + name + "_WIDTH  "  + str(bmp.width)+"\n")
sys.stdout.write("#define " + name + "_HEIGHT " + str(bmp.height)+"\n")
for block in range(len(blockColours)):  
    Encode(block, blockColours[block][0], blockColours[block][1])
    total_bytes += bytes
sys.stdout.write("// (")
sys.stdout.write(str(total_bytes))
sys.stdout.write(" bytes total)\n\n")    

