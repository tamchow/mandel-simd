#include "palette.h"

int load_palette(FILE* input)
{
	if(!input)
	{
		return IOERR_NOFILE;
	}
	fscanf_s(input, "%d", &palette_size);
	for(int i = 0; i < palette_size; ++i)
	{
		if (!fscanf_s(input, "%hhu,%hhu,%hhu", &palette[i].r, &palette[i].g, &palette[i].b))
		{
			return IOERR_EOF;
		}
	}
	return 0;
}

int save_palette(FILE* output)
{
	if (!output)
	{
		return IOERR_NOFILE;
	}
	fprintf(output, "%d", palette_size);
	for (int i = 0; i < palette_size; ++i)
	{
		if (!fprintf(output, "%hhu,%hhu,%hhu", palette[i].r, palette[i].g, palette[i].b))
		{
			return IOERR_EOF;
		}
	}
	return 0;
}