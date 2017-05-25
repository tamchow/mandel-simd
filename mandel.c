#define _CRT_SECURE_NO_WARNINGS 1

#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <time.h>
#include "color.h"
#include "palette.h"

#include "mandel.h"

//#define DEBUG 1

static const int MaxExtraIterations = 3;

/*
* source = {{centreX, centreY}, {width, height}}
* range = {{xMin, yMin}, {xMax, yMax}}
*/
void convertPointWidthToBounds(Configuration* configuration) {
	if (configuration->isPointWidth)
	{
		float halfWidth = configuration->bounds.end.x * 0.5f;
		float halfHeight = configuration->bounds.end.y * 0.5f;
		float centreX = configuration->bounds.start.x, centreY = configuration->bounds.start.y;
		fpair start = { centreX - halfWidth, centreY - halfHeight },
			end = { centreX + halfWidth, centreY + halfHeight };
		region bounds = {
			bounds.start = start,
			bounds.end = end
		};
		configuration->bounds = bounds;
		configuration->isPointWidth = false;
	}
}

void basicMandelbrot(channel* image, Configuration* configuration, const rgb* palette, const int paletteSize)
{
	// Allow ease of setting up zooming
	convertPointWidthToBounds(configuration);

	// We alias some values to avoid dereferencing pointers in a tight loop
	int startX = configuration->startX,
		startY = configuration->startY;

	int width = configuration->width,
		height = configuration->height;

	float realStart = configuration->bounds.start.x,
		imaginaryStart = configuration->bounds.start.y;

	float gradientScale = (isnan(configuration->gradientScale)) ? paletteSize - 1 : configuration->gradientScale;
	float gradientShift = configuration->gradientShift;

	float bailout = configuration->bailout;

	float indexScale = configuration->indexScale,
		indexWeight = configuration->fractionWeight;

	// Values which enable a scaling transform from screen-space coordinates to complex plane coordinates
	float xScale = fabsf(configuration->bounds.end.x - realStart) / width;
	float yScale = -fabsf(configuration->bounds.end.y - imaginaryStart) / height;

	// Don't trust the user - If you do trust the user, feel free to drop the safeguards and speed up the code by 2-3ms
	int maximumIterations = fabsf(configuration->maximumIterations);
	configuration->minimumIterations = fabsf(configuration->minimumIterations);
	float minimumIterations = (configuration->minimumIterations < maximumIterations) ? configuration->minimumIterations : maximumIterations;

	// Some formula parameters we can factor out as constants
	float oneOverLog2 = 1.0f / logf(2.0f);
	float bailoutSquared = bailout * bailout;
	float logMinIterations = logf(minimumIterations);
	float oneOverLogBase = 1.0f / (logf((float)maximumIterations) - logMinIterations);
	float halfOverLogBailout = 0.5f / logf(bailout);

	ColorMode mode = configuration->mode;
	int y;

	// Concurrency!
#pragma omp parallel for schedule(dynamic, CHUNKS) num_threads(THREADS)
	for (y = startY; y < height; ++y)
	{
		for (int x = startX; x < width; ++x)
		{
			// Calculate the complex plane coordinates of our current screen-space location
			float cr = x * xScale + realStart;
			float ci = y * yScale - imaginaryStart;
			float zrCurrent = 0.0f, ziCurrent = 0.0f, zrPrevious = 0.0f, ziPrevious = 0.0f;
			float modulusSquared = 0.0f;
			int iterations = 0;
			while (modulusSquared < bailoutSquared && iterations < maximumIterations) {
				// Calculate Z_(n+1) = Z_n ^ 2 + C,
				// where Z_(n+1) = (zrNext, ziNext), 
				//       Z_n     = (zrCurrent, zrNext), and
				//       Z_(n-1) = (zrPrevious, ziPrevious)
				float zrNext = zrCurrent * zrCurrent - ziCurrent * ziCurrent + cr;
				float ziNext = 2 * zrCurrent * ziCurrent + ci;
				// Calculates coordinate differences between
				float drCurrent = zrNext - zrCurrent, drPrevious = zrNext - zrPrevious,
					diCurrent = ziNext - ziCurrent, diPrevious = ziNext - ziPrevious;
				// Periodicity checking - speeds up the code.
				//
				// Checks if the distance between Z_(n+1) and Z_n or Z_(n+1) and Z_(n-1) is below an epsilon value, 
				// which indicates that	we're stuck in an infinite loop, and that the point under consideration currently is part of the set.
				if (((drCurrent*drCurrent + diCurrent*diCurrent) < tolerance) ||
					((drPrevious*drPrevious + diPrevious*diPrevious) < tolerance))
				{
					// The orbit has repeated points - we can expect an infinite cycle, so break out of the loop. Also see above comment.
					iterations = maximumIterations;
					break;
				}
				// Update values
				zrPrevious = zrCurrent;
				ziPrevious = ziCurrent;
				zrCurrent = zrNext;
				ziCurrent = ziNext;
				// Calculate and update the modulus of (now) Z_n
				modulusSquared = zrCurrent * zrCurrent + ziCurrent * ziCurrent;
				// Advance the iteration count
				++iterations;
			}
			rgb color;
			// Baseline values - reduces branching
			float index = (float)iterations;
			float bias = 0.0f;
			if (mode == SMOOTH && iterations < maximumIterations)
			{
				// Calculate the normalized iteration count
				index = iterations + 1 - indexWeight * logf(logf(modulusSquared) * halfOverLogBailout) * oneOverLog2;
				// Check for and fix any calculation errors
				index = isnan(index) ? 0.0f : (isinf(index) ? maximumIterations : index);
				// Retrieve fractional part
				bias = index - (long)index;
			}
			// Bring the normalized iteration count into the range [0, indexScale] from [0, 1]
			float scaledIterations = indexScale * (logf(index) - logMinIterations) * oneOverLogBase;
			// Truncate and apply palette roll, and transform it into a real palette index, taking care of bounds checking
			int actualIndex = (int)(scaledIterations * gradientScale + gradientShift) % paletteSize;
			if (iterations >= maximumIterations)
			{
				// Fixed color for points inside the set - My palette ensures that this is black.
				color = palette[0];
			}
			else
			{
				// Avoid a modulo when lerping - branch prediction will help us lose less time here
				// This saves about 40ms over a modulo when calculating `toColor`
				actualIndex = (actualIndex == paletteSize - 1) ? -1 : actualIndex;

				// We have our (one of) color(s)!
				rgb fromColor = palette[actualIndex];
				// Skip the modulo and lerp if unnecessary
				if (mode == NO_SMOOTH || bias == 0.0f) {
					color = fromColor;
				}
				else
				{
					rgb toColor = palette[actualIndex + 1];
					// lerp (linearly interpolate) the above 2 colors (fromColor and toColor) if necessary
					color.r = (channel)(fromColor.r + (toColor.r - fromColor.r) * bias);
					color.g = (channel)(fromColor.g + (toColor.g - fromColor.g) * bias);
					color.b = (channel)(fromColor.b + (toColor.b - fromColor.b) * bias);
				}
			}
			// Plot the pixel - linearize (map from 2D to 1D) the index into the image
			int offset = RGB_BitDepth * (y * width + x);
			// R-G-B, in that order
			image[offset] = color.r;
			image[offset + 1] = color.g;
			image[offset + 2] = color.b;
		}
	}
}

int main(const int argc, const char* argv[])
{
	/*
	 * I'm sorr I removed `getopt` - please re-add it as necessary, too much to comfigure below.
	 * See the C# version's argument processing for some clues as to what's important
	 */
	 /* Config */
	 // Deep Zoom 1 - doesn't work
 //	fpair startPoint = { 0.27969303810093984, 0.00838423653868096 },
 //		endPoint = { 3.2768e-12, 3.2768e-12 };
	 // Zoom 1 - works
 	fpair startPoint = { -0.1593247826659642, 1.0342115878556377 },
 		endPoint = { 0.0515625, 0.0515625 };
	 // Overall - Definitely works
//	fpair startPoint = { -2.5, -1 },
//		endPoint = { 1, 1 };
	region bounds = { startPoint, endPoint };
	Configuration configuration = {
		configuration.startX = 0,
		configuration.startY = 0,
		configuration.width = 3840,
		configuration.height = 3840,
		configuration.bounds = bounds,
		configuration.isPointWidth = true,
		configuration.maximumIterations = 1000,
		// Use about 1e9 for ColorMode == SMOOTH, 4.0 for ColorMode == NO_SMOOTH
		configuration.bailout = 1e9f,
		configuration.mode = SMOOTH,
		configuration.gradientScale = NAN,
		configuration.gradientShift = 0.0f,
		// Leave this at 1.0 unless really necessary - can potentially screw up smoothing
		configuration.fractionWeight = 1.0f,
		configuration.indexScale = 1.0f,
		// Leave this alone unless doing really deep zooms beyond 10^5x
		configuration.minimumIterations = 1
	};

	// Load the palette - Ensure that the relevant file is in the current directory and exists!
	FILE* paletteFile = fopen("./palette.txt", "r");

	int paletteSize = 0;
	rgb* palette = loadPalette(paletteFile, &paletteSize);
	if (palette == NULL)
	{
		fprintf(stderr, "Reading palette failed - Does `palette.txt` exist in the current directory?");
		return IOERR_NOFILE;
	}

	fclose(paletteFile);

	// Allocate memory for image
	channel* image = calloc(configuration.width * configuration.height * 3, sizeof(channel));

	// Start timer
	clock_t start = clock();
	// Do the render
	basicMandelbrot(image, &configuration, palette, paletteSize);
	// Stop timer
	clock_t end = clock();
	// Calculate time delta between starting and stopping of rendering
	float elapsedTimeInMilliSeconds = 1e3f * (end - start) / CLOCKS_PER_SEC;
	// Ouput the time delta
	printf("Time taken for execution = %f ms", elapsedTimeInMilliSeconds);
	// Clean up
	free(palette);

	// Write result	image
	FILE* output = fopen("./img.ppm", "wb");

	fprintf(output, "P6\n%d %d\n%d\n", configuration.width, configuration.height, 255);
	fwrite(image, sizeof(channel), configuration.width * configuration.height * RGB_BitDepth, output);
	fclose(output);
	// Clean up
	free(image);
	return 0;
}