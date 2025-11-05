# Mega Driving
The code has been updated to compile with SGDK 2.11. 
Earlier versions of SGDK will likely not compile.

```bash
find . -name main.c -exec sed -i 's/fix32Div/F32_div/g'  {} +
find . -name main.c -exec sed -i 's/fix16Div/F16_div/g'  {} +
find . -name main.c -exec sed -i 's/fix32ToInt/F32_toInt/g'  {} +
find . -name main.c -exec sed -i 's/fix16ToInt/F16_toInt/g'  {} +
```

This repository contains *experimental* code I've written for fake 3D roads and other 
Pseudo3D effects.

## Lou's Pseudo 3D Roads
The code in the "lou" folder was 
written based on [Lou's Pseudo 3d Page](http://www.extentofthejam.com/pseudo/).  It's 
provided here as an example of one possible way to implement pseudo 3d with SGDK.  *I am in 
no way saying that this is the best way to implement pseudo-3d roads.*  There are likely 
bugs and I haven't tried to optimize the code.


I had hoped to write this entirely in C.  Unfortunately, I wasn't able to pull off the color 
cycling used on the grass and road in C.  So the code is a mix of C and some assembly code.

The code has been tested on [BlastEM and Real Hardware](https://youtu.be/p99XATFhSpo)

### TODO
* Make it faster. It works but it's not really game-worthy.
* Improve steering code, feels "wrong" right now.
* Look into smoothing road side object and car motion in the Y direction.
* Add readme.md files to explain the code in each example.
* Read up on AI and make it game~ish/"playable"

## Spacer
I decided to try adapting my road code to the pseudo 3d effect seen in Space Harrier.
It'll probably end up in a lightgun game if I ever get around to it.

## Streeter
Just an attempt to replicate "Street Racer"'s road.

## Mesozoic 
...




