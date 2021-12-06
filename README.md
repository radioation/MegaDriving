# Mega Driving

I've been teaching myself SGDK for fun.  This repository contains *experimental* code I've 
written based on [Lou's Pseudo 3d Page](http://www.extentofthejam.com/pseudo/).  It's 
provided here as an example of one possible way to implement pseudo 3d with SGDK.  I am in 
no way saying that this is the best way to implement pseudo-3d roads.  There are likely 
bugs and I haven't tried to optimize the code yet.


I had hoped to write this entirely in C.  Unfortunately, I wasn't able to pull off the color 
cycling used on the grass and road in C.  So the code is a mix of C and some assembly code.

The code has been tested on [BlastEM and Real Hardware](https://youtu.be/p99XATFhSpo)

# TODO
* Improve steering code
* Look into smoothing out road side object and car motion in the Y direction.
* Add readme.md files to go over the code in each example.
