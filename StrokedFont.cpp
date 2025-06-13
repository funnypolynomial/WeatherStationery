#include <Arduino.h>
#include "Display.h"
#include "SparseInk.h"
#include "StrokedFont.h"

namespace StrokedFont
{
  // first byte in a definition is <character> then data bytes to <0xFF>
  // data bytes are 0bDxxxyyyy where D is 1 for a draw, 0 for a move, coords are {xxx,yyyy}
  // Normally, x is 0..6, y is 0..12, but across to 7 for '_' and down to 15 for descenders. (0,0) is top-left
  // two moves are an arc
  // character definitions must be in numerical order, but gaps/omissions (like lowercase) are OK
  // missing chars are shown as blank
  // if the defined chars are in a contiguous range, define these two and save a PROGMEM byte per char:
//#define FIRST_CHAR    '!'
//#define LAST_CHAR     '~'

#define MAX_X               6  // X's are 0..6 left-to-right
#define MAX_Y               12 // Y's are 00..12, top-to-bottom but can descend to 15
#define FULL_Y              18 // with arc descender, bottom is 15+3
#define GAP_X               2  // extra grid steps between chars
#define GAP_Y               2  // extra grid steps between lines
#define DRAW_FLAG           0b10000000
#define MASK_X              0b00000111 // 3 bits for X
#define SHIFT_X             4
#define MASK_Y              0b00001111 // 4 bits for Y
#define GET_X(_b)           (((_b) >> SHIFT_X) & MASK_X)
#define GET_Y(_b)           ((_b) & MASK_Y)
#if defined(FIRST_CHAR) && defined(FIRST_CHAR)
#undef FIND_CHAR
#define CHAR(_c)
#else
#define FIND_CHAR
#define CHAR(_ch)             (_ch), 
#endif
#define MOVE(_x, _y)          (0b00000000 | (((_x) & MASK_X) << SHIFT_X) | ((_y) & MASK_Y))
#define DRAW(_x, _y)          (DRAW_FLAG  | (((_x) & MASK_X) << SHIFT_X) | ((_y) & MASK_Y))
// two consecutive moves are an arc: centre x, y; radius, quadrants (0b0001 is 12-3, 0b0010 is 3-6, 0b0100 is 6-9, 0b1000 is 9-12 (clockwise))
#define ARC(_x, _y, _r, _q)   MOVE((_x), (_y)), MOVE((_r), _q)
#define DOT(_x, _y)           MOVE((_x), (_y)), DRAW((_x), (_y) + 1)
#define END                   (0b11111111) // equivalent to DRAW(7, 15)

  static const uint8_t pFontDefn[] PROGMEM = // ~740 bytes
  {
    // hand-crafted char definitions.
    CHAR(0x18)   MOVE(0,3), DRAW(3,0), DRAW(6,3), MOVE(3,0), DRAW(3,12), END, // <up arrow>
    CHAR(0x19)   MOVE(0,9), DRAW(3,12), DRAW(6,9), MOVE(3,0), DRAW(3,12), END, // <down arrow>
    CHAR(0x1A)   MOVE(3,3), DRAW(6,6), DRAW(3,9), MOVE(0,6), DRAW(6,6), END, // <right arrow>
    CHAR(0x1B)   MOVE(3,3), DRAW(0,6), DRAW(3,9), MOVE(0,6), DRAW(6,6), END, // <left arrow>

    CHAR('!')    MOVE(3,0), DRAW(3,9), DOT(3,11), END,
    CHAR('"')    MOVE(2,0), DRAW(2,2), MOVE(4,0), DRAW(4,2), END,
    CHAR('#')    MOVE(3,0), DRAW(1,12), MOVE(5,0), DRAW(3,12), MOVE(0,4), DRAW(6,4), MOVE(0,8), DRAW(6,8), END,
    CHAR('$')    MOVE(3,0), DRAW(3,12), ARC(3,4,2,0b1101), ARC(3,8,2,0b0111), END,
    CHAR('%')    MOVE(0,12), DRAW(6,0), ARC(1,1,1,0b1111), ARC(5,11,1,0b1111), END,
    CHAR('&')    MOVE(2,5), DRAW(6,12), MOVE(0,7), DRAW(0,9), ARC(3,2,2,0b1111), ARC(3,9,3,0b0110), ARC(3,7,3,0b1000), END,
    CHAR('\'')   MOVE(3,0), DRAW(3,2), END,
    CHAR('(')    MOVE(1,3), DRAW(1,9), ARC(4,3,3,0b1000), ARC(4,9,3,0b0100), END,
    CHAR(')')    MOVE(5,3), DRAW(5,9), ARC(2,3,3,0b0001), ARC(2,9,3,0b0010), END,
    CHAR('*')    MOVE(0,6), DRAW(6,6), MOVE(3,3), DRAW(3,9), MOVE(1,4), DRAW(5,8), MOVE(1,8), DRAW(5,4), END,
    CHAR('+')    MOVE(0,6), DRAW(6,6), MOVE(3,3), DRAW(3,9), END,
    CHAR(',')    DOT(3,11), MOVE(3,12), DRAW(2,13), END,
    CHAR('-')    MOVE(0,6), DRAW(6,6), END,
    CHAR('.')    DOT(3,11), END,
    CHAR('/')    MOVE(6,0), DRAW(0,12), END,

    CHAR('0')    MOVE(0,3), DRAW(0,9), DRAW(6,3), DRAW(6,9), ARC(3,3,3,0b1001), ARC(3,9,3,0b0110), END,
    CHAR('1')    MOVE(3,0), DRAW(3,12), MOVE(0,3), DRAW(3,0), MOVE(0,12), DRAW(6,12), END,
    CHAR('2')    MOVE(6,3), DRAW(6,5), DRAW(0,12), DRAW(6,12), ARC(3,3,3,0b1001), END,
    CHAR('3')    MOVE(3,0), DRAW(0,0), MOVE(0,12), DRAW(3,12), MOVE(0,6), DRAW(3,6), ARC(3,3,3,0b0011), ARC(3,9,3,0b0011), END,
    CHAR('4')    MOVE(4,0), DRAW(0,6), DRAW(6,6), MOVE(4,0), DRAW(4,12), END,
    CHAR('5')    MOVE(6,0), DRAW(0,0), DRAW(0,5), DRAW(3,6), ARC(3,9,3,0b0111), END,
    CHAR('6')    MOVE(0,3), DRAW(0,9), ARC(3,3,3,0b1001), ARC(3,9,3,0b1111), END,
    CHAR('7')    MOVE(0,0), DRAW(6,0), DRAW(0,12), END,
    CHAR('8')    ARC(3,3,3,0b1111), ARC(3,9,3,0b1111), END,
    CHAR('9')    MOVE(6,0), DRAW(6,12), ARC(3,3,3,0b1111), END,
    CHAR(':')    DOT(3,6), DOT(3,11), END,
    CHAR(';')    DOT(3,6), DOT(3,11), MOVE(3,12), DRAW(2,13), END,
    CHAR('<')    MOVE(6,2), DRAW(0,6), DRAW(6,10), END,
    CHAR('=')    MOVE(0,4), DRAW(6,4), MOVE(0,8), DRAW(6,8), END,
    CHAR('>')    MOVE(0,2), DRAW(6,6), DRAW(0,10), END,
    CHAR('?')    MOVE(3,6), DRAW(3,9), DOT(3,11), ARC(3,3,3,0b1011), END,

    CHAR('@')    MOVE(0,3), DRAW(0,9), MOVE(6,3), DRAW(6,5), ARC(4,5,2,0b1111), ARC(3,3,3,0b1001), ARC(3,9,3,0b0110), END,
    CHAR('A')    MOVE(0,3), DRAW(0,12), MOVE(6,3), DRAW(6,12), MOVE(0,6), DRAW(6,6), ARC(3,3,3,0b1001), END,
    CHAR('B')    MOVE(3,0), DRAW(0,0), DRAW(0,12), DRAW(3,12), MOVE(0,6), DRAW(3,6), ARC(3,3,3,0b0011), ARC(3,9,3,0b0011), END,
    CHAR('C')    MOVE(3, 0), DRAW(6, 0), MOVE(3, 12), DRAW(6, 12), MOVE(0, 3), DRAW(0, 9), ARC(3,3,3,0b1000), ARC(3,9,3,0b0100), END,
    CHAR('D')    MOVE(3, 0), DRAW(0,0), DRAW(0, 12), DRAW(3, 12), MOVE(6, 3), DRAW(6, 9), ARC(3,3,3,0b0001), ARC(3,9,3,0b0010), END,
    CHAR('E')    MOVE(6,0), DRAW(0,0), DRAW(0,12), DRAW(6,12), MOVE(0,6), DRAW(6,6), END,
    CHAR('F')    MOVE(6,0), DRAW(0,0), DRAW(0,12), MOVE(0,6), DRAW(6,6), END,
    CHAR('G')    MOVE(0,3), DRAW(0,9), MOVE(3,6), DRAW(6,6), DRAW(6,9), ARC(3,3,3,0b1001), ARC(3,9,3,0b0110), END,
    CHAR('H')    MOVE(0,0), DRAW(0,12), MOVE(6,0), DRAW(6,12), MOVE(0,6), DRAW(6,6), END,
    CHAR('I')    MOVE(3,0), DRAW(3,12), MOVE(0,0), DRAW(6,0), MOVE(0,12), DRAW(6,12), END,
    CHAR('J')    MOVE(4,0), DRAW(4,10), MOVE(0,0), DRAW(6,0), ARC(2,10,2,0b0110), END,
    CHAR('K')    MOVE(0,0), DRAW(0,12), MOVE(6,0), DRAW(0,6), DRAW(6,12), END,
    CHAR('L')    MOVE(0,0), DRAW(0,12), DRAW(6,12), END,
    CHAR('M')    MOVE(0,0), DRAW(0,12), MOVE(6,3), DRAW(6,12), MOVE(3,0), DRAW(3,12), ARC(3,3,3,0b1001), END,
    CHAR('N')    MOVE(0,12), DRAW(0,0), DRAW(6,12), DRAW(6,0), END,
    CHAR('O')    MOVE(0,3), DRAW(0,9), MOVE(6,3), DRAW(6,9), ARC(3,3,3,0b1001), ARC(3,9,3,0b0110), END,
    CHAR('P')    MOVE(0,0), DRAW(0,12), ARC(3,3,3,0b1111), END,
    CHAR('Q')    MOVE(0,3), DRAW(0,9), MOVE(6,3), DRAW(6,9), ARC(3,3,3,0b1001), ARC(3,9,3,0b0110), MOVE(3,8), DRAW(6,12), END,
    CHAR('R')    MOVE(3,0), DRAW(0,0), DRAW(0,12), MOVE(3,6), DRAW(0,6), DRAW(6,12), ARC(3,3,3,0b0011), END,
    CHAR('S')    ARC(3,3,3,0b1101), ARC(3,9,3,0b0111), END,
    CHAR('T')    MOVE(3,0), DRAW(3,12), MOVE(0,0), DRAW(6,0), END,
    CHAR('U')    MOVE(0,0), DRAW(0,9), MOVE(6,0), DRAW(6,9), ARC(3,9,3,0b0110), END,
    CHAR('V')    MOVE(0,0), DRAW(3,12), DRAW(6,0), END,
    CHAR('W')    MOVE(0,0), DRAW(0,9), MOVE(6,0), DRAW(6,9), MOVE(3,0), DRAW(3,12), ARC(3,9,3,0b0110), END,
    CHAR('X')    MOVE(0,0), DRAW(6,12), MOVE(0,12), DRAW(6,0), END,
    CHAR('Y')    MOVE(0,0), DRAW(0,3), MOVE(6,0), DRAW(6,9), ARC(3,3,3,0b0110), ARC(3,9,3,0b0110), END,
    CHAR('Z')    MOVE(0,0), DRAW(6,0), DRAW(0,12), DRAW(6,12), END,

    CHAR('[')    MOVE(5,0), DRAW(1,0), DRAW(1,12), DRAW(5,12), END,
    CHAR('\\')   MOVE(0,0), DRAW(6,12), END,
    CHAR(']')    MOVE(1,0), DRAW(5,0), DRAW(5,12), DRAW(1,12), END,
    CHAR('^')    MOVE(2,3), DRAW(3,0), DRAW(4,3), END,
    CHAR('_')    MOVE(0,12), DRAW(7,12), END,
    CHAR('`')    MOVE(3,0), DRAW(4,2), END,

    CHAR('a')    MOVE(6,6), DRAW(6,12), ARC(3,9,3,0b1111), END,
    CHAR('b')    MOVE(0,0), DRAW(0,12), ARC(3,9,3,0b1111), END,
    CHAR('c')    MOVE(3,6), DRAW(6,6), MOVE(3,12), DRAW(6,12), ARC(3,9,3,0b1100), END,
    CHAR('d')    MOVE(6,0), DRAW(6,12), ARC(3,9,3,0b1111), END,
    CHAR('e')    MOVE(0,9), DRAW(6,9), MOVE(3,12), DRAW(6,12), ARC(3,9,3,0b1101), END,
    CHAR('f')    MOVE(3,2), DRAW(3,12), MOVE(0,6), DRAW(6,6), MOVE(5,0), DRAW(6,0), ARC(5,2,2,0b1000), END,
    CHAR('g')    MOVE(6,6), DRAW(6,14), ARC(3,9,3,0b1111), ARC(3,14,3,0b0110), END,
    CHAR('h')    MOVE(0,0), DRAW(0,12), MOVE(6,8), DRAW(6,12), ARC(3,8,3,0b1001), END,
    CHAR('i')    DOT(3,2), MOVE(3,6), DRAW(3,12), END,
    CHAR('j')    DOT(6,2), MOVE(6,6), DRAW(6,14), ARC(3,14,3,0b0110), END,
    CHAR('k')    MOVE(0,0), DRAW(0,12), MOVE(0,9), DRAW(6,6), MOVE(0,9), DRAW(6,12), END,
    CHAR('l')    MOVE(3,0), DRAW(3,12), END,
    CHAR('m')    MOVE(0,6), DRAW(0,12), MOVE(6,9), DRAW(6,12), MOVE(3,6), DRAW(3,12), ARC(3,9,3,0b1001), END,
    CHAR('n')    MOVE(0,6), DRAW(0,12), MOVE(6,9), DRAW(6,12), ARC(3,9,3,0b1001), END,
    CHAR('o')    ARC(3,9,3,0b1111), END,
    CHAR('p')    MOVE(0,6), DRAW(0,14), ARC(3,9,3,0b1111), END,
    CHAR('q')    MOVE(6,6), DRAW(6,14), ARC(3,9,3,0b1111), END,
    CHAR('r')    MOVE(0,6), DRAW(0,12), ARC(3,9,3,0b1001), END,
    CHAR('s')    MOVE(3,6), DRAW(6,6), MOVE(0,12), DRAW(3,12), MOVE(0,9), DRAW(6,9), ARC(3,9,3,0b1010), END,
    CHAR('t')    MOVE(3,0), DRAW(3,12), MOVE(0,6), DRAW(6,6), END,
    CHAR('u')    MOVE(6,6), DRAW(6,12), MOVE(0,9), DRAW(0,6), ARC(3,9,3,0b0110), END,
    CHAR('v')    MOVE(0,6), DRAW(3,12), DRAW(6,6), END,
    CHAR('w')    MOVE(0,6), DRAW(0,9), MOVE(6,6), DRAW(6,9), MOVE(3,6), DRAW(3,12), ARC(3,9,3,0b0110), END,
    CHAR('x')    MOVE(0,6), DRAW(6,12), MOVE(6,6), DRAW(0,12), END,
    CHAR('y')    MOVE(6,6), DRAW(6,14), MOVE(0,9), DRAW(0,6), ARC(3,9,3,0b0110), ARC(3,14,3,0b0110), END,
    CHAR('z')    MOVE(0,6), DRAW(6,6), DRAW(0,12), DRAW(6,12), END,

    CHAR('{')    MOVE(3,2), DRAW(3,4), MOVE(3,8), DRAW(3,10), ARC(5,2,2,0b1000), ARC(1,4,2,0b0010), ARC(1,8,2,0b0001), ARC(5,10,2,0b0100), END,
    CHAR('|')    MOVE(3,0), DRAW(3,5), MOVE(3,6), DRAW(3,12), END,
    CHAR('}')    MOVE(3,2), DRAW(3,4), MOVE(3,8), DRAW(3,10), ARC(1,2,2,0b0001), ARC(5,4,2,0b0100), ARC(5,8,2,0b1000), ARC(1,10,2,0b0010), END,
    CHAR('~')    ARC(2,2,1,0b1001), ARC(4,2,1,0b0110), END,

    CHAR(0xB0)   ARC(3,2,2,0b1111),END, // <degrees>

    CHAR(0x00)
  };

  int cursorX = 0, cursorY = 0;
  int clip_firstRow = 0, clip_lastRow = 0;
  int italic_dX = 0, italic_dY = 0, italic_BaseY = 0;

  int ItalicOffset(int Y)
  {
    // return the offset factor, if applicable
    if (italic_dY)
    {
      int offs = italic_dX*(italic_BaseY - Y)/italic_dY;
      if ((italic_dX*(italic_BaseY - Y) % italic_dY) > italic_dY/2) // round
        offs++;
      return offs;
    }
    return 0;
  }

  void SetPixel(int x, int y)
  {
    if (!clip_lastRow || (clip_firstRow <= y && y <= clip_lastRow))
      SparseInk::Pixel(y, x + ItalicOffset(y));
  }

  const uint8_t* FindDefn(uint8_t ch)
  {
    // return pointer to first byte of ch's defintion, or null
#ifdef FIND_CHAR
    // search
    const uint8_t* pByte = pFontDefn;
    while (pgm_read_byte_near(pByte) < ch)
    {
      while (pgm_read_byte_near(++pByte) != END)
        ;
      pByte++;
    }
    return (pgm_read_byte_near(pByte) == ch) ? ++pByte : nullptr;
#else
    // use offset
    uint8_t defn =  FIRST_CHAR;
    const uint8_t* pByte = pFontDefn;
    while (defn < ch && defn <= LAST_CHAR)
    {
      while (pgm_read_byte_near(++pByte) != END)
        ;
      pByte++;
      defn++;
    }
    return (defn == ch) ? pByte : nullptr;
#endif
  }

  void Arc(int xm, int ym, int r, uint8_t quadrants)
  {
    // draw the given quadrants of an arc radius r at (xm, ym)
    // http://members.chello.at/~easyfilter/bresenham.html
    int x = -r, y = 0, err = 2-2*r;
    do {
      if (quadrants & 0b0001)
        SetPixel(xm+y, ym+x);
      if (quadrants & 0b0010)
        SetPixel(xm-x, ym+y);
      if (quadrants & 0b0100)
        SetPixel(xm-y, ym-x);
      if (quadrants & 0b1000)
        SetPixel(xm+x, ym-y);
      r = err;
      if (r <= y)
        err += ++y*2+1;
      if (r > x || err > y)
        err += ++x*2+1;
    } while (x < 0);
  }

  void Line(int x0, int y0, int x1, int y1)
  {
    // Draw a line {x0, y0} to {x1, y1}
    // Always drawn left-to-right
    // Results in a series of calls to Pixel()
    // https://en.wikipedia.org/wiki/Bresenham%27s_line_algorithm
    int dx, dy;
    int     sy;
    int er, e2;
    if (x0 > x1)
    {
      // ensure x0 <= x1;
      dx = x0; x0 = x1; x1 = dx;
      dy = y0; y0 = y1; y1 = dy;
    }

    dx = x1 - x0;
    dy = (y1 >= y0) ? y0 - y1 : y1 - y0;
    sy = (y0 <  y1) ? 1 : -1;
    er = dx + dy;

    while (true)
    {
      SetPixel(x0, y0);
      if ((x0 == x1) && (y0 == y1))
        break;
      e2 = 2 * er;
      if (e2 >= dy)
      {
        er += dy;
        x0++;
      }
      if (e2 <= dx)
      {
        er += dx;
        y0 += sy;
      }
    }
  }

  void DrawChar(int x0, int y0, char ch, int scaleNum, int scaleDen /*= 1*/, int charGap /*= 0*/)
  {
    // draw just the stroked char
    italic_BaseY = y0 + MAX_Y;
    if (!charGap)
      charGap = GAP_X;
    const uint8_t* pDefn = FindDefn(ch);
    if (pDefn)
    {
      int prevX = x0, prevY = y0;
      while (pgm_read_byte_near(pDefn) != END)
      {
        uint8_t defn = pgm_read_byte_near(pDefn++);
        int x = x0 + scaleNum*GET_X(defn)/scaleDen;
        int y = y0 + scaleNum*GET_Y(defn)/scaleDen;
        if (defn & DRAW_FLAG)
        {
          Line(prevX, prevY, x, y);
          prevX = x;
          prevY = y;
        }
        else if (pgm_read_byte_near(pDefn) & DRAW_FLAG)
        {
          prevX = x;
          prevY = y;
        }
        else
        {
          uint8_t arc = pgm_read_byte_near(pDefn++);
          int r = scaleNum*GET_X(arc)/scaleDen;
          uint8_t q = GET_Y(arc);
          Arc(x, y, r, q);
        }
      }
    }
    cursorX = x0 + scaleNum*(MAX_X + charGap)/scaleDen;
    cursorY = y0;
  }

  void DrawText(int x0, int y0, const char* str, int scaleNum, int scaleDen /*= 1*/, int charGap /*= 0*/, bool fromPROGMEM /*= false*/)
  {
    int lineX = x0;
    int len = (int)(fromPROGMEM ? strlen_P(str) : strlen(str));
    for (int i = 0; i < len; i++)
    {
      char ch = fromPROGMEM ? pgm_read_byte_near((str + i)) : str[i];
      if (ch == '\n')
      {
        x0 = lineX;
        y0 += scaleNum*(FULL_Y + GAP_Y)/scaleDen;
      }
      else
      {
        DrawChar(x0, y0, ch, scaleNum, scaleDen, charGap);
        x0 = cursorX;
      }
    }
    cursorY = y0 + scaleNum*(FULL_Y + GAP_Y)/scaleDen;
    if (clip_lastRow)
      cursorY = min(cursorY, clip_lastRow);
  }

  int Width(const char* str, int scaleNum, int scaleDen, int charGap /*= 0*/, bool fromPROGMEM /*= false*/)
  {
    // width of str
    if (!charGap)
      charGap = GAP_X;
    int len = (int)(fromPROGMEM ? strlen_P(str) : strlen(str));
    return len*scaleNum*(MAX_X + charGap)/scaleDen - scaleNum*charGap/scaleDen + ItalicOffset(0); // we remove the trailing gap
  }

  int Gap(int scaleNum, int scaleDen, int charGap /*= 0*/)
  {
    if (!charGap)
      charGap = GAP_X;
    return scaleNum*charGap/scaleDen;
  }

  int Height(int scaleNum, int scaleDen, bool descender)
  {
    // may include descender, excludes gap between lines
    return scaleNum*(descender?FULL_Y:MAX_Y /*+ GAP_Y*/)/scaleDen;
  }

  void SetItalic(int dX, int dY)
  {
    // sets the italic offset, dX per dY. 0 to turn off
    italic_dX = dX;
    italic_dY = dY;
  }

  void SetClip(int firstRow, int lastRow)
  {
    clip_firstRow = firstRow;
    clip_lastRow = lastRow;
  }
}
