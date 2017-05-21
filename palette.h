#pragma once
#include <stdio.h>
#include "color.h"

#define IOERR_NOFILE -1
#define IOERR_EOF -2

rgb* palette;//n by 3 array
int palette_size;

int load_palette(FILE* input);
int save_palette(FILE* output);