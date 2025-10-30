
ZMAP_LENGTH = 97
CENTER_LINE = 160
MAX_CENTER_LINE = 324  # duh
MIN_CENTER_LINE = -4


# so about 323 -(-4) = 327 total
# range is -4 -160 to 323 - 160 or: -164  to 163

#        if (centerLine > FASTFIX32(323))
#        {
#            centerLine = FASTFIX32(323);
#        }
#        else if (centerLine < FASTFIX32(-4))
#        {
#            centerLine = FASTFIX32(-4);
#        }
#
#        // update angleOfRoad for perspective steering.
#        fastfix32 step = FF32_div((centerLine - FASTFIX32(160)), // calc diff between center and positoin at front
#                FASTFIX32(ZMAP_LENGTH));                          // divide by the height of the road graphic.
#
#
#        fastfix32 current = FASTFIX32(0);
#        for (int i = ZMAP_LENGTH-1; i >= 0; --i)// farthest has lowest offset.
#        {
#            angleOfRoad[i] = current;
#            current = current + step;
#        }
#


print("// assumes centerline limited from -4 to 323, use +4 as the offset int")
print("fastfix32 perspective_step_from_centerline[] = {\n")

for center in range( MIN_CENTER_LINE, MAX_CENTER_LINE ):
    step = ( center - CENTER_LINE ) / ZMAP_LENGTH
    print(f"FASTFIX32( {step:.4f} ), ", end="")
print("\n};")


