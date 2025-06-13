#pragma once

// Virtual, compressed frame buffer
namespace SparseInk
{
  typedef void (*RuleCallback)(int row);
  enum Error {eNone = 0, eRowFull, eColumnFull, ePacked};

  void Clear();
  void Dump();
  void Pixel(byte row, byte col);
  int Pack();
  void SetRuleCallback(RuleCallback func);
  void SendRows(byte firstRow, byte lastRow, Display::Colour foreground, Display::Colour background);
  void Paint();

  extern int tableHighWater;
  extern Error error;
};
