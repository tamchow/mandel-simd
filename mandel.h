#pragma once
#include <stdbool.h>

// OpenMP parameters, change for system
#define THREADS  4	// The number of total system cores (hyperthreading inclusive)
#define  CHUNKS 256 // Should be close to `Configuration.maximumIterations`

const int RGB_BitDepth = 3;

/*
 * Poor man's `Tuple<float, float>`
 */
typedef struct fpair {
	float x, y;
} fpair;

/*
 * Poor man's `System.Drawing.Rectangle`
 * 
 * 	Structure:
 * 	
 * 	1. `Configuration.isPOintWidth == true`:
 * 	
 * 		(We assume the Argand (complex) plane as the coordinate space below)
 * 		region{ start{ x, y }, end { x, y } ] = 
 * 		{ {	real coordinate of central point (origin), imaginary coordinate of central point (origin) },
 * 		  {	expanse of the image along the real line (X-axis), expanse of the image along the imaginary line (Y-axis) }	}
 * 
 * 		1. `Configuration.isPOintWidth == false`:
 * 	
 * 		(We assume the Argand (complex) plane as the coordinate space below)
 * 		region{ start{ x, y }, end { x, y } ] = 
 * 		{ {	left real extreme (starting real value), left imaginary extreme (starting imaginary value) },
 * 		  {	right real extreme (ending real value), right imaginary extreme (ending imaginary value) } }
 * 		  
 * See commented-out example zooms in `main`.
 */
typedef struct region {
	fpair start, end;
} region;

/*
 * Poor man's nothing, however, specifies the options we have when rendering -
 * 
 * `SMOOTH` - which applies linear interpolation and smoothing to get a nice image, but is "medium" or "slow" speed, and,
 * `NO_SMOOTH` - which simply maps the iteration count to a color directly, but is fast(er).
 */
typedef enum ColorMode
{
	SMOOTH, NO_SMOOTH
} ColorMode;

typedef struct Configuration {
    /* Image Specification */
	// Screen-space starting X coordinate of section to render - [0, width)
	int startX;
	// Screen-space starting Y coordinate of section to render - [0, height)
	int startY;
	// Screen-space width of section to render
    int width;
	// Screen-space height of section to render
    int height;
    /* Fractal Specification */
	// Contains the region of the complex plane to render. See comment on definition.
	region bounds;
	// Indicates whether `bounds` above denotes a rectangle in the complex plane indicating the region to be rendered,
	// or whether it holds details about a particular central point and how mcuh to render around it.
	// The latter is useful for zooming.
	bool isPointWidth;
	// Obvious Mandelbrot stuff	-
	// Do not make negative or 0!
    int maximumIterations;
	float bailout;
	// SMOOTH = Slow, NO_SMOOTH = fast. See comment on definition.
	ColorMode mode;
	/*Image specifications*/
	// Implements normalized to palette index scaling
	// `gradientScale` being `NAN` is a signal that it should be automatically calculated - leave at NAN.
	// Do not make negative or 0!
	float gradientScale;
	// Implements primary palette offset - leave at 0.
	// Do not make negative!
	float gradientShift;
	// Implements palette-rolling (color repetition) - 
	// Increase at higher iteration counts to get more variety in the image, leave at 1 otherwise.
	// Do not make negative or 0!
	float indexScale;
	// Scales the significance of the fraction in the normalized iteration count, leave at 1.
	// Do not make negative or 0!
	float fractionWeight;
	// Lower cutoff iteration value - increase for bettwer detail in deep zooms (>10^4x) close to the set's boundary. Leave at 1 otherwise.
	// Do not make negative or 0!
	int minimumIterations;
} Configuration;

/**
 * A mathematical error tolerance limit - an `epsilon` value.
 * This is about the safe minimum for `float`s.
 * Lower epsilon => more precision.
 * The value used below is the maximum precision of a `float` (32-bit binary floating point) -
 * don't decrease this beyond 10^-8, or increase it beyond 10^-5,
 * rendering will be either very much errorneous in the 1st case and might not work at all in the 2nd (even if it does, you stand to lose detail).
 */
float tolerance = 1e-7;

void convertPointWidthToBounds(Configuration* configuration);