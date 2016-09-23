import Image
img = Image.open("Lena.png")
out = img.rotate(180)
out.save( "Lena_rotate.png", "png" ) 
