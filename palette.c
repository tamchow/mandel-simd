#include "palette.h"

int loadPalette(FILE* input, rgb* palette, int paletteSize)
{
	if(!input)
	{
		return IOERR_NOFILE;
	}
	int newPaletteSize = 0;
	fscanf(input, "%d", &newPaletteSize);
	for(int i = 0; i < MIN(newPaletteSize + 1, paletteSize); ++i)
	{
		if (!fscanf(input, "%hhu,%hhu,%hhu", &palette[i].r, &palette[i].g, &palette[i].b))
		{
			return IOERR_EOF;
		}
	}
	return newPaletteSize;
}

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