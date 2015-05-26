#include "imageio.h"

// Windows Imaging Component factory class (singleton)
static IWICImagingFactory			*wicFactory = NULL;

//
// Private API
//
HRESULT					createWICFactory(void);
IWICImagingFactory*		getWICFactory(void);
HRESULT					getWICFormatConverter(IWICFormatConverter **formatConverter);
HRESULT					loadWICBitmap(const std::wstring& imagePath, IWICBitmap **bitmap);
bgr8*					convertFloatImageToBGR8Image(int w, int h, const float *image);

// safe release COM interfaces
template <class T>
inline void SafeRelease(T **comInterface)
{
	if (*comInterface)
	{
		(*comInterface)->Release();
		*comInterface = NULL;
	}
}

//
// Public function implementation
//
int saveImage(const int w, const std::vector<float>& image, const std::wstring& imagePath)
{
	return saveImage(w, 1, &image[0], imagePath);
}

int saveImage(const int w, const int h, const float *floatImage, const std::wstring& imagePath)
{
	bgr8 *buffer = convertFloatImageToBGR8Image(w, h, floatImage);

	if (!buffer) return 0;

	int result = saveImage(w, h, buffer, imagePath);

	free(buffer);
	return result;
}

int saveImage(const int w, const int h, bgr8 *buffer, const std::wstring& imagePath)
{
	// validate image buffer parameter
	if (!buffer) return 1;

	HRESULT		hr;

	// get and validate WIC factory
	IWICImagingFactory *wicFactory = getWICFactory();

	if (!wicFactory) return 1;

	//	
	// export using WIC
	//
	IWICStream				*piStream = NULL;
	IWICBitmapEncoder		*piEncoder = NULL;
	IWICBitmapFrameEncode	*piBitmapFrame = NULL;
	IPropertyBag2			*pPropertybag = NULL;


	// create new stream
	hr = wicFactory->CreateStream(&piStream);

	// initialise stream from imagePath and setup for generic write
	if (SUCCEEDED(hr))
		hr = piStream->InitializeFromFilename(imagePath.c_str(), GENERIC_WRITE);

	// create new image encoder
	if (SUCCEEDED(hr))
		hr = wicFactory->CreateEncoder(GUID_ContainerFormatTiff, NULL, &piEncoder);

	// initialise the encode with the given stream
	if (SUCCEEDED(hr))
		hr = piEncoder->Initialize(piStream, WICBitmapEncoderNoCache);

	// create new frame
	if (SUCCEEDED(hr))
		hr = piEncoder->CreateNewFrame(&piBitmapFrame, &pPropertybag);

	// configure TIFF output
	if (SUCCEEDED(hr))
	{
		PROPBAG2 option = { 0 };
		VARIANT varValue;

		option.pstrName = L"TiffCompressionMethod";

		VariantInit(&varValue);

		varValue.vt = VT_UI1;
		varValue.bVal = WICTiffCompressionZIP;

		hr = pPropertybag->Write(1, &option, &varValue);

		if (SUCCEEDED(hr))
			hr = piBitmapFrame->Initialize(pPropertybag);
	}

	// set frame size
	if (SUCCEEDED(hr))
		hr = piBitmapFrame->SetSize(w, h);

	// set image format
	WICPixelFormatGUID formatGUID = GUID_WICPixelFormat24bppBGR;
	if (SUCCEEDED(hr))
		hr = piBitmapFrame->SetPixelFormat(&formatGUID);

	// verify formatGUID was returned with GUID_WICPixelFormat24bppBGR
	if (SUCCEEDED(hr))
		hr = IsEqualGUID(formatGUID, GUID_WICPixelFormat24bppBGR) ? S_OK : E_FAIL;

	// write image buffer to the bitmap frame
	if (SUCCEEDED(hr)) {

		int stride = (w * (sizeof(bgr8) << 3) + 7) / 8;
		hr = piBitmapFrame->WritePixels(h, stride, stride*h, (BYTE*)buffer);
	}

	// commit changes to bitmap frame
	if (SUCCEEDED(hr))
		hr = piBitmapFrame->Commit();

	// commit changes in encoder
	if (SUCCEEDED(hr))
		hr = piEncoder->Commit();

	// release resources
	SafeRelease(&piBitmapFrame);
	SafeRelease(&piEncoder);
	SafeRelease(&piStream);
	return 0;
}

// Save (assumed) UNORM float image stored as colour planes as a bgr8 image.
int saveImage(
			  const int w,
			  const int h,
			  float *R,
			  float *G,
			  float *B,
			  const std::wstring& imagePath
			  )
{
	bgr8 *I = static_cast<bgr8*>(malloc(w * h * sizeof(bgr8)));

	if (!I) return 1;

	float *rPtr = R;
	float *gPtr = G;
	float *bPtr = B;

	bgr8 *bgrPtr = I;

	for (int i = 0; i<h; i++)
	{
		for (int j = 0; j<w; j++, rPtr++, gPtr++, bPtr++, bgrPtr++)
		{
			BYTE _r = BYTE(*rPtr * 255.0f);
			BYTE _g = BYTE(*gPtr * 255.0f);
			BYTE _b = BYTE(*bPtr * 255.0f);

			*bgrPtr = bgr8(_b, _g, _r);
		}
	}

	return saveImage(w, h, I, imagePath);
}

// load a bitmap file from disk and return the image data in the CGFloatImage structure *result
int loadImage(const std::wstring& imagePath, CPFloatImage* result)
{
	if (!result) return 1;

	IWICBitmap			*textureBitmap = NULL;
	IWICBitmapLock		*lock = NULL;

	HRESULT hr = loadWICBitmap(imagePath.c_str(), &textureBitmap);

	if (!SUCCEEDED(hr)) return 1;

	// get image dimensions
	UINT w = 0, h = 0;
	hr = textureBitmap->GetSize(&w, &h);

	// lock image
	WICRect rect = { 0, 0, w, h };
	if (SUCCEEDED(hr))
		hr = textureBitmap->Lock(&rect, WICBitmapLockRead, &lock);

	// get pointer to image data
	UINT bufferSize = 0;
	BYTE *buffer = NULL;

	if (SUCCEEDED(hr))
		hr = lock->GetDataPointer(&bufferSize, &buffer);

	if (SUCCEEDED(hr))
	{
		// allocate buffers
		float *B = static_cast<float*>(malloc(w * h * sizeof(float)));
		float *G = static_cast<float*>(malloc(w * h * sizeof(float)));
		float *R = static_cast<float*>(malloc(w * h * sizeof(float)));
		float *A = static_cast<float*>(malloc(w * h * sizeof(float)));

		if (B && G && R && A)
		{
			// extract colour channels into float buffer
			float *bptr = B, *gptr = G, *rptr = R, *aptr = A;
			BYTE *imagePtr = buffer;

			for (UINT j = 0; j<h; j++)
			{
				for (UINT i = 0; i<w; i++, bptr++, gptr++, rptr++, aptr++, imagePtr += 4)
				{
					*bptr = static_cast<float>(imagePtr[0]) / 255.0f;
					*gptr = static_cast<float>(imagePtr[1]) / 255.0f;
					*rptr = static_cast<float>(imagePtr[2]) / 255.0f;
					*aptr = static_cast<float>(imagePtr[3]) / 255.0f;
				}
			}

			// store buffers in result
			result->w = w;
			result->h = h;
			result->blueChannel = B;
			result->greenChannel = G;
			result->redChannel = R;
			result->alphaChannel = A;
		}
		else
		{
			// could not allocate buffers - housekeeping to cleanup
			if (B) free(B);
			if (G) free(G);
			if (R) free(R);
			if (A) free(A);

			hr = E_FAIL;
		}
	}

	SafeRelease(&lock);
	SafeRelease(&textureBitmap);
	return (hr == S_OK) ? 0 : 1; // return 0 on success, otherwise return error code 1
}

// Load and return an IWICBitmap interface representing the image loaded from path.  
// No format conversion is done here - this is left to the caller so each delegate 
// can apply the loaded image data as needed.
HRESULT loadWICBitmap(const std::wstring& imagePath, IWICBitmap **bitmap)
{
	// validate image buffer parameter
	if (!bitmap) return E_FAIL;

	HRESULT		hr;

	// get and validate WIC factory
	IWICImagingFactory *wicFactory = getWICFactory();

	if (!wicFactory) return E_FAIL;

	IWICBitmapDecoder		*bitmapDecoder = NULL;
	IWICBitmapFrameDecode	*imageFrame = NULL;
	IWICFormatConverter		*formatConverter = NULL;

	*bitmap = NULL;

	// create image decoder
	hr = wicFactory->CreateDecoderFromFilename(
											   imagePath.c_str(),
											   NULL,
											   GENERIC_READ,
											   WICDecodeMetadataCacheOnDemand,
											   &bitmapDecoder
											   );

	// validate number of frames
	UINT numFrames = 0;

	if (SUCCEEDED(hr))
		hr = bitmapDecoder->GetFrameCount(&numFrames);

	// decode first image frame (default to first frame - for animated types add parameters to 
	// select frame later!)
	if (SUCCEEDED(hr) && numFrames>0)
		hr = bitmapDecoder->GetFrame(0, &imageFrame);

	if (SUCCEEDED(hr))
		hr = wicFactory->CreateFormatConverter(&formatConverter);

	WICPixelFormatGUID pixelFormat;

	// check we can convert to the required format GUID_WICPixelFormat32bppPBGRA
	if (SUCCEEDED(hr))
		hr = imageFrame->GetPixelFormat(&pixelFormat);

	BOOL canConvert = FALSE;

	if (SUCCEEDED(hr))
		hr = formatConverter->CanConvert(pixelFormat, GUID_WICPixelFormat32bppPBGRA, &canConvert);

	if (SUCCEEDED(hr) && canConvert == TRUE)
	{
		hr = formatConverter->Initialize(
										 imageFrame,					// Input source to convert
										 GUID_WICPixelFormat32bppPBGRA,	// Destination pixel format
										 WICBitmapDitherTypeNone,		// Specified dither pattern
										 NULL,							// Specify a particular palette 
										 0.f,							// Alpha threshold
										 WICBitmapPaletteTypeCustom		// Palette translation type
										 );
	}

	// convert and create bitmap from converter
	if (SUCCEEDED(hr))
		hr = wicFactory->CreateBitmapFromSource(formatConverter, WICBitmapCacheOnDemand, bitmap);

	// cleanup
	SafeRelease(&formatConverter);
	SafeRelease(&imageFrame);
	SafeRelease(&bitmapDecoder);
	return hr;
}

HRESULT initCOM(void)
{
	return CoInitialize(NULL);
}

void shutdownCOM(void)
{
	CoUninitialize();
}

//
// Private API implementation
//

// private function to create WIC factory if needed
HRESULT createWICFactory(void)
{
	return CoCreateInstance(
							CLSID_WICImagingFactory,
							NULL,
							CLSCTX_INPROC_SERVER,
							IID_PPV_ARGS(&wicFactory)
							);
}

IWICImagingFactory* getWICFactory(void)
{
	HRESULT		hr;

	if (!wicFactory)
	{
		hr = createWICFactory();

		if (!SUCCEEDED(hr)) return NULL;
	}
	return wicFactory;
}

HRESULT getWICFormatConverter(IWICFormatConverter **formatConverter)
{
	if (!formatConverter || !wicFactory)
		return E_FAIL;
	else
		return wicFactory->CreateFormatConverter(formatConverter);
}

bgr8 *convertFloatImageToBGR8Image(const int w, const int h, const float *image)
{
	// first find the largest absolute value in image (assume no negative values)
	float maxValue = 0.0f;
	const float *ptr = image;

	for (int i = 0; i<h; i++)
		for (int j = 0; j<w; j++, ptr++)
			maxValue = (fabs(*ptr) > maxValue) ? fabs(*ptr) : maxValue;

	// determine semi-positive status of image
	bool P = isSemiPositive<float>(w, h, image);

	// create new BGR8 image
	bgr8 *newImage = static_cast<bgr8*>(malloc(w * h * sizeof(bgr8)));

	if (newImage)
	{
		// store normalised floats in [0, 255] range
		ptr = image;
		bgr8 *bgrPtr = newImage;

		for (int i = 0; i<h; i++)
			for (int j = 0; j<w; j++, ptr++, bgrPtr++)
				if (P) // semi-positive image
					*bgrPtr = bgr8(BYTE((*ptr / maxValue) * 255.0f));
				else
					*bgrPtr = bgr8(BYTE((*ptr / maxValue + 1.0f) / 2.0f * 255.0f));
	}
	return newImage;
}