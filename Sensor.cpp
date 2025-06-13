#include <Arduino.h>
#include "Config.h"
#ifdef CONFIG_SENSOR_BME_BMP280
#include <pocketBME280.h>
#endif
#include "Sensor.h"
#include <Wire.h>

namespace Sensor
{
int32_t Round(int32_t value, int32_t divisor)
{
  // return value/divisor, rounded
  int32_t result = value/divisor;
  if ((abs(value) % divisor) >= (divisor / 2))
    result += (result < 0)?-1:+1;
  return result;
}
  
#ifdef CONFIG_SENSOR_BME_BMP280  
// BME280: read pressure, temperature & humidity
// Uses pocketBME280
// https://github.com/angrest/pocketBME280
pocketBME280 bme;

void Init()
{
  Wire.begin();
  bme.begin();
}

bool Read(int16_t& pressurehPa, int& temperatureC, int& humidityPercent)
{
  // return true if read all three
  bme.startMeasurement();
  int ctr = 100; // avoid infinite loop
  while (!bme.isMeasuring() && ctr--) 
    delay(1);
  while (bme.isMeasuring() && ctr--)
    delay(1);
  if (ctr <= 0)
    return false;
  temperatureC = Round(bme.getTemperature(), 100);
  pressurehPa = Round(bme.getPressure(), 100);
  uint32_t rawHumidity = bme.getHumidity();
  if (rawHumidity == 0)
    humidityPercent = -1; // consider it N/A
  else
    humidityPercent = Round(rawHumidity, 1024);
  return true;
}

uint32_t GetEntropy()
{
  // get some pseudo-randomness
  return bme.getTemperature() + bme.getPressure() + bme.getHumidity() + millis();
}
#else ////////////////////////////////////////////////////////
// SPL06/BME: read pressure & temperature
// A simple implementation of SPL06-007 pressure/temperature reading
// Based on https://github.com/rv701/SPL06-007, which at time of writing (Nov '23) is broken.
// Note that we DON'T check *_RDY flags
#define SPL06_I2C_ADDR 0x76
#define FLOAT float // float or double
enum RegisterIndices {PSR_B2, PSR_B1, PSR_B0,  
                      TMP_B2, TMP_B1, TMP_B0,   
                      PRS_CFG, TMP_CFG, 
                      MEAS_CFG, CFG_REG,   
                      COEF = 0x10};

// Oversampling
//  bits    value      scale    scale hex    
//  0b0000:    1x     524288   0x00080000
//  0b0001:    2x    1572864   0x00180000
//  0b0010:    4x    3670016   0x00380000
//  0b0011:    8x    7864320   0x00780000
//  Higher values use FIFO, untested

// 8x from above
#define OVERSAMPLE_BITS   0b0011
#define OVERSAMPLE_SCALE 7864320.0
  
void Write(uint8_t idx, uint8_t value)
{
  // Write the value at the given register/byte index
  Wire.beginTransmission(SPL06_I2C_ADDR);
  Wire.write(idx);
  Wire.write(value);
  Wire.endTransmission();  
}

uint8_t Read(uint8_t idx)
{
  // Return the value of the given register/byte index
  Wire.beginTransmission(SPL06_I2C_ADDR);
  Wire.write(idx); 
  Wire.endTransmission();
  
  Wire.requestFrom(SPL06_I2C_ADDR, 1);
  return Wire.read();
}

int32_t ReadValue(uint8_t I, uint8_t N, uint8_t M = 0)
{
  // Read a 24 or 16 bit value, MSB first, starting with the I'th register byte
  // Then, starting at bit M, extract N bits as a 2's complement value
  // Convert that to 32 bit 2's complement value

  // Deals with the general case of COEF values spread across multiple bytes
  // and also the simpler case of multi-byte values
  
  uint32_t val = 0UL;
  uint8_t P = (N > 16)?3:2;
  for (int idx = 0; idx < P; idx++)
    val = (val << 8) | Read(I + idx);
     
  uint32_t hiBit = 1UL << N;
  uint32_t mask  = hiBit - 1UL;
  hiBit >>= 1;
  
  // Shift down so M'th is least significant:
  val >>= M;
  // Mask off bits above N'th:
  val &= mask;
  
  // Sign-extend if -ve:
  if (val & hiBit)
    val |= 0xFFFFFFFFL & ~mask;
    
  return (int32_t)val;
}

void Init()
{
  // Init the device
  Wire.begin();
  delay(100);
  Write(PRS_CFG, OVERSAMPLE_BITS);               // 8x oversample, rate is N/A
  Write(TMP_CFG, 0b10000000 | OVERSAMPLE_BITS);  // External sensor, 8x oversample, rate is N/A
  Write(MEAS_CFG, 0b111);                        // continuous pressure and temperature reading
  Write(CFG_REG, 0x00);                          // no FIFO
}

bool Read(int16_t& pressurehPa, int& temperatureC, int& humidityPercent)
{
  // returns true
  // pressure in Pascals
  FLOAT c00 = ReadValue(COEF +  3, 20, 4); // could probably read all these once, at the start!
  FLOAT c10 = ReadValue(COEF +  5, 20);  
  FLOAT c01 = ReadValue(COEF +  8, 16);
  FLOAT c11 = ReadValue(COEF + 10, 16); 
  FLOAT c20 = ReadValue(COEF + 12, 16);
  FLOAT c21 = ReadValue(COEF + 14, 16);
  FLOAT c30 = ReadValue(COEF + 16, 16);
  
  FLOAT scaledT  = ReadValue(TMP_B2, 24)/OVERSAMPLE_SCALE;
  FLOAT scaledP  = ReadValue(PSR_B2, 24)/OVERSAMPLE_SCALE;
  pressurehPa = (int16_t)((c00 + scaledP*(c10 + scaledP*(c20 + scaledP*c30)) + scaledT*(c01 + scaledP*(c11 + scaledP*c21)))/100.0 + 0.5);
  
  // temperature in Celcius
  FLOAT c0 = ReadValue(COEF + 0, 12, 4);
  FLOAT c1 = ReadValue(COEF + 1, 12);
  temperatureC = (int)((c0/2.0) + (c1*scaledT) + 0.5);

  // pressure N/A on SPL06/BME
  humidityPercent = -1;
  return true;
}

uint32_t GetEntropy()
{
  // get some pseudo-randomness
  return ReadValue(TMP_B2, 24) + ReadValue(PSR_B2, 24) + millis();
}

#endif

};
