#pragma once
#include <stdio.h>
#include "color.h"
#include "mandel.h"

void writeOutput(FILE* outputFile, channel* image, const int width, const int height);

void writeOutputWithConfiguration(FILE* outputFile, channel* image, const Configuration* configuration);

void writeOutputSequence(const size_t count, const char** outputFileNames, channel** images, const int* widths, const int* heights);

void writeOutputSequenceWithConfiguration(const size_t count, char** outputFileNames, channel** images, Configuration** configurations);