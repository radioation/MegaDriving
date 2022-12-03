
// SGDK
#include <genesis.h>
#include "resources.h"

// image is 512x224.  Screen is 320, we want to move halfway
// (512-320)/2
#define SCROLL_CENTER -96

// Horizontal Scrolling values
s16 HscrollA[224];

// Create example curves.
// dx: curve amount.  Constant over the image
void CreateCurve(fix32 dx)
{
	fix32 current_x = FIX32(0); // current x shift
	fix32 ddx = FIX32(0);				// Cumulative Curve amount. Changes every line

	// start from the bottom of the screen and move up.
	for (u16 bgY = 223; bgY >= 116; bgY--)
	{
		ddx = fix32Add(dx, ddx);
		current_x = fix32Add(current_x, ddx);

		// store the current x in HscrollA to shift the road in the main loop
		HscrollA[bgY] = SCROLL_CENTER + fix32ToInt(current_x);
	}
}

void handleJoypad()
{
	u16 value = JOY_readJoypad(JOY_1);
	if (value & BUTTON_A)
	{
		// curve
		VDP_drawText("dx =  0.02", 15, 3);
		CreateCurve(FIX32(0.02));
	}
	if (value & BUTTON_B)
	{
		VDP_drawText("dx = -0.015", 15, 3);
		CreateCurve(FIX32(-0.015));
	}
	if (value & BUTTON_C)
	{
		VDP_drawText("dx =  0.005", 15, 3);
		CreateCurve(FIX32(0.005));
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
	for (int i = 0; i < 224; i++)
	{
		HscrollA[i] = SCROLL_CENTER;
	}

	//////////////////////////////////////////////////////////////
	// Setup background A
	PAL_setPalette(PAL1, road_pal.data, CPU);
	int ind = TILE_USER_INDEX;
	VDP_drawImageEx(BG_A, &road, TILE_ATTR_FULL(PAL1, FALSE, FALSE, FALSE, ind), 0, 0, FALSE, TRUE);

	VDP_drawText("Press A, B, or C to Curve", 15, 1);
	// Main loop
	while (TRUE)
	{
		// update curve based on user input (A,B,C)
		handleJoypad();

		// curve the road with horizontal scrolling
		VDP_setHorizontalScrollLine(BG_A, 0, HscrollA, 224, DMA_QUEUE);

		SYS_doVBlankProcess();
	}
}
