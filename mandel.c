#define _CRT_SECURE_NO_WARNINGS 1


#include <stdio.h>
#include <stdlib.h>
#include <math.h>

#ifdef _WIN32

#include <Windows.h>

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

#include "getopt.h"
#include "mandel.h"
#include "palette.h"

#if defined(__MSC__VER)
#ifdef _M_AMD64
#define __x86_64__ 1
#endif
#elif defined(__GNUC__)
#ifdef __amd64__
#define __x86_64__ 1
#endif
#endif

#define __x86_64__ 1

//#define DEBUG 1

void mandel_basic(unsigned char* image, struct spec* s);
void mandel_altivec(unsigned char* image, struct spec* s);
void mandel_avx(unsigned char* image, struct spec* s);
void mandel_sse2(unsigned char* image, struct spec* s);
void mandel_neon(unsigned char* image, struct spec* s);

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

void init_histogram(const struct spec* s)
{
	histogram = calloc(s->iterations + 1, sizeof(size_t));
	escapedata = calloc(s->height, sizeof(escapedatum*));
	for (size_t i = 0; i < s->height; ++i)
	{
		escapedata[i] = calloc(s->width, sizeof(escapedatum));
	}
}

// best ZP_SIZE experimentally is 2, so we can hard-code this too, 
// but it actually isn't faster hard-coded, but about 10-15ms slower @standard config
inline int any_of_and(float v1, float v2, float *vs1, float *vs2, size_t vs_size)
{
	int i;
	for (i = 0; i < vs_size; ++i)
	{
		if ((v1 == vs1[i]) && (v2 == vs2[i])) return 1;
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
	convert_point_width_spec_to_range(s);
	float xscale = fabsf(s->xlim.y - s->xlim.x) / s->width;
	float yscale = fabsf(s->ylim.y - s->ylim.x) / s->height;
	float depth_scale = palette_size - 1.0f;
	float log_2 = logf(2.0f);
	float denom = (2.0f*2.0f*logf(s->bailout_sq));
	float distmax = 0.0f;
	channel rgb[3];
	int y;
	//#pragma omp parallel for schedule(dynamic)
	for (y = 0; y < s->height; y++)
	{
		for (size_t x = 0; x < s->width; x++)
		{
			float cr = x * xscale + s->xlim.x;
			float ci = -(y * yscale + s->ylim.x);
			float zr = 0, zi = 0;//, zrp0 = zr, zrp1 = zr, zip0 = zi, zip1 = zi;
			float dzr = 0, dzi = 0;
			float m2 = ((zr * zr) + (zi * zi));
			float zrp[ZP_SIZE] = { zr, zr };
			float zip[ZP_SIZE] = { zi, zi };
			size_t k = 0;
			while ((k < s->iterations) && ( m2 < s->bailout_sq)) {
				float zr1 = zr * zr - zi * zi + cr;
				float zi1 = zr * zi + zr * zi + ci;
				float dzr1 = 2.0f * (zr * dzr - zi * dzi) + 1.0f;
				float dzi1 = 2.0f * (zr * dzi + dzr * zi);
				//periodicity checking - can speed up the code
				if ((((zr1 == zr) && (zi1 == zi)) || (any_of_and(zr1, zi1, zrp, zip, ZP_SIZE))))
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
				m2 = ((zr * zr) + (zi * zi));
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
				case GREYSCALE:
				{
					channel pixel = linear_map_channel(k, 0, s->iterations, depth_scale, s->index_mode);
					//comment out next 2 lines to not use linear interpolation
					channel pixel2 = linear_map_channel((((k + 1) > s->iterations) ? s->iterations : k + 1), 0, s->iterations, depth_scale, s->index_mode);
					pixel = pixel * (1.0f - mu) + pixel2 * (mu);
					rgb_pixel(image, s->width, y, x, pixel, pixel, pixel);
					/*channel color1[3], color2[3];
					lerp(rgb, color_from_rgb(color1, pixel, pixel, pixel), color_from_rgb(color2, pixel2, pixel2, pixel2), mu);
					color_pixel(image, s->width, y, x, rgb);*/
				}
				break;
				case ITERATION_COUNT:
				{
					size_t idx1 = linear_map_index(k, 0, s->iterations, depth_scale, palette_size, s->index_mode);
					size_t idx2 = linear_map_index((((k + 1) > s->iterations) ? s->iterations : k + 1), 0, s->iterations, depth_scale, palette_size, s->index_mode);
					lerp(rgb, palette[idx1], palette[idx2], mu);
					color_pixel(image, s->width, y, x, rgb);
					//for use without linear interpolation
					//color_pixel(image, s->width, y, x, palette[idx1]);
				}
				break;
				case DEM:
				{
					float dist = fabsf(sqrtf(m2 / (dzr * dzr + dzi * dzi)) * logf(m2));
					dist = isnan(dist) ? 0 : ((!isfinite(dist)) ? distmax : dist);
					distmax = (distmax < dist) ? dist : distmax;
					channel pixel = linear_map_channel(dist, 0.0f, distmax, depth_scale, s->index_mode);
					rgb_pixel(image, s->width, y, x, pixel, pixel, pixel);
				}
				break;
				default:
				{
					histogram[k]++;
					//escapedatum escapedatum = { k, mk };
					//escapedatum escapedatum = { k, k };
					escapedata[y][x] = k;
				}
			}
		}
	}
}

spec* readspec_unsafe(FILE* input, spec* spec)
{
	fscanf(input, "%zu %zu", &spec->width, &spec->height);
	fscanf(input, "%u", &spec->max_color_value);
	fscanf(input, "{%f %f},{%f %f}", &spec->xlim.x, &spec->xlim.y, &spec->ylim.x, &spec->ylim.y);
	fscanf(input, "%d %zu %f %d", &spec->is_point_width, &spec->iterations, &spec->bailout_sq, &spec->mode);
	return spec;
}

#ifdef __x86_64__
//#include <cpuid.h>

//static inline int
//is_avx_supported(void)
//{
//	unsigned int eax = 0, ebx = 0, ecx = 0, edx = 0;
//	__get_cpuid(1, &eax, &ebx, &ecx, &edx);
//	return ecx & bit_AVX ? 1 : 0;
//}

static inline int is_avx_supported(void)
{
	//XXX fixme
	return 1;
}
#endif // __x86_64__


int main(int argc, char* argv[])
{
	/* Config */
	fpair xlim = { -2.5, 1.5 },
		ylim = { -1.5, 1.5 };
	spec spec = {
		spec.width = 1440,
		spec.height = 1080,
		spec.max_color_value = 256,
		spec.xlim = xlim,
		spec.ylim = ylim,
		spec.is_point_width = false,
		spec.iterations = 256,
		spec.bailout_sq = 4.0f,
		spec.mode = DEM,
		spec.index_mode = SQRT
	};

	palette_size = spec.max_color_value;
	const char* optstring = "w:h:d:k:x:y:AS";

#if defined(__x86_64__)
	int use_avx = 0;
	int use_sse2 = 0;
#endif // __x86_64__


#if defined(__arm__) || defined(__aarch64__)
	int use_neon = 1;
	optstring = "w:h:d:k:x:y:N";
#endif // __arm__ || __aarch64__


#ifdef __ppc__
	int use_altivec = 1;
	optstring = "w:h:d:k:x:y:A";
#endif // __ppc__


	/* Parse Options */
	int option;
	while ((option = getopt(argc, argv, optstring)) != -1)
	{
		switch (option)
		{
		case 'w':
			spec.width = atoi(optarg);
			break;
		case 'h':
			spec.height = atoi(optarg);
			break;
		case 'd':
			spec.max_color_value = atoi(optarg);
			break;
		case 'k':
			spec.iterations = atoi(optarg);
			break;
		case 'x':
			sscanf(optarg, "%f:%f", &spec.xlim.x, &spec.xlim.y);
			break;
		case 'y':
			sscanf(optarg, "%f:%f", &spec.ylim.x, &spec.ylim.y);
			break;

#ifdef __x86_64__
		case 'A':
			use_avx = 0;
			break;
		case 'S':
			use_sse2 = 0;
			break;
#endif // __x86_64__


#if defined(__arm__) || defined(__aarch64__)
		case 'N':
			use_neon = 0;
			break;
#endif // __arm__ || __aarch64__


#ifdef __ppc__
		case 'A':
			use_altivec = 0;
			break;
#endif // __ppc__


		default:
			exit(EXIT_FAILURE);
			break;
		}
	}

	int nn = 1;

	/* Render */
	channel* images = malloc((size_t)(3 + nn) * (spec.width * spec.height * 3));

	//
	//	for (size_t i = 0; i < (size_t)(3 + nn)*(spec.width * spec.height * 3); i++)
	//	{
	//		images[i] = 0x55;
	//	}
	//	BenchmarkClock t0, t1;
	//
	//	
	//	GetBenchmarkClock(&t0);
	//
	//	int i;
	////#pragma omp parallel for schedule(dynamic, 1)
	//	for (i = 0; i < nn; i++)
	//	{
	//printf("i=%d\n", i);
	//printf("i=%llx\n", images + (size_t)(3 + nn)*(spec.width * spec.height * 3));
	int i = 0;
	channel* image = &images[(size_t)i * (spec.width * spec.height * 3)];
	if (argc > 1)
	{
		FILE* config_file = fopen(argv[1], "r");
		if (config_file)
		{
			spec = *readspec_unsafe(config_file, &spec);
		}
	}

	if (spec.mode == HISTOGRAM)
	{
		init_histogram(&spec);
	}

#ifdef __x86_64__
	if (use_avx && is_avx_supported())
		mandel_avx(image, &spec);
	else if (use_sse2)
		mandel_sse2(image, &spec);
#endif // __x86_64__


#if defined(__arm__) || defined(__aarch64__)
	if (use_neon)
		mandel_neon(image, &spec);
#endif // __arm__ || __aarch64__


#ifdef __ppc__
	if (use_altivec)
		mandel_altivec(image, &spec);
#endif // __ppc__


	else
		mandel_basic(image, &spec);


	//}

	//GetBenchmarkClock(&t1);
	//double delta = BenchmarkClockDelta(t1, t0);
	//printf("delta = %f\n", delta);

	FILE* output = fopen("./img.ppm", "w");
	/* Write result */
	fprintf(output, "P6\n%zd %zd\n%d\n", spec.width, spec.height, (spec.max_color_value == 0) ? 0 : 255);
	fwrite(image, spec.width * spec.height, 3, output);
	fclose(output);
	free(image);
	return 0;
}