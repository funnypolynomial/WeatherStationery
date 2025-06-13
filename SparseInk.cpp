#include <Arduino.h>
#include "Display.h"
#include "SparseInk.h"

namespace SparseInk {
  // limited to 200x200 rows & cols, 0..199
  //  {row0} { col0 } { col1 }...{0xFF}
  //  {row1} { col0 } { col1 }...{0xFF}
  //  ...
  //  but, {col} followed by a byte (len) 200..254 means a run of len-197
#define TABLE_SIZE 1000
#define END 255
#define COLOUR_FORE Display::MonoBlack
#define COLOUR_BACK Display::MonoWhite
#define COLOUR_GREY Display::MonoGrey
#define RUN_LEN_MIN 200
#define RUN_LEN_MAX 254
#define RUN_LEN_LOW 3

  byte table[TABLE_SIZE];
  int tableTop = 0; // index of first unused byte
  int tableHighWater = 0;
  Error error = eNone;
  
  void Clear()
  {
    // clear the table, just the <end> row
    *table = END;
    tableTop = 1;
    error = eNone;
  }

  void Pixel(byte row, byte col)
  {
    // add the given pixel to the sparse data
    if (col >= RUN_LEN_MIN || row >= DISPLAY_HEIGHT || error)
      return;
    // find the row
    byte* ptr = table;
    while (*ptr < row)
    {
      // go to the end of the row
      do
        ptr++;
      while (*ptr != END);
      ptr++;
    }
    if (*ptr > row) // insert new row, col, END
    {
      if (tableTop + 3 >= TABLE_SIZE)
      {
        error = eRowFull;
        return;
      }
      // shuffle
      ::memmove(ptr + 3, ptr, TABLE_SIZE - (ptr - table) - 3);
      tableTop += 3;
      *ptr++ = row;
      *ptr++ = col;
      *ptr   = END;
    }
    else
    {
      // update row
      byte len = 0;
      do
      {
        if (len > 1)
          ptr++;
        ptr++;
        len = *(ptr + 1);
        if (RUN_LEN_MIN <= len && len <= RUN_LEN_MAX)
          len -= (RUN_LEN_MIN - RUN_LEN_LOW);
        else
          len = 1;
      } while (*ptr < col);
      if (*ptr > col) // insert col before ptr
      {
        if (tableTop + 1 >= TABLE_SIZE)
        {
          error = eColumnFull;
          return;
        }
        // shuffle
        ::memmove(ptr + 1, ptr, TABLE_SIZE - (ptr - table) - 1);
        tableTop++;
        *ptr = col;
      }
      else
      {
        // already present
        if (len > 1)
          error = ePacked;
      }
    }
  }

  void Pack(byte*& ptr, byte& len)
  {
    // replace the run with the encoded length
    // ptr points past the last col in the run
    // eg 10,11,12,13,17 will call with ptr @ 17, len = 4
    // after 10,201,17 ptr @ 17, len = 0
    if (RUN_LEN_LOW <= len && len < (RUN_LEN_MAX - RUN_LEN_LOW))
    {
      // shuffle
      ::memmove(ptr - (len - 2), ptr, TABLE_SIZE - (ptr - table) - 1);
      *(ptr - (len - 1)) = RUN_LEN_MIN + (len - RUN_LEN_LOW);
      tableTop -= len - 2;
      ptr -= len - 2;
      len = 0;
    }
  }

  int Pack()
  {
    // packs the column data by replacing consecutive cols with encoded lengths
    // skips the rest of a row if it finds it's already been packed
    // returns bytes saved
    // NOTE: Not actually used here, an experiment.  Not a general solution, packed entries are not updated, use with caution!
    if (error)
      return -1;
    int start = tableTop;
    byte* ptr = table;
    while (*ptr != END)
    {
      byte start_col, run_len, col;
      ptr++; // over the row #
      // start a run
      start_col = *ptr;
      run_len = 1;
      while (*ptr != END)
      {
        byte len = *(ptr + 1);
        if (RUN_LEN_MIN <= len && len <= RUN_LEN_MAX)  // row has been packed, skip it
          do
            ptr++;
        while (*ptr != END);
        else
        {
          col = *ptr;
          if (col == (start_col + run_len) && run_len < (RUN_LEN_MAX - RUN_LEN_LOW))
            run_len++; // extend the run
          else
          {
            Pack(ptr, run_len); // pack what we have & start again
            start_col = *ptr;
            run_len = 1;
          }
          ptr++;
        }
      }
      Pack(ptr, run_len); // pack what we have at the end
      ptr++; // over the 0xFF at the end of the row
    }
    return start - tableTop;
  }

  RuleCallback ruleCallback = nullptr;
  void SetRuleCallback(RuleCallback func)
  {
    // called before each row buffer is sent
    ruleCallback = func;
  }

  void SendRows(byte firstRow, byte lastRow, Display::Colour foreground, Display::Colour background)
  {
    // just send all the row data between startRow & endRow (INCLUSIVE), using the given colours
    // sent after RuleCallback is called
    // no Display::Start* function is called
    byte* ptr = table;
    int currentRow = firstRow;
    do
    {
      byte row = *ptr++;
      if (row < firstRow)
      {
        // skip row
        while (*ptr++ != END)
          ;
      }
      else if (row <= lastRow)
      {
        Display::FillRowBuffer(background);
        while (currentRow < row) // leading whole blank rows
        {
          if (ruleCallback)
          {
            Display::FillRowBuffer(background);
            ruleCallback(currentRow);
          }
          Display::SendRowBuffer();
          currentRow++;
        }
        while (*ptr != END)
        {
          byte len = *(ptr + 1);
          if (RUN_LEN_MIN <= len && len <= RUN_LEN_MAX)
          {
            // found a run, use it
            len -= (RUN_LEN_MIN - RUN_LEN_LOW);
            Display::SetRowBufferAt(*ptr, foreground, len);
            ptr += 2;
          }
          else
            Display::SetRowBufferAt(*ptr++, foreground);
        }
        if (ruleCallback)
          ruleCallback(currentRow);
        Display::SendRowBuffer();
        currentRow++;
        ptr++;
      }
      else
        break;
    } while (true);

    // trailing whole blank rows
    Display::FillRowBuffer(background);
    while (currentRow <= lastRow)
    {
      if (ruleCallback)
      {
        Display::FillRowBuffer(background);
        ruleCallback(currentRow);
      }
      Display::SendRowBuffer();
      currentRow++;
    }
    tableHighWater = max(tableHighWater, tableTop);
  }

  void Paint()
  {
    byte* ptr = table;
    byte currentRow = 0;
    Display::StartMono();
    do
    {
      byte row = *ptr++;
      if (row != END)
      {
        Display::FillRowBuffer(COLOUR_BACK);
        currentRow++;
        while (currentRow < row) // leading whole blank rows
        {
          currentRow++;
          Display::SendRowBuffer();
        }
        while (*ptr != END)
        {
          byte len = *(ptr + 1);
          if (RUN_LEN_MIN <= len && len <= RUN_LEN_MAX)
          {
            // found a run, use it
            len -= (RUN_LEN_MIN - RUN_LEN_LOW);
            Display::SetRowBufferAt(*ptr, COLOUR_FORE, len);
            ptr += 2;
          }
          else
            Display::SetRowBufferAt(*ptr++, COLOUR_FORE);
        }
        Display::SendRowBuffer();
        ptr++;
      }
      else
        break;
    } while (true);

    // trailing whole blank rows
    Display::FillRowBuffer(COLOUR_BACK);
    while (currentRow < DISPLAY_HEIGHT)
    {
      currentRow++;
      Display::SendRowBuffer();
    }
    Display::StartRed();
    Display::FillRowBuffer(Display::ColourNone);
    for (int row = 0; row < DISPLAY_HEIGHT; row++)
      Display::SendRowBuffer();
    Display::Refresh();
  }

}
