#define _CRT_SECURE_NO_WARNINGS 1

#include <stdlib.h>
#include "palette.h"

/*
 * Read in a palette from a text file on disk, stored in the format exported by the C# version.
 * Essentially:
 * Line 1: Number of colors in palette - 1 (because the `MaxIterationColor` in C# is not part of it's palette but it's exported along with the palette)
 * Line 2 to Line <Number of colors in palette>: RGB triplets as unsigned byte literals separated by commas, i.e., <Red>,<Greem>,<Blue>
 */
rgb* loadPalette(FILE* input, int* paletteSize)
{
	if(!input)
	{
		// Illegal file pointer - potentially doesn't exist or is not readable
		return NULL;
	}
	int newPaletteSize = 0;
	fscanf(input, "%d", &newPaletteSize);
	// Account for `maxIterationColor` exported at the start of the palette by the C# version,
	// but not recorded in the count read above.
	// Alsp avoid an extra pointer dereference,	
	newPaletteSize = (*paletteSize = newPaletteSize + 1);
	rgb* palette = calloc(newPaletteSize, sizeof(rgb));
	for(int i = 0; i < newPaletteSize; ++i)
	{
		if (!fscanf(input, "%hhu,%hhu,%hhu", &palette[i].r, &palette[i].g, &palette[i].b))
		{
			// Unexpected EOF - handle it nicely!
			// Set the palette size to wherever we had to stop
			*paletteSize = i;
			// Ensure that we don't leak memory by using more than what we need
			palette = realloc(palette, *paletteSize);
			// Exit
			break;
		}
	}
	return palette;
}

/*
 * A function to export a palette in the format detailed above;
 * Not used in this implementation.
 */
int savePalette(FILE* output, rgb* palette, int paletteSize)
{
	if (!output)
	{
		return IOERR_NOFILE;
	}
	fprintf(output, "%d", paletteSize);
	for (int i = 0; i < paletteSize; ++i)
	{
		if (!fprintf(output, "%hhu,%hhu,%hhu", palette[i].r, palette[i].g, palette[i].b))
		{
			return IOERR_EOF;
		}
	}
	return 0;
}