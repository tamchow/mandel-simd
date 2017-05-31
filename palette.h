#pragma once

#include <stdio.h>
#include "color.h"

/*
 * Define IO error indicator constants
 */
#define  IOERR_NOFILE -1;
#define  IOERR_EOF -2;

rgb* loadPalette(FILE* input, int* paletteSize);
const rgb** repeatPalette(const rgb* palette, const int paletteCount);
rgb** padRepeatedPaletteWithLastEntry(rgb** palette, const int lastOccupiedIndex, const int paletteCount);