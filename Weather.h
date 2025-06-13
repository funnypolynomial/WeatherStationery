#pragma once

// Read the conditions, make a forecast
namespace Weather
{
  // FOR ENTERTAINMENT ONLY!
  void Init();
  bool Loop();
  char GetForecastLetter();
  char GetPressureTrend();
  int GetPressure();
  int GetTemperature();
  int GetHumidity();
  const char* GetForecastStr(char letter, const char*& pLine2);
  void GetInfo(int16_t& currentRawP, int16_t& currentAdjP, int16_t& oldAdjP, char& trend);
};
