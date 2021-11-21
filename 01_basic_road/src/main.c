
// SGDK
#include <genesis.h>
#include "resources.h"

#define VERTICAL_REZ 224  // number of line sin the screen.

// image is 512x224.  Screen is 320, we want to move halfway
// (512-320)/2
#define SCROLL_CENTER -96

// Zmap for tracking segment position
#define ZMAP_LENGTH 110
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
		{FIX16(-0.02), FIX16(-0.120)},
		{FIX16(-0.04), FIX16(-0.32)},
		{FIX16(-0.02), FIX16(-0.120)},
		{FIX16(0), FIX16(0)},
		{FIX16(0), FIX16(0)},
		{FIX16(0.06), FIX16(0.36)},
		{FIX16(-0.06), FIX16(-0.36)},
		{FIX16(0), FIX16(0)},
		{FIX16(0.02), FIX16(0.12)},
		{FIX16(0), FIX16(0)},
		{FIX16(-0.03), FIX16(-0.18)},
		{FIX16(0.03), FIX16(0.18)}};

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

// My interpretation of the pseudo-code in
// http://www.extentofthejam.com/pseudo/#curves
void update()
{
	fix16 current_x = FIX16(0); // Lou's pseudo 3d page says to use Half of the screen width,
															// but I've defined SCROLL_CENTER to handle this

	fix16 dx = FIX16(0);	// Curve amount, constant per segment.
	fix16 ddx = FIX16(0); // Curve amount, changes per line

	// for each line of the screen from the bottom to the top
	// I've defined the ZMAP to have the bottom of the screen
	// (nearest position) start at zmap[0]
	for (u16 y = 0; y < ZMAP_LENGTH; ++y)
	{
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

		// ddx += dx
		ddx = fix16Add(ddx, dx);
		// current_x += ddx
		current_x = fix16Add(current_x, ddx);

		// this_line.x = current_x
		// we'll use horizontal scrolling of BG_A to fake curves.
		HscrollA[223 - y] = SCROLL_CENTER + fix16ToInt(current_x);
	}

	// scroll the background
	background_position = fix16Sub(background_position, segments[bottom_segments_index].bgdx);
	for (u16 y = 0; y < 120; ++y)
	{
		HscrollB[y] = fix16ToInt(background_position);
	}

	// Move segments
	segment_position = fix16Add(segment_position, speed);
	if (fix16ToInt(segment_position) < 0) // 0 is nearest
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

int main(u16 hard)
{
	//////////////////////////////////////////////////////////////
	// http://www.extentofthejam.com/pseudo/
	// Z = Y_world / (Y_screen - (height_screen / 2))
	for (u16 i = 0; i < ZMAP_LENGTH; ++i)
	{
		zmap[i] = fix16Div(FIX16(-75), fix16Sub(FIX16(i), FIX16(112)));
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

	//////////////////////////////////////////////////////////////
	// Setup background A
	VDP_setPalette(PAL1, road.palette->data);
	int ind = TILE_USERINDEX;
	VDP_drawImageEx(BG_A, &road, TILE_ATTR_FULL(PAL1, FALSE, FALSE, FALSE, ind), 0, 0, FALSE, TRUE);
	ind += road.tileset->numTile;
	VDP_setPalette(PAL2, background.palette->data);
	VDP_drawImageEx(BG_B, &background, TILE_ATTR_FULL(PAL2, FALSE, FALSE, FALSE, ind), 0, 0, FALSE, TRUE);
	ind += background.tileset->numTile;

	//////////////////////////////////////////////////////////////
	// init segments
	bottom_segments_index = 0;
	segments_index = 1;
	segment_position = zmap[ZMAP_LENGTH - 1]; // put it at the farthest away point

	// set speed through z
	speed = FIX16(-0.1);

	// Main loop
	while (TRUE)
	{
		// update each frame
		update();

		// curve the road with horizontal scrolling.
		VDP_setHorizontalScrollLine(BG_A, 0, HscrollA, VERTICAL_REZ, DMA_QUEUE);
		// move the background
		VDP_setHorizontalScrollLine(BG_B, 0, HscrollB, 120, DMA_QUEUE);

		SYS_doVBlankProcess();
	}
}
