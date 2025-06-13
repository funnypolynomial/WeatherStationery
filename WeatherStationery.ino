#include <Arduino.h>
#include <SPI.h>
#include <Wire.h>
#include "Config.h"
// One (optional) external library:
#ifdef CONFIG_SENSOR_BME_BMP280
#include <pocketBME280.h>
#endif
#include "Display.h"
#include "Sensor.h"
#include "Page.h"

//                                           W e a t h e r S t a t i o n e r y
// --- What:
//  This project shows the weather conditions (air pressure, temperature, humidity) and a "forecast" on a 200x200
//  1.54 inch e-Paper display using a stroked font and icons.
// 
//  Starting from the top of the display:
//   * The current pressure is shown in hPa (or inHg)
//   * The pressure trend (rising/falling/steady) is show, or N/A
//   The current pressure, optionally reduced to sea level, and the pressure trend are used to infer a "forecast" 
//   by emulating the Zambretti Forecaster (https://en.wikipedia.org/wiki/Zambretti_Forecaster). This reuses code
//   from my Chrondrian project (https://hackaday.io/project/195665-chrondrian). 
//   The result is a forecast letter from 'A' ("Settled fine.") to 'Z' (Stormy, much rain.").
//   This range is mapped to an icon in (Sun, Cloud/Sun, Cloud, Cloud/Sun/Rain, Cloud/Rain, Cloud/Lightning). 
//   * The icon is shown.
//   * The descriptive text corresponding to the forecast letter is shown.
//   * Finally, the temperature (C or F) and the humidity (%) are shown together.
// 
//  A sensor reading (from a BME/BMP280 *OR* an SPL06/BME) is taken every half hour and the display updated.
//  The pressure trend is over 3 hours (6 readings). Prior to having a trend, it and the forecast are shown 
//  blank or N/A, but optionally, a random forcast, greyed out, can be shown, for entertainment.
//  In general, the forecast should *NOT* be taken seriously!
//  The display is https://www.jaycar.co.nz/duinotech-arduino-compatible-1-54-inch-monochrome-e-ink-display/p/XC3747
//  The sensor is, for example, https://www.jaycar.co.nz/duinotech-arduino-compatible-barometric-pressure-sensor/p/XC3702
//
// --- Why:
//  Mainly an excuse to play with a new display. I've done a fair bit with LCD's and the ePaper interface is 
//  extremely limited and the refresh rate very slow, so this wasn't about frame rates/fast updates for a change.
//  I've also done a lot with displaying text on an LCD, with a variety of bitmap fonts, many of which I've crafted
//  myself.  The *paper* aspect of this display led me to create a "stroked" font. This consists of line segments and
//  arc quadrants encoded in a fairly compact form, 740 bytes. 
//
// --- How:
//  The Arduino doesn't have the RAM to create a frame buffer, but things like rendering the forecast icon and 
//  drawing the lines and arcs in the stroked font are much easier if you can access a buffer of pixels.
//  I took the idea of a "sparse" algorithm from things like my ElitePetite (https://hackaday.io/project/183107-elitepetite)
//  and LittleZone (https://hackaday.io/project/185693-littlezone) and wrote a new one, simpler and not focussed on 
//  speed -- SparseInk.  This provides a "virtual" frame buffer which compresses the few pixels that are actually on, 
//  and is row-oriented, so sending the pixel information to the display is straight-forward.
//  Additionally, the display is built from the top down, in bands, so the number of pixels stored at one time is small.
//  The stroked font lines are a single pixel wide so the text is sometimes repeated offset by a pixel in the x and/or y 
//  direction for emphasis (bold).  There is also a slanted (italic) effect. Text and icons are dithered with grey pixels
//  to reduce their intensity.
//
// --- Configuration:
//  Several things can be configured via defines in Config.h. Units, sensor etc.
//
// --- Who:
// Mark Wilson, June 2025

void setup() 
{
  Sensor::Init(); // start it taking readings early
#if defined(DEBUG) || defined(DISPLAY_SERIALIZE)  
  Serial.begin(38400);
  Serial.println("WeatherStationery");
#endif  
  Page::Init();
#ifndef DEBUG
  Page::Splash(); 
  delay(3000);
#endif  
  Page::Loop();  
}


void loop() 
{
  Page::Loop();
}
