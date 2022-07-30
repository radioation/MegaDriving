#include <genesis.h>
#include "resources.h"

#define VERTICAL_REZ 224
// image is 512x224.  Screen is 320, we want to move halfway
//  512/2  - 320 /2
#define SCROLL_CENTER -96

// Keep track of the current line during Horizontal Interrupts
u16 lineDisplay = 0;

// Horizontal Scrolling values
s16 HscrollA[VERTICAL_REZ];
// Vertical Scrolling values ( to simulate hills )
extern u16 VscrollA[VERTICAL_REZ];

// color array for grass and road stripe palette changes.
extern u16 colors[VERTICAL_REZ];

// Zmap for tracking segment position -
#define ZMAP_LENGTH 111 
fix32 zmap[ZMAP_LENGTH];
fix32 hscrollIncrement[ZMAP_LENGTH];
fix32 hscrollIncrement2[ZMAP_LENGTH];
fix32 workScrollA[VERTICAL_REZ]; // working buffer to calculate actual scroll value
s16 scrollSteps = 0;


fix32 colorCyclePosition = FIX32(0);						// keep track of the color cycling position
fix16 backgroundPosition = FIX16(SCROLL_CENTER); // handle background X position

extern s16 horizonLine;										// keep track of where the horizon is

fix32 centerLine = FIX32(160); // center line at the front (bottom) of the screen

// Sprites
struct CP_SPRITE
{
	Sprite *sprite;
	fix32 posX;					// horizontal screen position
	fix32 posY;					// vertical screen position
	fix32 posZ;					// z position along the visible segments.
	fix32 position;			// track position along the *entire* road. start it at the farthest on background - 12.5
	fix32 speed;				// sped the car travles through the road
	u16 segment_index;	// current road segment of the sprite
	u16 offsetY;				// amount to move the sprite up (since these aren't centered)
	u16 offsetX;				// amount to move the sprite left (since these aren't centered)
	u16 updateY;				// refresh flag for update() 
};
s16 playerXDir = 0;
s16 playerYDir = 0;

struct CP_SPRITE *playerSprite;
struct CP_SPRITE *playerShadowSprite;
struct CP_SPRITE *bossSprite;
struct CP_SPRITE *bossShadowSprite;

// joypad event handler.  This gets called automatically by SGDK when the joypad
// state changes
static void joypadHandler(u16 joypadId, u16 changed, u16 state)
{
	if (joypadId == JOY_1)
	{
		if (state & BUTTON_RIGHT)
		{
			playerXDir = 1; // Speed up
		}
		else if (changed & BUTTON_RIGHT)
		{
			playerXDir = 0; // state change A-up. no longer accelerating.
		}

		if (state & BUTTON_LEFT)
		{
			playerXDir = -1; // Slow down
		}
		else if (changed & BUTTON_LEFT)
		{
			playerXDir = 0;
		}
	}
	else if (state & BUTTON_DOWN)
	{
	}
}

void updatePlayer()
{
	// player position affects H/Y
	if (playerXDir < 0)
	{
		playerSprite->posX = fix32Sub(playerSprite->posX, FIX32(2));
		if (playerSprite->posX < FIX32(playerSprite->offsetX))
		{
			playerSprite->posX = FIX32(playerSprite->offsetX);
		}
	}
	else if (playerXDir > 0)
	{
		playerSprite->posX = fix32Add(playerSprite->posX, FIX32(2));
		if (playerSprite->posX > FIX32(292))
		{
			playerSprite->posX = FIX32(292);
		}
	}
	playerShadowSprite->posX = playerSprite->posX;

	// determine frame
	SPR_setFrame(playerSprite->sprite, 0);
}

// My interpretation of the pseudo-code in
// http://www.extentofthejam.com/pseudo/#curves
// and the hills described in
// http://www.extentofthejam.com/pseudo/#hills
void update()
{

	if (fix32ToInt(playerSprite->posX) < 68)
	{
		for (int i = 223, j = ZMAP_LENGTH - 1; i > 223 - ZMAP_LENGTH; --i, --j)
		{
			workScrollA[i] = fix32Add(workScrollA[i], hscrollIncrement2[j]);
			HscrollA[i] = fix32ToInt(workScrollA[i]) + SCROLL_CENTER;
		}
		scrollSteps += 8;
	}
	else if (fix32ToInt(playerSprite->posX) < 148)
	{

		for (int i = 223, j = ZMAP_LENGTH - 1; i > 223 - ZMAP_LENGTH; --i, --j)
		{
			workScrollA[i] = fix32Add(workScrollA[i], hscrollIncrement[j]);
			HscrollA[i] = fix32ToInt(workScrollA[i]) + SCROLL_CENTER;
		}
		scrollSteps += 4;
	}
	else if (fix32ToInt(playerSprite->posX) > 252)
	{
		for (int i = 223, j = ZMAP_LENGTH - 1; i > 223 - ZMAP_LENGTH; --i, --j)
		{
			workScrollA[i] = fix32Sub(workScrollA[i], hscrollIncrement2[j]);
			HscrollA[i] = fix32ToInt(workScrollA[i]) + SCROLL_CENTER;
		}
		scrollSteps -= 8;
	}
	else if (fix32ToInt(playerSprite->posX) > 180)
	{
		for (int i = 223, j = ZMAP_LENGTH - 1; i > 223 - ZMAP_LENGTH; --i, --j)
		{
			workScrollA[i] = fix32Sub(workScrollA[i], hscrollIncrement[j]);
			HscrollA[i] = fix32ToInt(workScrollA[i]) + SCROLL_CENTER;
		}
		scrollSteps -= 4;
	}

	if (scrollSteps >= 64 || scrollSteps <= -64)
	{
		scrollSteps = 0;
		memset(workScrollA, 0, sizeof(workScrollA));
	}
}

int main(u16 hard)
{

	//////////////////////////////////////////////////////////////
	// precalculate some stuff
	fix32 step = fix32Div(FIX32(4), FIX32(ZMAP_LENGTH));	// divide bottom scroll increment by the height of the ground graphic.
	fix32 step2 = fix32Div(FIX32(8), FIX32(ZMAP_LENGTH)); // bigger step
	fix32 currentXDelta = FIX32(0);
	fix32 currentXDelta2 = FIX32(0);
	for (int i = 0; i < ZMAP_LENGTH; ++i)
	{
		// http://www.extentofthejam.com/pseudo/
		// Z = Y_world / (Y_screen - (height_screen / 2))
		zmap[i] = fix32Div(FIX32(-75), fix32Sub(FIX32(i), FIX32(113)));
		hscrollIncrement[i] = currentXDelta;
		currentXDelta = fix32Add(currentXDelta, step);
		hscrollIncrement2[i] = currentXDelta2;
		currentXDelta2 = fix32Add(currentXDelta2, step2);
		KLog_F4("i: ", FIX32(i), " z: ", zmap[i], " d4: ", hscrollIncrement[i], " d8: ", hscrollIncrement2[i]);
	}

	//////////////////////////////////////////////////////////////
	// VDP basic setup
	VDP_setScreenWidth320();
	VDP_setScrollingMode(HSCROLL_LINE, VSCROLL_PLANE);

	// initialize scrolling arrays
	for (int i = 0; i < VERTICAL_REZ; i++)
	{
		HscrollA[i] = SCROLL_CENTER;
		VscrollA[i] = 0;
		workScrollA[i] = FIX32(0);
	}

	//////////////////////////////////////////////////////////////
	// Setup scroll panes
	VDP_setPalette(PAL0, ground.palette->data);
	int ind = TILE_USERINDEX;
	VDP_drawImageEx(BG_A, &ground, TILE_ATTR_FULL(PAL0, FALSE, FALSE, FALSE, ind), 0, 0, FALSE, TRUE);

	//////////////////////////////////////////////////////////////
	// Setup Car Sprites
	SPR_init();
	// SPR_initEx(900);
	VDP_setPalette(PAL1, player.palette->data);
	playerSprite = malloc(sizeof(struct CP_SPRITE));
	playerSprite->position = FIX32(0);
	playerSprite->segment_index = 0;
	playerSprite->offsetY = 40; // 80~ish tall
	playerSprite->offsetX = 28; // 56~ish wide
	playerSprite->sprite = NULL;
	playerSprite->posX = FIX32(160.0);
	playerSprite->posY = FIX32(170.0);																													 //
	playerSprite->sprite = SPR_addSprite(&player,																								 // Sprite defined in resources
																			 fix32ToInt(playerSprite->posX) - playerSprite->offsetX, // starting X position
																			 fix32ToInt(playerSprite->posY) - playerSprite->offsetY, // starting Y position
																			 TILE_ATTR(PAL1,																				 // specify palette
																								 1,																						 // Tile priority ( with background)
																								 FALSE,																				 // flip the sprite vertically?
																								 FALSE																				 // flip the sprite horizontally
																								 ));

	SPR_setAnim(playerSprite->sprite, 1);
	SPR_setFrame(playerSprite->sprite, 0);
	SPR_setDepth(playerSprite->sprite, 0);

	playerShadowSprite = malloc(sizeof(struct CP_SPRITE));
	playerShadowSprite->position = FIX32(0);
	playerShadowSprite->segment_index = 0;
	playerShadowSprite->offsetY = 8;
	playerShadowSprite->offsetX = 28;
	playerShadowSprite->sprite = NULL;
	playerShadowSprite->posX = FIX32(160.0);
	playerShadowSprite->posY = FIX32(210.0);																																			 //
	playerShadowSprite->sprite = SPR_addSprite(&shadow,																														 // Sprite defined in resources
																						 fix32ToInt(playerShadowSprite->posX) - playerShadowSprite->offsetX, // starting X position
																						 fix32ToInt(playerShadowSprite->posY) - playerShadowSprite->offsetY, // starting Y position
																						 TILE_ATTR(PAL1,																										 // specify palette
																											 1,																												 // Tile priority ( with background)
																											 FALSE,																										 // flip the sprite vertically?
																											 FALSE																										 // flip the sprite horizontally
																											 ));

	VDP_setPalette(PAL2, boss.palette->data);
	bossSprite = malloc(sizeof(struct CP_SPRITE));
	bossSprite->position = FIX32(0);
	bossSprite->segment_index = 0;
	bossSprite->offsetY = 50; // 56 tall bottom is 51 ish
	bossSprite->offsetX = 44; // 88 wide
	bossSprite->sprite = NULL;
	bossSprite->posX = FIX32(162.0);
	bossSprite->posY = FIX32(60.0);																	 //
	bossSprite->sprite = SPR_addSprite(&boss,												 // Sprite defined in resources
																		 fix32ToInt(bossSprite->posX), // starting X position
																		 fix32ToInt(bossSprite->posY), // starting Y position
																		 TILE_ATTR(PAL2,							 // specify palette
																							 1,									 // Tile priority ( with background)
																							 FALSE,							 // flip the sprite vertically?
																							 FALSE							 // flip the sprite horizontally
																							 ));

	bossShadowSprite = malloc(sizeof(struct CP_SPRITE));
	bossShadowSprite->position = FIX32(0);
	bossShadowSprite->segment_index = 0;
	bossShadowSprite->offsetY = 50; // 56 tall bottom is 51 ish
	bossShadowSprite->offsetX = 44; // 88 wide
	bossShadowSprite->sprite = NULL;
	bossShadowSprite->posX = FIX32(180.0);
	bossShadowSprite->posY = FIX32(180.0);																			 //
	bossShadowSprite->sprite = SPR_addSprite(&shadow,														 // Sprite defined in resources
																					 fix32ToInt(bossShadowSprite->posX), // starting X position
																					 fix32ToInt(bossShadowSprite->posY), // starting Y position
																					 TILE_ATTR(PAL1,										 // specify palette
																										 1,												 // Tile priority ( with background)
																										 FALSE,										 // flip the sprite vertically?
																										 FALSE										 // flip the sprite horizontally
																										 ));
	//////////////////////////////////////////////////////////////
	// init segments
	colorCyclePosition = zmap[ZMAP_LENGTH - 1]; // put it at the farthest away point

	//////////////////////////////////////////////////////////////
	// Setup interrupt handlers
	SYS_disableInts();
	{
		VDP_setHIntCounter(0);
		VDP_setHInterrupt(1);
	}
	//	SYS_enableInts();

	// Asynchronous joystick handler.
	JOY_init();
	JOY_setEventHandler(joypadHandler);

	// Main loop
	scrollSteps = 0;
	while (TRUE)
	{
		// update background
		update();

		// Set player position
		updatePlayer();
		SPR_setPosition(playerSprite->sprite, fix32ToInt(playerSprite->posX) - playerSprite->offsetX, fix32ToInt(playerSprite->posY) - playerSprite->offsetY);
		SPR_setPosition(playerShadowSprite->sprite, fix32ToInt(playerShadowSprite->posX) - playerShadowSprite->offsetX, fix32ToInt(playerShadowSprite->posY) - playerShadowSprite->offsetY);

		// update tsprites
		SPR_update();

		VDP_setHorizontalScrollLine(BG_A, 0, HscrollA, VERTICAL_REZ, DMA_QUEUE);

		SYS_doVBlankProcess();
	}
}
