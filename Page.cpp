#include <Arduino.h>
#include "Config.h"
#include "Display.h"
#include "Sensor.h"
#include "Graphics.h"
#include "StrokedFont.h"
#include "SparseInk.h"
#include "Weather.h"
#include "Page.h"

namespace Page
{
#define ITOA(_value, _buffer) ::itoa((_value), (_buffer), 10)

  // pure grey is very faint! -- dither instead, alternate pixels are non-black
#ifdef DITHER_UNITS  
  bool ditherText = true;
#else
  bool ditherText = false;
#endif  
  bool ditherForecast = false;
  Display::Colour ditherAltColour = Display::MonoGrey;
  
#define NUM_RULE_BLOCKS 2
  byte ruleData[2*NUM_RULE_BLOCKS] = {0xFF, 0xFF, 0xFF, 0xFF}; // start/end cols
  void RuleFunc(int row)
  {
    // rule-based colouration (etc), called by SparseInk just before row buffer is sent
    if (ditherText || ditherForecast)
      // lighten alternate pixels
      for (int band = 0; band < NUM_RULE_BLOCKS; band++)
        for (int col = ruleData[2*band]; col <= ruleData[2*band+1]; col++)
        {
          if (Display::GetRowBufferAt(col) == Display::MonoBlack && (col % 2) != (row % 2))
            Display::SetRowBufferAt(col, ditherAltColour);
        }

#ifdef FOLD_CORNER    
    // fold corner for that "paper" look
    const int foldSize = 15;
    int r = row - (DISPLAY_HEIGHT - foldSize);
    if (r >= 0)
    {
      for (int i = 0; i < r; i++)
        Display::SetRowBufferAt(DISPLAY_HEIGHT - i - 1, Display::MonoBlack);
      for (int i = 0; i < foldSize - r; i++)
        Display::SetRowBufferAt(DISPLAY_HEIGHT - foldSize + i, r==0 ? Display::MonoBlack : Display::MonoGrey);
      if (r)
      {
        Display::SetRowBufferAt(DISPLAY_HEIGHT - foldSize, Display::MonoBlack);
        Display::SetRowBufferAt(DISPLAY_HEIGHT - r, Display::MonoBlack);
      }
    }
#endif    
  }

  byte sectionStart = 0xFF; // the first row in an updated section
  Display::Colour foreground = Display::MonoBlack;
  Display::Colour background = Display::MonoWhite;

  void SendRows(byte y)
  {
    // start/end a section of the display, built up as we go down
    if (sectionStart == 0xFF)
    {
      // first.
      sectionStart = y;
      SparseInk::Clear();
    }
    else if (sectionStart < y)
    {
      // next. send rows from the start to here (y), update start
      SparseInk::SendRows(sectionStart, y, foreground, background);
      sectionStart = y + 1;
      SparseInk::Clear();
    }
    if (y == DISPLAY_HEIGHT - 1) // reset
      sectionStart = 0xFF;
  }

// style flags passed to Text below
#define TXT_NORMAL  0b00001000  // plain test
#define TXT_DBL_VT  0b00001010  // repeat text 1 pixel below
#define TXT_DBL_HZ  0b00001100  // repeat text 1 pixel right
#define TXT_QUAD    0b00001111  // repeat text offs (1, 0), (0, 1) & (1, 1)
#define TXT_SPLIT   0b00010000  // split the test in half vertically, send each half to the display separately
#define TXT_ITALIC  0b00100000  // slant the text
#define TXT_CENTRE  0b01000000  // centre the text
#define TXT_NOSEND  0b10000000  // don't automatically send the test to the display

  char strBuffer[32];
  void Text(int x0, int y0, const char* pText, int scaleNum, int scaleDen, int charGap, uint8_t flags)
  {
    // if pText is *NOT* strBuffer, it is copied from PROGMEM to strBuffer
    // offset bits in flags are 0b0000PWRS for 
    //    PQ
    //    RS 
    // where P is (x0, y0). P is always drawn
    if (pText != strBuffer)
      pText = strcpy_P(strBuffer, pText);
    if (flags & TXT_ITALIC)
      StrokedFont::SetItalic(1, 4);
    if (flags & TXT_CENTRE)
      x0 = (DISPLAY_WIDTH - StrokedFont::Width(pText, scaleNum, scaleDen, charGap))/2;
    int lastRow = 0, loops = 1;
    if (flags & TXT_SPLIT)
    {
      loops = 2;
      lastRow = y0 + StrokedFont::Height(scaleNum, scaleDen, false)/2;
      StrokedFont::SetClip(y0, lastRow);
    }
    for (int loop = 0; loop < loops; loop++)
    {
      if (loop)
        StrokedFont::SetClip(lastRow + 1, DISPLAY_HEIGHT - 1);
      StrokedFont::DrawText(x0, y0, pText, scaleNum, scaleDen, charGap);
      if (flags & 0b0100)
        StrokedFont::DrawText(x0 + 1, y0, pText, scaleNum, scaleDen, charGap);
      if (flags & 0b0010)
        StrokedFont::DrawText(x0, y0 + 1, pText, scaleNum, scaleDen, charGap);
      if (flags & 0b0001)
        StrokedFont::DrawText(x0 + 1, y0 + 1, pText, scaleNum, scaleDen, charGap);
      if (!(flags & TXT_NOSEND))
      {
        SendRows(min(StrokedFont::cursorY, DISPLAY_HEIGHT - 1));
        SparseInk::Clear();
      }
    }
    StrokedFont::SetItalic(0, 0);
    StrokedFont::SetClip(0, 0);
  }

  bool firstLoop = true;
  void Init()
  {
    randomSeed(Sensor::GetEntropy()); // introduce some randomness
    Display::Init();
    Weather::Init();
    firstLoop = true;
  }

  const char pProgramNameStr[] PROGMEM = "WeatherStationery";
  const char pCreditStr[] PROGMEM = "MEW fecit MMXXV";
  //#define SPLASH_CREDIT_RED   // hard to read, but a good test
  void Splash()
  {
    // draw a splash, with the name, credit and all the icons
    foreground = Display::MonoBlack;
    background = Display::MonoWhite;
    Display::StartMono();
    SparseInk::SetRuleCallback(nullptr);
    SendRows(0);
    int num = 1, den = 1, gap = 5;
    Text(0, 5, pProgramNameStr, num, den, gap, TXT_QUAD | TXT_CENTRE | TXT_SPLIT);

    const int rows = 3;
    const int cols = Graphics::NumWeatherIcons/rows;
    const int rowHeight = Graphics::WeatherHeight() + 5;
    int x = (DISPLAY_WIDTH  - cols*Graphics::WeatherWidth())/2;
    int y = max((DISPLAY_HEIGHT - rows*rowHeight)/2, StrokedFont::cursorY + 1);
    for (int row = 0; row < rows; row++)
    {
      for (int col = 0; col < cols; col++)
        Graphics::Weather(x + col*Graphics::WeatherWidth(), y, row*cols + col);
      y += rowHeight;
      SendRows(y);
    }

    num = den = 1;
    gap = 0;
#ifdef SPLASH_CREDIT_RED
    // trailing rows of monochrome
    SendRows(DISPLAY_HEIGHT - 1);

    // red part
    Display::StartRed();
    foreground = Display::ColourRed;
    background = Display::ColourNone;
    SendRows(0);
    Text(0, DISPLAY_HEIGHT - StrokedFont::Height(num, den, false) - 3, pCreditStr, num, den, gap, TXT_CENTRE | TXT_ITALIC);
    // trailing rows of red
    SendRows(DISPLAY_HEIGHT - 1);
#else
    Text(0, DISPLAY_HEIGHT - StrokedFont::Height(num, den, false) - 3, pCreditStr, num, den, gap, TXT_CENTRE | TXT_ITALIC);
    // trailing rows
    SendRows(DISPLAY_HEIGHT - 1);

    Display::StartRed();
    Display::FillRowBuffer(Display::ColourNone);
    for (int row = 0; row < DISPLAY_HEIGHT; row++)
      Display::SendRowBuffer();
#endif
    Display::Refresh();
  }

  const char pStableTrendStr[] PROGMEM = "- STABLE -";
  const char pRisingTrendStr[] PROGMEM = "\x18 RISING \x18";
  const char pFallingTrendStr[] PROGMEM = "\x19 FALLING \x19";
  const char pNAStr[] PROGMEM = "N/A";
  const char phPaStr[] PROGMEM = "hPa";
  const char pinHgStr[] PROGMEM = "inHg";
  const char pCelsiusStr[] PROGMEM = "\xB0""C";
  const char pFahrenheitStr[] PROGMEM = "\xB0""F";
  uint8_t updateCounter = 0;
  
  void Paint(int pressure_hPa, char forecastLetter, char pressureTrend, int temperature_C, int humidity_Percent)
  {
    // does most of the work building up the page from the "top"
    foreground = Display::MonoBlack;
    background = Display::MonoWhite;
    Display::StartMono();
    SparseInk::SetRuleCallback(RuleFunc);
    const char* pStr;
    bool randomForecast = false;
#ifdef RANDOM_FORECAST_IF_NONE    
    if (!::isalpha(forecastLetter))
    {
      forecastLetter = random('A', 'Z' + 1);
      randomForecast = true;
    }
#endif      
    // **************** pressure
    SendRows(0);
#ifdef DEBUG
    ITOA(updateCounter, strBuffer);
    Graphics::PaintUpdateCounter(strBuffer);
    StrokedFont::DrawChar(DISPLAY_WIDTH - 10, 0, forecastLetter, 1);
#endif
    int num = 5, den = 2, gap = 0;
#ifdef CONFIG_HECTO_PASCALS
    ITOA(pressure_hPa, strBuffer);
#else
    // 1 hPa = 0.029529980164712 inHg
    ITOA(Sensor::Round(pressure_hPa*2953L, 10000L), strBuffer);
    // insert DP
    size_t len = strlen(strBuffer);
    if (len)
    {
      strBuffer[len + 1] = 0;
      strBuffer[len] = strBuffer[len - 1];
      strBuffer[len - 1] = '.';
    }
#endif
    int offset = StrokedFont::Width(strBuffer, num, den, gap) + 2; // +2 for the doubling-up
#ifdef CONFIG_HECTO_PASCALS
    strcat_P(strBuffer, phPaStr);
#else
    strcat_P(strBuffer, pinHgStr);
#endif
    int x = (DISPLAY_WIDTH - StrokedFont::Width(strBuffer, num, den, gap))/2;
    int y = 2;
    // these are columns
    ruleData[0] = x + offset;
    ruleData[1] = DISPLAY_WIDTH - 1;
    ruleData[2] = -1; // off
#ifdef DEBUG
    ruleData[1] -= 10; // exclude debug forecast letter
#endif
    Text(x, y, strBuffer, num, den, gap, TXT_QUAD | TXT_SPLIT);
    y = StrokedFont::cursorY + 1;

    // **************** pressure trend
    if (pressureTrend == 'S')
      pStr = pStableTrendStr;
    else if (pressureTrend == 'R')
      pStr = pRisingTrendStr;
    else if (pressureTrend == 'F')
      pStr = pFallingTrendStr;
    else
      pStr = pNAStr;
    num = 1; den = 1; gap = 4;
    ruleData[0] = -1; // off
    Text(x, y, pStr, num, den, gap, TXT_QUAD | TXT_ITALIC | TXT_CENTRE);
    
    // dither random forecast
    ditherForecast = randomForecast;
    if (randomForecast)
    {
      ruleData[0] = 0;
      ruleData[1] = DISPLAY_WIDTH - 1;
      ditherAltColour = Display::MonoWhite; // extra light
    }
    // **************** forecast icon
    y = StrokedFont::cursorY;
    if (::isalpha(forecastLetter))
    {
      // map the letter range A-Z to the icon range Sunny-Stormy
      int icon = min((forecastLetter - 'A')/(('Z'-'A' + 1)/Graphics::NumWeatherIcons), Graphics::NumWeatherIcons - 1);
      Graphics::Weather((DISPLAY_HEIGHT - Graphics::WeatherWidth())/2, y, icon);
    }
    y += Graphics::WeatherHeight();
    SendRows(y);

    // **************** forecast text
    gap = 0;
    y += 5;
    const char* pStr2;
    pStr = Weather::GetForecastStr(forecastLetter, pStr2);
    if (pStr == NULL)
      pStr = pNAStr;
    x = 8;
    if (pStr2 == NULL || pgm_read_byte_near(pStr2) == ' ')
    {
      // only one line
      y += StrokedFont::Height(num, den)/2;
      Text(x, y, pStr, num, den, gap, TXT_DBL_VT);
    }
    else
    {
      Text(x, y, pStr, num, den, gap, TXT_DBL_VT);
      Text(x, StrokedFont::cursorY, pStr2, num, den, gap, TXT_DBL_VT);
    }
    ditherForecast = false;
    ditherAltColour = Display::MonoGrey;

    // **************** temperature & humidity
    num = 5; den = 2;
    y = StrokedFont::cursorY + 6;

    int lenH = (int)strlen(ITOA(humidity_Percent, strBuffer)); // record length of Humidity w/out '%'
#ifdef CONFIG_CELCIUS
    int lenT = (int)strlen(ITOA(temperature_C, strBuffer)); // record length of Temperature w/out units, store in buffer
    strcat_P(strBuffer, pCelsiusStr); // then append units
#else
    int lenT = (int)strlen(ITOA(32 + 9*temperature_C/5, strBuffer)); // record length of Temperature w/out units, store in buffer
    strcat_P(strBuffer, pFahrenheitStr); // then append units
#endif
    int lenChar = StrokedFont::Width(" ", num, den, gap) + StrokedFont::Gap(num, den, gap); // width of a char, including gap to next
    int width = lenChar*((int)strlen(strBuffer) + lenH + 1); // width of chars with '%'
    int numSpaces = max((DISPLAY_WIDTH - width)/lenChar, 0) - 1; // spaces to add between T & H to make the line span the display
    if (humidity_Percent >= 0)
    {
      while (numSpaces--)
        strcat(strBuffer, " ");
      ITOA(humidity_Percent, strBuffer + strlen(strBuffer)); // append Humidity
      strcat(strBuffer, "%"); // and units
    }
    x = (DISPLAY_WIDTH - StrokedFont::Width(strBuffer, num, den, gap))/2; // centre
    // set the dithering regions
    ruleData[0] = x + lenT*lenChar;
    ruleData[1] = ruleData[0] + 2*lenChar;
    if (humidity_Percent >= 0)
    {
      ruleData[2] = x + StrokedFont::Width(strBuffer, num, den, gap) - (lenChar - StrokedFont::Gap(num, den, gap));
      ruleData[3] = DISPLAY_WIDTH - 1;
    }
    Text(x, y, strBuffer, num, den, gap, TXT_QUAD | TXT_SPLIT);

    // trailing rows
    SendRows(DISPLAY_HEIGHT - 1);
    SparseInk::SetRuleCallback(nullptr);

    // No red pixels:
    Display::StartRed();
    Display::FillRowBuffer(Display::ColourNone);
    for (int row = 0; row < DISPLAY_HEIGHT; row++)
      Display::SendRowBuffer();

    Display::Refresh();
  }

  void Loop()
  {
#ifdef DEMO
    Weather::Loop();
    Paint(Weather::GetPressure(), Weather::GetForecastLetter(), Weather::GetPressureTrend(), Weather::GetTemperature(), Weather::GetHumidity());
    while (true)
      ;
#else    
    if (Weather::Loop() || firstLoop)
    {
      Display::Init(); // wake
      Paint(Weather::GetPressure(), Weather::GetForecastLetter(), Weather::GetPressureTrend(), Weather::GetTemperature(), Weather::GetHumidity());
      updateCounter++;
      Display::Sleep();
      firstLoop = false;
    }
#endif    
  }
}
