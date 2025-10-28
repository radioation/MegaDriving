#include <genesis.h>
#include "resources.h"

#include "grass.h"

#define VERTICAL_REZ 224  // number of line sin the screen.

#define SCROLL_CENTER 0

// Zmap for tracking segment position (80, we're shorter than the lou examples)
#define ZMAP_LENGTH 80 
fix16 zmap[ZMAP_LENGTH];


// Road data
#define ROAD_SEGMENTS_LENGTH 13
typedef struct
{
    fix16 dx;  // rate of change for the road.
    fix16 bgdx; // rate of change for background. ( ignore for now )
} ROAD_SEGMENT;
const ROAD_SEGMENT segments[ROAD_SEGMENTS_LENGTH] = {
        {FIX16(0), FIX16(0)},
        {FIX16(-0.02), FIX16(0.120)},
        {FIX16(-0.04), FIX16(0.32)},
        {FIX16(-0.02), FIX16(0.120)},
        {FIX16(0), FIX16(0)},
        {FIX16(0), FIX16(0)},
        {FIX16(0.06), FIX16(-0.36)},
        {FIX16(-0.06), FIX16(0.36)},
        {FIX16(0), FIX16(0)},
        {FIX16(0.02), FIX16(-0.12)},
        {FIX16(0), FIX16(0)},
        {FIX16(-0.03), FIX16(0.18)},
        {FIX16(0.03), FIX16(-0.18)}};


u16 bottom_segments_index = 0;
u16 segments_index = 0;

// speed the 'vehicle' moves through the road
fix16 speed = FIX16(0.00);

// Horizontal scrolling values
s16 HscrollA[VERTICAL_REZ];
s16 HscrollB[VERTICAL_REZ];

// position variables.
fix16 segment_position = FIX16(0); // keep track fo the segment position onscreen
fix16 background_position = FIX16(SCROLL_CENTER); // handle background X position


void updateScrolling()
{
    fix16 current_x = FIX16(0); // Lou's pseudo 3d page says to use Half of the screen width,
                                                            // but I've defined SCROLL_CENTER to handle this

    fix16 dx = FIX16(0);    // Curve amount, constant per segment.
    fix16 ddx = FIX16(0); // Curve amount, changes per line


    // for each line of the screen from the bottom to the top
    for (u16 y = 0; y < ZMAP_LENGTH; ++y)
    {
        // I've defined the ZMAP to have the bottom of the screen
        // (nearest position) start at zmap[0]
        fix16 z = zmap[y];
        // if line of screen's Z Map position is below segment position
        if (z < segment_position)
        {
            // dx = bottom_segment.dx
            dx = segments[bottom_segments_index].dx;
        }
        else // if line of Screen's Z map position is above segment position.
        {
            // dx = segment.dx
            dx = segments[segments_index].dx;
        }

        ddx += dx;
        current_x += ddx;

        // this_line.x = current_x
        // we'll use horizontal scrolling of BG_A to fake curves.
        HscrollA[223 - y] = SCROLL_CENTER + F16_toInt(current_x);
    }

    // scroll the background
    background_position = background_position + segments[bottom_segments_index].bgdx;
    for (u16 y = 0; y < 120; ++y)
    {
        HscrollB[y] = F16_toInt(background_position);
    }

    // Move segments
    segment_position = segment_position + speed;
    if (F16_toInt(segment_position) < 0) // 0 is nearest
    {
        // bottom_segment = segment
        bottom_segments_index = segments_index;

        // segment.position = zmap.length - 1
        segment_position = zmap[ZMAP_LENGTH - 1]; // Send segment to farthest visible distance
        // fetch next segment from road
        segments_index++; // segment_index is used to get segment.dx
        if (segments_index == ROAD_SEGMENTS_LENGTH)
        {
            segments_index -= ROAD_SEGMENTS_LENGTH; // go back to the start
        }
    }
}




//extern void drawAt(char* dest, char* source, s16 startPixel, s16 lastTilePixel, s16 tiles );


int main(bool arg)
{

    //////////////////////////////////////////////////////////////
    // http://www.extentofthejam.com/pseudo/
    // Z = Y_world / (Y_screen - (height_screen / 2))
    for (u16 i = 0; i < ZMAP_LENGTH; ++i)
    {
        zmap[i] = F16_div(FIX16(-75), FIX16(i) - FIX16(112));
        KLog_f1("FIX16(", zmap[i]);
    }


    //////////////////////////////////////////////////////////////
    // VDP basic setup
    VDP_setBackgroundColor(16);
    VDP_setScreenWidth320();

    //////////////////////////////////////////////////////////////
    // initialize scrolling values to the center of the image.
    VDP_setScrollingMode(HSCROLL_LINE, VSCROLL_PLANE);
    for (int i = 0; i < VERTICAL_REZ; i++)
    {
        HscrollA[i] = SCROLL_CENTER;
        HscrollB[i] = SCROLL_CENTER;
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

    //////////////////////////////////////////////////////////////
    // init segments
    bottom_segments_index = 0;
    segments_index = 1;
    segment_position = zmap[ZMAP_LENGTH - 1]; // put it at the farthest away point

    // set speed through z
    speed = FIX16(-0.1);

    while(TRUE) {
          
        ///////////////////////////////////////////////////////
        // update scrolling values
        updateScrolling();


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
        VDP_setHorizontalScrollLine(BG_A, 0, HscrollA, VERTICAL_REZ, DMA_QUEUE);

        SYS_doVBlankProcess();
    }

}

