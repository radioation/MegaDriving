
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
fastfix32 zmap[ZMAP_LENGTH] = { FASTFIX32( 0.6637 ), FASTFIX32( 0.6696 ), FASTFIX32( 0.6757 ), FASTFIX32( 0.6818 ), FASTFIX32( 0.6881 ), FASTFIX32( 0.6944 ), FASTFIX32( 0.7009 ), FASTFIX32( 0.7075 ), FASTFIX32( 0.7143 ), FASTFIX32( 0.7212 ), FASTFIX32( 0.7282 ), FASTFIX32( 0.7353 ), FASTFIX32( 0.7426 ), FASTFIX32( 0.7500 ), FASTFIX32( 0.7576 ), FASTFIX32( 0.7653 ), FASTFIX32( 0.7732 ), FASTFIX32( 0.7812 ), FASTFIX32( 0.7895 ), FASTFIX32( 0.7979 ), FASTFIX32( 0.8065 ), FASTFIX32( 0.8152 ), FASTFIX32( 0.8242 ), FASTFIX32( 0.8333 ), FASTFIX32( 0.8427 ), FASTFIX32( 0.8523 ), FASTFIX32( 0.8621 ), FASTFIX32( 0.8721 ), FASTFIX32( 0.8824 ), FASTFIX32( 0.8929 ), FASTFIX32( 0.9036 ), FASTFIX32( 0.9146 ), FASTFIX32( 0.9259 ), FASTFIX32( 0.9375 ), FASTFIX32( 0.9494 ), FASTFIX32( 0.9615 ), FASTFIX32( 0.9740 ), FASTFIX32( 0.9868 ), FASTFIX32( 1.0000 ), FASTFIX32( 1.0135 ), FASTFIX32( 1.0274 ), FASTFIX32( 1.0417 ), FASTFIX32( 1.0563 ), FASTFIX32( 1.0714 ), FASTFIX32( 1.0870 ), FASTFIX32( 1.1029 ), FASTFIX32( 1.1194 ), FASTFIX32( 1.1364 ), FASTFIX32( 1.1538 ), FASTFIX32( 1.1719 ), FASTFIX32( 1.1905 ), FASTFIX32( 1.2097 ), FASTFIX32( 1.2295 ), FASTFIX32( 1.2500 ), FASTFIX32( 1.2712 ), FASTFIX32( 1.2931 ), FASTFIX32( 1.3158 ), FASTFIX32( 1.3393 ), FASTFIX32( 1.3636 ), FASTFIX32( 1.3889 ), FASTFIX32( 1.4151 ), FASTFIX32( 1.4423 ), FASTFIX32( 1.4706 ), FASTFIX32( 1.5000 ), FASTFIX32( 1.5306 ), FASTFIX32( 1.5625 ), FASTFIX32( 1.5957 ), FASTFIX32( 1.6304 ), FASTFIX32( 1.6667 ), FASTFIX32( 1.7045 ), FASTFIX32( 1.7442 ), FASTFIX32( 1.7857 ), FASTFIX32( 1.8293 ), FASTFIX32( 1.8750 ), FASTFIX32( 1.9231 ), FASTFIX32( 1.9737 ), FASTFIX32( 2.0270 ), FASTFIX32( 2.0833 ), FASTFIX32( 2.1429 ), FASTFIX32( 2.2059 ), FASTFIX32( 2.2727 ), FASTFIX32( 2.3438 ), FASTFIX32( 2.4194 ), FASTFIX32( 2.5000 ), FASTFIX32( 2.5862 ), FASTFIX32( 2.6786 ), FASTFIX32( 2.7778 ), FASTFIX32( 2.8846 ), FASTFIX32( 3.0000 ), FASTFIX32( 3.1250 ), FASTFIX32( 3.2609 ), FASTFIX32( 3.4091 ), FASTFIX32( 3.5714 ), FASTFIX32( 3.7500 ), FASTFIX32( 3.9474 ), FASTFIX32( 4.1667 ), FASTFIX32( 4.4118 ),
};

//fix16 scale[ZMAP_LENGTH];  // Could potentially store a scale value for handling sprites sizes and positions

// precalculated road variables for placing sprites
fastfix32 roadSideObjectOffset[VERTICAL_REZ]; // X offset from side of road ( for positioning the sprites
fastfix32 yToZdist[VERTICAL_REZ];
fastfix32 yToRoadCenter[VERTICAL_REZ];
fastfix32 roadSideOffset[VERTICAL_REZ]; // X offset from side of road ( for positioning the sprites


// Road data
typedef struct
{
    fastfix32 dx;			// rate of change for the road.
    fix16 bgdx; 	// rate of change for background. ( ignore for now )
    fastfix32 dy;			// rate of change for drawing road in y dir?
    fastfix32 length;	// length of this road segment.
    fastfix32 carX;		// amount to shift car in X dir for a segment
} ROAD_SEGMENT;


// predefined road segments
#define ROAD_SEGMENTS_LENGTH 31
const ROAD_SEGMENT segments[ROAD_SEGMENTS_LENGTH] = {
    {FASTFIX32(-0.000), FIX16(0.000), FASTFIX32(0.002), FASTFIX32(5), FASTFIX32(0)},
    // curve left slowly
    {FASTFIX32(-0.001), FIX16(-0.04), FASTFIX32(0.001), FASTFIX32(1), FASTFIX32(-0.6)},
    {FASTFIX32(-0.002), FIX16(-0.08), FASTFIX32(0.002), FASTFIX32(1), FASTFIX32(-1.2)},
    {FASTFIX32(-0.004), FIX16(-0.16), FASTFIX32(0.003), FASTFIX32(1), FASTFIX32(-2.4)},
    {FASTFIX32(-0.006), FIX16(-0.24), FASTFIX32(0.002), FASTFIX32(1), FASTFIX32(-3.6)},
    {FASTFIX32(-0.008), FIX16(-0.32), FASTFIX32(0.002), FASTFIX32(1), FASTFIX32(-4.8)},
    {FASTFIX32(-0.010), FIX16(-0.40), FASTFIX32(0.001), FASTFIX32(1), FASTFIX32(-6.0)},
    {FASTFIX32(-0.012), FIX16(-0.48), FASTFIX32(0.001), FASTFIX32(1), FASTFIX32(-7.2)},
    {FASTFIX32(-0.014), FIX16(-0.56), FASTFIX32(0.001), FASTFIX32(7), FASTFIX32(-8.4)},
    {FASTFIX32(-0.012), FIX16(-0.48), FASTFIX32(0.000), FASTFIX32(1), FASTFIX32(-7.2)},
    {FASTFIX32(-0.011), FIX16(-0.44), FASTFIX32(0.000), FASTFIX32(1), FASTFIX32(-6.6)},
    {FASTFIX32(-0.010), FIX16(-0.40), FASTFIX32(-0.001), FASTFIX32(1), FASTFIX32(6.0)},
    {FASTFIX32(-0.009), FIX16(-0.36), FASTFIX32(-0.002), FASTFIX32(1), FASTFIX32(5.4)},
    {FASTFIX32(-0.008), FIX16(-0.32), FASTFIX32(-0.003), FASTFIX32(1), FASTFIX32(4.8)},
    {FASTFIX32(-0.004), FIX16(-0.16), FASTFIX32(-0.001), FASTFIX32(4), FASTFIX32(2.4)},
    {FASTFIX32(-0.002), FIX16(-0.08), FASTFIX32(0.002), FASTFIX32(5), FASTFIX32(1.2)},

    {FASTFIX32(0.000), FIX16(0.000), FASTFIX32(0.000), FASTFIX32(7), FASTFIX32(0)},
    {FASTFIX32(0.000), FIX16(0.000), FASTFIX32(0.001), FASTFIX32(7), FASTFIX32(0)},
    // curve right
    {FASTFIX32(0.030), FIX16(1.20), FASTFIX32(-0.001), FASTFIX32(2), FASTFIX32(18.0)},
    {FASTFIX32(0.026), FIX16(1.04), FASTFIX32(-0.001), FASTFIX32(2), FASTFIX32(15.6)},
    {FASTFIX32(0.022), FIX16(0.88), FASTFIX32(-0.002), FASTFIX32(2), FASTFIX32(13.2)},
    {FASTFIX32(0.018), FIX16(0.72), FASTFIX32(-0.001), FASTFIX32(2), FASTFIX32(9.8)},
    {FASTFIX32(0.014), FIX16(0.56), FASTFIX32(-0.001), FASTFIX32(2), FASTFIX32(8.4)},
    {FASTFIX32(0.012), FIX16(0.48), FASTFIX32(-0.001), FASTFIX32(2), FASTFIX32(7.2)},
    {FASTFIX32(0.008), FIX16(0.32), FASTFIX32(-0.002), FASTFIX32(2), FASTFIX32(4.8)},
    {FASTFIX32(0.004), FIX16(0.16), FASTFIX32(-0.0015), FASTFIX32(2), FASTFIX32(2.4)},

    {FASTFIX32(0.002), FIX16(0.08), FASTFIX32(-0.001), FASTFIX32(7), FASTFIX32(1.2)},
    {FASTFIX32(0.000), FIX16(0.00), FASTFIX32(0.000), FASTFIX32(7), FASTFIX32(0)},
    {FASTFIX32(-0.002), FIX16(-0.08), FASTFIX32(0.000), FASTFIX32(7), FASTFIX32(1.2)},
    {FASTFIX32(-0.001), FIX16(-0.04), FASTFIX32(0.000), FASTFIX32(7), FASTFIX32(0.6)},

    {FASTFIX32(0.000), FIX16(0.000), FASTFIX32(0.000), FASTFIX32(7), FASTFIX32(0)}};

fastfix32 trackLength = FASTFIX32(0);
fastfix32 segmentDistances[ROAD_SEGMENTS_LENGTH];



fastfix32 colorCyclePosition = FASTFIX32(0);						// keep track of the color cycling position
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
fastfix32 centerLine = FASTFIX32(160); // center line at the front (bottom) of the screen
fastfix32 angleOfRoad[ZMAP_LENGTH];
s16 turning = 0;
fastfix32 steeringDir = FASTFIX32(0);
s8 accelerate = 0;

// Sprites
struct CP_SPRITE
{
    Sprite *sprite;
    fastfix32 posX;					// horizontal screen position
    fastfix32 posY;					// vertical screen position
    fastfix32 posZ;					// z position along the visible segments.
    fastfix32 position;			// track position along the *entire* road. start it at the farthest on background - 12.5
    fastfix32 speed;				// sped the car travles through the road
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
        roadsideObjs[i].posX = FASTFIX32(321);
        roadsideObjs[i].posY = FASTFIX32(160);
        // assign z positions along the zmap range.
        if (i < 2)
        {
            roadsideObjs[i].posZ = FASTFIX32(4.41);
        }
        else if (i < 4)
        {
            roadsideObjs[i].posZ = FASTFIX32(3.86);
        }
        else if (i < 6)
        {
            roadsideObjs[i].posZ = FASTFIX32(3.31);
        }
        else if (i < 8)
        {
            roadsideObjs[i].posZ = FASTFIX32(2.76);
        }
        else if (i < 10)
        {
            roadsideObjs[i].posZ = FASTFIX32(2.21);
        }
        else if (i < 12)
        {
            roadsideObjs[i].posZ = FASTFIX32(1.65);
        }
        else if (i < 14)
        {
            roadsideObjs[i].posZ = FASTFIX32(1.10);
        }
        else 
        {
            roadsideObjs[i].posZ = FASTFIX32(0.55);
        }
        roadsideObjs[i].updateY = 1;

        // set the 'type' of the roadside objects.	
        if (i == 1 || i == 6 || i == 8)
        {
            roadsideObjs[i].offsetY = 80;
            roadsideObjs[i].offsetX = 24;
            roadsideObjs[i].sprite = SPR_addSprite(&pine,
                    FF32_toInt(roadsideObjs[i].posX),
                    FF32_toInt(roadsideObjs[i].posY),
                    TILE_ATTR(PAL3, 0, FALSE, FALSE));
        }else if (i == 2 || i == 5 || i == 12 || i ==13 )
        {
            roadsideObjs[i].offsetY = 18;
            roadsideObjs[i].offsetX = 16;
            roadsideObjs[i].sprite = SPR_addSprite(&rock,
                    FF32_toInt(roadsideObjs[i].posX),
                    FF32_toInt(roadsideObjs[i].posY),
                    TILE_ATTR(PAL3, 0, FALSE, FALSE));
        }else if (i == 9 || i == 4)
        {
            roadsideObjs[i].offsetY = 40;
            roadsideObjs[i].offsetX = 24;
            roadsideObjs[i].sprite = SPR_addSprite(&sign,
                    FF32_toInt(roadsideObjs[i].posX),
                    FF32_toInt(roadsideObjs[i].posY),
                    TILE_ATTR(PAL1, 0, FALSE, FALSE));
        }
        else
        {
            roadsideObjs[i].offsetY = 40;
            roadsideObjs[i].offsetX = 24;
            roadsideObjs[i].sprite = SPR_addSprite(&bush,
                    FF32_toInt(roadsideObjs[i].posX),
                    FF32_toInt(roadsideObjs[i].posY),
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
        roadsideObjs[i].posZ = roadsideObjs[i].posZ - playerSprite->speed;
        if (roadsideObjs[i].posZ < zmap[0])
        {
            // moved past minZ so move back to the top
            roadsideObjs[i].posZ = roadsideObjs[i].posZ + zmap[ZMAP_LENGTH - 1];
            SPR_setVisibility(roadsideObjs[i].sprite, HIDDEN);
        }

        // figure out scale
        if (roadsideObjs[i].posZ < FASTFIX32(1.20))
        {
            SPR_setFrame(roadsideObjs[i].sprite, 0);
        }
        else if (roadsideObjs[i].posZ < FASTFIX32(1.76))
        {
            SPR_setFrame(roadsideObjs[i].sprite, 1);
        }
        else if (roadsideObjs[i].posZ < FASTFIX32(2.65))
        {
            SPR_setFrame(roadsideObjs[i].sprite, 2);
        }
        else if (roadsideObjs[i].posZ < FASTFIX32(3.00))
        {
            SPR_setFrame(roadsideObjs[i].sprite, 3.4);
        }
        else if (roadsideObjs[i].posZ < FASTFIX32(3.5))
        {
            SPR_setFrame(roadsideObjs[i].sprite, 4);
        }
        else //  if (roadsideObjs[i].posZ < FASTFIX32(4.44))
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
        playerSprite->speed = FASTFIX32(0);
        }
         */
        // start the other cars.
        if (state & BUTTON_C)
        {
            redCarSprite->speed = FASTFIX32(0.12);
            greenCarSprite->speed = FASTFIX32(0.09);
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
        playerSprite->speed = playerSprite->speed + FASTFIX32(0.033);
    }
    else if (accelerate == -1)
    {
        // breaks applied
        playerSprite->speed = playerSprite->speed - FASTFIX32(0.78);
    }
    else if (accelerate == 0)
    {
        // slow down naturally
        playerSprite->speed = playerSprite->speed - FASTFIX32(0.06);
    }
    // limit speed
    if (playerSprite->speed < FASTFIX32(0))
    {
        playerSprite->speed = FASTFIX32(0);
    }
    if (playerSprite->speed > FASTFIX32(4.0))
    {
        playerSprite->speed = FASTFIX32(2.0);
    }


    // handle turning
    if (turning == 1)
    {
        steeringDir = steeringDir + FASTFIX32(2.2);
        if (steeringDir > FASTFIX32(20))
        {
            steeringDir = FASTFIX32(20);
        }
    }
    else if (turning == -1)
    {
        steeringDir = steeringDir - FASTFIX32(2.2);
        if (steeringDir < FASTFIX32(-20))
        {
            steeringDir = FASTFIX32(-20);
        }
    }
    else
    {
        // pull back to center
        if (steeringDir < FASTFIX32(0.0))
        {
            steeringDir = steeringDir + FASTFIX32(3.2);
            if (steeringDir > FASTFIX32(0.0))
            {
                steeringDir = FASTFIX32(0.0);
            }
        }
        else if (steeringDir > FASTFIX32(0.0))
        {
            steeringDir = steeringDir - FASTFIX32(3.2);
            if (steeringDir < FASTFIX32(0.0))
            {
                steeringDir = FASTFIX32(0.0);
            }
        }
    }


    // Only update the car sprite and road position if moving
    if (playerSprite->speed > FASTFIX32(0.0))
    {

        // set frame based on steeringDir as long as we're moving forward.
        if (steeringDir < FASTFIX32(-12.00))
        {
            SPR_setFrame(playerSprite->sprite, 0);
        }
        else if (steeringDir < FASTFIX32(-0.02))
        {
            SPR_setFrame(playerSprite->sprite, 1);
        }
        else if (steeringDir > FASTFIX32(12.0))
        {
            SPR_setFrame(playerSprite->sprite, 4);
        }
        else if (steeringDir > FASTFIX32(0.02))
        {
            SPR_setFrame(playerSprite->sprite, 3);
        }
        else
        {
            // centered
            SPR_setFrame(playerSprite->sprite, 2);
        }


        // start shifting the road based on speed, steeringDir and road DX


        // >> So, instead of moving the player's car sprite, you keep it in the center of the
        // >> screen and move the road-- more importantly, **YOU MOVE THE POSITION OF THE
        // >> CENTER-LINE AT THE FRONT (BOTTOM) OF THE SCREEN**. Now, you want to assume that
        // >> the player is going to be looking at the road always, SO MAKE THE ROAD END AT
        // >> THE CENTER OF THE SCREEN. You'll need an angle-of-road variable for this. So,
        // >> CALCULATE THE DIFFERENCE BETWEEN THE CENTER OF THE SCREEN AND THE POSITION OF
        // >> THE FRONT (BOTTOM) OF THE ROAD, and DIVIDE BY THE HEIGHT OF THE ROAD'S GRAPHIC. That
        // >> will give you the amount to move the center of the road each line.


        // >>  variable for this. So, calculate the difference between the center of the screen
        // >>  and the position of the front of the road, and divide by the height of the
        // >>  road's graphic. That will give you the amount to move the center of the road
        // >>  each line.
        if (turning != 0)
        {
            //KLog_F1("steeringDir: ", steeringDir);
            centerLine = centerLine - steeringDir;
        }

        // also factor in the curve of the road segment
        centerLine = centerLine + segments[playerSprite->segment_index].carX;


        //KLog_F1( "centerLine: ", centerLine);
        // Limit how far the car can move to the side
        if (centerLine > FASTFIX32(323))
        {
            centerLine = FASTFIX32(328);
        }
        else if (centerLine < FASTFIX32(-4))
        {
            centerLine = FASTFIX32(-4);
        }

        // update angleOfRoad for perspective steering.
//        fastfix32 step = FF32_div((centerLine - FASTFIX32(160)), // calc diff between center and positoin at front
//                FASTFIX32(ZMAP_LENGTH));							// divide by the height of the road graphic.
//
//
//        fastfix32 current = FASTFIX32(0);
//        for (int i = ZMAP_LENGTH-1; i >= 0; --i)// farthest has lowest offset.
//        {
//            angleOfRoad[i] = current;
//            current = current + step;
//        }
    }
}

void updateCar(struct CP_SPRITE *carSprite, u8 onYourLeft)
{
    // update car position along track
    //KLog_F2("spd: ", carSprite->speed, " pos: ", carSprite->position);
    carSprite->position = carSprite->position + carSprite->speed;
    if (carSprite->position >= trackLength)
    {
        carSprite->position = FASTFIX32(0);
        carSprite->segment_index = 0;
    }
    while (carSprite->position > segmentDistances[carSprite->segment_index])
    {
        ++carSprite->segment_index;
    }

    // Z distance of car from player
    fastfix32 dist = carSprite->position - playerSprite->position;
    if (dist < FASTFIX32(0)																										// negative
            && playerSprite->position > (trackLength - zmap[ZMAP_LENGTH - 1]) // player close to end of track
            && (carSprite->position < zmap[ZMAP_LENGTH - 1])									// enemy close to beginning of track
       )
    {
        // recompute dist because of edge case
        dist = (carSprite->position + trackLength) - playerSprite->position;
    }

    // check if car should be visible to the player
    if (dist >= FASTFIX32(-0.5) && dist < zmap[ZMAP_LENGTH - 4])
    {
        // car is visible, so we try to  display it
        SPR_setVisibility(carSprite->sprite, VISIBLE);


        // compute a Y pos for the car (this could be made more efficient)
        u8 y = 0;
        for (u8 i = VERTICAL_REZ - 1; i > horizonLine; --i)
        {
            fastfix32 tmpZ = yToZdist[i];
            if (tmpZ > dist)
            {
                carSprite->posY = FASTFIX32(i - carSprite->offsetY);
                y = i;
                break;
            }
        }

        // scale the car sprites based on Z value
        if (dist < FASTFIX32(1.06))
        {
            SPR_setAnimAndFrame(carSprite->sprite, 0, 0);
        }
        else if (dist < FASTFIX32(1.26))
        {
            SPR_setAnimAndFrame(carSprite->sprite, 1, 0);
        }
        else if (dist < FASTFIX32(1.49))
        {
            SPR_setAnimAndFrame(carSprite->sprite, 2, 0);
        }
        else if (dist < FASTFIX32(1.8))
        {
            SPR_setAnimAndFrame(carSprite->sprite, 3, 0);
        }
        else if (dist < FASTFIX32(2.4))
        {
            SPR_setAnimAndFrame(carSprite->sprite, 4, 0);
        }
        else if (dist < FASTFIX32(3.08))
        {
            SPR_setAnimAndFrame(carSprite->sprite, 5, 0);
        }
        else
        {
            SPR_setAnimAndFrame(carSprite->sprite, 6, 0);
        }
        if (onYourLeft)
        {


            carSprite->posX = (yToRoadCenter[y] - FASTFIX32(carSprite->offsetX)) - (roadSideObjectOffset[y]>> 2);



        }
        else
        {
            carSprite->posX = (yToRoadCenter[y] - FASTFIX32(carSprite->offsetX)) + (roadSideObjectOffset[y] >> 2 );
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
    fastfix32 currentX = FASTFIX32(0); // Lou's pseudo 3d page says to use Half of the screen width,
                                       // but I've defined SCROLL_CENTER to handle this

    fastfix32 dx = FASTFIX32(0);	// Curve Amount, constant per segment.
    fastfix32 ddx = FASTFIX32(0); // Curve Amount, changes per line

    fastfix32 dy = FASTFIX32(0);	// Slope Amount
    fastfix32 ddy = FASTFIX32(0); // Slope Amount, changes per line

    fastfix32 currentDrawingPos = FASTFIX32(223); // The drawing loop would start at the beginning of the Z-map (nearest).  Basically the bottom of the screen
    horizonLine = 223;										// keep track of where the horizon is.  I"m starting at the bottom and will update as the rode gets computed

    // car position changes with speed
    playerSprite->position = playerSprite->position + playerSprite->speed;
    //KLog_F2("car pos: ", playerSprite->position, " track length: ", trackLength );
    if (playerSprite->position >= trackLength)
    {
        playerSprite->position = FASTFIX32(0);
        playerSprite->segment_index = 0;
    }
    while (playerSprite->position > segmentDistances[playerSprite->segment_index])
    {
        ++playerSprite->segment_index;
    }
    colorCyclePosition = colorCyclePosition - playerSprite->speed;
    if (FF32_toInt(colorCyclePosition) < 0)
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
        fastfix32 z = zmap[223 - bgY]; // zmap[0] is closest
        fastfix32 posZ = playerSprite->position + z;
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
                    posZ = posZ - trackLength;
                }
            }
        }

        ddx += dx;
        currentX += ddx;

        //////////////////////////////////////////////////////////////////////
        // Coloring
        //  For each Z, make one of the bits represent the shade
        //  of the road (dark or light). Then, just draw the
        //  appropriate road pattern or colors for that bit
        fastfix32 tmpz = colorCyclePosition - z;
        u16 zcolor = (u16)FF32_toInt(tmpz << 1); // >> 1);

        ddy += dy;
        s16 cdp = FF32_toInt(currentDrawingPos);				 // current vertical drawing position
        fastfix32 deltaDrawingPos = FASTFIX32(1) + ddy; // increment drawing position
        fastfix32 nextDrawingPos = currentDrawingPos - deltaDrawingPos;
        s16 ndp = FF32_toInt(nextDrawingPos); // figure out next drawing position

        // get perspective shift for current bgY
        int xShift = FF32_toInt(angleOfRoad[223-bgY]); // closest is 0
        fastfix32 currentRoadCenter = FASTFIX32(160 + xShift) + currentX;
        // repeat line if theres a gap greater than 1
        for (; cdp >= ndp; --cdp) //
        {
            if (cdp <= horizonLine) // if current drawing position is above the horizon
            {
                HscrollA[cdp] = SCROLL_CENTER + FF32_toInt(currentX) + xShift; // this_line.x = current x | using horizontal scrolling to fake curves
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
                    roadsideObjs[i].posY = currentDrawingPos - FASTFIX32(roadsideObjs[i].offsetY);

                    // figure out X position.
                    if (i % 2 == 0)
                    {
                        roadsideObjs[i].posX = (currentRoadCenter - (roadSideObjectOffset[bgY] + FASTFIX32(roadsideObjs[i].offsetX)));
                        if (roadsideObjs[i].posX < FASTFIX32(-40)) // hide sprite if it's offscreen
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
                        roadsideObjs[i].posX = (currentRoadCenter + (roadSideObjectOffset[bgY] - FASTFIX32(roadsideObjs[i].offsetX)));
                        if (roadsideObjs[i].posX > FASTFIX32(320)) // hide sprite if it's offscreen
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
    if (playerSprite->speed != FASTFIX32(0.0))
    {
        backgroundPosition = backgroundPosition - segments[playerSprite->segment_index].bgdx;
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
    /*
       DO BEFORE COMPILE>
       for (int i = 0; i < ZMAP_LENGTH; ++i)
       {
       zmap[i] = FF32_div(FASTFIX32(-75), (FASTFIX32(i) - FASTFIX32(113)));
    //scale[i] = F16_div(FIX16(1), zmap[i]);
    //KLog_F2("i: ", FASTFIX32(i), " z: ", zmap[i]);
    char txt[26];
    fastFix32ToStr(zmap[i], txt,9 );
    kprintf("i: %d z: %s", FASTFIX32(i), txt);
    }
     */


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
    fastfix32 sideStep = FASTFIX32(1.771);
    //
    // Looks better w/ more padding
    // (159 + 60) - (6+4) =  209
    // step size 209/96  2.177 << step size per line
    fastfix32 sideOffsetFromCenter = FASTFIX32(6);		// tree width is 88 ..   half of 88 is 44  | bush width is 48 -> 24 | rock witdh is 40 -> 20
    fastfix32 objectOffsetFromCenter = FASTFIX32(30); // tree width is 88 ..   half of 88 is 44  | bush width is 48 -> 24 | rock witdh is 40 -> 20
                                                      //fastfix32 rightFromCenter = FASTFIX32(11); // tree width is 88 ..   half of 88 is 44  | bush width is 48 -> 24 | rock witdh is 40 -> 20
                                                      //fastfix32 leftFromCenter = FASTFIX32(-11); // tree width is 56 ..   half of 88 is 44
    fastfix32 objectStep = FASTFIX32(2.177);
    for (int i = 224 - ZMAP_LENGTH; i < 224; i++)
    {
        roadSideObjectOffset[i] = objectOffsetFromCenter;
        objectOffsetFromCenter = objectOffsetFromCenter + objectStep;

        roadSideOffset[i] = sideOffsetFromCenter;
        sideOffsetFromCenter = sideOffsetFromCenter + sideStep;
        //roadSideObjectOffsetLeft[i] = leftFromCenter;
        //leftFromCenter = leftFromCenter - step;
        //KLog_F2(" i: ", FASTFIX32(i), "  road offset right: ", roadSideObjectOffsetRight[i]);

        // clear it for later use
        angleOfRoad[i] = FASTFIX32(0);
    }

    //////////////////////////////////////////////////////////////
    // prep road variables
    for (u8 i = 0; i < ROAD_SEGMENTS_LENGTH; ++i)
    {
        trackLength = trackLength + segments[i].length;
        segmentDistances[i] = trackLength;
        char txt[26];
        fastFix32ToStr(trackLength, txt,9 );
        kprintf("i: %d trackLength: %s", i, txt);
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
    playerSprite->position = FASTFIX32(0);
    playerSprite->segment_index = 0;
    playerSprite->offsetY = 50; // 56 tall bottom is 51 ish
    playerSprite->offsetX = 44; // 88 wide
    playerSprite->sprite = NULL;
    playerSprite->posX = FASTFIX32(116.0);
    playerSprite->posY = FASTFIX32(160.0);																	 // top pixel of car is at 162.  bottom pixel of car is at 211.  so dist 0 is y=160
    playerSprite->sprite = SPR_addSprite(&car,													 // Sprite defined in resources
            FF32_toInt(playerSprite->posX), // starting X position
            FF32_toInt(playerSprite->posY), // starting Y position
            TILE_ATTR(PAL2,								 // specify palette
                1,										 // Tile priority ( with background)
                FALSE,								 // flip the sprite vertically?
                FALSE								 // flip the sprite horizontally
                ));
    SPR_setFrame(playerSprite->sprite, 2);
    SPR_setDepth(playerSprite->sprite, 0);

    redCarSprite = malloc(sizeof(struct CP_SPRITE));
    redCarSprite->position = FASTFIX32(0);
    redCarSprite->segment_index = 0;
    redCarSprite->offsetY = 50; // 56 tall bottom is 51 ish
    redCarSprite->offsetX = 44; // 88 wide
    redCarSprite->speed = FASTFIX32(0.00);
    redCarSprite->sprite = NULL;
    redCarSprite->posX = FASTFIX32(40.0);
    redCarSprite->posY = FASTFIX32(160.0);																	 // bottom of car is ~12 from bottom of screen.
    redCarSprite->sprite = SPR_addSprite(&red_car,											 // Sprite defined in resources
            FF32_toInt(redCarSprite->posX), // starting X position
            FF32_toInt(redCarSprite->posY), // starting Y position
            TILE_ATTR(PAL2,								 // specify palette
                1,										 // Tile priority ( with background)
                FALSE,								 // flip the sprite vertically?
                TRUE									 // flip the sprite horizontally
                ));
    SPR_setAnimAndFrame(redCarSprite->sprite, 0, 0);
    SPR_setDepth(redCarSprite->sprite, 0);

    greenCarSprite = malloc(sizeof(struct CP_SPRITE));
    greenCarSprite->position = FASTFIX32(0);
    greenCarSprite->segment_index = 0;
    greenCarSprite->offsetY = 50; // 56 tall bottom is 51 ish
    greenCarSprite->offsetX = 44; // 88 wide
    greenCarSprite->speed = FASTFIX32(0.00);
    greenCarSprite->sprite = NULL;
    greenCarSprite->posX = FASTFIX32(192.0);
    greenCarSprite->posY = FASTFIX32(160.0);
    greenCarSprite->sprite = SPR_addSprite(&green_car,											 // Sprite defined in resources
            FF32_toInt(greenCarSprite->posX), // starting X position
            FF32_toInt(greenCarSprite->posY), // starting Y position
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
            SPR_setDepth(roadsideObjs[i].sprite, 224 - FF32_toInt(roadsideObjs[i].posY) - roadsideObjs[i].offsetY);
            // Draw object at new position
            SPR_setPosition(roadsideObjs[i].sprite, FF32_toInt(roadsideObjs[i].posX), FF32_toInt(roadsideObjs[i].posY));
        }

        // Draw car at now position
        SPR_setPosition(playerSprite->sprite, FF32_toInt(playerSprite->posX), FF32_toInt(playerSprite->posY));
        SPR_setPosition(redCarSprite->sprite, FF32_toInt(redCarSprite->posX), FF32_toInt(redCarSprite->posY));
        SPR_setPosition(greenCarSprite->sprite, FF32_toInt(greenCarSprite->posX), FF32_toInt(greenCarSprite->posY));

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
