* Get rid of FIX32?
  * FASTFIX16 and FIX16 won't have enough space?  FAStFIX might have
    enough decimal resolution, but will overflow for some screen positions ( 8-bits -128 to 128 )
    FIX16 has been too coarse for some road values

  * FASTFIX32 will OVERFLOW up for the  division by (2.7), we can't go to 64-bit numbers
    here

  * This division can be removed, it was only for keepin the cars in their lanes, 
    
```c++
    char txt1[26];
    fastFix32ToStr(yToRoadCenter[y], txt1,9 );
    char txt2[26];
    fastFix32ToStr(FASTFIX32(carSprite->offsetX), txt2,9 );
    char txt3[26];
    fastFix32ToStr(roadSideObjectOffset[y], txt3,9 );

    kprintf("yToRoadCenter[y] %s offset %s roadSideObjectOffset %s", txt1, txt2, txt3 );

    fastfix32 temp1 =  (yToRoadCenter[y] - FASTFIX32(carSprite->offsetX));
    fastFix32ToStr(temp1, txt1,9 );
    kprintf("temp1 %s ", txt1 );
    fastfix32 temp2 = FF32_div(roadSideObjectOffset[y], FASTFIX32(2.7));   <<< NO GOOD
    fastFix32ToStr(temp2, txt1,9 );
    kprintf("temp2 %s ", txt1 );

    carSprite->posX = (yToRoadCenter[y] - FASTFIX32(carSprite->offsetX)) - FF32_div(roadSideObjectOffset[y], FASTFIX32(2.7));

    fastFix32ToStr(carSprite->posX, txt1,9 );
    kprintf("carSprite->posX %s ", txt1 );


    fastfix32 ytc = FASTFIX32( 160.0 );
    fastfix32 os = FASTFIX32( 44.0 );
    fastfix32 rsoo = FASTFIX32( 238.968 );
    fastfix32 sub1 = ytc-os;
    fastfix32 twoPointSeven = FASTFIX32(2.7);
    fastfix32 div1 = FF32_div( rsoo, twoPointSeven );   <<< NO GOOD
    fastFix32ToStr(sub1, txt1,9 );
    fastFix32ToStr(div1, txt2,9 );
    kprintf("sub1 %s div1 %s", txt1,txt2 );



```
we get
```bash
KDEBUG MESSAGE: sub1 116.000000000 div1 -6.309900000
KDEBUG MESSAGE: yToRoadCenter[y] 160.000000000 offset 44.000000000 roadSideObjectOffset 238.990700000
KDEBUG MESSAGE: temp1 116.000000000
KDEBUG MESSAGE: temp2 -6.301500000
KDEBUG MESSAGE: carSprite->posX 122.301500000
```
  * THis divison could be a LUT? it's just to make the road front/bottom shift around when
   you steer. So just for looks. we could make an LUT to handle the *rough* position of the 
  car relative to centerline ( given the screen is 320 pixels wide, a 640 LUT isn't crazy )

```c++
        // update angleOfRoad for perspective steering.
        fix32 step = F32_div((centerLine - FIX32(160)), // calc diff between center and positoin at front
                FIX32(ZMAP_LENGTH));                            // divide by the height of the road graphic.
        fix32 current = FIX32(0);
        for (int i = ZMAP_LENGTH-1; i >= 0; --i)// farthest has lowest offset.
        {
            angleOfRoad[i] = current;
            current = current + step;
        }
```

  * we generate a LUT at runtime with `_div` but could pre-calc and store it in a C file


