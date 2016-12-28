
#define _CRT_SECURE_NO_WARNINGS 1


#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include "getopt.h"
#include "mandel.h"


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

#define __x86_64__ 1

void mandel_basic(unsigned char *image, const struct spec *s);
void mandel_altivec(unsigned char *image, const struct spec *s);
void mandel_avx(unsigned char *image, const struct spec *s);
void mandel_sse2(unsigned char *image, const struct spec *s);
void mandel_neon(unsigned char *image, const struct spec *s);

void
mandel_basic(unsigned char *image, const struct spec *s)
{
	float xdiff = s->xlim[1] - s->xlim[0];
	float ydiff = s->ylim[1] - s->ylim[0];
	float iter_scale = 1.0f / s->iterations;
	float depth_scale = s->depth - 1;
	int y;
#pragma omp parallel for schedule(dynamic, 1)
	for (y = 0; y < s->height; y++) {
		for (int x = 0; x < s->width; x++) {
			float cr = x * xdiff / s->width + s->xlim[0];
			float ci = y * ydiff / s->height + s->ylim[0];
			float zr = cr;
			float zi = ci;
			int k = 0;
			float mk = 0.0f;
			while (++k < s->iterations) {
				float zr1 = zr * zr - zi * zi + cr;
				float zi1 = zr * zi + zr * zi + ci;
				zr = zr1;
				zi = zi1;
				mk += 1.0f;
				if (zr * zr + zi * zi >= 4.0f)
					break;
			}
			mk *= iter_scale;
			mk = sqrtf(mk);
			mk *= depth_scale;
			int pixel = mk;
			image[y * s->width * 3 + x * 3 + 0] = pixel;
			image[y * s->width * 3 + x * 3 + 1] = pixel;
			image[y * s->width * 3 + x * 3 + 2] = pixel;
		}
	}
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

static inline int
is_avx_supported(void)
{
	//XXX fixme
	return 1;
}
#endif // __x86_64__

int
main(int argc, char *argv[])
{
	/* Config */
	struct spec spec = {
		.width = 1920 * 2,
		.height = 1080 * 2,
		.depth = 256,
		.xlim = {-2.5, 1.5},
		.ylim = {-1.5, 1.5},
		.iterations = 256
	};

#ifdef __x86_64__
	int use_avx = 1;
	int use_sse2 = 1;
	const char *optstring = "w:h:d:k:x:y:AS";
#endif // __x86_64__

#if defined(__arm__) || defined(__aarch64__)
	int use_neon = 1;
	const char *optstring = "w:h:d:k:x:y:N";
#endif // __arm__ || __aarch64__

#ifdef __ppc__
	int use_altivec = 1;
	const char *optstring = "w:h:d:k:x:y:A";
#endif // __ppc__

	/* Parse Options */
	int option;
	while ((option = getopt(argc, argv, optstring)) != -1) {
		switch (option) {
		case 'w':
			spec.width = atoi(optarg);
			break;
		case 'h':
			spec.height = atoi(optarg);
			break;
		case 'd':
			spec.depth = atoi(optarg);
			break;
		case 'k':
			spec.iterations = atoi(optarg);
			break;
		case 'x':
			sscanf(optarg, "%f:%f", &spec.xlim[0], &spec.xlim[1]);
			break;
		case 'y':
			sscanf(optarg, "%f:%f", &spec.ylim[0], &spec.ylim[1]);
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
	unsigned char *images = malloc((size_t)(3 + nn)*(spec.width * spec.height * 3));

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
	unsigned char *image = &images[(size_t)i*(spec.width * spec.height * 3)];



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


	/* Write result */
	fprintf(stdout, "P6\n%d %d\n%d\n", spec.width, spec.height, spec.depth - 1);
	fwrite(image, spec.width * spec.height, 3, stdout);
	free(image);

	getch();

	return 0;
}
