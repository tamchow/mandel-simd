#include <assert.h>
#include "color.h"
#include <math.h>

extern inline void color_pixel(channel* image, size_t scan, size_t y, size_t x, color rgb)
{
	for (int i = 0; i < 3; ++i) {
		image[y * scan * 3 + x * 3 + i] = rgb[i];
	}
}

extern inline void rgb_pixel(channel* image, size_t scan, size_t y, size_t x, channel r, channel g, channel b)
{
	image[y * scan * 3 + x * 3 + 0] = r;
	image[y * scan * 3 + x * 3 + 1] = g;
	image[y * scan * 3 + x * 3 + 2] = b;
}

extern inline color color_from_rgb(color color, channel r, channel g, channel b)
{
	color[0] = r;
	color[1] = g;
	color[2] = b;
	return color;
}

extern inline void rgb_from_color(color color, channel* r, channel* g, channel* b)
{
	*r = color[0];
	*g = color[1];
	*b = color[2];
}

extern inline channel linear_map_channel(float val, float min, float max, float density, Scaling_Mode mode)
{
	return (channel) linear_map_index(val, min, max, density, 256, mode);
}

//Grossly optimize this based on chosen-use case - lots of redundant computations are being done here
extern inline size_t linear_map_index(float val, float min, float max, float density, size_t colors, Scaling_Mode mode)
{
	assert((max > min) && (val <= max) && (val >= min));
	//drop these if the values involved are guaranteed to be always +ve
	/*min = fabsf(min);
	max = fabsf(max);
	val = fabsf(val);*/
	switch (mode) {
	case LINEAR:
		return ((size_t)(((val - min) / (max - min)) * density) % colors);
	case LOG:
		if(val == 0 || max == 0)
		{
			return 0;
		}
		if(min == 0 || min == 1)
		{
			return ((size_t)((logf(val) / logf(max)) * density) % colors);
		}
		return ((size_t)((logf(val / min) / logf(max / min)) * density) % colors);
	case SQRT:
	{
		float minsqrt = sqrtf(min);
		return ((size_t)(((sqrtf(val) - minsqrt) / (sqrtf(max) - minsqrt)) * density) % colors);
	}
	}
}

extern inline color lerp(color result, color fromcolor, color tocolor, float bias)
{
	assert((bias >= 0) && (bias <= 1));
	for (int i = 0; i < 3; ++i)
	{
		result[i] = (channel)(fromcolor[i] * bias + tocolor[i] * (1 - bias));
	}
	return result;
}