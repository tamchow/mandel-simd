#pragma once
#include <stddef.h>
#include <stdbool.h>
#include "color.h"

typedef struct fpair {
	float x, y;
} fpair;

typedef enum ColorMode
{
	GREYSCALE = 1, ITERATION_COUNT = 2, HISTOGRAM = 3, DEM = 4
} ColorMode;

typedef struct Size_tFloatPair{
	size_t k;
	float mu;
} escapedatum;

size_t* histogram;
//escapedatum **escapedata;

size_t** escapedata;

typedef struct spec {
    /* Image Specification */
    size_t width;
    size_t height;
    int max_color_value;
    /* Fractal Specification */
	fpair xlim;
    fpair ylim;
	bool is_point_width;
    size_t iterations;
	float bailout_sq;
	ColorMode mode;
	Scaling_Mode index_mode;
} spec;

void convert_point_width_spec_to_range(spec* s);