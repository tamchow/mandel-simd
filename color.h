#pragma once
#include <stdint.h>


typedef uint8_t channel;
typedef struct rgb {
	channel r;
	channel g;
	channel b;
} rgb;

inline rgb* lerp(rgb* result, rgb fromrgb, rgb torgb, float bias);
inline rgb* packRGB(rgb* result, channel r, channel g, channel b);
inline void unpackRGB(rgb rgb, channel * r, channel* g, channel* b);
inline int normalizeIndex(float val, float min, float max, float density, int colors);
inline void rgbPixel(channel* image, int scan, int y, int x, rgb rgb);
inline void colorPixel(channel* image, int scan, int y, int x, channel r, channel g, channel b);