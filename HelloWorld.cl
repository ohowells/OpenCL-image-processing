
// Input and output images stored as generic memory buffer objects
kernel void RGB_XYY(global const float* red_input, global const float* green_input, global const float* blue_input, 
				    global float *red_output,  global float *green_output,  global float *blue_output, 
					const int w)
{
	int baseX = get_global_id(0);
	int baseY = get_global_id(1);

	global const float *rsrcPixel = red_input   + (baseY * w) + baseX;
	global const float *gsrcPixel = green_input + (baseY * w) + baseX;
	global const float *bsrcPixel = blue_input  + (baseY * w) + baseX;

	global float *rdstPixel = red_output   + (baseY * w) + baseX;
	global float *gdstPixel = green_output + (baseY * w) + baseX;
	global float *bdstPixel = blue_output  + (baseY * w) + baseX;

	*rdstPixel = 0.4124f * *rsrcPixel + 0.3576f * *gsrcPixel + 0.1805f * *bsrcPixel;
	*gdstPixel = 0.2126f * *rsrcPixel + 0.7152f * *gsrcPixel + 0.0722f * *bsrcPixel;
	*bdstPixel = 0.0193f * *rsrcPixel + 0.1192f * *gsrcPixel + 0.9505f * *bsrcPixel;
}

kernel void XYY_XYZ(global const float* red_input, global const float* green_input, global const float* blue_input, 
				    global float *red_output,  global float *green_output,  global float *blue_output, 
					const int w)
{
	int baseX = get_global_id(0);
	int baseY = get_global_id(1);

	global const float *rsrcPixel = red_input   + (baseY * w) + baseX;
	global const float *gsrcPixel = green_input + (baseY * w) + baseX;
	global const float *bsrcPixel = blue_input  + (baseY * w) + baseX;

	global float *rdstPixel = red_output   + (baseY * w) + baseX;
	global float *gdstPixel = green_output + (baseY * w) + baseX;
	global float *bdstPixel = blue_output  + (baseY * w) + baseX; 

	*rdstPixel = *rsrcPixel / (*rsrcPixel + *gsrcPixel + *bsrcPixel);
	*gdstPixel = *gsrcPixel / (*rsrcPixel + *gsrcPixel + *bsrcPixel);
	*bdstPixel = *gsrcPixel / 2;
}

kernel void XYY_L(global const float* red_input, global const float* green_input, global const float* blue_input, 
				  global float *red_output,  global float *green_output,  global float *blue_output, 
				  const int w)
{
	int baseX = get_global_id(0);
	int baseY = get_global_id(1);

	global const float *rsrcPixel = red_input   + (baseY * w) + baseX;
	global const float *gsrcPixel = green_input + (baseY * w) + baseX;
	global const float *bsrcPixel = blue_input  + (baseY * w) + baseX;

	global float *rdstPixel = red_output   + (baseY * w) + baseX;
	global float *gdstPixel = green_output + (baseY * w) + baseX;
	global float *bdstPixel = blue_output  + (baseY * w) + baseX;

	float X = *rsrcPixel * (*bsrcPixel / *gsrcPixel);
	float Y = *bsrcPixel;
	float Z = (1 - *rsrcPixel - *gsrcPixel) * (*bsrcPixel / *gsrcPixel);

	*rdstPixel = 3.2405f * X + -1.5371f * Y + -0.4985f * Z;
	*gdstPixel = -0.9693f * X + 1.8760f * Y + 0.0416f * Z;
	*bdstPixel = 0.0556f * X + -0.2040f * Y + 1.0572f * Z;
}