#pragma once

// A simple stroked font consisting of lines and arc quadrants
namespace StrokedFont
{
  void DrawChar(int x0, int y0, char ch, int scaleNum, int scaleDen = 1, int charGap = 0);
  void DrawText(int x0, int y0, const char* str, int scaleNum, int scaleDen = 1, int charGap = 0, bool fromPROGMEM = false);
  int Width(const char* str, int scaleNum, int scaleDen = 1, int charGap = 0, bool fromPROGMEM = false);
  int Gap(int scaleNum, int scaleDen, int charGap = 0);
  int Height(int scaleNum, int scaleDen = 1, bool descender = true);
  void SetItalic(int dX, int dY); // dY=0 to turn off
  void SetClip(int firstRow, int lastRow); // lastRow=0 to turn off
  size_t Count();

  extern int cursorX, cursorY;
};
