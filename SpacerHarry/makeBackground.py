from PIL import Image, ImageDraw


width = 512
midpint = width / 2
bottomStripWidth = 32
topRow = 113
bottomLeftStart =  ( width - bottomStripWidth * width ) / 2;
height = 224

img = Image.new( mode="RGB", size = (width,height), color = (85,146,42) )
dImage = ImageDraw.Draw( img )

shape = [ (0, 0), ( 0, topRow - 1 ), (width-1, topRow - 1), (width-1,0), (0,0) ]
dImage.polygon( shape, fill = (255,0,255) , outline=(255,0,255))


for x in range( 0, width,2 ):
  shape = [ (x, topRow), ( bottomLeftStart + x*bottomStripWidth, height - 1 ), (bottomLeftStart +  x*bottomStripWidth + bottomStripWidth, height - 1), (x,topRow) ]
  dImage.polygon( shape, fill = (183,182,80) , outline=(183,182,80))



img.save("starter.png")




