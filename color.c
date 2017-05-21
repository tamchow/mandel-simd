#include <assert.h>
#include <math.h>
#include <stdlib.h>

#include "color.h"
extern inline void rgbPixel(channel* image, int scan, int y, int x, rgb color)
{
	colorPixel(image, scan, y, x, color.r, color.g, color.b);
}

extern inline void colorPixel(channel* image, int scan, int y, int x, channel r, channel g, channel b)
{
	image[y * scan * 3 + x * 3 + 0] = r;
	image[y * scan * 3 + x * 3 + 1] = g;
	image[y * scan * 3 + x * 3 + 2] = b;
}

extern inline rgb* packRGB(rgb* result, channel r, channel g, channel b)
{

	result->r = r;
	result->g = g;
	result->b = b;
	return result;
}

extern inline void unpackRGB(rgb color, channel* r, channel* g, channel* b)
{
	*r = color.r;
	*g = color.g;
	*b = color.b;
}
//Grossly optimize this based on chosen-use case - lots of redundant computations are being done here
extern inline int normalizeIndex(float val, float min, float max, float density, int rgbs)
{
	assert(max >= min && val <= max && (val >= min));
	/*if(!((max >= min) && (val <= max) && (val >= min)))
	{
		printf("%f<=%f<=%f", min, val, max);
		exit(EXIT_FAILURE);
	}*/
	//drop these if the values involved are guaranteed to be always +ve
	/*min = fabsf(min);
	max = fabsf(max);
	val = fabsf(val);*/
	if (val == 0 || max == 0)
	{
		return 0;
	}
	if (val == max)
	{
		return rgbs - 1;
	}
	float minsqrt = sqrtf(min);
	return (int)((sqrtf(val) - minsqrt) / (sqrtf(max) - minsqrt) * density) % rgbs;
}
/*
Also try (from Wikipedia article on linear interpolation):

	// Imprecise method, which does not guarantee v = v1 when t = 1, due to floating-point arithmetic error.
	// This form may be used when the hardware has a native fused multiply-add instruction.
	float lerp(float v0, float v1, float t) {
		return v0 + t * (v1 - v0);
	}
*/
extern inline rgb* lerp(rgb* result, rgb fromColor, rgb toColor, float bias)
{
	bias = fabsf(bias);
	bias = bias - (int)bias;
	result->r = (channel)(toColor.r + (fromColor.r - toColor.r) * bias);
	result->g = (channel)(toColor.g + (fromColor.g - toColor.g) * bias);
	result->b = (channel)(toColor.b + (fromColor.b - toColor.b) * bias);
	return result;
}