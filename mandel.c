// MSVC complains about non- *_s versions of I/O functions being deprecated if I don't define this - also see `palette.c`
// Doesn't interfere with compilation on POSIX systems (tested with GCC)
#define _CRT_SECURE_NO_WARNINGS 1

// Use double- or quad-precision in calculations instead of single precision
#define EXTREMELY_HIGH_PRECISION

// Don't trust the user
//#define CHECKS

// Benchmark mode
#define BENCHMARK

#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <time.h>

#include "palette.h"
#include "imageOutput.h"

#define MIN(a,b) (((a) < (b)) ? (a) : (b))

/*
* source (input format)  = {{centreX, centreY}, {radiusX, radiusY}}
* range  (output format) = {{xMin, yMin}, {xMax, yMax}}
*/
void convertPointWidthToBounds(Configuration* configuration) {
	if (configuration->bounds.isPointWidth)
	{
		fp radiusX = configuration->bounds.end.x;
		fp radiusY = configuration->bounds.end.y;
		fp centreX = configuration->bounds.start.x, centreY = configuration->bounds.start.y;
		fpair start = { centreX - radiusX, centreY - radiusY },
			end = { centreX + radiusX, centreY + radiusY };
		region bounds = {
			bounds.start = start,
			bounds.end = end,
			bounds.isPointWidth = false
		};
		configuration->bounds = bounds;
	}
}

static inline int doMandelbrotIteration(fp cr, fp ci, fp* modulusSquaredOut, fp tolerance, fp bailoutSquared, int maximumIterations, int periodicityCheckIterations)
{
	// Initialize orbit parameters of current point
	fp zrCurrent = 0.0f, ziCurrent = 0.0f, zrPrevious = 0.0f, ziPrevious = 0.0f;
	fp modulusSquared = 0.0f;
	int iterations = 0;
	bool alwaysDoPeriodicityCheck = periodicityCheckIterations == 1,
		neverDoPeriodicityCheck = periodicityCheckIterations == maximumIterations;
	// Execute the iterative Mandelbrot formula
	while (modulusSquared < bailoutSquared && iterations < maximumIterations) {
		// Calculate Z_(n+1) = Z_n ^ 2 + C,
		// where Z_(n+1) = (zrNext, ziNext), 
		//       Z_n     = (zrCurrent, zrNext), and
		//       Z_(n-1) = (zrPrevious, ziPrevious)
		fp zrNext = zrCurrent * zrCurrent - ziCurrent * ziCurrent + cr;
		fp ziNext = 2 * zrCurrent * ziCurrent + ci;

		if (alwaysDoPeriodicityCheck ||
			((!neverDoPeriodicityCheck) &&
			(iterations > 0) &&
				((iterations % periodicityCheckIterations) == 0))) {
			// Calculates coordinate differences between
			fp  drPrevious = zrNext - zrPrevious, diPrevious = ziNext - ziPrevious;
			// Periodicity checking - speeds up the code.
			if ((drPrevious*drPrevious + diPrevious*diPrevious) < tolerance)
			{
				// The orbit has repeated points - we can expect an infinite cycle, so break out of the loop. Also see above comment.
				iterations = maximumIterations;
				break;
			}
			// Update values
			zrPrevious = zrCurrent;
			ziPrevious = ziCurrent;
		}
		zrCurrent = zrNext;
		ziCurrent = ziNext;
		// Calculate and update the modulus of (now) Z_n
		modulusSquared = zrCurrent * zrCurrent + ziCurrent * ziCurrent;
		// Advance the iteration count
		++iterations;
	}
	*modulusSquaredOut = modulusSquared;
	return iterations;
}

static inline rgb pixelColor(const int iterations, const int maximumIterations,
	const fp indexScale, const fp indexWeight, const fp modulusSquared, const fp halfOverLogBailout,
	const fp logMinIterations, const fp oneOverLogBase,
	const fp gradientScale, const int gradientShift,
	const int paletteSize, const rgb* palette,
	const ColorMode mode, const fp oneOverLog2)
{
	rgb color;
	// Baseline values - reduces branching
	fp index = (fp)iterations;
	fp bias = 0.0f;
	if (mode == SMOOTH && iterations < maximumIterations)
	{
		// Calculate the normalized iteration count, in the range ~ (indexScale * [iteration, iteration + 1]).
		index = iterations + 1 - indexWeight * (log(log(modulusSquared) * halfOverLogBailout) * oneOverLog2);

		// Unecessary given prior checks

#ifdef CHECKS
		// Check for and fix any calculation errors
		index = isnan(index) ? 0.0f : (isinf(index) ? maximumIterations : index);
#endif
		// Retrieve fractional part
		bias = index - (long)index;
	}
	// Bring the normalized iteration count into the range [0, indexScale] from [0, 1] (apply palette roll),
	// applying logarithmic indexing to allow an even distribution of colors.
	fp scaledIterations = (log(indexScale * index) - logMinIterations) * oneOverLogBase;
	// Truncate and transform it into a real palette index, taking care of bounds checking
	int actualIndex = (int)(scaledIterations * gradientScale + gradientShift);
	// Rely on branch prediction - the below conditions should be rare on recommended configurations (about 1 in 5 chance of branch mispredict?)
	// Conditions are in order of most commonly expected to least commonly expected
	if (actualIndex == paletteSize)
	{
		actualIndex = 0;
	}
	else if (actualIndex > paletteSize)
	{
		actualIndex %= paletteSize;
	}
	// The option below is not necessary for positive values of the configuration parameters.
#ifdef CHECKS
	else if (actualIndex < 0) {
		// Palette rollover for negative indices
		actualIndex = paletteSize + (actualIndex % paletteSize);
	}
#endif
	if (iterations >= maximumIterations)
	{
		// Fixed color for points inside the set - My supplied palette ensures that this is black.
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
			// lerp (linearly interpolate) the above 2 colors (fromColor and toColor) to get a gradient
			color.r = (channel)(fromColor.r + (toColor.r - fromColor.r) * bias);
			color.g = (channel)(fromColor.g + (toColor.g - fromColor.g) * bias);
			color.b = (channel)(fromColor.b + (toColor.b - fromColor.b) * bias);
		}
	}
	return color;
}

#define oneOver16 (0.0625f)
void basicMandelbrot(channel* image, Configuration* configuration, const rgb* palette, const int paletteSize)
{
	// Allow ease of setting up zooming
	convertPointWidthToBounds(configuration);

	// We alias some values to avoid dereferencing pointers in a tight loop
	fp realStart = configuration->bounds.start.x,
		imaginaryStart = configuration->bounds.start.y,
		imaginaryEnd = configuration->bounds.end.y;

#ifdef CHECKS
	// Don't trust the user - If you do trust the user, feel free to drop the safeguards and speed up the code by some amount
	int startX = clampAbove(configuration->startX, 0),
		startY = clampAbove(configuration->startY, 0);

	int width = clampAbove(configuration->width, 0),
		height = clampAbove(configuration->height, 0);

	fp gradientScale = isnan(configuration->gradientScale) ? paletteSize - 1 : clampAbove(configuration->gradientScale, 1.0f);
	int gradientShift = clampAbove(configuration->gradientShift, 0);

	fp bailout = clampAbove(configuration->bailout, 0.0f);

	// These checks avoid NANs when calculating logs if the intermediates go negative
	fp indexScale = clampAbove(configuration->indexScale, 0.0f),
		indexWeight = clampAbove(configuration->fractionWeight, 0.0f);

	int maximumIterations = clampAbove(configuration->maximumIterations, 1);
	fp minimumIterations = (fp)clamp(configuration->minimumIterations, 1, maximumIterations);

	fp toleranceFactor = clamp(configuration->toleranceFactor, 0.0f, 1.0f);

	int periodicityCheckIterations = clamp(configuration->periodicityCheckIterations, 1, maximumIterations);
#else
	// Trust the user?
	int startX = configuration->startX,
		startY = configuration->startY;

	int width = configuration->width,
		height = configuration->height;

	fp gradientScale = isnan(configuration->gradientScale) ? paletteSize - 1 : configuration->gradientScale;
	int gradientShift = configuration->gradientShift;

	fp bailout = configuration->bailout;

	fp indexScale = configuration->indexScale,
		indexWeight = configuration->fractionWeight;

	int maximumIterations = configuration->maximumIterations;
	fp minimumIterations = (fp)configuration->minimumIterations;

	fp toleranceFactor = configuration->toleranceFactor;	   

	int periodicityCheckIterations = configuration->periodicityCheckIterations;
#endif

	// Values which enable a scaling transform from screen-space coordinates to complex plane coordinates
	fp xScale = fabs(configuration->bounds.end.x - realStart) / width;
	fp yScale = fabs(imaginaryEnd - imaginaryStart) / height;

	ColorMode mode = configuration->mode;
	if (mode == NO_SMOOTH)
	{
		bailout = 4.0f;
	}
	// Some formula parameters we can factor out as constants
	fp bailoutSquared = bailout * bailout;
	fp logMinIterations = log(minimumIterations);
	fp oneOverLogBase = 1.0f / (log((fp)maximumIterations) - logMinIterations);
	fp halfOverLogBailout = 0.5f / log(bailout);
	fp oneOverLog2 = (1.0f / log(2.0f));

	// Fraction-of-a-pixel tolerance is agreeably quite nice both time and quality-wise
	fp tolerance = toleranceFactor * MIN(fabs(xScale), fabs(yScale));
	int y;
	// Concurrency!
#pragma omp parallel for schedule(dynamic, CHUNKS) num_threads(THREADS)
	// Maintain row-major order traversal to improve cache locality
	// Iterate over rows while calculating the complex plane coordinates of our current screen-space location as necessary
	for (y = startY; y < height; ++y)
	{
		// Cannot factor this one out thanks to OpenMP making the above loop non-squential
		fp ci = imaginaryEnd - y * yScale;
		fp cr = realStart;
		// Iterate over columns
		for (int x = startX; x < width; ++x, cr += xScale)
		{
			fp modulusSquared = 0.0f;
			fp xPlus1 = cr + 1.0f, xMinusQuarter = cr - 0.25f, ySquared = ci * ci;
			fp q = xMinusQuarter*xMinusQuarter + ySquared;
			rgb color = palette[0];
			// Preemptively avoid iterating points knwon to be inside the set (the main cardioid and the period-2 bulb)
			if (((xPlus1 * xPlus1 + ySquared) >= oneOver16) &&
				((q * (q + xMinusQuarter)) >= (0.25f * ySquared))) {
				int iterations = doMandelbrotIteration(
					cr, ci,
					&modulusSquared,
					tolerance, bailoutSquared,
					maximumIterations, periodicityCheckIterations);
				color = pixelColor(
					iterations, maximumIterations,
					indexScale, indexWeight,
					modulusSquared, halfOverLogBailout,
					logMinIterations, oneOverLogBase,
					gradientScale, gradientShift,
					paletteSize, palette,
					mode, oneOverLog2);
			}

			// Plot the pixel - linearize (map from 2D to 1D) the index into the image
			int offset = RGB_BitDepth * (y * width + x);

#ifdef PPM_OUTPUT
			// PPM Output requires RGB order
			image[offset] = color.r;
			image[offset + 1] = color.g;
			image[offset + 2] = color.b;
#else
			// TGA Output requires BGR order
			image[offset] = color.b;
			image[offset + 1] = color.g;
			image[offset + 2] = color.r;
#endif			
		}
	}
}

void basicMandelbrotSequence(const size_t count, channel** images, Configuration** configurations, const rgb** palettes, const int* paletteSizes)
{
	double totalTimeInMilliseconds = 0.0;
	for (size_t index = 0; index < count; ++index)
	{
		images[index] = calloc(configurations[index]->width * configurations[index]->height * RGB_BitDepth, sizeof(channel));
		if (images[index] == NULL)
		{
			printf(
				"Allocating memory for image buffer failed - is there enough free memory for the requested size (%d, %d)?\n",
				configurations[index]->width, configurations[index]->height);
			continue;
		}
#ifdef BENCHMARK
		clock_t start = clock();
		basicMandelbrot(images[index], configurations[index], palettes[index], paletteSizes[index]);
		clock_t end = clock();
		double elapsedTimeInMilliSeconds = 1e3 * (end - start) / CLOCKS_PER_SEC;
		totalTimeInMilliseconds += elapsedTimeInMilliSeconds;
		printf("Time taken for execution of render number %zu was %lf ms\n", index, elapsedTimeInMilliSeconds);
#else
		basicMandelbrot(images[index], configurations[index], palettes[index], paletteSizes[index]);
#endif
	}
#ifdef BENCHMARK

	printf("Total time taken for execution of all renders was %lf ms\n", totalTimeInMilliseconds);
#endif
}

void basicMandelbrotSequenceSinglePalette(const size_t count, channel** images, Configuration** configurations, const rgb* palette, const int paletteSize)
{
	double totalTimeInMilliseconds = 0.0;
	for (size_t index = 0; index < count; ++index)
	{
		// Doing it the Copy-And-Paste way for efficiency reasons
		images[index] = calloc(configurations[index]->width * configurations[index]->height * RGB_BitDepth, sizeof(channel));
		if (images[index] == NULL)
		{
			printf(
				"Allocating memory for image buffer failed - is there enough free memory for the requested size (%d, %d)?\n",
				configurations[index]->width, configurations[index]->height);
			continue;
		}
#ifdef BENCHMARK
		clock_t start = clock();
		basicMandelbrot(images[index], configurations[index], palette, paletteSize);
		clock_t end = clock();
		double elapsedTimeInMilliSeconds = 1e3 * (end - start) / CLOCKS_PER_SEC;
		totalTimeInMilliseconds += elapsedTimeInMilliSeconds;
		printf("Time taken for execution of render number %zu was %lf ms\n", index, elapsedTimeInMilliSeconds);
#else
		basicMandelbrot(images[index], configurations[index], palette, paletteSize);
#endif
	}
#ifdef BENCHMARK

	printf("Total time taken for execution of all renders was %lf ms\n", totalTimeInMilliseconds);
#endif
}

char** generateSequentialFileNames(int count)
{
	char directoryName[] = "./";
	char baseName[] = "image";
#ifdef PPM_OUTPUT
	char suffix[] = ".ppm";
#else
	char suffix[] = ".tga";
#endif
	char** fileNames = calloc(count, FILE_NAME_MAX_LENGTH * sizeof(char));
	if (fileNames != NULL)
	{
		for (int i = 0; i < count; ++i)
		{
			char* fileName = calloc(FILE_NAME_MAX_LENGTH, sizeof(char));
			if (fileName != NULL)
			{
				if (snprintf(fileName, FILE_NAME_MAX_LENGTH, "%s%s%0d%s", directoryName, baseName, i, suffix) <= 0)
				{
					continue;
				}
				fileNames[i] = fileName;
			}
		}
	}
	return fileNames;
}

static inline fp calculateWidthGivenRadiusAsHeight(const fp radiusAsHeight, const int height, const int width)
{
	return ((fp)width * radiusAsHeight) / height;
}

fpair convertScreenSpaceCoordinatesToComplexSpace(const int x, const int y, const fpair complexCentre, const fp radius, const int width, const int height)
{
	fpair newCoordinates;
	fp radiusY = radius, radiusX = calculateWidthGivenRadiusAsHeight(radiusY, height, width);
	fp xScale = (2 * radiusX) / width, yScale = (2 * radiusY) / height;
	newCoordinates.x = x * xScale + (complexCentre.x - radiusX);
	newCoordinates.y = (complexCentre.y + radiusY) - y * yScale;
	return newCoordinates;
}

#define CONFIG_PARAMETER_COUNT 12
Configuration** readConfigurationsFromFile(FILE* configurationFile, int* configurationsCount)
{
	if (configurationFile == NULL)
	{
		return NULL;
	}
	uint32_t numberOfConfigurations = 0;
	if (!fscanf(configurationFile, "%u", &numberOfConfigurations)) {
		printf("Configuration file is not in the correct format.\n");
	}
	*configurationsCount = (int)numberOfConfigurations;
	Configuration** configurations = calloc(numberOfConfigurations, sizeof(Configuration*));
	if (configurations != NULL)
	{
		for (uint32_t i = 0; i < numberOfConfigurations; ++i)
		{
			Configuration* configuration = malloc(sizeof(Configuration));
			fp realCentre = 0.0f, imaginaryCentre = 0.0f, radius = 0.0f;
			int mode = 0;
			if (fscanf(configurationFile,
#ifdef HIGH_PRECISION
				"%d:%d,%d,{%lf:%lf:%lf},%lf:%d:%lf:%d,%d,%lf",
#elif defined(EXTREMELY_HIGH_PRECISION)
				"%d:%d,%d,{%Lf:%Lf:%Lf},%Lf:%d:%Lf:%d,%d,%Lf",
#else
				"%d:%d,%d,{%f:%f:%f},%f:%d:%f:%d,%d,%f",
#endif
				&configuration->width, &configuration->height, &configuration->maximumIterations,
				&realCentre, &imaginaryCentre, &radius,
				&configuration->indexScale, &configuration->minimumIterations,
				&configuration->bailout, &configuration->periodicityCheckIterations, &mode, &configuration->toleranceFactor)
				!= CONFIG_PARAMETER_COUNT)
			{
				// Unexpected EOF - handle it nicely!
				// Set the palette size to wherever we had to stop
				*configurationsCount = i;
				// Ensure that we don't leak memory by using more than what we need
				configurations = realloc(configurations, (*configurationsCount * sizeof(Configuration*)));
				// Exit	loop
				break;
			}
			fp height = radius,
				width = calculateWidthGivenRadiusAsHeight(height, configuration->height, configuration->width);
			fpair centre = { realCentre, imaginaryCentre }, expanse = { width, height };
			region bounds = { centre, expanse, true };
			configuration->bounds = bounds;
			configuration->startX = 0;
			configuration->startY = 0;
			configuration->fractionWeight = 1.0f;
			configuration->gradientScale = NAN;
			configuration->gradientShift = 0;
			configuration->mode = (ColorMode)mode;
			configurations[i] = configuration;
		}
	}
	return configurations;
}

static inline void freeAll(void** items, size_t count)
{
	for (size_t i = 0; i < count; ++i)
	{
		free(items[i]);
	}
	free(items);
}

int main(const int argc, const char** argv) {
	if (argc <= 1) {
		printf("No configuration specified; Usage: ./mandel <path-to-configuration-file> [path-to-palette-file]\n");
		return EXIT_FAILURE;
	}
	// Load configuration
	FILE* configurationFile = fopen(argv[1], "r");
	if (configurationFile == NULL)
	{
		printf("Configuration File does not exist or is not accessible.\n");
		return EXIT_FAILURE;
	}
	int configurationCount = 0;
	Configuration** configurations = readConfigurationsFromFile(configurationFile, &configurationCount);
	if (configurations == NULL)
	{
		printf("Configurations could not be read from file. Is it in the documented format?\n");
		return EXIT_FAILURE;
	}

	fclose(configurationFile);

	// Load palette
	const char* paletteFileName = argc > 2 ? argv[2] : "./palette.pal";
	FILE* paletteFile = fopen(paletteFileName, "rb");
	int paletteSize = 0;
	rgb* palette = loadPalette(paletteFile, &paletteSize);
	if (palette == NULL)
	{
		printf("Reading palette failed - Does `palette.pal` exist in the current directory, and is it in the documented format?\n");
		return EXIT_FAILURE;
	}
	fclose(paletteFile);

	// Create buffer for storing rendered images
	channel** images = calloc(configurationCount, sizeof(channel*));
	if (images == NULL)
	{
		printf("Allocating buffer for holding a sequence of %d images failed.\n",
			configurationCount);
		return EXIT_FAILURE;
	}

	// Render
	basicMandelbrotSequenceSinglePalette(configurationCount, images, configurations, palette, paletteSize);
	free(palette);

	// Generate sequential output file names
	char** outputFileNames = generateSequentialFileNames(configurationCount);

	// Write output
	writeOutputSequenceWithConfiguration(configurationCount, outputFileNames, images, configurations);

	// Cleanup
	freeAll((void**)outputFileNames, configurationCount);
	freeAll((void**)images, configurationCount);
}