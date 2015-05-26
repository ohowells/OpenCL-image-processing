//
// imageio is a simple library to load and save image buffers using WIC
//
#ifndef _IMAGE_IO_
#define _IMAGE_IO_

#include <windows.h>
#include <wincodec.h>
#include <vector>
#include <string>

struct bgr8
{
	BYTE	b, g, r;

	bgr8(int i)
		: b(BYTE(i)), g(BYTE(i)), r(BYTE(i))
	{
	}
	bgr8(BYTE _bgr)
		: b(_bgr), g(_bgr), r(_bgr)
	{
	}
	bgr8(BYTE _b, BYTE _g, BYTE _r)
		: b(_b), g(_g), r(_r)
	{
	}
};

// struct to model a pixel for the format BGRA with 8 bpp
struct BGRA8
{
	BYTE	b, g, r, a;

	BGRA8(int i)
		: b(BYTE(i)), g(BYTE(i)), r(BYTE(i)), a(BYTE(i))
	{
	}
	BGRA8(BYTE _bgra)
		: b(_bgra), g(_bgra), r(_bgra), a(_bgra)
	{
	}
	BGRA8(BYTE _b, BYTE _g, BYTE _r, BYTE _a)
		: b(_b), g(_g), r(_r), a(_a)
	{
	}
};


// encapsulate BGRA8 image buffer
struct CPBitmapImage
{
	int			w, h;
	BGRA8		*buffer;
};

// store a BGRA8 image as floating point values in the range [0, 1].  Each channel 
// (Blue, Green, Red, Alpha) is stored in a seperate buffer
struct CPFloatImage
{
	int			w, h;
	float		*redChannel;
	float		*greenChannel;
	float		*blueChannel;
	float		*alphaChannel;

	CPFloatImage(void)
	{
		w = h = 0;
		redChannel = greenChannel = blueChannel = alphaChannel = nullptr;
	}
};

// return true if all of the pixels in the image are >= 0, false otherwise.  It is assumed 
// operator>= that takes a scalar for comparison is defined for type T
template <typename T>
bool isSemiPositive(const int w, const int h, const T* image)
{
	bool		P = true;
	const		T *ptr = image;

	for (int i = 0; i < (w * h) && P; P = (*ptr >= T(0)), ptr++, i++);
	return P;
}


// load a bitmap image using WIC and return the RGBA channels as floating point arrays in *result
int loadImage(const std::wstring& imagePath, CPFloatImage* result);

// save a 1D std::vector float array to the image file specified in imagePath
int saveImage(const int w, const std::vector<float>& image, const std::wstring& imagePath);

// save a raw 2D float image to disk that is w pixels wide and h pixel high
int saveImage(const int w, const int h, const float *floatImage, const std::wstring& imagePath);

// save a bgr8 image to disk that is w pixels wide and h pixel high
int saveImage(const int w, const int h, bgr8 *buffer, const std::wstring& imagePath);

// Save (assumed) UNORM float image stored as colour planes as a bgr8 image.
int saveImage(
			  const int w,
			  const int h,
			  float *R,
			  float *G,
			  float *B,
			  const std::wstring& imagePath
			  );

// Initialise COM so we can access WIC functionality
HRESULT initCOM(void);

// Shutdown COM when done
void shutdownCOM(void);
#endif