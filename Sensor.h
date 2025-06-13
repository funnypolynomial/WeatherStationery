#pragma once

// Interface to pressure etc sensor
// BME/BMP280: read pressure, temperature & humidity
// SPL06/BME: read pressure & temperature

namespace Sensor
{
  void Init();
  bool Read(int16_t& pressurehPa, int& temperatureC, int& humidityPercent);
  int32_t Round(int32_t value, int32_t divisor);
  uint32_t GetEntropy();
};
