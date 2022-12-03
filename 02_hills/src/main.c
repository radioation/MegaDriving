
// SGDK
#include <genesis.h>
#include "resources.h"

#define VERTICAL_REZ 224

// image is 512x224.  Screen is 320, we want to move halfway
// (512-320)/2
#define SCROLL_CENTER -96

// keep track of the current line during Horizontal Interrupts
u16 lineDisplay = 0;

// Horizontal Scrolling values
s16 HscrollA[VERTICAL_REZ];
// Vertical Scrolling values ( to simulate hills )
s16 VscrollA[VERTICAL_REZ];

static void VIntHandler()
{
	// Make sure HIntHander starts with line 0
	lineDisplay = 0;
}

HINTERRUPT_CALLBACK HIntHandler()
{
	// Set vertical scrolling based on hill calculations
	VDP_setVerticalScroll(BG_A, VscrollA[lineDisplay]);

	// Move to next line for the next horizontal interrupt.
	lineDisplay++;
}

// My interpretation of the hills described in
// http://www.extentofthejam.com/pseudo/#hills

// dy1 : slope amount, constant for segment 1
// dy2 : slope amount, constant for segment 2
// segment line : transition point between dy1 and dy2
void CreateHills(fix32 dy1, fix32 dy2, u16 segmentLine)
{
	fix32 current_drawing_pos = FIX32(223); // The drawing loop would start at the beginning of the Z-map (nearest).  Basically the bottom of the screen
	s16 horizon_line = 223;									// keep track of where the horizon is.  I"m starting at the bottom and will update as the rode gets computed

	fix32 dy = dy1; // y delta determines if we're sloping up or down

	fix32 ddy = FIX32(0); // slope amount, changes per line

	// iterate over every line in the road, closest to farthest Z
	for (u16 bgY = 223; bgY > 115; bgY--)
	{
		s16 cdp = fix32ToInt(current_drawing_pos); // get current drawing position as an int

		if (bgY == segmentLine) // simulate two segments
		{
			dy = dy2; // hill value for segment 2
		}

		if (cdp <= horizon_line) // if current drawing position is above the horizon
		{
			VscrollA[cdp] = bgY - cdp; // set vertical scroll amount for current drawing position
			horizon_line = cdp;				 // update horizon line
		}

		ddy = fix32Add(dy, ddy);
		fix32 delta_drawing_pos = fix32Add(FIX32(1), ddy); // increment drawing position

		fix32 next_drawing_pos = fix32Sub(current_drawing_pos, delta_drawing_pos); // figure out next drawing position
		s16 ndp = fix32ToInt(next_drawing_pos);
		KLog_S2(" cdp: ", cdp, " ndp: ", ndp);
		if (cdp - ndp > 1) // need to set Vertical scrolling value if the next drawing position is farther than one line.
		{
			// repeat line
			//	cdp + 1;
			for (; cdp > ndp; --cdp)
			{
				KLog_S2("    bgY: ", bgY, "   bgY- cdp: ", (bgY - cdp));
				if (cdp <= horizon_line)
				{
					VscrollA[cdp] = bgY - cdp;
					horizon_line = cdp;
				}
			}
		}
		current_drawing_pos = next_drawing_pos; // move to next drawing position
	}

	// hide anything above the horizon line.
	for (s16 h = horizon_line - 1; h >= 16; --h)
	{
		VscrollA[h] = -h;
	}
}

void handleJoypad()
{
	u16 value = JOY_readJoypad(JOY_1);

	if (value & BUTTON_A)
	{
		VDP_drawText("dy1: 0.015, dy2: 0, seg-line: 0       ", 13, 1);
		CreateHills(FIX32(0.015), FIX32(0), 0);
	}
	if (value & BUTTON_B)
	{
		VDP_drawText("dy1: -0.01, dy2: 0, seg-line: 0      ", 13, 1);
		CreateHills(FIX32(-0.01), FIX32(0), 0);
	}
	if (value & BUTTON_C)
	{
		VDP_drawText("dy1: 0.015, dy2: -0.01, seg-line: 160", 13, 1);
		CreateHills(FIX32(0.015), FIX32(-0.01), 160);
	}
	if (value & BUTTON_X)
	{
		VDP_drawText("dy1: -0.007, dy2: 0.06, seg-line: 160", 13, 1);
		CreateHills(FIX32(-0.007), FIX32(0.06), 160);
	}
}

int main(bool hard)
{

	//////////////////////////////////////////////////////////////
	// VDP basic setup
	VDP_setBackgroundColor(16);
	VDP_setScreenWidth320();

	//////////////////////////////////////////////////////////////
	// Initialize horizontal scrolling values to the center of the image
	VDP_setScrollingMode(HSCROLL_LINE, VSCROLL_PLANE);
	for (int i = 0; i < VERTICAL_REZ; i++)
	{
		HscrollA[i] = SCROLL_CENTER;
		VscrollA[i] = 0;
	}

	// setup initial hill (no slope)
	CreateHills(FIX32(0), FIX32(0.04), 145);

	//////////////////////////////////////////////////////////////
	// Setup background A
	PAL_setPalette(PAL1, road_pal.data, CPU);
	int ind = TILE_USER_INDEX;
	VDP_drawImageEx(BG_A, &road, TILE_ATTR_FULL(PAL1, FALSE, FALSE, FALSE, ind), 0, 0, FALSE, TRUE);

	//////////////////////////////////////////////////////////////
	// Setup interrupt handlers
	SYS_disableInts();
	{
		VDP_setHIntCounter(0);
		VDP_setHInterrupt(1);

		SYS_setHIntCallback(HIntHandler);
		SYS_setVIntCallback(VIntHandler);
	}


	SYS_enableInts();
	// Main loop
	while (TRUE)
	{
		VDP_drawText("Press A, B, C or X", 15, 0);
		// update
		handleJoypad();

		// do horizontal scrolling to center the background
		VDP_setHorizontalScrollLine(BG_A, 0, HscrollA, VERTICAL_REZ, DMA_QUEUE);
		SYS_doVBlankProcess();
	}
}
