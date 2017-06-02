#define _CRT_SECURE_NO_WARNINGS 1

#include <stdlib.h>
#include "palette.h"

#define PAL_COLOR_LENGTH (RGB_BitDepth + 1)

/*
 * Read in a palette from a text file on disk, stored in Microsoft PAL format
 * Format specification here: http://worms2d.info/Palette_file
 */
rgb* loadPalette(FILE* input, int* paletteSize)
{
	if (!input)
	{
		// Illegal file pointer - potentially doesn't exist or is not readable
		return NULL;
	}
	uint16_t newPaletteSize = 0;
	// Skip header and irrelevant sections
	long startOffset = 4 + 4 + 4 + 4 + 4 + 2;
	if (fseek(input, startOffset, SEEK_CUR) != 0)
	{
		return NULL;
	}
	if (!fread((void*)&newPaletteSize, sizeof(uint16_t), 1, input))
	{
		return NULL;
	}
	
	// Avoid an extra pointer dereference,	
	*paletteSize = (int)newPaletteSize;
	rgb* palette = calloc(newPaletteSize, sizeof(rgb));
	// Avoid pesky, if rare, segfaults and memory allocation issues.
	if (palette != NULL) {
		for (uint16_t i = 0; i < newPaletteSize; ++i)
		{
			channel color[PAL_COLOR_LENGTH] = { 0,0,0,0 };
			if (!fread((void*)color, sizeof(channel), PAL_COLOR_LENGTH, input))
			{
				// Unexpected EOF - handle it nicely!
				// Set the palette size to wherever we had to stop
				*paletteSize = i;
				// Ensure that we don't leak memory by using more than what we need
				palette = realloc(palette, ((*paletteSize) * sizeof(rgb)));
				// Exit	loop
				break;
			}
			palette[i].r = color[0];
			palette[i].g = color[1];
			palette[i].b = color[2];
		}
	}
	return palette;
}

const rgb** repeatPalette(const rgb* palette, const int paletteCount)
{
	const rgb** palettes = calloc((size_t)paletteCount, sizeof(rgb*));
	// Avoid pesky segfaults and acces violations
	if (palettes != NULL)
	{
		for (int index = 0; index < paletteCount; ++index)
		{
			palettes[index] = palette;
		}
	}
	return palettes;
}

rgb** padRepeatedPaletteWithLastEntry(rgb** palettes, const int lastOccupiedIndex, const int paletteCount)
{
	// Avoid pesky segfaults and acces violations - doing some bounds checking to bail out early in case of erroneous input
	if (lastOccupiedIndex >= 0 && lastOccupiedIndex < paletteCount)
	{
		palettes = realloc(palettes, (size_t)paletteCount);
		if(palettes != NULL)
		{
			for (int index = lastOccupiedIndex; index < paletteCount; ++index)
			{
				palettes[index] = palettes[lastOccupiedIndex];
			}
		}
	}
	return palettes;
}
