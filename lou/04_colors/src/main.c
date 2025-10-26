
// SGDK
#include <genesis.h>
#include "resources.h"

#define VERTICAL_REZ 224
void LineDark();
void LineLight();
// image is 512x224.  Screen is 320, we want to move halfway
//  512/2  - 320 /2
#define SCROLL_CENTER -96

// Keep track of the current line during Horizontal Interrupts
u16 lineDisplay = 0;

// Horizontal Scrolling values
s16 HscrollA[VERTICAL_REZ];
s16 HscrollB[VERTICAL_REZ];
// Vertical Scrolling values ( to simulate hills )
s8 VscrollA[VERTICAL_REZ];

// color banding array
u8 colors[VERTICAL_REZ];
u8 line_color = 0; // 0 -uninit, 1-light, 2-dark
u8 side_color = 0;
u8 grass_color = 0; // 0 -uninit, 1-light, 2-dark

// Zmap for tracking segment position
#define ZMAP_LENGTH 110
fix16 zmap[ZMAP_LENGTH];

// Road data
#define ROAD_SEGMENTS_LENGTH 15
typedef struct
{
	fix16 dx;		// rate of change for the road.
	fix16 bgdx; // rate of change for background. ( ignore for now )
	fix32 dy;		//  rate of change for drawing road in y dir?
} ROAD_SEGMENT;

const ROAD_SEGMENT segments[ROAD_SEGMENTS_LENGTH] = {
		{FIX16(0), FIX16(0), FIX32(-0.001)},
		{FIX16(-0.02), FIX16(-0.48), FIX32(0.002)},
		{FIX16(-0.04), FIX16(-1.28), FIX32(-0.001)},
		{FIX16(-0.02), FIX16(-0.48), FIX32(0.0)},
		{FIX16(0), FIX16(0), FIX32(0.001)},
		{FIX16(0), FIX16(0), FIX32(0.0025)},
		{FIX16(0.03), FIX16(0.64), FIX32(-0.002)},
		{FIX16(-0.03), FIX16(-0.64), FIX32(0)},
		{FIX16(0), FIX16(0), FIX32(0.001)},
		{FIX16(0), FIX16(0), FIX32(-0.0025)},
		{FIX16(0), FIX16(0), FIX32(0.002)},
		{FIX16(0.0), FIX16(0.0), FIX32(0)},
		{FIX16(0), FIX16(0), FIX32(0.0)},
		{FIX16(-0.02), FIX16(-0.48), FIX32(0)},
		{FIX16(0.02), FIX16(0.48), FIX32(0)}};

u16 bottom_segments_index = 0;
u16 segments_index = 0;

// Speed the 'vehicle' moves through teh road
fix16 speed = FIX16(0.00);

// position variables.
fix16 segment_position = FIX16(0); // keep track of the segment position on screen
fix16 background_position = FIX16(SCROLL_CENTER); // handle background X position
s16 horizon_line = 223;							// keep track of where the horizon is

void HIntHandler()
{
	// set vertical scroll based on hill calculations
	VDP_setVerticalScroll(BG_A, VscrollA[lineDisplay]);

/*
	// set color.  No Worky!
	//asm("move.l #0xC0020000, 0xC00004");
	if (lineDisplay % 2)
	{
		asm volatile ( "move.l #0xC0020000, 0xC00004\n\t");
		if (colors[lineDisplay] & 1)
		{
			asm volatile (
				"\tmove.w #0x666, 0xC00000.l\n"
			);
		}
		else
		{
			asm volatile (
				"move.w #0xFFF, 0xC00000 \n\t"
			);
		}
	}
	else
	{
		asm("move.l #0xC0060000, 0xC00004");
		if (colors[lineDisplay] & 1)
		{
			asm("move.w #0x060, 0xC00000");
		}
		else
		{
			asm("move.w #0x0C0, 0xC00000");
		}
	}
*/

	// Move to the next line for the next horizontal interrupt.
	lineDisplay++;
}

void VIntHandler()
{
	// Make sure HInt always starts with line 0
	lineDisplay = 0;
}

// My interpretation of the pseudo-code in
// http://www.extentofthejam.com/pseudo/#curves
// and the hills described in
// http://www.extentofthejam.com/pseudo/#hills
void update()
{
	fix16 current_x = FIX16(0); // Lou's pseudo 3d page says to use Half of the screen width,
															// but I've defined SCROLL_CENTER to handle this

	fix16 dx = FIX16(0);	// Curve Amount, constant per segment.
	fix16 ddx = FIX16(0); // Curve Amount, changes per line

	fix32 dy = FIX32(0);	// Slope Amount
	fix32 ddy = FIX32(0); // Slope Amount, changes per line

	fix32 current_drawing_pos = FIX32(223); // The drawing loop would start at the beginning of the Z-map (nearest).  Basically the bottom of the screen
	horizon_line = 223;									// keep track of where the horizon is.  I"m starting at the bottom and will update as the rode gets computed

	// for each line of the screen from the bottom to the top
	//for (y = 0; y < ZMAP_LENGTH; ++y)  // no longer works because up-hill/down-hill won't be exaclty 1.  ++y isn't valid

	// HILL: draw loop starts at the beginning of the Z map ( nearest = 0 ) and stops at teh end (farthest = ZMAP_LENGTH )
	//     * flat roads decrements the drawing position each line by 1
	//		 * if we decrement the drawing position by 2 (doubling lines) the road gets drawn twice as high.
	//     * by varying the amount we decrement the drawing position we can draw a hill that starts flat and curves upwards
	//				* if the next drawing position is more than one line from the current drawing position, the currnet ZMap line is repeated until
	// 					we get there, producing a scalien effect.
	for (u16 bgY = 223; bgY > 113; bgY--)
	{

		//////////////////////////////////////////////////////////////////////
		// Road Bending
		fix16 z = zmap[223 - bgY]; // zmap[0] is closest
		// if line of screen's Z map position is below segment position
		if (z < segment_position)
		{
			// dx = bottom_segment.dx
			dx = segments[bottom_segments_index].dx;
			dy = segments[bottom_segments_index].dy;
		}
		else // if line of Screen's Z map position is above segment position.
		{
			// dx = segment.dx
			dx = segments[segments_index].dx;
			dy = segments[segments_index].dy;
		}

		ddx += dx;
		current_x += ddx;


		//////////////////////////////////////////////////////////////////////
		// Coloring
		//  For each Z, make one of the bits represent the shade
		//  of the road (dark or light). Then, just draw the
		//  appropriate road pattern or colors for that bit
		u8 zmapval = (u8)F16_toInt(segment_position - z);

		ddy += dy;
		s16 cdp = F32_toInt(current_drawing_pos);				 // current vertical drawing position
		fix32 delta_drawing_pos = FIX32(1) + ddy; // increment drawing position
		fix32 next_drawing_pos = current_drawing_pos - delta_drawing_pos;
		s16 ndp = F32_toInt(next_drawing_pos); // figure out next drawing position
		// repeat line if theres a gap greater than 1
		for (; cdp > ndp; --cdp) //
		{
			if (cdp <= horizon_line) // if current drawing position is above the horizon
			{
				HscrollA[cdp] = SCROLL_CENTER + F16_toInt(current_x); // this_line.x = current x | using horizontal scrolling to fake curves
				VscrollA[cdp] = bgY - cdp;														 // set the vertical scroll amount for the current drawing position
				horizon_line = cdp;																		 // update horizon line

				// coloring
				colors[cdp] = zmapval & 1;
			}
		}

		current_drawing_pos = next_drawing_pos; // move to next drawing position
	}

	// hide anything above the horizon line
	for (s16 h = horizon_line; h >= 0; --h)
	{
		VscrollA[h] = -h;
	}


	// scroll the background
	background_position = background_position - segments[bottom_segments_index].bgdx;
	for (u16 y = 0; y < 160; ++y)
	{
		HscrollB[y] = F16_toInt(background_position);
	}


	// Move segments
	segment_position = segment_position + speed;
	if (F16_toInt(segment_position) < 0)
	{
		// bottom_segment = segment
		bottom_segments_index = segments_index;
		// segment.position = zmap.length - 1
		segment_position = zmap[ZMAP_LENGTH - 1]; // Send segment to farthest visible distance

		// get next segment from road
		segments_index++; // segment_index is used to get segment.dx
		if (segments_index == ROAD_SEGMENTS_LENGTH)
		{
			segments_index -= ROAD_SEGMENTS_LENGTH; // go back to the start
		}
	}
}

int main(bool hard)
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
	VDP_setScrollingMode(HSCROLL_LINE, VSCROLL_PLANE);
	for (int i = 0; i < VERTICAL_REZ; i++)
	{
		HscrollA[i] = SCROLL_CENTER;
		HscrollB[i] = SCROLL_CENTER;
		VscrollA[i] = 0;
	}

	//////////////////////////////////////////////////////////////
	// Setup scroll panes
	PAL_setPalette(PAL0, road_pal.data, CPU);
	int ind = TILE_USER_INDEX;
	VDP_drawImageEx(BG_A, &road, TILE_ATTR_FULL(PAL0, FALSE, FALSE, FALSE, ind), 0, 0, FALSE, TRUE);
	ind += road.tileset->numTile;
	PAL_setPalette(PAL2, background_pal.data, CPU);
	VDP_drawImageEx(BG_B, &background, TILE_ATTR_FULL(PAL2, FALSE, FALSE, FALSE, ind), 0, 0, FALSE, TRUE);
	ind += background.tileset->numTile;
	
	VDP_setVerticalScroll(BG_B, 0);

	//////////////////////////////////////////////////////////////
	// init segments
	bottom_segments_index = 0;
	segments_index = 1;
	segment_position = zmap[ZMAP_LENGTH - 1]; // put it at the farthest away point

	// set spped through z
	speed = FIX16(-0.20);

	//////////////////////////////////////////////////////////////
	// Setup interrupt handlers
	SYS_disableInts();
	{
		VDP_setHIntCounter(0);
		VDP_setHInterrupt(1);

		//SYS_setHIntCallback(HIntHandler);
		//SYS_setVIntCallback(VIntHandler);
	}
	SYS_enableInts();

	// Main loop
	//fix16 colorCycle = FIX16(0.0);
	u16 lastSet = -1;
	while (TRUE)
	{
		// update
		update();

		/* sort of works 
		// cycle colors
		colorCycle = fix16Add( colorCycle, FIX16(0.5));
		u16 temp = F16_toInt(colorCycle);
		if (temp != lastSet)
		{
			lastSet = temp;
			if (temp == 0)
			{
				PAL_setColor(1, RGB24_TO_VDPCOLOR(0x666666));
				PAL_setColor(2, RGB24_TO_VDPCOLOR(0xFFFFFF));
				PAL_setColor(3, RGB24_TO_VDPCOLOR(0x666666));

				PAL_setColor(5, RGB24_TO_VDPCOLOR(0x008800));
				PAL_setColor(6, RGB24_TO_VDPCOLOR(0x00CC00));
				PAL_setColor(7, RGB24_TO_VDPCOLOR(0x008800));
			}
			else if (temp == 1)
			{
				PAL_setColor(1, RGB24_TO_VDPCOLOR(0xFFFFFF));
				PAL_setColor(2, RGB24_TO_VDPCOLOR(0x666666));
				PAL_setColor(3, RGB24_TO_VDPCOLOR(0x666666));

				PAL_setColor(5, RGB24_TO_VDPCOLOR(0x00CC00));
				PAL_setColor(6, RGB24_TO_VDPCOLOR(0x008800));
				PAL_setColor(7, RGB24_TO_VDPCOLOR(0x008800));
			}
			else
			{
				PAL_setColor(1, RGB24_TO_VDPCOLOR(0x666666));
				PAL_setColor(2, RGB24_TO_VDPCOLOR(0x666666));
				PAL_setColor(3, RGB24_TO_VDPCOLOR(0xFFFFFF));

				PAL_setColor(5, RGB24_TO_VDPCOLOR(0x008800));
				PAL_setColor(6, RGB24_TO_VDPCOLOR(0x008800));
				PAL_setColor(7, RGB24_TO_VDPCOLOR(0x00CC00));
			}
			if (temp > 3)
			{
				colorCycle = FIX16(0);
			}
		} 
		*/

		// curve the road with horizontal scrolling
		VDP_setHorizontalScrollLine(BG_A, 0, HscrollA, VERTICAL_REZ, DMA_QUEUE);
		// move the background
		VDP_setHorizontalScrollLine(BG_B, 0, HscrollB, 160, DMA_QUEUE);

		SYS_doVBlankProcess();
	}
}
