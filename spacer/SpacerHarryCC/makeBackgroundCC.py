from PIL import Image, ImageDraw
import math

# 28 squares? 112 squares really, but 

width = 512
midpint = width / 2
buttomTileWidth = 48
height = 400   # try differnt values to get slightly different tile results.  I like 400~ish the best so far.

bottomLeftStart =  ( width - buttomTileWidth * width ) / 2;

img = Image.new( mode="P", size = (width,height))
img.putpalette([
  0,0,0,  # 0
  63,0,0, # red 1
  127,0,0,
  190,0,0,
  255,0,0,
  0,0,63,   # blue 5
  0,0,127,
  0,0,190,
  0,0,255,
  0,63,0,   # green 9
  0,127,0, 
  0,190,0,
  0,255,0,
  0,63,63,   # 13
  0,127,127, 
  0,190,190

  ])
dImage = ImageDraw.Draw( img )

worldY =  -500


start = 0  # bottom based on Lou's page.
end = 159  # up
lowZ = worldY/ (start - height/2)
highZ = worldY/ (end - height/2)

tiles = 25 
tileZ = ( highZ - lowZ ) / tiles

lastZ = lowZ
tileYs = []
for y in range( start, end, 1 ):
  #z = int( (1+(y-159)/15.9)  * 47/ (y - 160))
  z =  worldY/ (y - height/2)
  msg = ""
  if z - lastZ > tileZ:
    # use which ever was closer.
    stepBackZ = worldY/ (y-1 - height/2)  
    if stepBackZ - lastZ  >= z - lastZ:
      tileYs.append( height - y  )  # subtract from height to get back into pixel coords
    else:
      tileYs.append(height - y-1 )
    lastZ += tileZ 
  if z - lastZ == tileZ:
    tileYs.append(  y )



currStart = start
for y in tileYs:
  print( y )


# draw from bottom up
currY = height - 1
# Tile width variables
buttomTileWidth = 48
topStripWidth = 1
tileWidthDelta = (buttomTileWidth - topStripWidth) / (end - start )
print(tileWidthDelta)
currTileWidth = buttomTileWidth

# tile depth variables
tiles2 = tiles * 4 
tileZ2 = ( highZ - lowZ ) / tiles2
lastZ = lowZ
# tile color starting indicies
color1 = 1
color2 = 5
for endY in tileYs:
  # how many X iterations?
  xCount = math.ceil(width/(2* currTileWidth)) +1
  colorOffset = 0
  for y in range ( currY, endY, -1 ) :
    # color choice now depends on a different distance
    if  currY - endY > 4 :
      # use all 4 possible color offset
      z = worldY/ ( (height -1 )- y  - height/2)
      if z - lastZ > tileZ2:
        z2 = worldY/ ( (height )- y  - height/2)
        if z2 >= z :
          colorOffset+=1
          lastZ += tileZ2
        if ( colorOffset > 3 ) :
          colorOffset = 3;
    elif  currY - endY > 2 :
      # use 3 of 4 possible color offset
      if y < currY-1:
        colorOffset += 1
    elif  currY - endY > 1 :
      # use 2 of 4 possible color offset
      colorOffset = 0

    # spread out from center.
    p1 = round(width/2 -1)
    p2 = round(p1 - currTileWidth)
    for x in range( 0, xCount ) :
      dImage.line( [(p1,y) , (p2,y)], fill =  color1 + colorOffset if x%2 == 0 else color2 + colorOffset )
      # and opposites
      dImage.line( [(511-p1,y) , (512-p2,y)], fill =  color2 + colorOffset if x%2 == 0 else color1 + colorOffset )
      p1 = p2 -1
      p2 = p1 - currTileWidth

    currTileWidth -= tileWidthDelta

  # handle next tile
  currY = endY
  lastZ += tileZ2  # next tile start
  if color1 == 1:
    color1 = 5
    color2 = 1
  else:
    color1 = 1
    color2 = 5

# save a starter image for the project
img.save("starter.png")




