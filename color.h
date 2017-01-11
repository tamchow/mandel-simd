#pragma once
#include <stddef.h>


typedef unsigned char channel;
typedef channel* color;

typedef enum Scaling_Modes
{
	SQRT, LOG, LINEAR, DIRECT
} Scaling_Mode;

inline color lerp(color result, color fromcolor, color tocolor, float bias);
inline color color_from_rgb(color color, channel r, channel g, channel b);
inline void rgb_from_color(color color, channel * r, channel* g, channel* b);
inline channel linear_map_channel(float val, float min, float max, float density, Scaling_Mode mode);
inline size_t linear_map_index(float val, float min, float max, float density, size_t colors, Scaling_Mode mode);
inline void color_pixel(channel* image, size_t scan, size_t y, size_t x, color rgb);
inline void rgb_pixel(channel* image, size_t scan, size_t y, size_t x, channel r, channel g, channel b);