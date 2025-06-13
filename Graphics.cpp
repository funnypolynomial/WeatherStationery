#include <Arduino.h>
#include "Display.h"
#include "SparseInk.h"
#include "Graphics.h"

namespace Graphics
{
  // Include the arrays of graphics data (just icons)
  #include "RegionData.h"
  // Assemble the graphics data into groups
  /////////////////////////
  
  //////////////////////
  static const uint8_t* const SunTable[] PROGMEM =
  {
    Sun0, // ring and LHS
    Sun1, // RHS
  };
  
  int WeatherWidth() { return Sun_WIDTH; }
  int WeatherHeight() { return Sun_HEIGHT; }
  
  //////////////////////
  static const uint8_t* const CloudTable[] PROGMEM =
  {
    Cloud0, // body
    Cloud1, // base
    Cloud2, // marker
  };
  
  //////////////////////
  static const uint8_t* const StormTable[] PROGMEM =
  {
    Storm0, // drops
    Storm1, // lightning
  };
  
  void RegionOffset(const uint8_t* ptr, int& x, int& y)
  {
    // Provide the offset to the region
    x = pgm_read_byte_near(ptr++);
    y = pgm_read_byte_near(ptr++);
  }

  void PaintInkRegion(int x0, int y0, const uint8_t* ptr, bool offset)
  {
    // Paint the region pointed to by ptr, at x0, y0, with colour
    if (offset)
    {
      x0 += pgm_read_byte_near(ptr++);
      y0 += pgm_read_byte_near(ptr++);
    }
    else
      ptr += 2;
    uint8_t w;
    do
    {
      w = pgm_read_byte_near(ptr++);
      if (w & 0x80)  // skip rows
        y0 += (w & 0x7F);
      else if (w) // draw strip, offset follows
      {
        uint8_t offs = pgm_read_byte_near(ptr++);
        if (offs & 0x80)
        {
          y0++;
        }
        offs &= 0x7F;
        for (int p = 0; p < w; p++)
          SparseInk::Pixel(y0, x0 + offs + p);
      }
    } while (w);
  }

  void Weather(int x0, int y0, int idx)
  {
    // Paint the idx'th weather icon
    bool sunBurst = false;
    switch (idx)
    {
    case Sun_Icon:
      PaintInkRegion(x0, y0, pgm_read_ptr_near(SunTable + 0), true);
      PaintInkRegion(x0, y0, pgm_read_ptr_near(SunTable + 1), true);
      break;
    case Cloud_Sun_Icon:
      PaintInkRegion(x0, y0, pgm_read_ptr_near(CloudTable + 0), true);
      PaintInkRegion(x0, y0, pgm_read_ptr_near(CloudTable + 1), true);
      sunBurst = true;
      break;
    case Cloud_Icon:
      PaintInkRegion(x0, y0, pgm_read_ptr_near(CloudTable + 0), true);
      PaintInkRegion(x0, y0, pgm_read_ptr_near(CloudTable + 1), true);
      break;
    case Cloud_Sun_Rain_Icon:
      PaintInkRegion(x0, y0, pgm_read_ptr_near(CloudTable + 0), true);
      PaintInkRegion(x0, y0, pgm_read_ptr_near(StormTable + 0), true);
      sunBurst = true;
      break;
    case Cloud_Rain_Icon:
      PaintInkRegion(x0, y0, pgm_read_ptr_near(CloudTable + 0), true);
      PaintInkRegion(x0, y0, pgm_read_ptr_near(StormTable + 0), true);
      break;
    case Cloud_Lightning_Icon:
      PaintInkRegion(x0, y0, pgm_read_ptr_near(CloudTable + 0), true);
      PaintInkRegion(x0, y0, pgm_read_ptr_near(StormTable + 1), true);
      break;
    default:
      break;
    }

    if (sunBurst)
    {
      int x, y;
      RegionOffset(pgm_read_ptr_near(CloudTable + 2), x, y);
      PaintInkRegion(x0 + x, y0 + y, pgm_read_ptr_near(SunTable + 1), false);
    }
  }

  ////////////////////////////////////////////////////////////
  // Tiny font, for diagnostics
  // 5x 3-bit rows.  b30 is top-left corner, b0 is bottom right
  const uint16_t font3x5[] PROGMEM =
  {
    //   0       1       2       3       4       5       6       7       8       9
    075557, 022222, 071747, 071717, 055711, 074717, 074757, 071111, 075757, 075711,
  };
  // draw the number at row 1 col 1, 3x5 font but doubled-up
  void PaintUpdateCounter(const char* pNum)
  {
    for (int row = 0; row < 10; row++)
      for (int idx = 0; idx < (int)strlen(pNum); idx++)
      {
        uint16_t defn = (pNum[idx] != ' ')?pgm_read_word_near(font3x5 + pNum[idx] - '0') : 0;
        defn >>= 3*(4 - row/2);
        for (int col = 0; col < 6; col++)
          if (defn & (0b0000100 >> col/2))
            SparseInk::Pixel(row + 1, 2*idx*4 + col + 1);
      }
  }
};
