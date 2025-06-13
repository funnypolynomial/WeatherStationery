#!/usr/bin/env python
'''
WeatherStationery enclosure
Plywood Rimu Stain 3mm
-- nope, 3mm acrylic

enclosure is an outer shell, PCB sits inside. 
additional plates on the inside top and bottom
back plate sits inside
box/finger joints
From the front, the enclosure is
* a PCB acting as the front plate, plain but with a square void for the ePaper display and 4 holes in the corners
* the display PCB pressed against the inside of the front plate, attached are 4 standoffs which connect back to the clamp plate
* an internal wood clamp plate, attached to the display PCB and to the front plate, via screws
* the Arduino PCB with ATMEL board on the front side, 8-lead connector to the display, 2 leads to USB breakout, pressure sensor on the back side
* an internal wood PCB support for the Arduino PCB, attached to the clamp plate with standoffs, with a void for the sensor
* the wooden back plate of the enclosure, with a void for the sensor
* a pierced cap over the sensor void

'''

import inkex
from inksnek import *
import sys
import math
sys.path.append('C:/inksnek/extras/')
from read_gerber_holes import *
from stroked_font import *

class MyDesign(inkex.Effect):
  def __init__(self):
    inkex.Effect.__init__(self)

  def boxJointPath(self, n, totalLength, mainSignX, mainSignY, tabSignX, tabSignY, adjust = False):
    # the path moving in the main direction, with the finger joints moving in the tab direction
    # signs should be +1, -1 or 0
    # if adjust, compensate for shorter side due to tabs being outies
    delta = self.materialThickness if adjust else 0.0
    gap = ((totalLength + 2.0*delta) - n*self.fingerLength)/(n + 1)
    path = inksnek.path_line_by(mainSignX*(gap - delta), mainSignY*(gap - delta))
    for i in range(n):
        path += inksnek.path_line_by(+tabSignX*self.materialThickness, +tabSignY*self.materialThickness)
        path += inksnek.path_line_by(mainSignX*self.fingerLength, mainSignY*self.fingerLength)
        path += inksnek.path_line_by(-tabSignX*self.materialThickness, -tabSignY*self.materialThickness)
        if i == n - 1:
            path += inksnek.path_line_by(mainSignX*(gap - delta), mainSignY*(gap - delta))
        else:
            path += inksnek.path_line_by(mainSignX*gap, mainSignY*gap)
    return path
  
  def left_rightPanel(self, group):
    # general side panel
    path = inksnek.path_move_to(0, 0)
    path += self.boxJointPath(self.fingersDepth, self.outerDepth, 0.0, +1.0, +1.0, 0.0) # front to back
    path += self.boxJointPath(self.fingersHeight, self.outerHeight, +1.0, 0.0, 0.0, -1.0) # bottom to top
    path += self.boxJointPath(self.fingersDepth, self.outerDepth, 0.0, -1.0, -1.0, 0.0) # back to front
    path += inksnek.path_close()
    inksnek.add_path(group, path, inksnek.cut_style)
    if self.showRects:
        inksnek.add_rect(group, 0, 0, self.outerHeight, self.outerDepth, inksnek.ignore_style)
        inksnek.add_rect(group, self.materialThickness, 0, self.innerHeight, self.innerDepth, inksnek.ignore_style)
        
  def top_bottomPanel(self, group, bottom):
    # general upper/lower panel
    if self.explode or bottom:
        path = inksnek.path_move_to(self.materialThickness, 0)
        path += self.boxJointPath(self.fingersDepth, self.outerDepth, 0.0, +1.0, -1.0, 0.0) # front to back
        path += self.boxJointPath(self.fingersWidth, self.outerWidth - 2.0*self.materialThickness, +1.0, 0.0, 0.0, -1.0) # left to right
        path += self.boxJointPath(self.fingersDepth, self.outerDepth, 0.0, -1.0, +1.0, 0.0) # back to front
        path += inksnek.path_line_by(-(self.outerWidth - 2.0*self.materialThickness), 0.0)
    else:
        path = inksnek.path_move_to(self.materialThickness, 0)
        path += inksnek.path_line_by(self.outerWidth - 2.0*self.materialThickness, 0.0)
    inksnek.add_path(group, path, inksnek.cut_style)      
    if self.showRects:
        inksnek.add_rect(group, 0, 0, self.outerWidth, self.outerDepth, inksnek.ignore_style)
        inksnek.add_rect(group, self.materialThickness, 0, self.innerWidth, self.innerDepth, inksnek.ignore_style)
      
  def leftPanel(self, group):
    # left
    inksnek.add_annotation(group, 3.5, 2.0, "LEFT")
    self.left_rightPanel(group)
    self.windDecoration(group)

  def topPanel(self, group):
    # top
    inksnek.add_annotation(group, 3.5, 2.0, "TOP")
    self.top_bottomPanel(group, False)
    self.sunDecoration(group)

  def rightPanel(self, group):
    # right
    inksnek.add_annotation(group, 3.5, 2.0, "RIGHT")
    self.left_rightPanel(group)
    self.rainDecoration(group)

  def backPanel(self, group):
    # back
    inksnek.add_annotation(group, 3.5, 4.0, "BACK")
    path = inksnek.path_move_to(self.materialThickness, self.materialThickness)
    path += self.boxJointPath(self.fingersHeight, self.outerHeight - 2.0*self.materialThickness, 0.0, +1.0, -1.0, 0.0, True) # top to bottom
    path += self.boxJointPath(self.fingersWidth, self.outerWidth - 2.0*self.materialThickness, +1.0, 0.0, 0.0, +1.0) # right to left
    path += self.boxJointPath(self.fingersHeight, self.outerHeight - 2.0*self.materialThickness, 0.0, -1.0, +1.0, 0.0, True) # bottom to top
    path += self.boxJointPath(self.fingersWidth, self.outerWidth - 2.0*self.materialThickness, -1.0, 0.0, 0.0, -1.0) # left to right
    inksnek.add_path(group, path, inksnek.cut_style)      
    if self.showRects:
        inksnek.add_rect(group, 0, 0, self.outerWidth, self.outerHeight, inksnek.ignore_style)
        inksnek.add_rect(group, self.materialThickness, self.materialThickness, self.innerWidth, self.innerHeight, inksnek.ignore_style)
    # hole for USB
    usb = inksnek.add_group(group, inksnek.translate_group((self.outerWidth - self.usbPCBWidth)/2.0, 2.0*self.materialThickness))
    path = inksnek.path_move_to(0,0)
    path += inksnek.path_horz_by(self.usbPCBWidth)
    path += inksnek.path_vert_by(self.usbPCBThickness)
    path += inksnek.path_horz_by(-(self.usbPCBWidth - self.usbPCBConnectorWidth)/2.0)
    path += inksnek.path_vert_by(self.usbPCBHeight - self.usbPCBThickness)
    path += inksnek.path_horz_by(-self.usbPCBConnectorWidth)
    path += inksnek.path_vert_by(-(self.usbPCBHeight - self.usbPCBThickness))
    path += inksnek.path_horz_by(-(self.usbPCBWidth - self.usbPCBConnectorWidth)/2.0)
    path += inksnek.path_close()
    inksnek.add_path(usb, path, inksnek.cut_style)
    # connector?
    #inksnek.add_rect(usb, 0, 0, self.usbPCBWidth, self.usbConnectorHeight, inksnek.ignore_style)
    # inner plates etc
    inksnek.add_rect(group, self.materialThickness, self.materialThickness, self.innerWidth, self.materialThickness, inksnek.ignore_style)
    inksnek.add_rect(group, self.materialThickness, self.outerHeight - 2.0*self.materialThickness, self.innerWidth, self.materialThickness, inksnek.ignore_style)
    self.eInkBoard(group, False)
    self.pcbHoles(group, False)
    self.insidePlateHoles(inksnek.add_group(group, inksnek.translate_group(self.materialThickness, 2.0*self.materialThickness)), True, True)
    # project PCB
    pcbGroup = inksnek.add_group(group, inksnek.translate_group((self.outerWidth - self.projectPCBSize)/2.0, (self.outerHeight - self.projectPCBSize)/2.0))
    self.projectPCB(pcbGroup, False)
    # show cover
    self.sensorCover(pcbGroup, self.sensorCentreX, self.sensorCentreY, inksnek.ignore_style)
    # text
    txt = "WeatherStationery"
    scale = 0.25
    gap = 4
    dims = self.font.textDims(scale, txt, gap)
    y = self.outerHeight - self.ventOffsetY - self.ventHeight - dims[1]
    inksnek.add_path(group, self.font.text((self.outerWidth - dims[0])/2, y, scale, txt, gap), inksnek.etch_style)
    gap = 2 # tweak - space the letters a little
    inksnek.add_path(group, self.font.text(self.materialThickness*1.5, self.materialThickness*2.0, scale, "MEW", gap), inksnek.etch_style)
    txt = "MMXXV"
    inksnek.add_path(group, self.font.text(self.outerWidth - self.materialThickness*1.5 - self.font.textDims(scale, txt, gap)[0], self.materialThickness*2.0, scale, txt, gap), inksnek.etch_style)
    if self.V2:
        txt = "4K@"  # tweak - "for Kathy"
        dims = self.font.textDims(scale, txt, gap)
        y -= 2*dims[1]
        inksnek.add_path(group, self.font.text((self.outerWidth - dims[0])/2, y, scale, txt, gap), inksnek.etch_style)


  def bottomPanel(self, group):
    # bottom/base
    inksnek.add_annotation(group, 3.5, 2.0, "BOTTOM")
    self.top_bottomPanel(group, True)
    usb = inksnek.add_group(group, inksnek.translate_group((self.outerWidth - self.usbPCBWidth)/2.0, self.outerDepth - self.usbPCBDepth))
    self.addUSBPCB(usb, False)
    txt = "\x18 Open @ \x18's"
    scale = 0.2
    dims = self.font.textDims(scale, txt)
    if self.V2: # tweak - better placement
        inksnek.add_path(group, self.font.text((self.outerWidth - self.font.textDims(scale, txt)[0])/2.0, self.innerDepth - self.usbPCBDepth, 0.2, txt), inksnek.etch_style)
    else:
        inksnek.add_path(group, self.font.text((self.outerWidth - self.font.textDims(scale, txt)[0])/2.0, self.innerDepth - self.materialThickness, 0.2, txt), inksnek.etch_style)
    #inksnek.debug(self.font.font_defn)
  
  def addUSBPCB(self, group, inner):
    # represent USB breakout
    r = self.usbPCBHoleR if inner else self.usbPCBHeadHoleR
    inksnek.add_rect(group, 0, 0, self.usbPCBWidth, self.usbPCBDepth, inksnek.ignore_style)
    inksnek.add_hole(group, self.usbPCBHoleOffset[0], self.usbPCBHoleOffset[1], r)
    inksnek.add_hole(group, self.usbPCBWidth - self.usbPCBHoleOffset[0], self.usbPCBHoleOffset[1], r)
    if inner:
        inksnek.add_rect(group, 0, 0, self.usbPCBWidth, self.usbJointCutout, inksnek.cut_style)
      
  def innerBasePlate(self, group):
    # plate inside enclosure, on the base plate, support for USB
    inksnek.add_annotation(group, 3.5, 2.0, "INNER BASE")
    depth = self.innerDepth - self.frontPlatePCBThickness
    inksnek.add_rect(group, 0, 0, self.innerWidth, depth, inksnek.cut_style)
    inksnek.add_rect(group, 0, -self.frontPlatePCBThickness, self.innerWidth, self.frontPlatePCBThickness, inksnek.ignore_style, "LRB")
    usb = inksnek.add_group(group, inksnek.translate_group((self.innerWidth - self.usbPCBWidth)/2.0, depth - self.usbPCBDepth + self.materialThickness))
    self.addUSBPCB(usb, True)
    # potential cut-out to allow for display PCB
    if not self.V2: # tweak - not needed
        tab = 1
        gap = 2
        inksnek.add_rect(group, (self.innerWidth - (self.displayPCBWidth + gap))/2, tab, (self.displayPCBWidth + gap), 10, inksnek.cut_style, "TLR")

  def innerTopPlate(self, group):
    # plate inside enclosure, on the top plate, stop for PCB front plate
    inksnek.add_annotation(group, 3.5, 2.0, "INNER TOP")
    depth = self.innerDepth - self.frontPlatePCBThickness
    inksnek.add_rect(group, 0, 0, self.innerWidth, depth, inksnek.cut_style)
    inksnek.add_rect(group, 0, -self.frontPlatePCBThickness, self.innerWidth, self.frontPlatePCBThickness, inksnek.ignore_style, "LRB")
    # cut sensor cover
    self.sensorCover(group, self.innerWidth/2.0, self.innerHeight/2.0, inksnek.cut_style)
  
  def hole(self, group, x, y, r, style, r2 = None, r3 = None):
    inksnek.add_hole(group, x, y, r, style)
    if inksnek.mode == inksnek.PRINT:
        inksnek.add_X_marker(group, x, y, r) # mark the centre for checking
    if not r2 is None:
        inksnek.add_hole(group, x, y, r2, inksnek.ignore_style)
    if not r3 is None:
        inksnek.add_hole(group, x, y, r3, inksnek.ignore_style)
    
  def eInkBoard(self, group, cut):
    # represent the eInk PCB & display
    eink = inksnek.add_group(group, inksnek.translate_group(self.materialThickness, self.materialThickness))
    # PCB
    inksnek.add_rect(eink, (self.innerWidth - self.displayPCBWidth)/2.0, self.displayPCBOffsetY, self.displayPCBWidth, self.displayPCBHeight, inksnek.ignore_style)
    eink = inksnek.add_group(eink, inksnek.translate_group((self.innerWidth - self.displayPCBWidth)/2.0, self.displayPCBOffsetY))
    # display
    inksnek.add_rect(eink, (self.displayPCBWidth - self.displayScreenSize)/2.0, self.displayScreenOffsetY, self.displayScreenSize, self.displayScreenSize, inksnek.ignore_style)
    # holes
    style = inksnek.cut_style if cut else inksnek.ignore_style
    r2 = 3.0 if cut else None
    r = 3.75/2.0 # wiggle room
    self.hole(eink, self.displayHoleOffset[0], self.displayHoleOffset[1], r, style, r2)
    self.hole(eink, self.displayPCBWidth - self.displayHoleOffset[0], self.displayHoleOffset[1], r, style, r2)
    self.hole(eink, self.displayHoleOffset[0], self.displayPCBHeight - self.displayHoleOffset[1], r, style, r2)
    self.hole(eink, self.displayPCBWidth - self.displayHoleOffset[0], self.displayPCBHeight - self.displayHoleOffset[1], r, style, r2)
  
  def pcbHoles(self, group, cutHoles):
    # holes in front plate PCB
    holes = inksnek.add_group(group, inksnek.translate_group(self.materialThickness, self.materialThickness))
    dX = inksnek.perf_board_pitch*1.5
    dY = inksnek.perf_board_pitch*3.5
    style = inksnek.cut_style if cutHoles else inksnek.ignore_style
    r = 3.5/2.0 # wiggle room
    r2 = 3.0 if cutHoles else None
    self.hole(holes, dX, dY, r, style, r2)
    self.hole(holes, self.frontPlatePCBWidth - dX, dY, r, style, r2)
    self.hole(holes, dX, self.frontPlatePCBHeight - dY, r, style, r2)
    self.hole(holes, self.frontPlatePCBWidth - dX, self.frontPlatePCBHeight - dY, r, style, r2)
    
  def insideFrontPlate(self, group):
    # vertical plate inside enclosure, clamps to display PCB & front
    # height adjusted for base plates and shrunk for kerf 
    kerf = 0.0 # kerf srinkage not needed? there's kerf on the plate cuts
    inksnek.add_annotation(group, 16.0, 8.0, "INSIDE FRONT")
    radius = 2.0
    inksnek.add_round_rect(group, self.materialThickness + kerf, 2.0*self.materialThickness + kerf, self.innerWidth - 2.0*kerf, self.innerHeight - 2.0*self.materialThickness - 2.0*kerf, radius, inksnek.cut_style)
    self.eInkBoard(group, True)
    self.pcbHoles(group, True)
    # void for display connector
    hSpan = 21.0
    vSpan = 25.0
    inksnek.add_rect(group, self.materialThickness + (self.innerWidth - hSpan)/2.0, 2.0*self.materialThickness + self.innerHeight - 2.0*self.materialThickness - vSpan, hSpan, vSpan, inksnek.cut_style, "LRB")
    # some text to experiment with cleaning up etch if there's scorching
    inksnek.add_annotation(group, self.materialThickness + self.innerWidth/2.0, self.innerHeight - 2.0*self.materialThickness - 5.0, "TEST", 5.0, inksnek.etch_style, inksnek.CENTRE_ALIGN)
    # standoffs between inside plates
    self.insidePlateHoles(inksnek.add_group(group, inksnek.translate_group(self.materialThickness + kerf, 2.0*self.materialThickness + kerf)), False, False)
    if self.V2:
        # tweak - gap for pwr wires?
        hSpan = 18.0
        vSpan = 8.0
        inksnek.add_rect(group, (self.innerWidth - hSpan)/2.0 + self.materialThickness, 2.0*self.materialThickness + kerf, hSpan, vSpan, inksnek.cut_style, "TLR")

    
  def insideBackPlate(self, group):
    # vertical plate inside enclosure, hold project PCB etc
    # height adjusted for base plates and shrunk for kerf
    kerf = 0.0 # kerf shrinkage not needed? there's kerf on the plate cuts too
    inksnek.add_annotation(group, 16.0, 8.0, "INSIDE BACK")
    if self.V2:
        # vent tweak
        hSpan = 10.0
        vSpan = 5.0
        inksnek.add_rect(group, self.materialThickness + (self.innerWidth - hSpan)/2.0, 2.0*self.materialThickness + self.innerHeight - 2.0*self.materialThickness - vSpan, hSpan, vSpan, inksnek.cut_style, "LRB")
    # standoffs between inside plates
    self.insidePlateHoles(inksnek.add_group(group, inksnek.translate_group(self.materialThickness + kerf, 2.0*self.materialThickness + kerf)), False, False)
    radius = 2.0
    group = inksnek.add_group(group, inksnek.translate_group(self.materialThickness + kerf, 2.0*self.materialThickness + kerf))
    width = self.innerWidth - 2.0*kerf
    height = self.innerHeight - 2.0*self.materialThickness - 2.0*kerf
    inksnek.add_round_rect(group, 0, 0, width, height, radius, inksnek.cut_style)
    # void for USB connector
    hSpan = self.usbPCBWidth + 5.0
    vSpan = self.usbPCBHeight + 5.0
    inksnek.add_rect(group, (self.innerWidth - hSpan)/2.0, 0.0, hSpan, vSpan, inksnek.cut_style, "TLR")
    # project PCB
    self.projectPCB(inksnek.add_group(group, inksnek.translate_group((width - self.projectPCBSize)/2.0, (height - self.projectPCBSize)/2.0)), True)

    
  def projectPCB(self, group, cutHoles):
    # the project PCB with ATMEL etc
    inksnek.add_round_rect(group, 0, 0, self.projectPCBSize, self.projectPCBSize, 1, inksnek.ignore_style)
    holeStyle = inksnek.cut_style if cutHoles else inksnek.ignore_style
    for hole in range(2):
        self.hole(group, self.projectPCBHoleOffsets[2*hole], self.projectPCBHoleOffsets[2*hole + 1], 3.1/2.0, holeStyle, 6.0/2.0)
    for hole in range(4):
        self.hole(group, self.projectPCBSensorHoleOffset[0] + hole*inksnek.perf_board_pitch, self.projectPCBSensorHoleOffset[1], 1.5/2.0, inksnek.ignore_style)
    offs = self.projectPCBSensorDims[0]
    dims = self.projectPCBSensorDims[1]
    h = dims[1] if not cutHoles else 2*offs[1]  # inside plate, just room for header socket
    self.sensorCentreX = self.projectPCBSensorHoleOffset[0] + offs[0] + dims[0]/2
    self.sensorCentreY = self.projectPCBSensorHoleOffset[1] + offs[1] - h + h/2
    inksnek.add_rect(group, self.projectPCBSensorHoleOffset[0] + offs[0], self.projectPCBSensorHoleOffset[1] + offs[1] - h, dims[0], h, inksnek.cut_style)
  
  def insidePlateHoles(self, group, vent, back):
    # holes to join inside plates (and misc)
    inset = 5.0
    headR = 6.0/2.0
    holeR = 3.1/2.0 if not self.V2 or not back else 4.0/2.0 # tweak - a bit wider
    y = (self.innerHeight - 2.0*self.materialThickness)/2.0
    self.hole(group, inset, y, holeR, inksnek.cut_style, headR)
    self.hole(group, self.innerWidth - inset, y, holeR, inksnek.cut_style, headR)
    if back:
        # arrows indicating nuts to undo to open
        len = 6.0 # tweak - a bit longer
        inksnek.add_path(group, inksnek.path_move_to(inset, y - len - headR) + inksnek.path_arrow_to(inset, y - headR  - 0.75, len/2.0), inksnek.etch_style)
        inksnek.add_path(group, inksnek.path_move_to(self.innerWidth - inset, y - len - headR) + inksnek.path_arrow_to(self.innerWidth - inset, y - headR  - 0.75, len/2.0), inksnek.etch_style)
    if vent:    
        # vent
        #self.hole(group, self.innerWidth/2, self.innerHeight - 8.0, 1.5, inksnek.cut_style)
        x,y,w,h,r = self.innerWidth/2,self.innerHeight - self.ventOffsetY, self.usbPCBConnectorWidth, self.ventHeight, 1.0
        inksnek.add_round_rect(group, x - w/2.0, y - h/2.0, w, h, r, inksnek.cut_style)
        
  def sensorCover(self, group, x, y, style):
    # outside cap on sensor, centred at (x,y) holes
    dims = self.projectPCBSensorDims[1]
    size = max(dims[0], dims[1]) + 4.5
    inksnek.add_round_rect(group, x - size/2.0, y - size/2.0, size, size, 2.0, style)
    # slots
    r = 1.0 # m2 
    rHead = 3.5/2.0 # m2 head
    n = 5
    step = size/n
    x = x - size/2.0 + step/2.0
    y = y - size/2.0 + step/2.0
    self.hole(group, x + 0*step, y + 2*step, r, inksnek.cut_style, rHead)
    self.hole(group, x + 4*step, y + 2*step, r, inksnek.cut_style, rHead)
    inksnek.add_round_rect(group, x + 2*step - r, y + 0*step, 2.0*r, (n-1)*step, r, style)
    inksnek.add_round_rect(group, x + 1*step - r, y + 1*step, 2.0*r, 2*step, r, style)
    inksnek.add_round_rect(group, x + 3*step - r, y + 1*step, 2.0*r, 2*step, r, style)
    # grid of holes, nah
    # n = 5
    # step = size/n
    # x = x - size/2.0 + step/2.0
    # y = y - size/2.0 + step/2.0
    # for sy in range(n):
    #   for sx in range(n):
    #       stl = inksnek.cut_style if sy == int(n/2) and (sx == 0 or sx == n-1) else style
    #       inksnek.add_hole(group, x + sx*step, y + sy*step, 1.0, stl)
    
  def sunDecoration(self, group):
    # sun for top
    if not self.decorate:
        return
    r = 7
    inksnek.add_circle(group, self.outerWidth/2.0, self.outerDepth/2.0, r, inksnek.etch_style)
    rays = 24
    dAng = 360/rays
    for ang in range(rays):
        d = 2 if ang % 2 else 0
        path = inksnek.path_move_to(self.outerWidth/2.0, self.outerDepth/2.0)
        path += inksnek.path_move_by(inksnek.polar_to_rectangular(r*1.5 - d, ang*dAng))
        path += inksnek.path_line_by(inksnek.polar_to_rectangular(self.innerWidth/2 - r*1.75, ang*dAng))
        inksnek.add_path(group, path, inksnek.etch_style)
        
  def windDecoration(self, group):
    # wind for left
    if not self.decorate:
        return
    delta = 10
    r = 5
    x = self.outerHeight/2.0
    len = self.outerDepth - 2*delta
    path = inksnek.path_move_to(x, self.outerDepth - delta)
    path += inksnek.path_vert_by(-len)
    path += inksnek.path_arc(x + r, self.outerDepth - delta - len, -r, 270, 45)
    inksnek.add_path(group, path, inksnek.etch_style)
    delta = 5
    x = self.outerHeight/2.0 + r
    len = self.outerDepth - 5*delta
    path = inksnek.path_move_to(x, self.outerDepth - delta)
    path += inksnek.path_vert_by(-len)
    path += inksnek.path_arc(x + r, self.outerDepth - delta - len, -r, 270, 45)
    inksnek.add_path(group, path, inksnek.etch_style)
    delta = 7
    x = self.outerHeight/2.0 - r
    len = self.outerDepth - 5*delta
    path = inksnek.path_move_to(x, self.outerDepth - delta)
    path += inksnek.path_vert_by(-len)
    path += inksnek.path_arc(x - r, self.outerDepth - delta - len, r, 90, 315)
    inksnek.add_path(group, path, inksnek.etch_style)

  def calcArcIntersectionAngle(self, r1, r2, t, d):
    # two circles r1 & r2, r1 is on baseline, r2 is distance d above baseline
    # centers are distance t apart on baseline
    # r2 > r1 probably
    # returns angle in degrees from 12 O'Clock to the point where arcs intersect, on r2
    u = (r2 + d) - r1 # vertical separation between centres
    q = math.sqrt(t**2 + u**2) # length of line joining centres
    theta = math.atan2(t, u)# angle of line joining centres, from 6 O'Clock
    phi = math.acos((r2**2 + q**2 - r1**2)/(2.0*r2*q)) # subtended angle between line joining centres and line from r2 centre to intersection (cosine law)
    return 180.0 - math.degrees(theta + phi)

  def rainDecoration(self, group):
    # cloud+rain for right
    if not self.decorate:
        return
    scaleSize = 4.125
    rain = inksnek.add_group(group, inksnek.translate_group(self.outerHeight + scaleSize, 0.0) + inksnek.rotate_group(-90.0))
    # LHS small arc
    r1 = 2.0*scaleSize
    t1 = 2.5*scaleSize
    inksnek.add_arc(rain, self.outerDepth/2.0 - t1, self.outerHeight/2 + r1, r1, 180, 50, inksnek.etch_style)
    r2 = 3.0*scaleSize
    d = 0.0
    startAngle = -self.calcArcIntersectionAngle(r1, r2, t1, d)
    
    # RHS small arc
    r1 = 1.5*scaleSize
    t2 = 2.75*scaleSize
    inksnek.add_arc(rain, self.outerDepth/2.0 + t2, self.outerHeight/2 + r1, r1, 300, 180, inksnek.etch_style)
    endAngle = +self.calcArcIntersectionAngle(r1, r2, t2, d)
    # middle large arc touching small arcs
    inksnek.add_arc(rain, self.outerDepth/2.0, self.outerHeight/2 + r2 + d, r2, startAngle, endAngle, inksnek.etch_style)
    # line along bottom of cloud
    inksnek.add_line_by(rain, self.outerDepth/2.0 - t1, self.outerHeight/2, t1 + t2, 0, inksnek.etch_style)

    # rain lines
    lines = 5
    dX = 5.0*scaleSize/lines
    for line in range(lines):
        path = inksnek.path_move_to(self.outerDepth/2.0 - dX*lines/2 + line*dX + dX/2.0, self.outerHeight/2 - scaleSize/2.0)
        path += inksnek.path_line_by(-scaleSize/2.0, -2.5*scaleSize)
        inksnek.add_path(rain, path, inksnek.etch_style)
    
  def gerberHoles(self, group):
    # show the PCB holes
    g = inksnek.add_group(group, inksnek.translate_group(0.0, 0.0))
    gerber = read_gerber_holes()
    holes = gerber.read_zip_file("D:/dev/Inksnek/Designs/WeatherPage/Gerber_WeatherStationery_PCB_2025-04-04.zip")
    for hole in holes:
        inksnek.add_circle(g, hole[0], hole[1], hole[2]/2,inksnek.ignore_style)
    
    
  def effect(self):  # the main entry point for the design
    # initialise Inksnek
    self.materialThickness = 3.0
    inksnek.setup(self, inksnek.A4, inksnek.ACRYLIC, self.materialThickness, 'mm', inksnek.FINAL)
    
    self.V2 = True # acrylic with tweaks
    self.showRects = True
    self.decorate = True
    self.explode = True # separate the plates vs common cuts
    self.explodedGap = self.materialThickness if self.explode else -self.materialThickness
    self.kerf = 0.1 # just a guess about allowance to make for kerf
    self.font = StrokedFont()
    # the front plate (PCB)
    self.frontPlatePCBWidth = 45.720
    self.frontPlatePCBHeight = 62.230
    self.frontPlatePCBThickness = 1.2
    # make the inside a little bigger than the PCB because the enclosure will be a little smaller due to kerf on the box joints
    self.innerHeight = self.frontPlatePCBHeight + 2.0*self.kerf
    self.innerWidth = self.frontPlatePCBWidth + 2.0*self.kerf
    # inner depth is from the front edge to the inside of the back plate
    self.innerDepth  = 0.0
    #self.innerDepth += self.frontPlatePCBThickness
    #self.innerDepth += 3.4 # display screw heads + PCB
    self.innerDepth += 4.5 # front of front PCB to back of display PCB (measured)
    self.innerDepth += 18.0 # standoff to clamp plate
    self.innerDepth += 3.0 # front inside clamp plate
    self.innerDepth += 18.0 + 6.0  # standoffs to PCB plate
    self.innerDepth += 2*1.0 # nylon washers, for some play
    self.innerDepth += 3.0 # PCB plate
    self.innerDepth += 6.0 # standoff bewteen back plates
    self.innerDepth += 1.0 # nylon washer, for some play
    
    self.outerHeight = self.innerHeight + 2.0*self.materialThickness
    self.outerWidth = self.innerWidth + 2.0*self.materialThickness
    self.outerDepth = self.innerDepth + self.materialThickness
    self.fingerLength = 5.0
    self.fingersDepth = 3 # finger joints from front to back
    self.fingersHeight = 3 # finger joints from top to bottom
    self.fingersWidth = 3 # finger joints from left to right
    
    # eink
    self.displayPCBWidth = 33.0
    self.displayPCBHeight = 48.0
    self.displayHoleOffset = (2.5, 2.5) # dX, dY
    self.displayScreenSize = 28.0
    self.displayScreenOffsetY = 13.0 # on PCB, from non-connector end to display edge
    self.displayPCBOffsetY = 2.48 + 1.27 # from inside of enclosure to lower (from non-connector end)
    
    # usb breakout
    self.usbPCBWidth = 14.25
    self.usbPCBDepth = 15.0
    self.usbPCBHeight = 5.0 # including connector
    self.usbPCBConnectorWidth = 9.0
    self.usbPCBThickness = 2.0 # just the PCB
    self.usbPCBHoleR = 1.6 # m3+
    self.usbPCBHeadHoleR = 3.1 # m3+
    self.usbPCBHoleOffset = (3.0, 5.5) # dX, dY
    self.usbConnectorHeight = 15.0 # if there's a header strip/connector
    self.usbJointCutout = 2.5 # gap for solder joints for breakout to sit flat
    
    # project PCB
    self.projectPCBSize = 10*inksnek.perf_board_pitch # close enough, actually 25.8
    # not exact on PCB, but gap is exact
    self.projectPCBHoleOffsets = (3.5*inksnek.perf_board_pitch, 4.5*inksnek.perf_board_pitch, 6.5*inksnek.perf_board_pitch, 4.5*inksnek.perf_board_pitch) # dX dY from bottom-left
    self.projectPCBSensorHoleOffset = (3.5*inksnek.perf_board_pitch, 2.5*inksnek.perf_board_pitch) # dX dY from bottom-left to VCC
    self.projectPCBSensorDims = ((-2.0, +2.0), (12.0, 15.0)) # (dX, dY) from VCC & (width, height)
    self.sensorCentreX = self.sensorCentreY = 0
    # top vent
    self.ventOffsetY = 8.5
    self.ventHeight = 2.5
   
    design = inksnek.add_group(inksnek.top_group, inksnek.translate_group(8.0, 8.0))

    self.leftPanel(inksnek.add_group(design, inksnek.translate_group(0.0, 0.0)))
    self.topPanel(inksnek.add_group(design, inksnek.translate_group(self.outerHeight + self.explodedGap, 0.0)))
    self.rightPanel(inksnek.add_group(design, inksnek.translate_group(self.outerHeight + self.explodedGap + self.outerWidth + self.explodedGap, 0.0)))
    self.backPanel(inksnek.add_group(design, inksnek.translate_group(self.outerHeight + self.explodedGap, self.outerDepth + self.explodedGap)))
    self.bottomPanel(inksnek.add_group(design, inksnek.translate_group(0.0, self.outerDepth + self.materialThickness)))
    
    self.innerBasePlate(inksnek.add_group(design, inksnek.translate_group(self.outerWidth + self.outerHeight + self.explodedGap + 5, self.outerDepth + self.explodedGap + 5)))
    self.innerTopPlate(inksnek.add_group(design, inksnek.translate_group(self.outerWidth + self.outerHeight + self.explodedGap + 5, 2*(self.outerDepth + self.explodedGap) + 5)))
    gap = self.materialThickness if self.explode else 0.0
    self.insideFrontPlate(inksnek.add_group(design, inksnek.translate_group(0.0, 2.0*self.outerDepth + gap)))
    gap = 0 if self.explode else -2.0*self.materialThickness
    self.insideBackPlate(inksnek.add_group(design, inksnek.translate_group(self.outerWidth, self.outerDepth + self.outerHeight + gap)))
    
    self.gerberHoles(design)

    
if __name__ == '__main__':
    e = my_design()
    e.affect()
