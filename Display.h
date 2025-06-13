#pragma once

// Interact with the ePaper display

// optionally dump graphics to serial (see resources\convert.py):
//#define DISPLAY_SERIALIZE
#ifdef DISPLAY_SERIALIZE
#define SERIALISE_ON(_on) Display::_serialise=_on;
#else
#define SERIALISE_ON(_on) 
#endif

#define DISPLAY_WIDTH  200
#define DISPLAY_HEIGHT 200
namespace Display
{
  enum Colour {MonoBlack, MonoGrey, MonoWhite, // 2 bpp
               ColourNone, ColourRed};         // 1 bpp, off = red
  
  void Init();
  void Reset();
  void Sleep();
  void SendCommand(byte data);
  void SendData(byte data);
  void StartMono();
  void StartRed();
  void Refresh();
  
  extern byte rowBuffer[DISPLAY_WIDTH/4];  // a single buffer with space for a row of mono or red pixels
  void FillRowBuffer(Colour clr);
  void SetRowBufferAt(int col, Colour clr);
  void SetRowBufferAt(int col, Colour clr, int len);
  Colour GetRowBufferAt(int col);

  // writing pixels at a cursor pos, auto-advances
  void StartRowBufferWrite(int col = 0);
  int GetRowBufferWriteCol();
  void WriteRowBuffer(Colour clr, int len = 1);
  
  void SendRowBuffer();
  
  void FillRowBuffer(byte* buff, Colour clr);
  void SetRowBufferAt(byte* buff, int col, Colour clr);
  void SetRowBufferAt(byte* buff, int col, Colour clr, int len);
  
  void SendRowBuffer(byte* buff);

  extern bool _serialise;
};
