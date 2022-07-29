from PIL import Image, ImageDraw

width = 512
midpint = 256
height = 224
step = 32

img = Image.new( mode="RGB", size = (width,height), color = "darkgreen" )
dImage = ImageDraw.Draw( img )


for x in range( 0, width,2 ):
    print(x) 
    shape = [ (x, 0), ( x*32, 223 ), ( x*32 + 32, 223), (x,0 ) ]
    dImage.polygon( shape, fill = "green" , outline="green")



img.save("test.png")




