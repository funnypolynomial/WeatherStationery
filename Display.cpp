#include <Arduino.h>
#include <SPI.h>
#include "Pins.h"
#include "Config.h"
#include "Display.h"

namespace Display {
// Note that the Jaycar site
//    https://www.jaycar.co.nz/duinotech-arduino-compatible-1-54-inch-monochrome-e-ink-display/p/XC3747
// suggests partial updates are supported.  I'm, not so sure. In any case, this code only does full updates of the entires screeb.
// Red is implemented but not used.
// command values
#define CMD_PANEL_SETTING                               0x00
#define CMD_POWER_SETTING                               0x01
#define CMD_POWER_OFF                                   0x02
#define CMD_POWER_ON                                    0x04
#define CMD_BOOSTER_SOFT_START                          0x06
#define CMD_DATA_START_TRANSMISSION_1                   0x10
#define CMD_DISPLAY_REFRESH                             0x12
#define CMD_DATA_START_TRANSMISSION_2                   0x13
#define CMD_PLL_CONTROL                                 0x30
#define CMD_VCOM_AND_DATA_INTERVAL_SETTING              0x50
#define CMD_TCON_RESOLUTION                             0x61
#define CMD_VCM_DC_SETTING_REGISTER                     0x82

// forward's
void SetLUTs();
void SendCommand(byte cmd);
void SendData(byte data);
void WaitUntilIdle();
// a single buffer with space for a row of mono or red pixels
byte rowBuffer[DISPLAY_WIDTH/4];

bool monoBufferMode = true;  // mono/red mode, set in StartMono()/StartColour(), not by colour setting

#ifdef DISPLAY_SERIALIZE
// if defined, swithes it off or on
bool _serialise = false;
#endif

void Init()
{
  // initialise the display, buffer etc
  ::memset(rowBuffer, 0, sizeof(rowBuffer));
  pinMode(PIN_DISPLAY_CS,   OUTPUT);
  pinMode(PIN_DISPLAY_RST,  OUTPUT);
  pinMode(PIN_DISPLAY_DC,   OUTPUT);
  pinMode(PIN_DISPLAY_BUSY, INPUT); 
  SPI.begin();
  SPI.beginTransaction(SPISettings(2000000, MSBFIRST, SPI_MODE0));
  
  // hardware init
  Reset();
  SendCommand(CMD_POWER_SETTING);
  SendData(0x07);
  SendData(0x00);
  SendData(0x08);
  SendData(0x00);
  SendCommand(CMD_BOOSTER_SOFT_START);
  SendData(0x07);
  SendData(0x07);
  SendData(0x07);
  SendCommand(CMD_POWER_ON);
  
  WaitUntilIdle();
  
  SendCommand(CMD_PANEL_SETTING);
  SendData(0xCF);
  SendCommand(CMD_VCOM_AND_DATA_INTERVAL_SETTING);
  SendData(0x17);
  SendCommand(CMD_PLL_CONTROL);
  SendData(0x39);
  SendCommand(CMD_TCON_RESOLUTION);
  SendData(0xC8);
  SendData(0x00);
  SendData(0xC8);
  SendCommand(CMD_VCM_DC_SETTING_REGISTER);
  SendData(0x0E);
  
  SetLUTs();
}

void Reset()
{
  // wake
  digitalWrite(PIN_DISPLAY_RST, LOW); 
  delay(200);
  digitalWrite(PIN_DISPLAY_RST, HIGH);
  delay(200);      
}

void SendCommand(byte cmd)
{
  digitalWrite(PIN_DISPLAY_DC, LOW);
  digitalWrite(PIN_DISPLAY_CS, LOW);
  SPI.transfer(cmd);
  digitalWrite(PIN_DISPLAY_CS, HIGH);
}

void SendData(byte data)
{
  digitalWrite(PIN_DISPLAY_DC, HIGH);
  digitalWrite(PIN_DISPLAY_CS, LOW);
  SPI.transfer(data);
  digitalWrite(PIN_DISPLAY_CS, HIGH);
}

void StartMono()
{
  // start updating the mono buffer
  monoBufferMode = true;
  delay(2);
  SendCommand(CMD_DATA_START_TRANSMISSION_1);
  delay(2);
}

void StartRed()
{
  // start updating the red buffer
  monoBufferMode = false;
  delay(2);
  SendCommand(CMD_DATA_START_TRANSMISSION_2);
  delay(2);   
}

void WaitUntilIdle()
{
  // wait until busy goes high
  while (!digitalRead(PIN_DISPLAY_BUSY))
    delay(100);
}

void Refresh()
{
  // update the display from the buffers
  delay(2);
  SendCommand(CMD_DISPLAY_REFRESH);
  WaitUntilIdle();  
}

void Sleep()
{
  // enter deep sleep, call Init to rewake
  SendCommand(CMD_VCOM_AND_DATA_INTERVAL_SETTING);
  SendData(0x17);
  SendCommand(CMD_VCM_DC_SETTING_REGISTER);  // to solve Vcom drop
  SendData(0x00);
  SendCommand(CMD_POWER_SETTING);  // power setting
  SendData(0x02);  // gate switch to external
  SendData(0x00);
  SendData(0x00);
  SendData(0x00);
  WaitUntilIdle();
  SendCommand(CMD_POWER_OFF);  // power off
}

static const byte pLUTData[] PROGMEM =
{
  // Monochrome
  0x20,  // lut_vcom0
    0x0E, 0x14, 0x01, 0x0A, 0x06, 0x04, 0x0A, 0x0A,
    0x0F, 0x03, 0x03, 0x0C, 0x06, 0x0A, 0x00,
  0x21,  // lut_w
    0x0E, 0x14, 0x01, 0x0A, 0x46, 0x04, 0x8A, 0x4A,
    0x0F, 0x83, 0x43, 0x0C, 0x86, 0x0A, 0x04,
  0x22,  // lut_b
    0x0E, 0x14, 0x01, 0x8A, 0x06, 0x04, 0x8A, 0x4A,
    0x0F, 0x83, 0x43, 0x0C, 0x06, 0x4A, 0x04,
  0x23,  // lut_g1
    0x8E, 0x94, 0x01, 0x8A, 0x06, 0x04, 0x8A, 0x4A,
    0x0F, 0x83, 0x43, 0x0C, 0x06, 0x0A, 0x04,
  0x24,  // lut_g2
    0x8E, 0x94, 0x01, 0x8A, 0x06, 0x04, 0x8A, 0x4A,
    0x0F, 0x83, 0x43, 0x0C, 0x06, 0x0A, 0x04,

  // Red
  0x25,  // lut_vcom1
    0x03, 0x1D, 0x01, 0x01, 0x08, 0x23, 0x37, 0x37,
    0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x26,  // lut_red0
    0x83, 0x5D, 0x01, 0x81, 0x48, 0x23, 0x77, 0x77,
    0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x27,  // lut_red1
    0x03, 0x1D, 0x01, 0x01, 0x08, 0x23, 0x37, 0x37,
    0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    
  0x00
};

void SetLUTs()
{
  // set look-up tables
  const byte* pLUT = pLUTData;
  while (pgm_read_byte_near(pLUT))
  {
    SendCommand(pgm_read_byte_near(pLUT++));
    for (int i = 0; i < 15; i++)
      SendData(pgm_read_byte_near(pLUT++));
  }
}

void FillRowBuffer(byte* buff, Colour clr)
{
  // set all pixels in the row buffer
  byte val = 0b00000000;
  switch (clr)
  {
    case MonoGrey   : val = 0b10101010; break;
    case MonoWhite  : val = 0b11111111; break;
    case ColourNone : val = 0b11111111; break;
    default         : val = 0b00000000; break;
  }
  ::memset(buff, val, DISPLAY_WIDTH/4);  
}    

void SetRowBufferAt(byte* buff, int col, Colour clr)
{
  // set a pixel in the row buffer
  byte val = 0b00000000;
  if (clr < ColourNone)
  {
    // 2 bpp
    if (clr != MonoBlack)
      val = (clr == MonoWhite)?0b11000000:0b10000000;
    byte lsr = 2*(col % 4);
    col >>= 2;
    buff[col] &= ~(0b11000000 >> lsr);
    buff[col] |=  (val        >> lsr);
  }
  else
  {
    // 1 bpp
    if (clr != ColourRed)
      val = 0b10000000;
    byte lsr = col % 8;
    col >>= 3;
    buff[col] &= ~(0b10000000 >> lsr);
    buff[col] |=  (val        >> lsr);
  }
}

void SetRowBufferAt(byte* buff, int col, Colour clr, int len)
{
  // set a series of pixel in the row buffer
  while (len--)
    SetRowBufferAt(buff, col++, clr);
}

int rowBufferWriteCol = 0;
void StartRowBufferWrite(int col /*= 0*/)
{
  // start writing pixels in the global row buffer, sets the write position/cursor
  rowBufferWriteCol = col;
}

int GetRowBufferWriteCol()
{
  // return the current write position
  return rowBufferWriteCol;
}

void WriteRowBuffer(Colour clr, int len /*= 1*/)
{
  // set a line of pixels in the global row buffer, at the current position (see StartRowBuffer())
  // updates position
  SetRowBufferAt(rowBufferWriteCol, clr, len);
  rowBufferWriteCol += len;
}

void SendRowByte(byte b)
{
  // wrapper to send the data byte to the display and optionally out the serial port
  SendData(b);
#ifdef DISPLAY_SERIALIZE
  if (_serialise)
  {
    Serial.print((int)(b >> 4), HEX);
    Serial.print((int)(b & 0x0F), HEX);
  }
#endif   
}

void SendRowBuffer(byte* buff)
{
  // send the entire row of pixels to the display
  byte* pBuffer = buff;
  if (monoBufferMode)
    for (size_t i = 0; i < DISPLAY_WIDTH/4; i++)
      SendRowByte(*pBuffer++);
  else
    for (size_t i = 0; i < DISPLAY_WIDTH/8; i++)
      SendRowByte(*pBuffer++);
#ifdef DISPLAY_SERIALIZE  
  if (_serialise)
    Serial.println();
#endif        
}

// these use the global rowBuffer
void FillRowBuffer(Colour clr)
{
  FillRowBuffer(rowBuffer, clr);
}

void SetRowBufferAt(int col, Colour clr)
{
  SetRowBufferAt(rowBuffer, col, clr);
}

void SetRowBufferAt(int col, Colour clr, int len)
{
  SetRowBufferAt(rowBuffer, col, clr, len);
}

Colour GetRowBufferAt(int col)
{
  // get a pixel from the global row buffer
  if (monoBufferMode)
  {
    // 2 bpp
    byte lsl = 2*(col % 4);
    col >>= 2;
    byte val = (rowBuffer[col] << lsl) & 0b11000000;
    if (val == 0b11000000)
      return MonoWhite;
    else if (val == 0b10000000)
      return MonoGrey;
    else
      return MonoBlack;
  }
  else
  {
    // 1 bpp
    byte lsl = col % 8;
    col >>= 3;
    byte val = (rowBuffer[col] << lsl) & 0b10000000;
    if (val)
      return ColourNone;
    else
      return ColourRed;
  }
}

void SendRowBuffer()
{
  // send the entire row of pixels to the display
  SendRowBuffer(rowBuffer);
}
}
