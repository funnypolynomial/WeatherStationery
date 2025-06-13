#pragma once

// Schematic
// (non-prototype)
//        +-----------------+
//  [SDA]-+(SDA)  <1>  (SCL)+-[SCL]             +------------+ 
//        |                 |                   | BME/BMP280 | 
// [BUSY]-+A0    LEO     ~D9+-[CS]              |            | 
//        |      TINY       |            [5VDC]-+VIN         | 
//  [N/C]-+A1           ~D10+-[DC]              |            |
//        |                 |             [GND]-+GND         |
//  [N/C]-+A2           ~D11+-[RST]             |            | 
//        |                 |             [SCL]-+SCL         | 
// [5VDC]-+"+"           "0"+-[GND]             |            | 
//        |                 |             [SDA]-+SDA         | 
// [MOSI]-+(MOSI) <2>   (SCK)+-[SCK]            +------------+ 
//        +------|USB|------+
//
//
//      +------------------------------+
//      |                              |
//      |                          BUSY+-[BUSY]
//      |                           RST+-[RST] 
//      |                            DC+-[DC] 
//      |       eInk Display         CS+-[CS]  
//      |         200x200           CLK+-[SCK]
//      |                           DIN+-[MOSI]
//      |                           GND+-[GND]
//      |                           VCC+-[5VDC]
//      |                              |
//      +------------------------------+
//
// Notes:
// Leonardo Tiny: https://www.jaycar.co.nz/leonardo-tiny-atmega32u4-main-board/p/XC4431
// eInk Display: https://www.jaycar.co.nz/duinotech-arduino-compatible-1-54-inch-monochrome-e-ink-display/p/XC3747
// Sensor: https://www.trademe.co.nz for my BME/BMP280, no longer listed, search for BME/BMP280/GYBMEP?
//   https://www.jaycar.co.nz/duinotech-arduino-compatible-barometric-pressure-sensor/p/XC3702 ("SPL06/BME", no humidity, stock varies?)
// Matching [LABELS] are connected.
//  <1>: SDA & SCL are pads on the underside of the Leo
//  <2>: MOSI & SCK are ICSP pads on the underside of the Leo

#if defined(ARDUINO_AVR_LEONARDO_ETH)
// Leo Tiny -- final
#define PIN_DISPLAY_BUSY  A0 
#define PIN_DISPLAY_RST   11
#define PIN_DISPLAY_DC    10
#define PIN_DISPLAY_CS    9

// SPI
#define PIN_DISPLAY_SCK   15
#define PIN_DISPLAY_MOSI  16

#elif defined(ARDUINO_AVR_UNO)
// Uno -- prototype
#define PIN_DISPLAY_BUSY  7 
#define PIN_DISPLAY_RST   8
#define PIN_DISPLAY_DC    9
#define PIN_DISPLAY_CS    10

// SPI
#define PIN_DISPLAY_SCK   13
#define PIN_DISPLAY_MOSI  11
#else
board not implemented!
#endif
