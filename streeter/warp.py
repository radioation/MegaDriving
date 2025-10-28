#!/usr/bin/env python


#
# python3 warp.py  -s road.png  -o test2.png
#


import os,  argparse, logging
import numpy as np
import math
from PIL import Image, ImageDraw
import sys

from pathlib import Path

# 320 x 224
COLS = 640
ROWS = 224
ROAD_HEIGHT = 80
START_ROW = 223-80


def find_coeffs(pa, pb):
    matrix = []
    for p1, p2 in zip(pa, pb):
        matrix.append([p1[0], p1[1], 1, 0, 0, 0, -p2[0]*p1[0], -p2[0]*p1[1]])
        matrix.append([0, 0, 0, p1[0], p1[1], 1, -p2[1]*p1[0], -p2[1]*p1[1]])

    A = np.matrix(matrix, dtype=float)
    B = np.array(pb).reshape(8)

    res = np.dot(np.linalg.inv(A.T * A) * A.T, B)
    return np.array(res).reshape(8)


def main(args, loglevel): 
  logging.basicConfig(format="%(levelname)s: %(message)s", level=loglevel)
  street_filename = args.street_image
  output_filename = args.output_filename
  reps = args.reps
  num_steps = args.steps
  move_lines = args.move_lines
  step_size = args.move_lines / args.steps
  logging.info("WORKING ON:" + street_filename );
  with Image.open( street_filename ) as street_img :
  
    orig_width, orig_height = street_img.size 
 
    street_work_img = Image.new('RGB', (street_img.width,  reps * street_img.height ))
    for r in range(reps + 1):
        street_work_img.paste(street_img, (0,  r * street_img.height))  
  
    width, height = street_work_img.size 
    print(f'width: {width} height: {height}')
    street_work_img.save( "work.png")

    new_width = 2 * width
    new_height = ROAD_HEIGHT
    print(f' new width: {new_width} new height: {new_height}')
    print( [(0, 0), (width, 0), (width, height), (0, height)] )
    print(  [(-2 + new_width/2, 0), (2 + new_width/2, 0), (new_width, new_height), (0, new_height)])
    offset = 0
    for step in range(num_steps): 
      coeffs = find_coeffs(
          [(-2 + new_width/2, 0), (2 + new_width/2, 0), (new_width, new_height), (0, new_height)],    # dest
          [(0, orig_height - offset), (width, orig_height- offset), (width, height-orig_height- offset), (0, height-orig_height- offset)],  # source
          )
      print( coeffs )
      new_img = street_work_img.transform((new_width, new_height), Image.PERSPECTIVE, coeffs, Image.NEAREST)
      offset += step_size
      new_img.save( output_filename + "_" + str(step) + "_" + street_filename )
     




if __name__ == '__main__':
  parser = argparse.ArgumentParser( 
  description = "Create warped pseudo3d images for SGDK",
  epilog = "As an alternative to the commandline, params can be placed in a file, one per line, and specified on the commandline like '%(prog)s @params.conf'.",
  fromfile_prefix_chars = '@' )
  # parameter list
  parser.add_argument(
      "-v",
      "--verbose",
      help="increase output verbosity",
      action="store_true")

  parser.add_argument( "-s",
      "--street_image",
      default = 'street.png',
      help = "street image filename",
      metavar = "ARG")


  parser.add_argument( "-o",
      "--output_filename",
      default = "output.png",
      help = "Output filename",
      metavar = "ARG")

  parser.add_argument( "-r",
      "--reps",
      default = 50,
      type=int,
      help = "how many times to repeate the image vertically",
      metavar = "ARG") 


  parser.add_argument( "-S",
      "--steps",
      default = 12,
      type=int,
      help = "How many steps to move the image",
      metavar = "ARG") 

  parser.add_argument( "-m",
      "--move_lines",
      default = 48,
      type=int,
      help = "How many lines to move the image",
      metavar = "ARG") 

  args = parser.parse_args()

  # Setup logging
  if args.verbose:
    loglevel = logging.INFO
  else:
    loglevel = logging.WARNING

  main(args, loglevel)
