
// SGDK
#include <genesis.h>
#include "resources.h"

#define HORIZONTAL_REZ 224
void LineDark();
void LineLight();
// image is 512x224.  Screen is 320, we want to move halfway
//  512/2  - 320 /2
#define SCROLL_CENTER -96

// Keep track of the current line during Horizontal Interrupts
u16 lineDisplay = 0;

// Horizontal Scrolling values
s16 HscrollA[HORIZONTAL_REZ];
s16 HscrollB[HORIZONTAL_REZ];
// Vertical Scrolling values ( to simulate hills )
s8 VscrollA[HORIZONTAL_REZ];
fix32 roadOffsetRight[224]; // X offset from side of road ( for positioning the sprites 
fix32 roadOffsetLeft[224]; // X offset from side of road ( for positioning the sprites 


// color banding array
u8 colors[HORIZONTAL_REZ];
u8 line_color = 0; // 0 -uninit, 1-light, 2-dark
u8 side_color = 0;
u8 grass_color = 0; // 0 -uninit, 1-light, 2-dark


// Zmap for tracking segment position
#define ZMAP_LENGTH 110 // slighty more than the bg horizon 
fix32 zmap[ZMAP_LENGTH];
fix32 scale[ZMAP_LENGTH];

// Road data
#define ROAD_SEGMENTS_LENGTH 15
typedef struct
{
	fix32 dx;		// rate of change for the road.
	fix32 bgdx; // rate of change for background. ( ignore for now )
	fix32 dy;		//  rate of change for drawing road in y dir?
} ROAD_SEGMENT;

const ROAD_SEGMENT segments[ROAD_SEGMENTS_LENGTH] = {
		{FIX32(0), FIX32(0), FIX32(-0.000)},
		{FIX32(-0.02), FIX32(-0.48), FIX32(0.002)},
		{FIX32(-0.04), FIX32(-1.28), FIX32(-0.001)},
		{FIX32(-0.02), FIX32(-0.48), FIX32(0.0)},
		{FIX32(0), FIX32(0), FIX32(0.001)},
		{FIX32(0), FIX32(0), FIX32(0.0025)},
		{FIX32(0.03), FIX32(0.64), FIX32(-0.002)},
		{FIX32(-0.03), FIX32(-0.64), FIX32(0)},
		{FIX32(0), FIX32(0), FIX32(0.001)},
		{FIX32(0), FIX32(0), FIX32(-0.0025)},
		{FIX32(0), FIX32(0), FIX32(0.002)},
		{FIX32(0.0), FIX32(0.0), FIX32(0)},
		{FIX32(0), FIX32(0), FIX32(0.0)},
		{FIX32(-0.02), FIX32(-0.48), FIX32(0)},
		{FIX32(0.02), FIX32(0.48), FIX32(0)}};

u16 bottom_segments_index = 0;
u16 segments_index = 0;

// Speed the 'vehicle' moves through teh road
fix32 speed = FIX32(0.00);

// position variables.
fix32 segment_position = FIX32(0); // keep track of the segment position on screen
fix32 background_position = FIX32(SCROLL_CENTER); // handle background X position
s16 horizon_line = 223;							// keep track of where the horizon is


// Sprites
struct CP_SPRITE {
	Sprite *sprite;
	fix32 pos_x;
	fix32 pos_y;
	fix32 zpos; // track position along the road. start it at the farthest on background - 12.5
	u8 update_y;
};
struct CP_SPRITE carSprite;
#define NUMBER_OF_TREES 6
struct CP_SPRITE trees[NUMBER_OF_TREES];

void createTrees() {
	for (u16 i = 0; i < NUMBER_OF_TREES; ++i)
	{
		//trees[i] = malloc( sizeof(struct CP_SPRITE));
		if( i < 2 ) {
			trees[i].zpos = FIX32(12.5);
			// sprite width is 56 - so 28
			/*
			if ( i%2 ) {
				trees[i].pos_x = FIX32(160 - 28 - 15);
			} else {
				trees[i].pos_x = FIX32(160 - 28 + 15);
			}
			trees[i].pos_y = FIX32(42);
			*/
		} else if( i < 4 ) {
			trees[i].zpos = FIX32(8);
			/*
			if ( i%2 ) {
				trees[i].pos_x = FIX32(160 - 28 - 45);
			} else {
				trees[i].pos_x = FIX32(160 - 28 + 45);
			}
			trees[i].pos_y = FIX32(74);
			*/
		} else {
			trees[i].zpos = FIX32(4);
			/*
			if ( i%2 ) {
				trees[i].pos_x = FIX32(160 - 28 - 45);
			} else {
				trees[i].pos_x = FIX32(160 - 28 + 45);
			}

			trees[i].pos_y = FIX32(74);
			*/
		}
		trees[i].update_y = 1;
		trees[i].sprite = SPR_addSprite(&tree,
																		 fix32ToInt(trees[i].pos_x),
																		 fix32ToInt(trees[i].pos_y),
																		 TILE_ATTR(PAL3, 0, FALSE, FALSE));
		SPR_setFrame( trees[i].sprite, 4 );
		SPR_setDepth( trees[i].sprite, 3 );
	}

}

void updateTrees() {
	for (u16 i = 0; i < NUMBER_OF_TREES; ++i)
	{
		// figure out z position
		trees[i].zpos = fix32Add( trees[i].zpos, speed );
		if( trees[i].zpos < FIX32(-0.0)) {
			trees[i].zpos = FIX32(12.5);
		}
		// figure out scale 
		if( trees[i].zpos < FIX32( 1.01)  ) {
			SPR_setFrame( trees[i].sprite, 0 );
		}else if( trees[i].zpos < FIX32( 1.32 ) ) {
			SPR_setFrame( trees[i].sprite, 1 );
		}else if( trees[i].zpos < FIX32(2.01) ) {
			SPR_setFrame( trees[i].sprite, 2 );
		}else if( trees[i].zpos < FIX32(4.15) ) {
			SPR_setFrame( trees[i].sprite, 3 );
		}else {
			SPR_setFrame( trees[i].sprite, 4 );
		}
		// actual Y will have to come from background update
		trees[i].update_y = 1;
	}
}


// My interpretation of the pseudo-code in
// http://www.extentofthejam.com/pseudo/#curves
// and the hills described in
// http://www.extentofthejam.com/pseudo/#hills
void update()
{
	fix32 current_x = FIX32(0); // Lou's pseudo 3d page says to use Half of the screen width,
															// but I've defined SCROLL_CENTER to handle this

	fix32 dx = FIX32(0);	// Curve Amount, constant per segment.
	fix32 ddx = FIX32(0); // Curve Amount, changes per line

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
		fix32 z = zmap[223 - bgY]; // zmap[0] is closest
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

		// ddx += dx
		ddx = fix32Add(ddx, dx);
		// current_x += ddx
		current_x = fix32Add(current_x, ddx);


		//////////////////////////////////////////////////////////////////////
		// Coloring
		//  For each Z, make one of the bits represent the shade
		//  of the road (dark or light). Then, just draw the
		//  appropriate road pattern or colors for that bit
		u8 zmapval = (u8)fix32ToInt(fix32Sub(segment_position, z));

		ddy = fix32Add(dy, ddy);
		s16 cdp = fix32ToInt(current_drawing_pos);				 // current vertical drawing position
		fix32 delta_drawing_pos = fix32Add(FIX32(1), ddy); // increment drawing position
		fix32 next_drawing_pos = fix32Sub(current_drawing_pos, delta_drawing_pos);
		s16 ndp = fix32ToInt(next_drawing_pos); // figure out next drawing position
		// repeat line if theres a gap greater than 1
		for (; cdp > ndp; --cdp) //
		{
			if (cdp <= horizon_line) // if current drawing position is above the horizon
			{
				HscrollA[cdp] = SCROLL_CENTER + fix32ToInt(current_x); // this_line.x = current x | using horizontal scrolling to fake curves
				VscrollA[cdp] = bgY - cdp;														 // set the vertical scroll amount for the current drawing position
				horizon_line = cdp;																		 // update horizon line

				// coloring
				colors[cdp] = zmapval & 1;

			}
		}

		// sprite update
		for (u16 i = 0; i < NUMBER_OF_TREES; ++i)
		{
			if (trees[i].update_y == 1)
			{
				// check if z is close.
				if (z > trees[i].zpos)
				{
					trees[i].pos_y = fix32Sub(current_drawing_pos, FIX32(75));
					if( i%2 == 0 ) {
						trees[i].pos_x = fix32Add( fix32Add( FIX32(160), current_x ), roadOffsetLeft[bgY] );
					} else {
						trees[i].pos_x = fix32Add( fix32Add( FIX32(160), current_x ), roadOffsetRight[bgY] );
					}
					trees[i].update_y = 0;
				}
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
	background_position = fix32Sub(background_position, segments[bottom_segments_index].bgdx);
	for (u16 y = 0; y < 160; ++y)
	{
		HscrollB[y] = fix32ToInt(background_position);
	}

	// Move segments
	segment_position = fix32Add(segment_position, speed);
	if (fix32ToInt(segment_position) < 0)
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

int main(u16 hard)
{
	//////////////////////////////////////////////////////////////
	// http://www.extentofthejam.com/pseudo/
	// Z = Y_world / (Y_screen - (height_screen / 2))
	// this gets me 0.65 nearest and 25.0 farthest
	for (int i = 0; i < ZMAP_LENGTH; ++i)
	{
		zmap[i] = fix32Div(FIX32(-75), fix32Sub(FIX32(i), FIX32(112)));
		scale[i] = fix32Div(FIX32(1), zmap[i]);
		KLog_F3("i: ", FIX32(i), " z: ", zmap[i], " s: ", scale[i]);
	}


	//////////////////////////////////////////////////////////////
	// Precompute road offsets for roadside sprites
	// 116 | 262-256 = 6
	// 223 | 415-256 = 159
	// 159 - 6 =  153 
	// 223 - 116 =  107
	// step size 153/107  1.43  << step size per line 
	// 
	// Looks better w/ more padding
	// (159 + 42) - (6+3) =  192 
	// step size 192/107  1.794 << step size per line 
	fix32 rightFromCenter = FIX32( -22 ); // tree width is 56 ..   half of 56 is 28 
	fix32 leftFromCenter = FIX32( -34 ); // tree width is 56 ..   half of 56 is 28 
	fix32 step = FIX32( 1.794 );
	for (int i = 224 - ZMAP_LENGTH; i < 224; i++)
	{
		roadOffsetRight[i] = rightFromCenter;
		rightFromCenter = fix32Add(rightFromCenter, step);
		roadOffsetLeft[i] = leftFromCenter;
		leftFromCenter = fix32Sub(leftFromCenter, step);
		KLog_F2(" i: ", FIX32(i), "  road offset: ", roadOffsetRight[i]);
	}



	//////////////////////////////////////////////////////////////
	// VDP basic setup
	VDP_setBackgroundColor(16);
	VDP_setScreenWidth320();
	VDP_setScrollingMode(HSCROLL_LINE, VSCROLL_PLANE);
	for (int i = 0; i < HORIZONTAL_REZ; i++)
	{
		HscrollA[i] = SCROLL_CENTER;
		HscrollB[i] = SCROLL_CENTER;
		VscrollA[i] = 0;
	}

	//////////////////////////////////////////////////////////////
	// Setup scroll panes
	VDP_setPalette(PAL0, road.palette->data);
	int ind = TILE_USERINDEX;
	VDP_drawImageEx(BG_A, &road, TILE_ATTR_FULL(PAL0, FALSE, FALSE, FALSE, ind), 0, 0, FALSE, TRUE);
	ind += road.tileset->numTile;
	VDP_setPalette(PAL1, background.palette->data);
	VDP_drawImageEx(BG_B, &background, TILE_ATTR_FULL(PAL1, FALSE, FALSE, FALSE, ind), 0, 0, FALSE, TRUE);
	ind += background.tileset->numTile;

	VDP_setVerticalScroll(BG_B, 0);

	//////////////////////////////////////////////////////////////
	// Setup Sprites
	SPR_init();
	VDP_setPalette(PAL2, car.palette->data);
	carSprite.sprite = NULL;
	carSprite.pos_x = FIX32(116.0);
	carSprite.pos_y = FIX32(160.0);
	carSprite.sprite = SPR_addSprite(&car,												// Sprite defined in resources
																	 fix32ToInt(carSprite.pos_x), // starting X position
																	 fix32ToInt(carSprite.pos_y), // starting Y position
																	 TILE_ATTR(PAL2,							// specify palette
																						 1,									// Tile priority ( with background)
																						 FALSE,							// flip the sprite vertically?
																						 FALSE							// flip the sprite horizontally
																						 ));
	SPR_setFrame(carSprite.sprite, 2);

	VDP_setPalette(PAL3, tree.palette->data);
	createTrees();

	//////////////////////////////////////////////////////////////
	// init segments
	bottom_segments_index = 0;
	segments_index = 1;
	segment_position = zmap[ZMAP_LENGTH - 1]; // put it at the farthest away point

	// set spped through z
	speed = FIX32(-0.40);
	//speed = FIX32(-0.1);

	//////////////////////////////////////////////////////////////
	// Setup interrupt handlers
	SYS_disableInts();
	{
		VDP_setHIntCounter(0);
		VDP_setHInterrupt(1);
	}
	SYS_enableInts();

	// Main loop
	u16 lastSet = -1;
	while (TRUE)
	{
		// update
		updateTrees();
		update();



		for (u16 i = 0; i < NUMBER_OF_TREES; ++i)
		{
			// update z-order  for trees
    	SPR_setDepth(trees[i].sprite, 224 - fix32ToInt(trees[i].pos_y) );
			// Draw tree at new position
			SPR_setPosition(trees[i].sprite, fix32ToInt(trees[i].pos_x), fix32ToInt(trees[i].pos_y));

		}

		// Draw car at now position
		SPR_setPosition(carSprite.sprite, fix32ToInt(carSprite.pos_x), fix32ToInt(carSprite.pos_y));

		fix32 dx = segments[bottom_segments_index].dx;
		if (dx < FIX32(-0.02) )
		{
			SPR_setFrame(carSprite.sprite, 0);
		}
		else if (dx < FIX32(0.0) )
		{
			SPR_setFrame(carSprite.sprite, 1);
		}
		else if (dx >= FIX32(0.02) )
		{
			SPR_setFrame(carSprite.sprite, 4);
		}
		else if (dx > FIX32(0.0) )
		{
			SPR_setFrame(carSprite.sprite, 3);
		}
		else
		{
			SPR_setFrame(carSprite.sprite, 2);
		}

		SPR_update();

		// curve the road with horizontal scrolling
		VDP_setHorizontalScrollLine(BG_A, 0, HscrollA, HORIZONTAL_REZ, DMA_QUEUE);
		// move the background
		VDP_setHorizontalScrollLine(BG_B, 0, HscrollB, 160, DMA_QUEUE);

		fix32 h= FIX32( horizon_line - 113 );
		h = fix32Div( h, FIX32(6));
		VDP_setVerticalScroll( BG_B, fix32ToInt( h ));

		SYS_doVBlankProcess();
	}
}
