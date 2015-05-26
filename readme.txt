This code implements a number of image processing operations using OpenCL.

Using the map pattern, implement a OpenCL kernel to convert an image, where each pixel 
is stored with red, green and blue components (an RGB colour space) into an image 
where each pixel is stored with chromaticity (x, y) and luminance (Y).


First convert RGB into intermediate XYZ values:

X = 0.4124 * R  +  0.3576 * G  +  0.1805 * B
Y = 0.2126 * R  +  0.7152 * G  +  0.0722 * B
Z = 0.0193 * R  +  0.1192 * G  +  0.9505 * B


Second calculate xyY as:

x = X / (X + Y + Z)
y = Y / (X + Y + Z)
Y = Y

…where Y is the luminance of the given pixel.  The output of this stage will be a new 
image where each pixel is stored in the xyY colour space instead of the RGB colour space.

Second OpenCL kernel to take the resulting image from step 1 and set the luminance of each 
pixel to be 50% of its original value.

Third OpenCL kernel to convert the xyY image you modified in step 2 back into the RGB colour 
space and save the resulting image to disk.

First convert the xyY values back into intermediate XYZ values…

X = x * (Y / y)
Y = Y
Z = (1- x- y) * (Y/ y)


…then convert the intermediate XYZ values into RGB values:

R =  3.2405 * X  +  -1.5371 * Y  +  -0.4985 * Z
G = -0.9693 * X  +   1.8760 * Y  +   0.0416 * Z
B =  0.0556 * X  +  -0.2040 * Y  +   1.0572 * Z