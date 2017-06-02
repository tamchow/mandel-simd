#pragma once
#include <stdbool.h>

//#define PPM_OUTPUT

#include "imageOutput.h"

// OpenMP parameters, change for system

// The number of total system cores (hyperthreading inclusive)
#define THREADS  4
// Should be a power of 2, to facilitate loop unrolling by the compiler, and auto-vectorization
// Lower values generally work better (i.e., faster) - 1 is the minimum.
// Unfrtunately these need to be specified at compile time
#define  CHUNKS 1

#define FILE_NAME_MAX_LENGTH 35 // Assume max value for `count` will have no more than 9 digits

#ifdef EXTREMELY_HIGH_PRECISION
typedef long double fp;
#define log logl
#define fabs fabsl
#define sin sinl
#define cos cosl
#elif defined(HIGH_PRECISION)
typedef double fp;
#define log log
#define fabs fabs
#define sin sin
#define cos cos
#else
typedef float fp;
#define log logf
#define fabs fabsf
#define sin sinf
#define cos cosf
#endif


/*
 * Poor man's `Tuple<float, float>`
 */
typedef struct fpair {
	fp x, y;
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
	// Indicates whether this region denotes a rectangle in the complex plane indicating the region to be rendered,
	// or whether it holds details about a particular central point and how mcuh to render around it.
	// The latter is useful for zooming.
	bool isPointWidth;
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
	/*
	* The coordinates below are of `int`, not `uint32_t` or `size_t`, to reflect the limits of the PNG file format on file-size.
	* Why be concerned about PNG when we output to PPM?
	* Simple - no-one cares about the raw PPM files - they're converted to PNG for archival or export.
	* Note - export the PPM files to PNG or TIFF, not JPEG or GIF - as the latter have lossy compression and can reduce image detail.
	*/
	
	// Screen-space starting X coordinate of section to render
	int startX;
	// Screen-space starting Y coordinate of section to render
	int startY;
	// Screen-space width of section to render
    int width;
	// Screen-space height of section to render
    int height;
    
	/* Fractal Specification */
	
	// Contains the region of the complex plane to render. See comment on definition.
	region bounds;
	// The angle by which to rotate screen space coordinates during transformation to complex space
	fp angle;
	
	// Obvious Mandelbrot stuff
    int maximumIterations;
	// Lower values speed up execution (not by much) but reduce image quality - the value used below is an experimental optimum.
	// Increase the bailout for zooming in and `ColorMode == SMOOTH`, and possibly vice-versa.
	fp bailout;
	
	// SMOOTH = Slow, NO_SMOOTH = fast. See comment on definition.
	ColorMode mode;
	
	/*Image specifications*/
	
	// Implements normalized to palette index scaling
	// `gradientScale` being `NAN` is a signal that it should be automatically calculated - leave at NAN.
	fp gradientScale;
	// Implements primary palette offset - leave at 0.
	int gradientShift;
	
	// Implements palette-rolling (color repetition) - 
	// Increase at higher iteration counts to get more variety in the image, leave at 1 otherwise.
	//
	// Note: This also multiplies the error in the normalized iteration count itself, 
	// so beyond about 1.4, banding becomes noticeable but not horrible up until about 7.0
	fp indexScale;
	// Scales the significance of the fraction in the normalized iteration count, leave at 1.
	fp fractionWeight;
	
	// Lower cutoff iteration value - increase for bettwer detail in deep zooms (>10^4x) close to the set's boundary. Leave at 1 otherwise.
	int minimumIterations;
	// The number of iterations after which to do the periodicity check.
	// `maximumIterations/8` is suggested by the fast Mandelbrot renderer AlmondBread.
	int periodicityCheckIterations;
	// The fraction of a pixel's dimension to judge the same.
	fp toleranceFactor;
} Configuration;

/*
 * Clamping (value-restricting) functions,
 * Defined as a macro so it can be type-generic (work for both `int`s and `float`s, in our case).
 */
#define clamp(value, min, max) ( ((value) < (MIN)) ? (MIN) : ( ((value) > (max)) ? (max) : (value) ) )

#define clampAbove(value, min) ( ((value) < (MIN)) ? (MIN) : (value) )

fpair convertScreenSpaceCoordinatesToComplexSpace(
	const int x, const int y,
	const fpair complexCentre, const fp radius, const fp angle,
	const int startX, const int startY, 
	const int width, const int height);

void convertPointWidthToBounds(Configuration* configuration);

void basicMandelbrot(channel* image, Configuration* configuration, const rgb* palette, const int paletteSize);

void basicMandelbrotSequence(const size_t count, channel** images, Configuration** configurations, const rgb** palettes, const int* paletteSizes);

void basicMandelbrotSequenceSinglePalette(const size_t count, channel** images, Configuration** configurations, const rgb* palette, const int paletteSize);

char** generateSequentialFileNames(int count);

Configuration** readConfigurationsFromFile(FILE* configurationFile, int* configurationsCount);