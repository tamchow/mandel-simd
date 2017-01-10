#include "palette.h"

int load_palette(FILE* input)
{
	if(!input)
	{
		return IOERR_NOFILE;
	}
	fscanf_s(input, "%zu", &palette_size);
	for(size_t i = 0; i < palette_size; ++i)
	{
		if (!fscanf_s(input, "%zu", &palette[i]))
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
	fprintf(output, "%zu", palette_size);
	for (size_t i = 0; i < palette_size; ++i)
	{
		if (!fprintf(output, "%zu", &palette[i]))
		{
			return IOERR_EOF;
		}
	}
	return 0;
}