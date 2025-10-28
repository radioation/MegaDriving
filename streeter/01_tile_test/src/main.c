#include <genesis.h>
#include "resources.h"

#include "grass.h"


extern void drawAt(char* dest, char* source, s16 startPixel, s16 lastTilePixel, s16 tiles );


int main(bool arg)
{
    VDP_setScreenWidth320();
    PAL_setPalette( PAL0, road_images_pal.data, CPU );

    //////////////////////////////////////////////////////////////////////
    // setup road
    int ind = TILE_USER_INDEX;
    int roadIndex = ind;
    // Load the plane tiles into VRAM
    VDP_loadTileSet(road_images.tileset, ind, CPU);


    // setup the tiles
    VDP_setTileMapEx(BG_A, road_images.tilemap, TILE_ATTR_FULL(PAL0, FALSE, FALSE, FALSE, roadIndex),
            0,               // Plane X destination
            18 ,             // plane Y destination
            0,               // Region X start position
            0,               // Region Y start position
            40, // width  (went with 64 becasue default width is 64.  Viewable screen is 40)
            10,             // height
            CPU);

    //////////////////////////////////////////////////////////////////////
    // Load the grass into VRAM with VDP_loadTileData
    s16 grassIndex = roadIndex + road_images.tileset->numTile;
    u32 grassColumn[80]; // 10 rows in the column * 8 rows per tile is 80 elements.
    memset(grassColumn, 0, sizeof(grassColumn));
    memcpy(grassColumn, grass, sizeof(grassColumn) ); //sizeof(grassColumn)); // copy the column data into ram
    VDP_loadTileData((const u32 *)grassColumn,       // tile data pointer
            grassIndex,                    // index
            10,                          // number of tiles to load
            DMA_QUEUE                    // transfer method
            );

    for (u16 x = 0; x < 40; ++x)
    {
        // make a column out of it.
        VDP_fillTileMapRectInc(BG_B,
                TILE_ATTR_FULL(PAL2,      // Palette
                    0,         // Priority
                    0,         // Flip Vertical
                    0,         // FLip Horizontal
                    grassIndex), // tile index
                x,                        // x
                18,                       // y
                1,                        // width
                10                         // height (10 tiles)
                );
    }

    s16 offset = 1;
    s16 frame = 0;
    s16 delay = 0;
    while(TRUE) {
        ///////////////////////////////////////////////////////
        // update the col data

        memcpy(grassColumn, grass[frame],  sizeof(grassColumn ));
        VDP_loadTileData((const u32 *)grassColumn, // tile data pointer
                grassIndex,              // index
                10,                     // number of tiles to load
                DMA_QUEUE              // transfer method
                );
        delay +=1;
        if( delay > 2 ) {
            delay = 0;
            frame += 1;
            if (frame > 5)
            {
                frame = 0;
            }
        }

        // naive approach? blast the entire rect, does look ok.
        VDP_setTileMapEx(BG_A, road_images.tilemap, TILE_ATTR_FULL(PAL0, FALSE, FALSE, FALSE, roadIndex),
                0,               // Plane X destination
                18,//27,             // plane Y destination
                0,               // Region X start position
                frame*10, //+9,               // Region Y start position
                40, // width  (went with 64 becasue default width is 64.  Viewable screen is 40)
                10, // 1,             // height
                DMA_QUEUE);



        SYS_doVBlankProcess();
    }

}

