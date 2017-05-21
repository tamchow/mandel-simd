#define _CRT_SECURE_NO_WARNINGS 1

#ifdef _WIN32

#include <Windows.h>
#endif


#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include "color.h"

#ifdef _WIN32

typedef LARGE_INTEGER BenchmarkClock;


void GetBenchmarkClock(BenchmarkClock* timestamp)
{
	QueryPerformanceCounter(timestamp);
}

double BenchmarkClockDelta(BenchmarkClock end, BenchmarkClock begin)
{
	static LARGE_INTEGER frequency = { 0 };

	if (!frequency.QuadPart)
		QueryPerformanceFrequency(&frequency);


	return 1e3 * ((double)end.QuadPart - (double)begin.QuadPart) / (double)frequency.QuadPart;
}

#endif

#include "mandel.h"
#include "palette.h"

//#define DEBUG 1

void mandel_basic(unsigned char* image, struct spec* s);

/*
* source = {{x, xwidth}, {y, ywidth}}
* range = {{xmin, xmax}, {ymin, ymax}}
*/
fpair* points_width_to_ranges(fpair range[2], fpair source[2])
{
	for (int i = 0; i < 2; ++i)
	{
		float halfrange = source[i].y / 2.0f;
		range[i].x = source[i].x - halfrange;
		range[i].y = source[i].x + halfrange;
	}
	return range;
}

void convert_point_width_spec_to_range(spec* s) {
	if (s->is_point_width)
	{
		fpair range[2], source[2] = { s->xlim, s->ylim };
		points_width_to_ranges(range, source);
		s->xlim = range[0];
		s->ylim = range[1];
	}
}

// best ZP_SIZE experimentally is 2, so we can hard-code this too, 
// but it actually isn't faster hard-coded, but about 10-15ms slower @standard config
inline int any_of_and(float v1, float v2, float *vs1, float *vs2, int vs_size)
{
	for (int i = 0; i < vs_size; ++i)
	{
		if (v1 == vs1[i] && v2 == vs2[i]) return 1;
	}
	return 0;
}

// hard-coded for 20ms faster speed @standard config
inline void populate_periodicity(float *zrp, float *zip, int size, float zr, float zi)
{
	int more_size = size;
	while (size)
	{
		zrp[size] = zrp[--size];
		zip[more_size] = zip[--more_size];
	}
	zrp[0] = zr;
	zip[0] = zi;
}

#define ZP_SIZE 2

void mandel_basic(unsigned char* image, spec* s)
{
	FILE* logFile = fopen("./countdata.csv", "w");
	convert_point_width_spec_to_range(s);
	float xscale = fabsf(s->xlim.y - s->xlim.x) / s->width;
	float yscale = fabsf(s->ylim.y - s->ylim.x) / s->height;
	float depth_scale = palette_size - 1.0f;
	float log_2 = logf(2.0f);
	float denom = 2.0f*2.0f*logf(s->bailout_sq);
	channel rgb[3];
	//#pragma omp parallel for schedule(dynamic)
	for (int y = s->startY; y < s->height; y++)
	{
		for (int x = s->startX; x < s->width; x++)
		{
			float cr = x * xscale + s->xlim.x;
			float ci = -(y * yscale + s->ylim.x);
			float zr = 0, zi = 0;//, zrp0 = zr, zrp1 = zr, zip0 = zi, zip1 = zi;
			float dzr = 0, dzi = 0;
			float m2 = zr * zr + zi * zi;
			float zrp[ZP_SIZE] = { zr, zr };
			float zip[ZP_SIZE] = { zi, zi };
			int k = 0;
			while (k < s->iterations && m2 < s->bailout_sq) {
				float zr1 = zr * zr - zi * zi + cr;
				float zi1 = zr * zi + zr * zi + ci;
				float dzr1 = 2.0f * (zr * dzr - zi * dzi) + 1.0f;
				float dzi1 = 2.0f * (zr * dzi + dzr * zi);
				//periodicity checking - can speed up the code
				if (zr1 == zr && zi1 == zi || any_of_and(zr1, zi1, zrp, zip, ZP_SIZE))
				{
					k = s->iterations;
					break;
				}
				/*if (((zr1 == zr) && (zi1 == zi)) || ((zr1 == zrp0) && (zi1 == zip0)) || ((zr1 == zrp1) && (zi1 == zip1)))
				{
					k = s->iterations;
					break;
				}*/
				//populate_periodicity(zrp, zip, ZP_SIZE, zr, zi);
				zrp[1] = zrp[0];
				zip[1] = zip[0];
				zrp[0] = zr;
				zip[0] = zi;
				/*zrp1 = zrp0;
				zip1 = zip0;
				zrp0 = zr;
				zip0 = zi;*/
				dzr = dzr1;
				dzi = dzi1;
				zr = zr1;
				zi = zi1;
				m2 = zr * zr + zi * zi;
				++k;
			}
			float mk = 0.0f;
			if (k < s->iterations) {
				mk = fabsf(k + 1 - logf(logf(m2) / denom) / log_2);
			}
			/*mk *= iter_scale;
			mk = sqrtf(k);
			mk *= depth_scale;*/
			float mu = mk - (int)mk;
			switch (s->mode) {
				//test it out using grayscale
			case NO_SMOOTH:
			{
				channel rgb = (channel)(k >= s->iterations ? 255 : ((float)k / s->iterations) * 255);
				colorPixel(image, s->width, s->startY + y, s->startX + x, rgb, rgb, rgb);
			}
			break;
			case SMOOTH:
			{

			}
			break;
			default: break;
			}
		}
	}
}

int main(int argc, char* argv[])
{
	/* Config */
	fpair xlim = { -2.5, 1.5 },
		ylim = { -1.5, 1.5 };
	spec spec = {
		spec.startX = 0,
		spec.startY = 0,
		spec.width = 640,
		spec.height = 480,
		spec.xlim = xlim,
		spec.ylim = ylim,
		spec.is_point_width = false,
		spec.iterations = 256,
		spec.bailout_sq = 4.0f,
		spec.mode = NO_SMOOTH
	};

	/* Render */
	channel* image = malloc(spec.width * spec.height * 3 * sizeof(unsigned char));

	mandel_basic(image, &spec);

	FILE* output = fopen("./img.ppm", "wb");
	/* Write result */
	fprintf(output, "P6\n%d %d\n%d\n", spec.width, spec.height, 255);
	fwrite(image, sizeof(unsigned char), spec.width * spec.height * 3, output);
	fclose(output);
	free(image);
	return 0;
}