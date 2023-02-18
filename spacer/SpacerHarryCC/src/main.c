#include <genesis.h>
#include "resources.h"

#define VERTICAL_REZ 224

// image is 512x214.  Screen is 320, meet halfway  -> 512/2 - 320/2
#define SCROLL_CENTER -96
s16 hScrollB[ VERTICAL_REZ ];

const u16 palette[16] = 
{
  0x0E00,
  0x0E00,
  0x0E00,
  0x0E00,

  0x000E,
  0x000E,
  0x000E,
  0x000E,

  0x0E00,
  0x0E00,
  0x0E00,
  0x0E00,

  0x000E,
  0x000E,
  0x000E,
  0x000E

};





int main(bool in)
{
  //////////////////////////////////////////////////////////////
  // VDP basic setup
  VDP_setScreenWidth320();
  VDP_setScrollingMode(HSCROLL_LINE, VSCROLL_PLANE);

  // initialize scrolling arrays
  for (int i = 0; i < VERTICAL_REZ; i++)
  {
    hScrollB[i] = SCROLL_CENTER;
  }




  //////////////////////////////////////////////////////////////
  // Setup scroll panes
  PAL_setPalette(PAL0, bg_b_pal.data, CPU);
  PAL_setColors(1, palette, 8, CPU);
  int ind = TILE_USER_INDEX;
  VDP_drawImageEx(BG_B, &bg_b, TILE_ATTR_FULL(PAL0, FALSE, FALSE, FALSE, ind), 0, 0, FALSE, TRUE);
  u16 paletteOffset = 0;
  u16 ticks = 0;
  while(TRUE)
  {
    PAL_setColors(1, palette + paletteOffset, 8, DMA_QUEUE);
    ++ticks;
    if( ticks == 2 ) {
      ++paletteOffset;
      ticks = 0;
      if( paletteOffset > 7 ) {
        paletteOffset = 0;
      }
    }
    VDP_setHorizontalScrollLine(BG_B, 0, hScrollB, VERTICAL_REZ, DMA_QUEUE);

    SYS_doVBlankProcess();
  }

}
