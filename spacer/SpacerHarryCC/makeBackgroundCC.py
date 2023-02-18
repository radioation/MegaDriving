from PIL import Image, ImageDraw
import math

# 28 squares? 112 squares really, but 

width = 512
midpint = width / 2
bottomStripWidth = 48
height = 400   # try differnt values to get slightly different tile results.  I like 400~ish the best so far.

bottomLeftStart =  ( width - bottomStripWidth * width ) / 2;

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
  0,63,63,   # yellow 13
  0,127,127, 
  0,190,190

  ])
dImage = ImageDraw.Draw( img )

worldY =  -500


start = 0  # bottom based on Lou's page.
end = 160  # up
lowZ = worldY/ (start - height/2)
highZ = worldY/ (end - height/2)

tiles = 26 
tileZ = ( highZ - lowZ ) / tiles
#print( "tileZ: ", tileZ )

lastZ = lowZ
tileYs = []
for y in range( start, end, 1 ):
  #z = int( (1+(y-159)/15.9)  * 47/ (y - 160))
  z =  worldY/ (y - height/2)
  msg = ""
  if z - lastZ > tileZ:
    # use which ever was closer.
    stepBackZ = worldY/ (y-1 - height/2)  
    print( stepBackZ - lastZ, z - lastZ )
    if stepBackZ - lastZ  > z - lastZ:
      #print("use current")
      tileYs.append( height - y  )  # subtract from height to get back into pixel coords
    else:
      #print("use stepback")
      tileYs.append(height - y-1 )
    lastZ += tileZ 
  if z - lastZ == tileZ:
    tileYs.append(  y )


  #print( y, z, msg)

currStart = start
for y in tileYs:
  print( y )


# draw from bottom up
currY = height - 1
bottomStripWidth = 48
topStripWidth = 1
widthDelta = (bottomStripWidth - topStripWidth) / (end - start )
currWidth = bottomStripWidth
for endY in tileYs:
  print(currY, endY )
  # how many X iterations?
  xCount = math.ceil(width/(2* currWidth)) +1
  print( xCount )
  for y in range ( currY, endY, -1 ) :
    # spread out from center.
    p1 = int(width/2 -1)
    p2 = int(p1 - currWidth)
    for x in range( 0, xCount ) :
      #print(currWidth, widthDelta, " pts: ",  p2, y );
      dImage.line( [(p1,y) , (p2,y)], fill =  3 if x%2 == 0 else 7 )
      # and opposites
      dImage.line( [(511-p1,y) , (512-p2,y)], fill =  7 if x%2 == 0 else 3 )
      p1 = p2 -1
      p2 = p1 - currWidth

    currWidth -= widthDelta
  currY = endY


  # go right or mirror? startXRight = width/2 


#for x in range( 0, width,2 ):
#  shape = [ (x, topRow), ( bottomLeftStart + x*bottomStripWidth, height - 1 ), (bottomLeftStart +  x*bottomStripWidth + bottomStripWidth, height - 1), (x,topRow) ]
#  dImage.polygon( shape, fill = (183,182,80) , outline=(183,182,80))
#
#
#
#shape = [(0,0), (0, 50), ( 50, 50 ), ( 50, 0 ) ]
#dImage.polygon( shape, fill = 1, outline = 1)
img.save("starter.png")




