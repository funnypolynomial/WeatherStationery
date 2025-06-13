#pragma once

// Draw forecast icon etc
namespace Graphics
{
  enum WeatherIcon {Sun_Icon, Cloud_Sun_Icon, Cloud_Icon, Cloud_Sun_Rain_Icon, Cloud_Rain_Icon, Cloud_Lightning_Icon, NumWeatherIcons};
  void Weather(int x0, int y0, int idx);
  int WeatherWidth();
  int WeatherHeight();

  void PaintUpdateCounter(const char* pNum);
};
