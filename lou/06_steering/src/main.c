
// SGDK
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
s16 HscrollB[VERTICAL_REZ];
// Vertical Scrolling values ( to simulate hills )
extern u16 VscrollA[VERTICAL_REZ];

// color array for grass and road stripe palette changes.
extern u16 colors[VERTICAL_REZ];

// Zmap for tracking segment position -
#define ZMAP_LENGTH 97 
fix32 zmap[ZMAP_LENGTH];
//fix16 scale[ZMAP_LENGTH];  // Could potentially store a scale value for handling sprites sizes and positions

// precalculated road variables for placing sprites
fix32 roadSideObjectOffset[VERTICAL_REZ]; // X offset from side of road ( for positioning the sprites
fix32 yToZdist[VERTICAL_REZ];
fix32 yToRoadCenter[VERTICAL_REZ];
fix32 roadSideOffset[VERTICAL_REZ]; // X offset from side of road ( for positioning the sprites


// Road data
typedef struct
{
	fix32 dx;			// rate of change for the road.
	fix16 bgdx; 	// rate of change for background. ( ignore for now )
	fix32 dy;			// rate of change for drawing road in y dir?
	fix32 length;	// length of this road segment.
	fix32 carX;		// amount to shift car in X dir for a segment
} ROAD_SEGMENT;


// predefined road segments
#define ROAD_SEGMENTS_LENGTH 31
const ROAD_SEGMENT segments[ROAD_SEGMENTS_LENGTH] = {
		{FIX32(-0.000), FIX16(0.000), FIX32(0.002), FIX32(5), FIX32(0)},
		// curve left slowly
		{FIX32(-0.001), FIX16(-0.04), FIX32(0.001), FIX32(1), FIX32(-0.6)},
		{FIX32(-0.002), FIX16(-0.08), FIX32(0.002), FIX32(1), FIX32(-1.2)},
		{FIX32(-0.004), FIX16(-0.16), FIX32(0.003), FIX32(1), FIX32(-2.4)},
		{FIX32(-0.006), FIX16(-0.24), FIX32(0.002), FIX32(1), FIX32(-3.6)},
		{FIX32(-0.008), FIX16(-0.32), FIX32(0.002), FIX32(1), FIX32(-4.8)},
		{FIX32(-0.010), FIX16(-0.40), FIX32(0.001), FIX32(1), FIX32(-6.0)},
		{FIX32(-0.012), FIX16(-0.48), FIX32(0.001), FIX32(1), FIX32(-7.2)},
		{FIX32(-0.014), FIX16(-0.56), FIX32(0.001), FIX32(7), FIX32(-8.4)},
		{FIX32(-0.012), FIX16(-0.48), FIX32(0.000), FIX32(1), FIX32(-7.2)},
		{FIX32(-0.011), FIX16(-0.44), FIX32(0.000), FIX32(1), FIX32(-6.6)},
		{FIX32(-0.010), FIX16(-0.40), FIX32(-0.001), FIX32(1), FIX32(6.0)},
		{FIX32(-0.009), FIX16(-0.36), FIX32(-0.002), FIX32(1), FIX32(5.4)},
		{FIX32(-0.008), FIX16(-0.32), FIX32(-0.003), FIX32(1), FIX32(4.8)},
		{FIX32(-0.004), FIX16(-0.16), FIX32(-0.001), FIX32(4), FIX32(2.4)},
		{FIX32(-0.002), FIX16(-0.08), FIX32(0.002), FIX32(5), FIX32(1.2)},

		{FIX32(0.000), FIX16(0.000), FIX32(0.000), FIX32(7), FIX32(0)},
		{FIX32(0.000), FIX16(0.000), FIX32(0.001), FIX32(7), FIX32(0)},
		// curve right
		{FIX32(0.030), FIX16(1.20), FIX32(-0.001), FIX32(2), FIX32(18.0)},
		{FIX32(0.026), FIX16(1.04), FIX32(-0.001), FIX32(2), FIX32(15.6)},
		{FIX32(0.022), FIX16(0.88), FIX32(-0.002), FIX32(2), FIX32(13.2)},
		{FIX32(0.018), FIX16(0.72), FIX32(-0.001), FIX32(2), FIX32(9.8)},
		{FIX32(0.014), FIX16(0.56), FIX32(-0.001), FIX32(2), FIX32(8.4)},
		{FIX32(0.012), FIX16(0.48), FIX32(-0.001), FIX32(2), FIX32(7.2)},
		{FIX32(0.008), FIX16(0.32), FIX32(-0.002), FIX32(2), FIX32(4.8)},
		{FIX32(0.004), FIX16(0.16), FIX32(-0.0015), FIX32(2), FIX32(2.4)},

		{FIX32(0.002), FIX16(0.08), FIX32(-0.001), FIX32(7), FIX32(1.2)},
		{FIX32(0.000), FIX16(0.00), FIX32(0.000), FIX32(7), FIX32(0)},
		{FIX32(-0.002), FIX16(-0.08), FIX32(0.000), FIX32(7), FIX32(1.2)},
		{FIX32(-0.001), FIX16(-0.04), FIX32(0.000), FIX32(7), FIX32(0.6)},

		{FIX32(0.000), FIX16(0.000), FIX32(0.000), FIX32(7), FIX32(0)}};

fix32 trackLength = FIX32(0);
fix32 segmentDistances[ROAD_SEGMENTS_LENGTH];



fix32 colorCyclePosition = FIX32(0);						// keep track of the color cycling position
fix16 backgroundPosition = FIX16(SCROLL_CENTER); // handle background X position

extern s16 horizonLine;										// keep track of where the horizon is

/*
Perspective-style Steering
It's much less interesting looking to have a game in which when you steer, it
only moves the car sprite. So, instead of moving the player's car sprite, you
keep it in the center of the screen and move the road-- more importantly, you
move the position of the center-line at the front (bottom) of the screen. Now,    <<< centerLine
you want to assume that the player is going to be looking at the road always, so
make the road end at the center of the screen. You'll need an angle-of-road				<<< angleOfRoad
variable for this. So, calculate the difference between the center of the screen
and the position of the front of the road, and divide by the height of the
road's graphic. That will give you the amount to move the center of the road
each line.
*/
fix32 centerLine = FIX32(160); // center line at the front (bottom) of the screen
fix32 angleOfRoad[ZMAP_LENGTH];
s16 turning = 0;
fix32 steeringDir = FIX32(0);
s8 accelerate = 0;

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

struct CP_SPRITE *playerSprite;
struct CP_SPRITE *redCarSprite;
struct CP_SPRITE *greenCarSprite;
#define NUMBER_OF_ROADSIDE_OBJS 16
struct CP_SPRITE *roadsideObjs; //[NUMBER_OF_ROADSIDE_OBJS];

// This function creates NUMBER_OF_ROADSIDE_OBJS sprites to move along 
// side of the road.   FOr simplicity, these sprites are allocated once
// and simply cycle through the z-positions in the z-map range..  
//
// In a real game I'd x give each roadside object a well-defined position
// on the road.   Sprites would be allocated when they become visible
// and deallocated once they're invisible.
void createRoadsideObjs()
{
	// allocate space for all side objects
	roadsideObjs = malloc(NUMBER_OF_ROADSIDE_OBJS * sizeof(struct CP_SPRITE));
	memset(roadsideObjs, 0, sizeof(roadsideObjs));

	for (u16 i = 0; i < NUMBER_OF_ROADSIDE_OBJS; ++i)
	{
		// actual position doesn't matter yet.  Jsut setting them slightly
		// outside of the screen.
		roadsideObjs[i].posX = FIX32(321);
		roadsideObjs[i].posY = FIX32(160);
		// assign z positions along the zmap range.
		if (i < 2)
		{
			roadsideObjs[i].posZ = FIX32(4.41);
		}
		else if (i < 4)
		{
			roadsideObjs[i].posZ = FIX32(3.86);
		}
		else if (i < 6)
		{
			roadsideObjs[i].posZ = FIX32(3.31);
		}
		else if (i < 8)
		{
			roadsideObjs[i].posZ = FIX32(2.76);
		}
		else if (i < 10)
		{
			roadsideObjs[i].posZ = FIX32(2.21);
		}
		else if (i < 12)
		{
			roadsideObjs[i].posZ = FIX32(1.65);
		}
		else if (i < 14)
		{
			roadsideObjs[i].posZ = FIX32(1.10);
		}
		else 
		{
			roadsideObjs[i].posZ = FIX32(0.55);
		}
		roadsideObjs[i].updateY = 1;

		// set the 'type' of the roadside objects.	
		if (i == 1 || i == 6 || i == 8)
		{
			roadsideObjs[i].offsetY = 80;
			roadsideObjs[i].offsetX = 24;
			roadsideObjs[i].sprite = SPR_addSprite(&pine,
																							F32_toInt(roadsideObjs[i].posX),
																							F32_toInt(roadsideObjs[i].posY),
																							TILE_ATTR(PAL3, 0, FALSE, FALSE));
		}else if (i == 2 || i == 5 || i == 12 || i ==13 )
		{
			roadsideObjs[i].offsetY = 18;
			roadsideObjs[i].offsetX = 16;
			roadsideObjs[i].sprite = SPR_addSprite(&rock,
																							F32_toInt(roadsideObjs[i].posX),
																							F32_toInt(roadsideObjs[i].posY),
																							TILE_ATTR(PAL3, 0, FALSE, FALSE));
		}else if (i == 9 || i == 4)
		{
			roadsideObjs[i].offsetY = 40;
			roadsideObjs[i].offsetX = 24;
			roadsideObjs[i].sprite = SPR_addSprite(&sign,
																							F32_toInt(roadsideObjs[i].posX),
																							F32_toInt(roadsideObjs[i].posY),
																							TILE_ATTR(PAL1, 0, FALSE, FALSE));
		}
		else
		{
			roadsideObjs[i].offsetY = 40;
			roadsideObjs[i].offsetX = 24;
			roadsideObjs[i].sprite = SPR_addSprite(&bush,
																							F32_toInt(roadsideObjs[i].posX),
																							F32_toInt(roadsideObjs[i].posY),
																							TILE_ATTR(PAL3, 0, FALSE, FALSE));
		}
	}
}

//  update the z position ands scale of the roadside objects 
void updateRoadsideObjs()
{
	for (u16 i = 0; i < NUMBER_OF_ROADSIDE_OBJS; ++i)
	{
		// position the roadside objects in the Y dir
		// figure out z position by subtracting the player speed (moves obj toward player)
		roadsideObjs[i].posZ = fix32Sub(roadsideObjs[i].posZ, playerSprite->speed);
		if (roadsideObjs[i].posZ < zmap[0])
		{
			// moved past minZ so move back to the top
			roadsideObjs[i].posZ = fix32Add(roadsideObjs[i].posZ, zmap[ZMAP_LENGTH - 1]);
			SPR_setVisibility(roadsideObjs[i].sprite, HIDDEN);
		}

		// figure out scale
		if (roadsideObjs[i].posZ < FIX32(1.20))
		{
			SPR_setFrame(roadsideObjs[i].sprite, 0);
		}
		else if (roadsideObjs[i].posZ < FIX32(1.76))
		{
			SPR_setFrame(roadsideObjs[i].sprite, 1);
		}
		else if (roadsideObjs[i].posZ < FIX32(2.65))
		{
			SPR_setFrame(roadsideObjs[i].sprite, 2);
		}
		else if (roadsideObjs[i].posZ < FIX32(3.00))
		{
			SPR_setFrame(roadsideObjs[i].sprite, 3.4);
		}
		else if (roadsideObjs[i].posZ < FIX32(3.5))
		{
			SPR_setFrame(roadsideObjs[i].sprite, 4);
		}
		else //  if (roadsideObjs[i].posZ < FIX32(4.44))
		{
			SPR_setFrame(roadsideObjs[i].sprite, 5);
		}
		/* getting rid of the last level of scaling for visuals
		else
		{
			SPR_setFrame(roadsideObjs[i].sprite, 6);
		}
		*/

		// Flag current object for Y (and X) position calculation in update();
		roadsideObjs[i].updateY = 1;
	}
}

// joypad event handler.  This gets called automatically by SGDK when the joypad
// state changes
static void joypadHandler(u16 joypadId, u16 changed, u16 state)
{
	if (joypadId == JOY_1)
	{
		if (state & BUTTON_A)
		{
			accelerate = 1; // Speed up 
		}
		else if (changed & BUTTON_A)
		{
			accelerate = 0; // state change A-up. no longer accelerating.
		}

		if (state & BUTTON_B)
		{
			accelerate = -1; // Slow down
		}
		else if (changed & BUTTON_B)
		{
			accelerate = 0;
		}

		/*
		if (state & BUTTON_C)
		{
			// full stop for testing purposes.
			playerSprite->speed = FIX32(0);
		}
*/
		// start the other cars.
		if (state & BUTTON_C)
		{
			redCarSprite->speed = FIX32(0.12);
			greenCarSprite->speed = FIX32(0.09);
			redCarSprite->position = playerSprite->position;
			greenCarSprite->position = playerSprite->position;
		}

		if (state & BUTTON_LEFT)
		{
			turning = -1; // turn left
		}
		else if (state & BUTTON_RIGHT)
		{
			turning = 1; // turn right
		}
		else
		{
			turning = 0; // not turning.
		}
	}
}

void updatePlayer()
{
	// handle accleartion
	if (accelerate == 1)
	{
		// speed up
		playerSprite->speed = fix32Add(playerSprite->speed, FIX32(0.033));
	}
	else if (accelerate == -1)
	{
		// breaks applied
		playerSprite->speed = fix32Sub(playerSprite->speed, FIX32(0.78));
	}
	else if (accelerate == 0)
	{
		// slow down naturally
		playerSprite->speed = fix32Sub(playerSprite->speed, FIX32(0.06));
	}
	// limit speed
	if (playerSprite->speed < FIX32(0))
	{
		playerSprite->speed = FIX32(0);
	}
	if (playerSprite->speed > FIX32(4.0))
	{
		playerSprite->speed = FIX32(2.0);
	}


	// handle turning
	if (turning == 1)
	{
		steeringDir = fix32Add(steeringDir, FIX32(2.2));
		if (steeringDir > FIX32(20))
		{
			steeringDir = FIX32(20);
		}
	}
	else if (turning == -1)
	{
		steeringDir = fix32Sub(steeringDir, FIX32(2.2));
		if (steeringDir < FIX32(-20))
		{
			steeringDir = FIX32(-20);
		}
	}
	else
	{
		// pull back to center
		if (steeringDir < FIX32(0.0))
		{
			steeringDir = fix32Add(steeringDir, FIX32(3.2));
			if (steeringDir > FIX32(0.0))
			{
				steeringDir = FIX32(0.0);
			}
		}
		else if (steeringDir > FIX32(0.0))
		{
			steeringDir = fix32Sub(steeringDir, FIX32(3.2));
			if (steeringDir < FIX32(0.0))
			{
				steeringDir = FIX32(0.0);
			}
		}
	}


	// Only update the car sprite and road position if moving
	if (playerSprite->speed > FIX32(0.0))
	{

		// set frame based on steeringDir as long as we're moving forward.
		if (steeringDir < FIX32(-12.00))
		{
			SPR_setFrame(playerSprite->sprite, 0);
		}
		else if (steeringDir < FIX32(-0.02))
		{
			SPR_setFrame(playerSprite->sprite, 1);
		}
		else if (steeringDir > FIX32(12.0))
		{
			SPR_setFrame(playerSprite->sprite, 4);
		}
		else if (steeringDir > FIX32(0.02))
		{
			SPR_setFrame(playerSprite->sprite, 3);
		}
		else
		{
			// centered
			SPR_setFrame(playerSprite->sprite, 2);
		}

		// start shifting the road based on speed, steeringDir and road DX
		// >>  variable for this. So, calculate the difference between the center of the screen
		// >>  and the position of the front of the road, and divide by the height of the
		// >>  road's graphic. That will give you the amount to move the center of the road
		// >>  each line.
		if (turning != 0)
		{
			//KLog_F1("steeringDir: ", steeringDir);
			centerLine = fix32Sub(centerLine, steeringDir);
		}

		// also factor in the curve of the road segment
		centerLine = fix32Add(centerLine, segments[playerSprite->segment_index].carX);


		//KLog_F1( "centerLine: ", centerLine);
		// Limit how far the car can move to the side
		if (centerLine > FIX32(323))
		{
			centerLine = FIX32(328);
		}
		else if (centerLine < FIX32(-4))
		{
			centerLine = FIX32(-4);
		}

		// update angleOfRoad for perspective steering.
		fix32 step = F32_div(fix32Sub(centerLine, FIX32(160)), // calc diff between center and positoin at front
													FIX32(ZMAP_LENGTH));							// divide by the height of the road graphic.
		fix32 current = FIX32(0);
		for (int i = ZMAP_LENGTH-1; i >= 0; --i)// farthest has lowest offset.
		{
			angleOfRoad[i] = current;
			current = fix32Add(current, step);
		}
	}
}

void updateCar(struct CP_SPRITE *carSprite, u8 onYourLeft)
{
	// update car position along track
	//KLog_F2("spd: ", carSprite->speed, " pos: ", carSprite->position);
	carSprite->position = fix32Add(carSprite->position, carSprite->speed);
	if (carSprite->position >= trackLength)
	{
		carSprite->position = FIX32(0);
		carSprite->segment_index = 0;
	}
	while (carSprite->position > segmentDistances[carSprite->segment_index])
	{
		++carSprite->segment_index;
	}

	// Z distance of car from player
	fix32 dist = fix32Sub(carSprite->position, playerSprite->position);
	if (dist < FIX32(0)																										// negative
			&& playerSprite->position > (trackLength - zmap[ZMAP_LENGTH - 1]) // player close to end of track
			&& (carSprite->position < zmap[ZMAP_LENGTH - 1])									// enemy close to beginning of track
	)
	{
		// recompute dist because of edge case
		dist = fix32Sub(fix32Add(carSprite->position, trackLength), playerSprite->position);
	}

	// check if car should be visible to the player
	if (dist >= FIX32(-0.5) && dist < zmap[ZMAP_LENGTH - 4])
	{
		// car is visible, so we try to  display it
		SPR_setVisibility(carSprite->sprite, VISIBLE);
		
		
		// compute a Y pos for the car (this could be made more efficient)
		u8 y = 0;
		for (u8 i = VERTICAL_REZ - 1; i > horizonLine; --i)
		{
			fix32 tmpZ = yToZdist[i];
			if (tmpZ > dist)
			{
				carSprite->posY = FIX32(i - carSprite->offsetY);
				y = i;
				break;
			}
		}

		// scale the car sprites based on Z value
		if (dist < FIX32(1.06))
		{
			SPR_setAnimAndFrame(carSprite->sprite, 0, 0);
		}
		else if (dist < FIX32(1.26))
		{
			SPR_setAnimAndFrame(carSprite->sprite, 1, 0);
		}
		else if (dist < FIX32(1.49))
		{
			SPR_setAnimAndFrame(carSprite->sprite, 2, 0);
		}
		else if (dist < FIX32(1.8))
		{
			SPR_setAnimAndFrame(carSprite->sprite, 3, 0);
		}
		else if (dist < FIX32(2.4))
		{
			SPR_setAnimAndFrame(carSprite->sprite, 4, 0);
		}
		else if (dist < FIX32(3.08))
		{
			SPR_setAnimAndFrame(carSprite->sprite, 5, 0);
		}
		else
		{
			SPR_setAnimAndFrame(carSprite->sprite, 6, 0);
		}
		if (onYourLeft)
		{
			carSprite->posX = fix32Sub(fix32Sub(yToRoadCenter[y], FIX32(carSprite->offsetX)), F32_div(roadSideObjectOffset[y], FIX32(2.7)));
		}
		else
		{
			carSprite->posX = fix32Add(fix32Sub(yToRoadCenter[y], FIX32(carSprite->offsetX)), F32_div(roadSideObjectOffset[y], FIX32(2.7)));
		}
	}
	else
	{
		SPR_setVisibility(carSprite->sprite, HIDDEN);
	}
}

// My interpretation of the pseudo-code in
// http://www.extentofthejam.com/pseudo/#curves
// and the hills described in
// http://www.extentofthejam.com/pseudo/#hills
void update()
{
	fix32 currentX = FIX32(0); // Lou's pseudo 3d page says to use Half of the screen width,
														 // but I've defined SCROLL_CENTER to handle this

	fix32 dx = FIX32(0);	// Curve Amount, constant per segment.
	fix32 ddx = FIX32(0); // Curve Amount, changes per line

	fix32 dy = FIX32(0);	// Slope Amount
	fix32 ddy = FIX32(0); // Slope Amount, changes per line

	fix32 currentDrawingPos = FIX32(223); // The drawing loop would start at the beginning of the Z-map (nearest).  Basically the bottom of the screen
	horizonLine = 223;										// keep track of where the horizon is.  I"m starting at the bottom and will update as the rode gets computed

	// car position changes with speed
	playerSprite->position = fix32Add(playerSprite->position, playerSprite->speed);
	//KLog_F2("car pos: ", playerSprite->position, " track length: ", trackLength );
	if (playerSprite->position >= trackLength)
	{
		playerSprite->position = FIX32(0);
		playerSprite->segment_index = 0;
	}
	while (playerSprite->position > segmentDistances[playerSprite->segment_index])
	{
		++playerSprite->segment_index;
	}
	colorCyclePosition = fix32Sub(colorCyclePosition, playerSprite->speed);
	if (F32_toInt(colorCyclePosition) < 0)
	{
		colorCyclePosition = zmap[ZMAP_LENGTH - 1]; // Send segment to farthest visible distance
	}

	// for each line of the screen from the bottom to the top
	//for (y = 0; y < ZMAP_LENGTH; ++y)  // no longer works because up-hill/down-hill won't be exaclty 1.  ++y isn't valid

	// HILL: draw loop starts at the beginning of the Z map ( nearest = 0 ) and stops at teh end (farthest = ZMAP_LENGTH )
	//     * flat roads decrements the drawing position each line by 1
	//		 * if we decrement the drawing position by 2 (doubling lines) the road gets drawn twice as high.
	//     * by varying the amount we decrement the drawing position we can draw a hill that starts flat and curves upwards
	//				* if the next drawing position is more than one line from the current drawing position, the currnet ZMap line is repeated until
	// 					we get there, producing a scalien effect.
	u8 zSegmentPos = playerSprite->segment_index;
	for (u16 bgY = 223; bgY > 126; bgY--)
	{
		//////////////////////////////////////////////////////////////////////
		// Road Bending.
		//  Now using a continuous check against variable length segments
		//  instead of top_segment/bottom_segment.  So now I'm searching
		//  for visible segments.
		fix32 z = zmap[223 - bgY]; // zmap[0] is closest
		fix32 posZ = fix32Add(playerSprite->position, z);
		//KLog_F2("posZ: ", posZ, " z: ", z);
		bool found = FALSE;
		while (!found)
		{
			if (posZ < segmentDistances[zSegmentPos])
			{
				//  posZ distance is less than the current segment distance so we get dx and dy to bend the road
				dx = segments[zSegmentPos].dx;
				dy = segments[zSegmentPos].dy;
				found = TRUE;
				break; // we're done here.
			}
			else
			{
				// posZ is past the current segment, so go to the next segment.
				++zSegmentPos;
				if (zSegmentPos >= ROAD_SEGMENTS_LENGTH)
				{
					zSegmentPos = zSegmentPos - ROAD_SEGMENTS_LENGTH;
					posZ = fix32Sub(posZ, trackLength);
				}
			}
		}

		// ddx += dx
		ddx = fix32Add(ddx, dx);
		// currentX += ddx
		currentX = fix32Add(currentX, ddx);

		//////////////////////////////////////////////////////////////////////
		// Coloring
		//  For each Z, make one of the bits represent the shade
		//  of the road (dark or light). Then, just draw the
		//  appropriate road pattern or colors for that bit
		fix32 tmpz = fix32Sub(colorCyclePosition, z);
		u16 zcolor = (u16)F32_toInt(tmpz << 1); // >> 1);

		ddy = fix32Add(dy, ddy);
		s16 cdp = F32_toInt(currentDrawingPos);				 // current vertical drawing position
		fix32 deltaDrawingPos = fix32Add(FIX32(1), ddy); // increment drawing position
		fix32 nextDrawingPos = fix32Sub(currentDrawingPos, deltaDrawingPos);
		s16 ndp = F32_toInt(nextDrawingPos); // figure out next drawing position

		// get perspective shift for current bgY
		int xShift = F32_toInt(angleOfRoad[223-bgY]); // closest is 0
		fix32 currentRoadCenter = fix32Add(FIX32(160 + xShift), currentX);
		// repeat line if theres a gap greater than 1
		for (; cdp >= ndp; --cdp) //
		{
			if (cdp <= horizonLine) // if current drawing position is above the horizon
			{
				HscrollA[cdp] = SCROLL_CENTER + F32_toInt(currentX) + xShift; // this_line.x = current x | using horizontal scrolling to fake curves
				VscrollA[cdp] = bgY - cdp;																		 // set the vertical scroll amount for the current drawing position
				horizonLine = cdp;																						 // update horizon line
				yToZdist[cdp] = z;
				yToRoadCenter[cdp] = currentRoadCenter;

				// set coloring of current line for ASM
				colors[cdp] = zcolor & 1;
			}
		}

		// road side object update
		for (u16 i = 0; i < NUMBER_OF_ROADSIDE_OBJS; ++i)
		{
			if (roadsideObjs[i].updateY == 1)
			{
				// check if z is close. to this object.
				if (z > roadsideObjs[i].posZ)
				{
					// note: doing this will make y-motion jumpy up close. Consider interpoating if not *TOO* expensive.
					roadsideObjs[i].posY = fix32Sub(currentDrawingPos, FIX32(roadsideObjs[i].offsetY));

					// figure out X position.
					if (i % 2 == 0)
					{
						roadsideObjs[i].posX = fix32Sub(currentRoadCenter, fix32Add(roadSideObjectOffset[bgY], FIX32(roadsideObjs[i].offsetX)));
						if (roadsideObjs[i].posX < FIX32(-40)) // hide sprite if it's offscreen
						{
							SPR_setVisibility(roadsideObjs[i].sprite, HIDDEN);
						}
						else
						{
							SPR_setVisibility(roadsideObjs[i].sprite, VISIBLE);
						}
					}
					else
					{
						roadsideObjs[i].posX = fix32Add(currentRoadCenter, fix32Sub(roadSideObjectOffset[bgY], FIX32(roadsideObjs[i].offsetX)));
						if (roadsideObjs[i].posX > FIX32(320)) // hide sprite if it's offscreen
						{
							SPR_setVisibility(roadsideObjs[i].sprite, HIDDEN);
						}
						else
						{
							SPR_setVisibility(roadsideObjs[i].sprite, VISIBLE);
						}
					}
					roadsideObjs[i].updateY = 0;
				}
			}
		}

		currentDrawingPos = nextDrawingPos; // move to next drawing position
	}

	// hide anything above the horizon line
	for (s16 h = horizonLine; h >= 0; --h)
	{
		VscrollA[h] = -h;
	}

	// scroll the background
	if (playerSprite->speed != FIX32(0.0))
	{
		backgroundPosition = fix16Sub(backgroundPosition, segments[playerSprite->segment_index].bgdx);
		for (u16 y = 12; y < 160; ++y)
		{
			HscrollB[y] = F16_toInt(backgroundPosition);
		}
	}
}

int main(bool hard)
{

	//////////////////////////////////////////////////////////////
	// http://www.extentofthejam.com/pseudo/
	// Z = Y_world / (Y_screen - (height_screen / 2))
	for (int i = 0; i < ZMAP_LENGTH; ++i)
	{
		zmap[i] = F32_div(FIX32(-75), fix32Sub(FIX32(i), FIX32(113)));
		//scale[i] = F16_div(FIX16(1), zmap[i]);
		//KLog_F3("i: ", FIX16(i), " z: ", zmap[i], " s: ", scale[i]);
		KLog_F2("i: ", FIX32(i), " z: ", zmap[i]);
	}

	//////////////////////////////////////////////////////////////
	// Precompute road offsets for roadside sprites
	//  using pixels from the road.png image.
	// 116 | 262-256 = 6
	// 223 | 443-256 = 187   (edge of line)
	// 223 | 432-256 = 176   (middle of line)
	// 187 - 6 =  181  : X Run (edge)
	// 223 - 127 =  96  : Y RUn
	// step size 181/126  1.885   << step size per line
	//
	// 176 - 6 =  170  : X Run (middle)
	// step size 170/96  1.7708  << step size per line
	fix32 sideStep = FIX32(1.771);
	//
	// Looks better w/ more padding
	// (159 + 60) - (6+4) =  209
	// step size 209/96  2.177 << step size per line
	fix32 sideOffsetFromCenter = FIX32(6);		// tree width is 88 ..   half of 88 is 44  | bush width is 48 -> 24 | rock witdh is 40 -> 20
	fix32 objectOffsetFromCenter = FIX32(30); // tree width is 88 ..   half of 88 is 44  | bush width is 48 -> 24 | rock witdh is 40 -> 20
	//fix32 rightFromCenter = FIX32(11); // tree width is 88 ..   half of 88 is 44  | bush width is 48 -> 24 | rock witdh is 40 -> 20
	//fix32 leftFromCenter = FIX32(-11); // tree width is 56 ..   half of 88 is 44
	fix32 objectStep = FIX32(2.177);
	for (int i = 224 - ZMAP_LENGTH; i < 224; i++)
	{
		roadSideObjectOffset[i] = objectOffsetFromCenter;
		objectOffsetFromCenter = fix32Add(objectOffsetFromCenter, objectStep);

		roadSideOffset[i] = sideOffsetFromCenter;
		sideOffsetFromCenter = fix32Add(sideOffsetFromCenter, sideStep);
		//roadSideObjectOffsetLeft[i] = leftFromCenter;
		//leftFromCenter = fix32Sub(leftFromCenter, step);
		//KLog_F2(" i: ", FIX32(i), "  road offset right: ", roadSideObjectOffsetRight[i]);

		// clear it for later use
		angleOfRoad[i] = FIX32(0);
	}

	//////////////////////////////////////////////////////////////
	// prep road variables
	for (u8 i = 0; i < ROAD_SEGMENTS_LENGTH; ++i)
	{
		trackLength = fix32Add(trackLength, segments[i].length);
		segmentDistances[i] = trackLength;
		KLog_F2(" i: ", FIX32(i), "  trackLength: ", trackLength);
	}

	//////////////////////////////////////////////////////////////
	// VDP basic setup
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
	PAL_setPalette(PAL0, background_pal.data, CPU);
	int ind = TILE_USER_INDEX;
	VDP_drawImageEx(BG_A, &road, TILE_ATTR_FULL(PAL0, FALSE, FALSE, FALSE, ind), 0, 0, FALSE, TRUE);
	ind += road.tileset->numTile;
	VDP_drawImageEx(BG_B, &background, TILE_ATTR_FULL(PAL0, FALSE, FALSE, FALSE, ind), 0, 0, FALSE, TRUE);
	ind += background.tileset->numTile;

	VDP_setVerticalScroll(BG_B, 0);

	//////////////////////////////////////////////////////////////
	// Setup Car Sprites
	//SPR_init();
	SPR_initEx(900);
	PAL_setPalette(PAL2, car_pal.data, CPU);
	playerSprite = malloc(sizeof(struct CP_SPRITE));
	playerSprite->position = FIX32(0);
	playerSprite->segment_index = 0;
	playerSprite->offsetY = 50; // 56 tall bottom is 51 ish
	playerSprite->offsetX = 44; // 88 wide
	playerSprite->sprite = NULL;
	playerSprite->posX = FIX32(116.0);
	playerSprite->posY = FIX32(160.0);																	 // top pixel of car is at 162.  bottom pixel of car is at 211.  so dist 0 is y=160
	playerSprite->sprite = SPR_addSprite(&car,													 // Sprite defined in resources
																			 F32_toInt(playerSprite->posX), // starting X position
																			 F32_toInt(playerSprite->posY), // starting Y position
																			 TILE_ATTR(PAL2,								 // specify palette
																								 1,										 // Tile priority ( with background)
																								 FALSE,								 // flip the sprite vertically?
																								 FALSE								 // flip the sprite horizontally
																								 ));
	SPR_setFrame(playerSprite->sprite, 2);
	SPR_setDepth(playerSprite->sprite, 0);

	redCarSprite = malloc(sizeof(struct CP_SPRITE));
	redCarSprite->position = FIX32(0);
	redCarSprite->segment_index = 0;
	redCarSprite->offsetY = 50; // 56 tall bottom is 51 ish
	redCarSprite->offsetX = 44; // 88 wide
	redCarSprite->speed = FIX32(0.00);
	redCarSprite->sprite = NULL;
	redCarSprite->posX = FIX32(40.0);
	redCarSprite->posY = FIX32(160.0);																	 // bottom of car is ~12 from bottom of screen.
	redCarSprite->sprite = SPR_addSprite(&red_car,											 // Sprite defined in resources
																			 F32_toInt(redCarSprite->posX), // starting X position
																			 F32_toInt(redCarSprite->posY), // starting Y position
																			 TILE_ATTR(PAL2,								 // specify palette
																								 1,										 // Tile priority ( with background)
																								 FALSE,								 // flip the sprite vertically?
																								 TRUE									 // flip the sprite horizontally
																								 ));
	SPR_setAnimAndFrame(redCarSprite->sprite, 0, 0);
	SPR_setDepth(redCarSprite->sprite, 0);

	greenCarSprite = malloc(sizeof(struct CP_SPRITE));
	greenCarSprite->position = FIX32(0);
	greenCarSprite->segment_index = 0;
	greenCarSprite->offsetY = 50; // 56 tall bottom is 51 ish
	greenCarSprite->offsetX = 44; // 88 wide
	greenCarSprite->speed = FIX32(0.00);
	greenCarSprite->sprite = NULL;
	greenCarSprite->posX = FIX32(192.0);
	greenCarSprite->posY = FIX32(160.0);
	greenCarSprite->sprite = SPR_addSprite(&green_car,											 // Sprite defined in resources
																				 F32_toInt(greenCarSprite->posX), // starting X position
																				 F32_toInt(greenCarSprite->posY), // starting Y position
																				 TILE_ATTR(PAL2,									 // specify palette
																									 1,											 // Tile priority ( with background)
																									 FALSE,									 // flip the sprite vertically?
																									 FALSE									 // flip the sprite horizontally
																									 ));
	SPR_setAnimAndFrame(greenCarSprite->sprite, 2, 0);
	SPR_setDepth(greenCarSprite->sprite, 0);

	PAL_setPalette(PAL1, sign_pal.data, CPU);
	PAL_setPalette(PAL3, bush_pal.data, CPU);
	createRoadsideObjs();

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
	SYS_enableInts();

	// Asynchronous joystick handler.
	JOY_init();
	JOY_setEventHandler(joypadHandler);

	// Main loop
	while (TRUE)
	{

		// update
		updateRoadsideObjs();
		update();
		updatePlayer();
		updateCar(redCarSprite, 1);
		updateCar(greenCarSprite, 0);

		for (u16 i = 0; i < NUMBER_OF_ROADSIDE_OBJS; ++i)
		{
			// update z-order  for roadsideObjs
			SPR_setDepth(roadsideObjs[i].sprite, 224 - F32_toInt(roadsideObjs[i].posY) - roadsideObjs[i].offsetY);
			// Draw object at new position
			SPR_setPosition(roadsideObjs[i].sprite, F32_toInt(roadsideObjs[i].posX), F32_toInt(roadsideObjs[i].posY));
		}

		// Draw car at now position
		SPR_setPosition(playerSprite->sprite, F32_toInt(playerSprite->posX), F32_toInt(playerSprite->posY));
		SPR_setPosition(redCarSprite->sprite, F32_toInt(redCarSprite->posX), F32_toInt(redCarSprite->posY));
		SPR_setPosition(greenCarSprite->sprite, F32_toInt(greenCarSprite->posX), F32_toInt(greenCarSprite->posY));

		SPR_update();

		// curve the road with horizontal scrolling
		VDP_setHorizontalScrollLine(BG_A, 0, HscrollA, VERTICAL_REZ, DMA_QUEUE);
		// move the background
		VDP_setHorizontalScrollLine(BG_B, 0, HscrollB, 160, DMA_QUEUE);

		fix16 h = FIX16(horizonLine - 113);
		//h = F16_div(h, FIX16(6));
		h = h >> 2;
		VDP_setVerticalScroll(BG_B, F16_toInt(h));

		SYS_doVBlankProcess();
	}
}
