#pragma once
#include <stdbool.h>

#define THREADS  8
#define  CHUNKS 256

const int RGB_BitDepth = 3;

typedef struct fpair {
	float x, y;
} fpair;

typedef struct region {
	fpair start, end;
} region;

typedef enum ColorMode
{
	SMOOTH, NO_SMOOTH
} ColorMode;

typedef struct Configuration {
    /* Image Specification */
	int startX;
	int startY;
    int width;
    int height;
    /* Fractal Specification */
	region bounds;
	bool isPointWidth;
    int iterations;
	float bailout;
	ColorMode mode;
	/*Image specifications*/
	// `gradientScale` being `NAN` is a signal that it should be automatically calculated
	float gradientScale;
	float gradientShift;
	float indexScale;
	float fractionWeight;
	int minimumIterations;
} Configuration;

float tolerance = 1e-5f;

void convertPointWidthToBounds(Configuration* configuration);