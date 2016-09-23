import sys
import Image
filename = sys.argv[1]
img = Image.open(filename)
out = img.rotate(180)
out.save( "ans2", "png" ) 
