#include <genesis.h>
#include "grass.h"


extern void drawAt(char* dest, char* source, s16 startPixel, s16 lastTilePixel, s16 tiles );


int main(bool arg)
{


    //////////////////////////////////////////////////////////////////////
    // Load the grass into VRAM with VDP_loadTileData
    s16 colIndex = TILE_USER_INDEX;
    u32 colTiles[80]; // 10 rows in the column * 8 rows per tile is 80 elements.
    memset(colTiles, 0, sizeof(colTiles));
    memcpy(colTiles, grass, sizeof(colTiles) ); //sizeof(colTiles)); // copy the column data into ram
    VDP_loadTileData((const u32 *)colTiles,       // tile data pointer
            colIndex,                    // index
            10,                          // number of tiles to load
            DMA_QUEUE                    // transfer method
            );

    for (u16 x = 0; x < 40; ++x)
    {
        // make a column out of it.
        VDP_fillTileMapRectInc(BG_B,
                TILE_ATTR_FULL(PAL2,      // Palette
                    1,         // Priority
                    0,         // Flip Vertical
                    0,         // FLip Horizontal
                    colIndex), // tile index
                x,                        // x
                20,                       // y
                1,                        // width
                10                         // height (10 tiles)
                );
    }

    s16 offset = 1;
    s16 grassFrame = 0;
    s16 delay = 0;
    while(TRUE) {
        ///////////////////////////////////////////////////////
        // update the col data

        memcpy(colTiles, grass[grassFrame],  sizeof(colTiles ));
        VDP_loadTileData((const u32 *)colTiles, // tile data pointer
                colIndex,              // index
                10,                     // number of tiles to load
                DMA_QUEUE              // transfer method
                );
        delay +=1;
        if( delay > 3 ) {
            delay = 0;
            grassFrame += 1;
            if (grassFrame > 5)
            {
                grassFrame = 0;
            }
        }

        SYS_doVBlankProcess();
    }

}

