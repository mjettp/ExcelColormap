
#include "stdafx.h"

#include <windows.h>
#include <stdio.h>
#include <stdint.h>
#include <cmath>

#include "Classes.h"

float clamp(float v)
{
	const float t = v < 0.0f ? 0.0f : v;
	return t > 1.0f ? 1.0f : t;
}

RGBColor GetRGBColour(float t, float vmin, float vmax)
{
	if (t > vmax)
		t = vmax;

	if (t < vmin)
		t = vmin;

	t = ((2.0f * (t - vmin)) / (vmax - vmin)) - 1.0f;

	float red = clamp(1.5f - abs(2.0f * t - 1.0f));
	float green = clamp(1.5f - abs(2.0f * t));
	float blue = clamp(1.5f - abs(2.0f * t + 1.0f));

	RGBColor c = { red, green, blue };
	
	return c;
}

size_t UnicodeToChar(const wchar_t * src, char * dest, size_t dest_len) {
	size_t i;
	wchar_t code;

	i = 0;

	while (src[i] != '\0' && i < (dest_len - 1)) {
		code = src[i];
		if (code < 128)
			dest[i] = char(code);
		else {
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

extern "C" __declspec(dllexport) uint32_t WINAPI InterpolateSingleValue(float value, float min, float max)
{
	return GetRGBColour(value, min, max).EncodeRGB();
}

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
		
		char buff[256] = { 0 };
		UnicodeToChar(v->path, buff, 256);

		imwrite(buff, *image.dstImage);
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