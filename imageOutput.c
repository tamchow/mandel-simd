#define _CRT_SECURE_NO_WARNINGS 1

#include <stdlib.h>
#include "targa.h"
#include "imageOutput.h"

void writeOutput(FILE* outputFile, channel* image, const int width, const int height)
{
#ifdef PPM_OUTPUT
	fprintf(outputFile, "P6\n%d %d\n%d\n", width, height, 255);
	fwrite(image, sizeof(channel), width * height * RGB_BitDepth, outputFile);
#else
	tga_result result = tga_write_bgr_rle_FILE(outputFile, image, (const uint16_t)width, (const uint16_t)height, RGB_BitDepth * 8);
	if (result != TGA_NOERR)
	{
		printf("Error while writing output: %50s\n", tga_error(result));
	}
#endif
	fclose(outputFile);
}

void writeOutputWithConfiguration(FILE* outputFile, channel* image, const Configuration* configuration)
{
	writeOutput(outputFile, image, configuration->width, configuration->height);
}

void writeOutputSequence(const size_t count, const char** outputFileNames, channel** images, const int* widths, const int* heights)
{
	for (size_t index = 0; index < count; ++index)
	{
		FILE* outputFile = fopen(outputFileNames[index], "wb");
		if (outputFile == NULL)
		{
			printf("Error opening output file %35s\n", outputFileNames[index]);
			continue;
		}
		writeOutput(outputFile, images[index], widths[index], heights[index]);
	}
}

void writeOutputSequenceWithConfiguration(const size_t count, char** outputFileNames, channel** images, Configuration** configurations)
{
	for (size_t index = 0; index < count; ++index)
	{
		FILE* outputFile = fopen(outputFileNames[index],"wb");
		if (outputFile == NULL)
		{
			printf("Error opening output file %35s\n", outputFileNames[index]);
			continue;
		}
		writeOutputWithConfiguration(outputFile, images[index], configurations[index]);	
	}
}