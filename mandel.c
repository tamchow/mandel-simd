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
	convertPointWidthToBounds(configuration);

	float toleranceSquared = tolerance * tolerance;
	//float addend = ((configuration->mode == SMOOTH) ? MaxExtraIterations : 0);

	int startX = configuration->startX, startY = configuration->startY;
	int width = configuration->width, height = configuration->height;
	float realStart = configuration->bounds.start.x, imaginaryStart = configuration->bounds.start.y;
	float xscale = fabsf(configuration->bounds.end.x - realStart) / width;
	float yscale = -fabsf(configuration->bounds.end.y - imaginaryStart) / height;
	float gradientScale = (isnan(configuration->gradientScale)) ? paletteSize - 1 : configuration->gradientScale;
	float gradientShift = configuration->gradientShift;
	float bailout = configuration->bailout;
	float indexScale = configuration->indexScale, indexWeight = configuration->fractionWeight;
	int maxIterations = configuration->iterations;
	float oneOverLog2 = 1.0f / logf(2.0f);
	float bailoutSquared = bailout * bailout;

	float logMinIterations = logf(configuration->minimumIterations);
	float oneOverLogBase = 1.0f / (logf(maxIterations) - logMinIterations);
	float halfOverLogBailout = 0.5 / logf(bailout);
	ColorMode mode = configuration->mode;
	int y;
	#pragma omp parallel for schedule(dynamic, CHUNKS) num_threads(THREADS)
	for (y = startY; y < height; ++y)
	{
		for (int x = startX; x < width; ++x)
		{
			float cr = x * xscale + realStart;
			float ci = y * yscale - imaginaryStart;
			float zrCurrent = 0.0f, ziCurrent = 0.0, zrPrevious = 0, ziPrevious = ziCurrent;
			float modulusSquared = 0.0f;
			int iterations = 0;
			//float expIter = 0.0f;
			while (modulusSquared < bailoutSquared && iterations < maxIterations) {
				float zrNext = zrCurrent * zrCurrent - ziCurrent * ziCurrent + cr;
				float ziNext = 2 * zrCurrent * ziCurrent + ci;
				float drCurrent = zrNext - zrCurrent, drPrevious = zrNext - zrPrevious,
					diCurrent = ziNext - ziCurrent, diPrevious = ziNext - ziPrevious;
				//periodicity checking - can speed up the code
				if (((drCurrent*drCurrent + diCurrent*diCurrent) < toleranceSquared) ||
					((drPrevious*drPrevious + diPrevious*diPrevious) < toleranceSquared))
				{
					iterations = maxIterations;
					break;
				}
				zrPrevious = zrCurrent;
				ziPrevious = ziCurrent;
				zrCurrent = zrNext;
				ziCurrent = ziNext;
				modulusSquared = zrCurrent * zrCurrent + ziCurrent * ziCurrent;
				//expIter += exp(-sqrt(modulusSquared));
				++iterations;
			}
			//			if (mode == SMOOTH) {
			//				// Run a few more iterations to reduce error in fractional iteration count
			//				for (int totalIterations = iterations + MaxExtraIterations; iterations < totalIterations; ++iterations)
			//				{
			//					float zrNext = zrCurrent * zrCurrent - ziCurrent * ziCurrent + cr;
			//					float ziNext = 2 * zrCurrent * ziCurrent + ci;
			//					zrCurrent = zrNext;
			//					ziCurrent = ziNext;
			//					modulusSquared = zrCurrent * zrCurrent + ziCurrent * ziCurrent;
			//				}
			//			}
			rgb color;
			float index = iterations;
			float bias = 0.0f;
			if (mode == SMOOTH && iterations < maxIterations)
			{
				float smoothed = logf(logf(modulusSquared) * halfOverLogBailout) * oneOverLog2;
				float temporaryIndex = iterations + 1 - indexWeight * smoothed;
				temporaryIndex = isnan(temporaryIndex) ? 0.0f : (isinf(temporaryIndex) ? maxIterations : temporaryIndex);
				bias = temporaryIndex - (long)temporaryIndex; // Retrieve fractional part
				index = (float)(long)temporaryIndex; // Truncation
			}
			//float scaledIterations = ((indexScale * index) - scaledMinIterations) / (scaledMaxIterations - scaledMinIterations);
			float scaledIterations = indexScale * (logf(index) - logMinIterations) * oneOverLogBase;
			int actualIndex = (int)(scaledIterations * gradientScale + gradientShift) % paletteSize;
			if (iterations >= maxIterations)
			{
				color = palette[0];
			}
			else
			{
				rgb fromColor = palette[actualIndex];
				if (mode == NO_SMOOTH || bias == 0.0) {
					color = fromColor;
				}
				else
				{
					rgb toColor = palette[(actualIndex + 1) % paletteSize];
					// lerp the colors if necessary
					color.r = (channel)(toColor.r + (fromColor.r - toColor.r) * bias);
					color.g = (channel)(toColor.g + (fromColor.g - toColor.g) * bias);
					color.b = (channel)(toColor.b + (fromColor.b - toColor.b) * bias);
				}
			}
			int offset = RGB_BitDepth * (y * width + x);
			image[offset] = color.r;
			image[offset + 1] = color.g;
			image[offset + 2] = color.b;
		}
	}
}

int main(const int argc, const char* argv[])
{
	/* Config */
	// Deep Zoom 1 - doesn't work
//	fpair startPoint = { 0.27969303810093984, 0.00838423653868096 },
//		endPoint = { 3.2768e-12, 3.2768e-12 };
	// Zoom 1 - works
//	fpair startPoint = { -0.1593247826659642, 1.0342115878556377 },
//		endPoint = { 0.0515625, 0.0515625 };
	// Overall - Definitely works
	fpair startPoint = { -2.5, -1 },
		endPoint = { 1, 1 };
	region bounds = { startPoint, endPoint };
	Configuration spec = {
		spec.startX = 0,
		spec.startY = 0,
		spec.width = 3840,
		spec.height = 2160,
		spec.bounds = bounds,
		spec.isPointWidth = false,
		spec.iterations = 256,
		spec.bailout = 4.0f,
		spec.mode = NO_SMOOTH,
		spec.gradientScale = NAN,
		spec.gradientShift = 0.0f,
		spec.fractionWeight = 1.0f,
		spec.indexScale = 1.0f,
		spec.minimumIterations = 1
	};

	int paletteSize = 769;
	rgb* palette = calloc(paletteSize, sizeof(rgb));

	FILE* paletteFile = fopen("./palette.txt", "r");

	loadPalette(paletteFile, palette, paletteSize);

	fclose(paletteFile);
	/* Render */
	channel* image = calloc(spec.width * spec.height * 3, sizeof(channel));

	clock_t start = clock();
	basicMandelbrot(image, &spec, palette, paletteSize);
	clock_t end = clock();
	float elapsedTimeInMilliSeconds = 1e3 * (end - start) / CLOCKS_PER_SEC;
	printf("Time taken for execution = %f ms", elapsedTimeInMilliSeconds);
	free(palette);

	FILE* output = fopen("./img.ppm", "wb");
	/* Write result */
	fprintf(output, "P6\n%d %d\n%d\n", spec.width, spec.height, 255);
	fwrite(image, sizeof(channel), spec.width * spec.height * RGB_BitDepth, output);
	fclose(output);
	free(image);
	getchar();
	return 0;
}