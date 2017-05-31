/* ---------------------------------------------------------------------------
 * Truevision Targa Reader/Writer
 * Copyright (C) 2001-2003 Emil Mikulic.
 *
 * Source and binary redistribution of this code, with or without changes, for
 * free or for profit, is allowed as long as this copyright notice is kept
 * intact.  Modified versions must be clearly marked as modified.
 *
 * This code is provided without any warranty.  The copyright holder is
 * not liable for anything bad that might happen as a result of the
 * code.
 *
 * Modified for exclusively little-endian systems,
 * with parts unconcerned with writing truecolor images discarded.
 * -------------------------------------------------------------------------*/

#define _CRT_SECURE_NO_WARNINGS 1

#include <stdlib.h>
#include <memory.h> /* memcpy, memcmp */
#include "targa.h"

static const char tgaFooter[] =
"\0\0\0\0" /* extension area offset */
"\0\0\0\0" /* developer directory offset */
"TRUEVISION-XFILE.";

static const size_t tgaFooterLength = 26; /* tga_id + \0 */

typedef enum { RAW, RLE } PacketType;

/* ---------------------------------------------------------------------------
 * Convert the numerical <errcode> into a verbose error string.
 *
 * Returns: an error string
 */
const char *tga_error(const tga_result errcode)
{
	switch (errcode)
	{
	case TGA_NOERR:
		return "no error";
	case TGAERR_FOPEN:
		return "error opening file";
	case TGAERR_EOF:
		return "premature end of file";
	case TGAERR_WRITE:
		return "error writing to file";
	case TGAERR_CMAP_TYPE:
		return "invalid color map type";
	case TGAERR_IMG_TYPE:
		return "invalid image type";
	case TGAERR_NO_IMG:
		return "no image data included";
	case TGAERR_CMAP_MISSING:
		return "color-mapped image without color map";
	case TGAERR_CMAP_PRESENT:
		return "non-color-mapped image with extraneous color map";
	case TGAERR_CMAP_LENGTH:
		return "color map has zero length";
	case TGAERR_CMAP_DEPTH:
		return "invalid color map depth";
	case TGAERR_ZERO_SIZE:
		return "the image dimensions are zero";
	case TGAERR_PIXEL_DEPTH:
		return "invalid pixel depth";
	case TGAERR_NO_MEM:
		return "out of memory";
	case TGAERR_NOT_CMAP:
		return "image is not color mapped";
	case TGAERR_RLE:
		return "RLE data is corrupt";
	case TGAERR_INDEX_RANGE:
		return "color map index out of range";
	case TGAERR_MONO:
		return "image is mono";
	default:
		return "unknown error code";
	}
}

#define PIXEL(ofs) ( row + (ofs)*bpp )
#define SAME(ofs1, ofs2) (memcmp(PIXEL(ofs1), PIXEL(ofs2), bpp) == 0)

/* ---------------------------------------------------------------------------
* Find the length of the current RLE packet.  This is a helper function
* called from tga_write_row_RLE().
*/
static uint8_t rle_packet_len(const uint8_t *row, const uint16_t pos, const uint16_t width, const uint16_t bpp, const PacketType type)
{
	uint8_t len = 2;

	if (pos == width - 1) return 1;
	if (pos == width - 2) return 2;

	if (type == RLE)
	{
		while (pos + len < width)
		{
			if (SAME(pos, pos + len))
				len++;
			else
				return len;

			if (len == 128) return 128;
		}
	}
	else /* type == RAW */
	{
		while (pos + len < width)
		{
			if (rle_packet_type(row, pos + len, width, bpp) == RAW)
				len++;
			else
				return len;
			if (len == 128) return 128;
		}
	}
	return len; /* hit end of row (width) */
}

/* ---------------------------------------------------------------------------
* Determine whether the next packet should be RAW or RLE for maximum
* efficiency.  This is a helper function called from rle_packet_len() and
* tga_write_row_RLE().
*/
static PacketType rle_packet_type(const uint8_t *row, const uint16_t pos, const uint16_t width, const uint16_t bpp)
{
	if (pos == width - 1) return RAW; /* one pixel */
	if (SAME(pos, pos + 1)) /* dupe pixel */
	{
		if (bpp > 1) return RLE; /* inefficient for bpp=1 */

								 /* three repeats makes the bpp=1 case efficient enough */
		if ((pos < width - 2) && SAME(pos + 1, pos + 2)) return RLE;
	}
	return RAW;
}

/* ---------------------------------------------------------------------------
 * Write one row of an image to <fp> using RLE.  This is a helper function
 * called from tga_write_to_FILE().  It assumes that <src> has its header
 * fields set up correctly.
 */
static tga_result tga_write_row_RLE(FILE *fp, const tga_image *src, const uint8_t *row)
{
#define WRITE(src, size) \
        if (fwrite(src, size, 1, fp) != 1) return TGAERR_WRITE

	uint16_t pos = 0;
	uint16_t bpp = src->pixel_depth / 8;

	while (pos < src->width)
	{
		PacketType type = rle_packet_type(row, pos, src->width, bpp);
		uint8_t len = rle_packet_len(row, pos, src->width, bpp, type);
		uint8_t packet_header;

		packet_header = len - 1;
		if (type == RLE) packet_header |= BIT(7);

		WRITE(&packet_header, 1);
		if (type == RLE)
		{
			WRITE(PIXEL(pos), bpp);
		}
		else /* type == RAW */
		{
			WRITE(PIXEL(pos), bpp*len);
		}

		pos += len;
	}

	return TGA_NOERR;
#undef WRITE
}

#undef SAME
#undef PIXEL

/* ---------------------------------------------------------------------------
 * Writes a Targa image to <fp> from <src>.
 *
 * Returns: TGA_NOERR on success, or a TGAERR_* code on failure.
 *          On failure, the contents of the file are not guaranteed
 *          to be valid.
 */
tga_result tga_write_to_FILE(FILE *fp, const tga_image *src)
{
#define WRITE(srcptr, size) \
        if (fwrite(srcptr, size, 1, fp) != 1) return TGAERR_WRITE

#define WRITE16(src) \
        { uint16_t _temp = (src); \
          if (fwrite(&_temp, 2, 1, fp) != 1) return TGAERR_WRITE; }

	WRITE(&src->image_id_length, 1);
	WRITE(&src->color_map_type, 1);
	WRITE(&src->image_type, 1);

	WRITE16(src->color_map_origin);
	WRITE16(src->color_map_length);
	WRITE(&src->color_map_depth, 1);

	WRITE16(src->origin_x);
	WRITE16(src->origin_y);

	if (src->width == 0 || src->height == 0)
		return TGAERR_ZERO_SIZE;
	WRITE16(src->width);
	WRITE16(src->height);
	WRITE(&src->pixel_depth, 1);

	WRITE(&src->image_descriptor, 1);

	if (src->image_id_length > 0)
		WRITE(&src->image_id, src->image_id_length);

	if (src->color_map_type == TGA_COLOR_MAP_PRESENT)
		WRITE(src->color_map_data +
		(src->color_map_origin * src->color_map_depth / 8),
			src->color_map_length * src->color_map_depth / 8);

	uint16_t row;
	for (row = 0; row < src->height; row++)
	{
		tga_result result = tga_write_row_RLE(fp, src,
			src->image_data + row*src->width*src->pixel_depth / 8);
		if (result != TGA_NOERR) return result;
	}

	WRITE(tgaFooter, tgaFooterLength);

	return TGA_NOERR;
#undef WRITE
#undef WRITE16
}



/* Convenient writing functions --------------------------------------------*/

/*
 * This is just a helper function to initialise the header fields in a
 * tga_image struct.
 */
static void init_tga_image(tga_image *img, uint8_t *image, const uint16_t width, const uint16_t height, const uint8_t depth)
{
	img->image_id_length = 0;
	img->color_map_type = TGA_COLOR_MAP_ABSENT;
	img->image_type = TGA_IMAGE_TYPE_NONE; /* override this below! */
	img->color_map_origin = 0;
	img->color_map_length = 0;
	img->color_map_depth = 0;
	img->origin_x = 0;
	img->origin_y = 0;
	img->width = width;
	img->height = height;
	img->pixel_depth = depth;
	img->image_descriptor = TGA_T_TO_B_BIT;
	img->image_id = NULL;
	img->color_map_data = NULL;
	img->image_data = image;
}

tga_result tga_write_bgr_rle_FILE(FILE *file, uint8_t *image, const uint16_t width, const uint16_t height, const uint8_t depth)
{
	tga_image img;
	init_tga_image(&img, image, width, height, depth);
	img.image_type = TGA_IMAGE_TYPE_BGR_RLE;
	return tga_write_to_FILE(file, &img);
}


/* ---------------------------------------------------------------------------
 * Free the image_id, color_map_data and image_data buffers of the specified
 * tga_image, if they're not already NULL.
 */
void tga_free_buffers(tga_image *img)
{
	if (img->image_id != NULL)
	{
		free(img->image_id);
		img->image_id = NULL;
	}
	if (img->color_map_data != NULL)
	{
		free(img->color_map_data);
		img->color_map_data = NULL;
	}
	if (img->image_data != NULL)
	{
		free(img->image_data);
		img->image_data = NULL;
	}
}