#pragma once

#include <stdio.h>
#include "color.h"

/*
 * Define IO error indicator constants
 */
#define  IOERR_NOFILE -1;
#define  IOERR_EOF -2;

rgb* loadPalette(FILE* input, int* paletteSize);
int savePalette(FILE* output, rgb* palette, int paletteSize);