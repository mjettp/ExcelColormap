
#include "stdafx.h"

#include <windows.h>
#include <stdio.h>
#include <stdint.h>
#include <cmath>

#include "Classes.h"


/// <summary>
/// Clamps a float value
/// </summary>
/// <param name="value">Value to be clamped</param>
/// <returns>Clamped value</returns>
float clamp(float value)
{
	const float t = value < 0.0f ? 0.0f : value;
	return t > 1.0f ? 1.0f : t;
}



/// <summary>
/// Converts a given scalar to a color in the MATLAB Jet colormap
/// See https://stackoverflow.com/a/46628410
/// </summary>
/// <param name="value">Value to be converted</param>
/// <param name="vmin">Minimum interval value</param>
/// <param name="vmax">Maximum interval value</param>
/// <returns>RGBColor struct holding the converted color</returns>
RGBColor GetRGBColour(float value, float vmin, float vmax)
{
	//check if the given scalar is within the range
	if (value > vmax)
		value = vmax;

	if (value < vmin)
		value = vmin;

	//MATLAB Jet colormap defined from [-1, 1], so we need to adjust our value for the new interval
	value = ((2.0f * (value - vmin)) / (vmax - vmin)) - 1.0f;

	float red = clamp(1.5f - abs(2.0f * value - 1.0f));
	float green = clamp(1.5f - abs(2.0f * value));
	float blue = clamp(1.5f - abs(2.0f * value + 1.0f));

	RGBColor c = { red, green, blue };
	
	return c;
}


/// <summary>
/// Converts a given unicode string to its multibyte counterpart
/// See https://stackoverflow.com/a/36875023
/// </summary>
/// <param name="src">Unicode string to be converted</param>
/// <param name="dest">Char buffer receiving the data</param>
/// <param name="dest_len">Maximum buffer lenght</param>
/// <returns>Buffer size</returns>
size_t UnicodeToChar(const wchar_t * src, char * dest, size_t dest_len) 
{
	size_t i;
	wchar_t code;

	i = 0;

	while (src[i] != '\0' && i < (dest_len - 1))
	{
		code = src[i];

		if (code < 128)
			dest[i] = char(code);
		else 
		{
			dest[i] = '?';
			if (code >= 0xD800 && code <= 0xD8FF)
				// lead surrogate, skip the next code unit, which is the trail
				i++;
		}

		i++;
	}

	dest[i] = '\0';

	return i - 1;
}

/// <summary>
/// Interpolate a given scalar to a color in the MATLAB Jet colormap
/// </summary>
/// <param name="value">Value to be converted</param>
/// <param name="vmin">Minimum interval value</param>
/// <param name="vmax">Maximum interval value</param>
/// <returns>unsigned int holding the converted color</returns>
extern "C" __declspec(dllexport) uint32_t WINAPI InterpolateSingleValue(float value, float min, float max)
{
	return GetRGBColour(value, min, max).EncodeRGB();
}

/// <summary>
/// Interpolate a given scalar to a color in the MATLAB Jet colormap, taking CellInfo and Settings as inputs.
/// This function is useful to deal with ranges in VBA, directly passing the actual cell.
/// After interpolation it opens an OpenCV window showing the smoothed (Gaussian) image and saves it.
/// </summary>
/// <param name="pCell">Structure holding the cell informations</param>
/// <param name="v">Structure holding the function settings</param>
/// <returns>unsigned int holding the converted color</returns>
extern "C" __declspec(dllexport) uint32_t WINAPI Interpolate(CellInfo* pCell, Settings* v)
{
	if (!image.srcImage)
	{
		image.srcImage = new Mat((int)v->width, (int)v->height, CV_8UC3, Scalar(0, 0, 0));

		image.valueMin = v->minValue;
		image.valueMax = v->maxValue;

		image.wStep = ceil(v->width / v->columnsCount);
		image.hStep = ceil(v->height / v->rowsCount);

		image.irangeColumns = v->columnsCount;
		image.irangeRows = v->rowsCount;
	}

	RGBColor color = GetRGBColour(pCell->value, image.valueMin, image.valueMax);

	if (!image.firstCol && !image.firstRow)
	{
		image.firstRow = pCell->row;
		image.firstCol = pCell->col;
	}
	
	Rect rect((int)((pCell->col - image.firstCol) * image.wStep), (int)((pCell->row - image.firstRow) * image.hStep), (int)image.wStep, (int)image.hStep);
	rectangle(*image.srcImage, rect, Scalar(color.b * 255u, color.g * 255u, color.r * 255u), CV_FILLED);

	if (((pCell->col - image.firstCol + 1) == image.irangeColumns) && ((pCell->row - image.firstRow + 1) == image.irangeRows))
	{
		image.dstImage = new Mat((int)image.width, (int)image.height, CV_8UC3, Scalar(0, 0, 0));

		GaussianBlur(*image.srcImage, *image.dstImage, Size(v->kernelX, v->kernelY), v->sigmaX, v->sigmaY);

		imshow("Output", *image.dstImage);
		
		if (v->path)
		{
			char buff[256] = { 0 };
			UnicodeToChar(v->path, buff, 256);

			imwrite(buff, *image.dstImage);
		}
	}

	return color.EncodeRGB();
}

BOOL APIENTRY DllMain(HMODULE hModule, DWORD dwReason, LPVOID lpReserved)
{
	switch (dwReason)
	{
	case DLL_PROCESS_ATTACH:
	case DLL_THREAD_ATTACH:
	case DLL_THREAD_DETACH:
	case DLL_PROCESS_DETACH:
		break;
	}
	return TRUE;
}