#pragma once
#include <stdbool.h>

typedef struct fpair {
	float x, y;
} fpair;

typedef enum ColorMode
{
	SMOOTH, NO_SMOOTH
} ColorMode;

typedef struct spec {
    /* Image Specification */
	int startX;
	int startY;
    int width;
    int height;
    /* Fractal Specification */
	fpair xlim;
    fpair ylim;
	bool is_point_width;
    int iterations;
	float bailout_sq;
	ColorMode mode;
} spec;

void convert_point_width_spec_to_range(spec* s);