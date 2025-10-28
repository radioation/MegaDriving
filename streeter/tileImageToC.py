#!/usr/bin/env python
from PIL import Image
import sys
import os,  argparse, logging

#
# python3 tileImageToC.py -i 01_tile_test/grass.png -o 01_tile_test/src/grass.c -n grass
#

def main(args, loglevel): 
  logging.basicConfig(format="%(levelname)s: %(message)s", level=loglevel)
  input_filename = args.input_image
  array_name = args.name
  logging.info("WORKING ON:" + input_filename );

  with Image.open(input_filename) as img, open( args.output_filename, "w") as outfile:

    width, height = img.size

    cols = int(width/8)  # each column is a 'frame' to copy to VDP
    rows = int(height/8)
    
    px = img.load()
    
    outfile.write("#include <genesis.h>\n\n")
    outfile.write(f'const u32 {array_name}[{cols}][{8*rows}] = ')
    outfile.write("{\n")
    for tileX in range( 0, cols ):
      outfile.write("  {\n")
      for tileY in range( 0, rows ) :
        outfile.write("    // tile %d \n" % tileY )
        for y in range(0,8):
          outfile.write("    0x")
          for x in range(0,8):
            outfile.write( f'{px[x + tileX * 8,y + tileY * 8 ]:x}' )
          outfile.write(",\n")
      outfile.write("  },\n")
    outfile.write("\n};\n\n")



if __name__ == '__main__':
  parser = argparse.ArgumentParser( 
  description = "Create C array from PNG SGDK",
  epilog = "As an alternative to the commandline, params can be placed in a file, one per line, and specified on the commandline like '%(prog)s @params.conf'.",
  fromfile_prefix_chars = '@' )
  # parameter list
  parser.add_argument(
      "-v",
      "--verbose",
      help="increase output verbosity",
      action="store_true")

  parser.add_argument( "-i",
      "--input_image",
      default = 'grass.png',
      help = "input image filename",
      metavar = "ARG")

  parser.add_argument( "-o",
      "--output_filename",
      default = "out.c",
      help = "Output filename",
      metavar = "ARG")

  parser.add_argument( "-n",
      "--name",
      default = "grass",
      help = "Array Name",
      metavar = "ARG")

  args = parser.parse_args()

  # Setup logging
  if args.verbose:
    loglevel = logging.INFO
  else:
    loglevel = logging.WARNING

  main(args, loglevel)
