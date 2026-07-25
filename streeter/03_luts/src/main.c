#include <genesis.h>
#include "resources.h"

#include "grass.h"
#include "track.h"

u32 lastLoggedFPS = 0;

#define VERTICAL_REZ 224  // number of lines in the screen.

// A is 320 wide, so no need to offset it
#define SCROLL_CENTER_A 0
// OTOH back ground B is 512 wide  (512-320)/2 = -96
#define SCROLL_CENTER_B -96

fastfix16 zmap[ZMAP_LENGTH];

#define SKY_HEIGHT 144


// speed the 'vehicle' moves through the road
fastfix16 speed = FASTFIX16(0.00);

// Horizontal scrolling values
s16 HscrollA[ZMAP_LENGTH];
s16 HscrollB[VERTICAL_REZ];

// position variables.
u16 pos = 0;
fix16 background_position = FIX16(SCROLL_CENTER_B); // handle background X position


void updateScrolling()
{
    u16 offset = pos_to_scroll_data_offset[pos];
    memcpy( HscrollA, scroll_data + offset, 160 );

    // scroll the background
    background_position += pos_to_bg_dx[pos];
    for (u16 y = 0; y < SKY_HEIGHT; y++ )
    {
        HscrollB[y] = F16_toInt(background_position);
    }
}






int main(bool arg)
{


    //////////////////////////////////////////////////////////////
    // VDP basic setup
    VDP_setBackgroundColor(16);
    VDP_setScreenWidth320();

    //////////////////////////////////////////////////////////////
    // initialize scrolling values to the center of the image.
    VDP_setScrollingMode(HSCROLL_LINE, VSCROLL_PLANE);
    for (int i = 0; i < VERTICAL_REZ; i++)
    {
        HscrollB[i] = SCROLL_CENTER_B;
    }



    //////////////////////////////////////////////////////////////////////
    // setup road
    PAL_setPalette( PAL0, road_images_pal.data, CPU );
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
    // setup sky
    PAL_setPalette( PAL1, dark_sky_pal.data, CPU );
    s16 skyIndex = roadIndex + road_images.tileset->numTile;
    VDP_drawImageEx(BG_B, &dark_sky, TILE_ATTR_FULL(PAL0, FALSE, FALSE, FALSE, skyIndex), 0, 0, FALSE, TRUE);
 
    //////////////////////////////////////////////////////////////////////
    // Load the grass into VRAM with VDP_loadTileData
    s16 grassIndex = skyIndex + dark_sky.tileset->numTile;
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
                TILE_ATTR_FULL(PAL1,      // Palette
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

    // set speed through z
    speed = FASTFIX16(-0.1);

    while(TRUE) {
          
        ///////////////////////////////////////////////////////
        // update scrolling values
        kprintf("updateScrolling()");
        BLASTEM_PROFIL_START
        updateScrolling();
        BLASTEM_PROFIL_END


        ///////////////////////////////////////////////////////
        // update the grass col data
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
        pos++;
        if( pos  >= POS_DATA_LEN ) {
            pos = 0;
        }

        ///////////////////////////////////////////////////////
        // Handle forward motion
        // naive approach? blast the entire rect, does look ok.
        VDP_setTileMapEx(BG_A, road_images.tilemap, TILE_ATTR_FULL(PAL0, FALSE, FALSE, FALSE, roadIndex),
                0,               // Plane X destination
                18,//27,             // plane Y destination
                0,               // Region X start position
                frame*10, //+9,               // Region Y start position
                40, // width  (went with 64 becasue default width is 64.  Viewable screen is 40)
                10, // 1,             // height
                DMA_QUEUE);

        // curve the road with horizontal scrolling.
        VDP_setHorizontalScrollLine(BG_A, ROAD_START_LINE, HscrollA, ZMAP_LENGTH, DMA_QUEUE); // TODO: scroll the bottom 80 lines instead of the entire VERTICAL_REZ
        VDP_setHorizontalScrollLine(BG_B, 0, HscrollB, SKY_HEIGHT, DMA_QUEUE);
        u32 fps = SYS_getFPS();
        if (fps != lastLoggedFPS)
        {
            kprintf("FPS: %d", fps);
            lastLoggedFPS = fps;
        }



        SYS_doVBlankProcess();
    }

}

