#include "Cursors.h"

#include <stdio.h>
#include <SDL.h>

//---------------------------------------------------------------------------------------------------------------------
SDL_Cursor *createCursorFromImage(const char *image[])
{
  int i, row, col;
  Uint8 data[4 * 32];
  Uint8 mask[4 * 32];
  int hot_x, hot_y;

  i = -1;
  for (row = 0; row < 32; ++row)
  {
    for (col = 0; col < 32; ++col)
    {
      if (col % 8)
      {
        data[i] <<= 1;
        mask[i] <<= 1;
      }
      else
      {
        ++i;
        data[i] = mask[i] = 0;
      }

      switch (image[row][col])
      {
      case '.':
        data[i] |= 0x01;
        mask[i] |= 0x01;
        break;

      case 'X':
        mask[i] |= 0x01;
        break;

      case ' ':
        break;
      }
    }
  }

  if (sscanf(image[row], "%d, %d", &hot_x, &hot_y) != 2)
    return nullptr;

  return SDL_CreateCursor(data, mask, 32, 32, hot_x, hot_y);
}
