#pragma once
#include <stdio.h>
#include "color.h"

#define IOERR_NOFILE -1
#define IOERR_EOF -2

color* palette;//n by 3 array
size_t palette_size;

int load_palette(FILE* input);
int save_palette(FILE* output);
void create_smooth_palette(color* colors, float* points, size_t size);