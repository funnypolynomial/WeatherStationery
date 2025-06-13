#include <Arduino.h>
#include "Sensor.h"
#include "Config.h"
#include "Weather.h"

namespace Weather// FOR ENTERTAINMENT ONLY!
{

// This code emulates the Zambretti Forecaster (https://en.wikipedia.org/wiki/Zambretti_Forecaster)  
// It's not going to be accurate, consider it entertainment; don't use it to guide planting your crops.
// The forecaster is a physical, analog, device consisting of three disks. 
// Dialing in wind direction and barometric pressure at sea level reveals letters in three windows, Rising, Falling and Steady.
// These relate to the pressure trend over the last three hours; Steady is less than 1.6hPa change.
// The Rising and Falling windows have arrows marked W and S for Winter and Summer.
// The "forecast" is found by reading the letter in the appropriate window, at the appropriate arrow, if applicable.
// On the back of the device is a table of forecasts corresponding to letters, for example, A is "Settled fine".
//
// This is implemented below as follows:
// The pressure read from the sensor is first transformed:
//  * to sea level, if CONFIG_ALTITUDE_METERS is defined
// The result is used as the current pressure, and the trend is calculated from the reading 3 hours ago.
// Readings are taken on the hour and half hour.
// The conversion of the current pressure to a letter, for a given trend, is done via the p<Trend>PressureTable[]'s below.
// Season is ignored
// Wind is ignored.
// The forecast is shown as the text and an icon.
// For the icon, the assumption is that they go from left to right from good to bad, and so do the letters A-Z
// All pressures are in deca Pascals, dPa = hPa*10  In other words, 1 DP of pressure in hPa. Temperatures are in C

typedef int16_t tPressure;  // dPa

const tPressure kNullPressure = 0;  // Don't have a value
const tPressure kPressureTrendThreshold = 16; // i.e. 1.6hPa
// The algorithm works with pressures in this range
const tPressure ZambrettiMinPressure =  9470;
const tPressure ZambrettiMaxPressure = 10500;

uint32_t loopTimerMS = 0;
int currentTemperature = 0;
int currentHumidity = 0;
tPressure currentPressure = kNullPressure;  // unadjusted
tPressure adjustedPressure = kNullPressure;  // adjusted for MSL and range. Used in forecast
char currentForecastLetter = '?';
char currentTrendLetter = '?';

#define NUM_READINGS 6  // 3 hours worth, every half hour
tPressure pressureReadings[NUM_READINGS];  // [0] is oldest. Adjusted
uint32_t kReadingTimeoutMS = 50UL;

// My family says this:
//#define STORMY "Stormy-ormy"
#define STORMY "Stormy"

// A '\0' denotes a line break
//            123456789012345678901234 (24 chars per line)
#define FORECASTS \
    FCAST(A, "Settled fine.\0 ") \
    FCAST(B, "Fine weather.\0 ") \
    FCAST(C, "Becoming fine.\0 ") \
    FCAST(D, "Fine,\0" \
             "becoming less settled.") \
    FCAST(E, "Fine,\0" \
             "possible showers.") \
    FCAST(F, "Fairly fine,\0" \
             "improving.") \
    FCAST(G, "Fairly fine,\0" \
             "possible showers early.") \
    FCAST(H, "Fairly fine,\0" \
             "showery later.") \
    FCAST(I, "Showery early,\0" \
             "improving.") \
    FCAST(J, "Changeable,\0" \
             "mending.") \
    FCAST(K, "Fairly fine,\0" \
             "showers likely.") \
    FCAST(L, "Rather unsettled,\0" \
             "clearing later.") \
    FCAST(M, "Unsettled,\0" \
             "probably improving.") \
    FCAST(N, "Showery,\0" \
             "bright intervals.") \
    FCAST(O, "Showery,\0" \
             "becoming more unsettled.") \
    FCAST(P, "Changeable,\0" \
             "some rain.") \
    FCAST(Q, "Unsettled,\0" \
             "short fine intervals,") \
    FCAST(R, "Unsettled,\0" \
             "rain later.") \
    FCAST(S, "Unsettled,\0" \
             "rain at times.") \
    FCAST(T, "Very unsettled,\0" \
             "finer at times.") \
    FCAST(U, "Rain at times,\0" \
             "worse later.") \
    FCAST(V, "Rain at times,\0" \
             "becoming very unsettled.") \
    FCAST(W, "Rain,\0" \
             "at frequent intervals.") \
    FCAST(X, "Very unsettled,\0" \
             "rain.") \
    FCAST(Y, STORMY",\0" \
             "possibly improving.") \
    FCAST(Z, STORMY",\0" \
             "much rain.")
             
// Multi-str. Single string const with multiple strings, 0 delimited. double 0 at the end
#define FCAST(_letter, _str) _str "\0"
const char pForecastMStr[] PROGMEM = FORECASTS;

const char* GetForecastStr(char letter, const char*& pLine2)
{
  // returns the forecast corresponding to the letter, from the mstr above, or null
  pLine2 = NULL;
  if (::isalpha(letter))
  {
    const char* pStr = pForecastMStr;
    while (letter-- != 'A')
    {
      while (pgm_read_byte_near(pStr++))
        ;
      while (pgm_read_byte_near(pStr++))
        ;
    }
    pLine2 = pStr;
    while (pgm_read_byte_near(pLine2++))
      ;
    return pStr;
  }
  return nullptr;
}

tPressure AdjustedPressure(tPressure pressure)
{
  // Adjust to sea level
#ifdef CONFIG_ALTITUDE_METERS  
  float pressure_hPa = (float)(pressure)/10.0f;
  return (tPressure)(0.5 + 10*pressure_hPa * pow(1.0 - (0.0065 * CONFIG_ALTITUDE_METERS) / (currentTemperature + (0.0065 * CONFIG_ALTITUDE_METERS) + 273.15), -5.257));
#else
  return pressure;
#endif  
}

// 
#define PRESSURE_OFFSET 947 // tables below store the difference from this, in a byte, hPa, at MSL
#define P(_p) ((char)((_p) - PRESSURE_OFFSET))

// Rising pressure
const char pRisingPressureTable[]  PROGMEM = {    'A',     'B',     'C',     'F',     'G',     'I',     'J',     'L',     'M',     'Q',     'T',     'Y',     'Z',     '\0',
                                              P(1030), P(1022), P(1012), P(1007), P(1000), P( 995), P( 990), P( 984), P( 978), P( 970), P( 965), P( 959), P( 947)};
// Falling pressure
const char pFallingPressureTable[] PROGMEM = {    'A',     'B',     'D',     'H',     'O',     'R',     'U',     'V',     'X',     '\0',
                                              P(1050), P(1040), P(1024), P(1018), P(1010), P(1004), P( 998), P( 991), P( 985)};
// Steady pressure
const char pSteadyPressureTable[]  PROGMEM = {    'A',     'B',     'E',     'K',     'N',     'P',     'S',     'W',     'X',     'Z',     '\0',
                                              P(1033), P(1023), P(1014), P(1008), P(1000), P( 994), P( 989), P( 981), P( 974), P( 960)};

tPressure ReadTablePressure(const char* pPressures)
{
  // read a pressure from one of the tables above
  return 10*(pgm_read_byte_near(pPressures) + PRESSURE_OFFSET);
}

char LookupForecast(tPressure currentPressureMSL, const char* pTable, int8_t seasonAdjustment)
{
  // pTable is <letters><NUL><offset pressures>
  // Returns the letter corresponding to the table pressure closest to currentPressure, or ' ' if off table
  // seasonAdjustment is +1/0/-1
  const char* pLetters = pTable;
  const char* pPressures = pTable + strlen_P(pTable) + 1;
  tPressure tablePressure = ReadTablePressure(pPressures++);
  if (currentPressureMSL > tablePressure)
    return ' '; // outside range
  const char* pClosestLetter = pLetters++;
  tPressure minDiff = abs(currentPressureMSL - tablePressure);

  while (pgm_read_byte_near(pLetters))
  {
    tablePressure = ReadTablePressure(pPressures);
    tPressure diff = abs(currentPressureMSL - tablePressure);
    if (diff < minDiff)
    {
      minDiff = diff;
      pClosestLetter = pLetters;
    }
    pLetters++;
    pPressures++;
  }
  if (currentPressureMSL < tablePressure)
    return ' ';  // outside range
    
  if (seasonAdjustment == +1 && pgm_read_byte_near(pClosestLetter + 1))
    pClosestLetter++;
  else if (seasonAdjustment == -1 && pClosestLetter != pTable)
    pClosestLetter--;

  return pgm_read_byte_near(pClosestLetter);
}

void Init()
{
  // N/A values
  for (int i = 0; i < NUM_READINGS; i++)
    pressureReadings[i] = kNullPressure;
  Sensor::Read(currentPressure, currentTemperature, currentHumidity);
  loopTimerMS = millis();
}

bool Loop()
{
  // Takes a pressure readings every half hour
  // Pressure is adjusted to MSL
  // Updates the forecast based on the current adjusted pressure and the trend from the most recent reading and one 3 hours ago (adjusted)
  // Based on
  // https://communities.sas.com/t5/Streaming-Analytics/Zambretti-Algorithm-for-Weather-Forecasting/td-p/679487
  // or https://integritext.net/DrKFS/zambretti.htm

#ifdef DEMO
  {
    currentPressure = 1000;
    pressureReadings[1] = currentPressure*10 - kPressureTrendThreshold; // rising
    currentTemperature = 25;
    currentHumidity = 50;
#else    
  uint32_t nowMS = millis();
  if ((nowMS - loopTimerMS) > 30UL*60000UL)
  {
    loopTimerMS = nowMS;
    Sensor::Read(currentPressure, currentTemperature, currentHumidity);
#endif    
    adjustedPressure = AdjustedPressure(currentPressure*10);
      
    for (int i = 1; i < NUM_READINGS; i++)
      pressureReadings[i-1] = pressureReadings[i];
    pressureReadings[NUM_READINGS - 1] = adjustedPressure;
    tPressure oldPressure = pressureReadings[0];
      
    currentForecastLetter = '?';
    if (oldPressure != kNullPressure)
    {
      if ((adjustedPressure - oldPressure) >= +kPressureTrendThreshold)
      {
        currentTrendLetter = 'R';
        currentForecastLetter = LookupForecast(adjustedPressure, pRisingPressureTable, 0);
      }
      else if ((adjustedPressure - oldPressure) <= -kPressureTrendThreshold)
      {
        currentTrendLetter = 'F';
        currentForecastLetter = LookupForecast(adjustedPressure, pFallingPressureTable, 0);
      }
      else
      {
        currentTrendLetter = 'S';
        currentForecastLetter = LookupForecast(adjustedPressure, pSteadyPressureTable, 0);
      }
    }
    return true;
  }
  return false;
}

char GetForecastLetter()
{
  // '?' means N/A
  return currentForecastLetter;
}

char GetPressureTrend()
{
  return currentTrendLetter;
}

int GetPressure()
{
  return currentPressure;
}

int GetTemperature()
{
  return currentTemperature;
}

int GetHumidity()
{
  return currentHumidity;
}

void GetInfo(int16_t& currentRawP, int16_t& currentAdjP, int16_t& oldAdjP, char& trend)
{
  currentRawP = currentPressure;
  currentAdjP = adjustedPressure;
  oldAdjP = pressureReadings[0];
  trend =  currentTrendLetter;
}

}
