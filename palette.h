#pragma once
#define _CRT_SECURE_NO_WARNINGS 1
#include <stdio.h>
#include "color.h"

#define MIN(x, y) ((x < y) ? x : y)

#define IOERR_NOFILE -1
#define IOERR_EOF -2

int loadPalette(FILE* input, rgb* palette, int paletteSize);
int savePalette(FILE* output, rgb* palette, int paletteSize);