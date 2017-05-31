/* ---------------------------------------------------------------------------
* Truevision Targa Reader/Writer
* $Id: targa.h,v 1.8 2004/10/09 09:30:26 emikulic Exp $
*
* Copyright (C) 2001-2003 Emil Mikulic.
*
* Source and binary redistribution of this code, with or without
* changes, for free or for profit, is allowed as long as this copyright
* notice is kept intact.  Modified versions have to be clearly marked
* as modified.
*
* This code is provided without any warranty.  The copyright holder is
* not liable for anything bad that might happen as a result of the
* code.	 *
*
* Modified for exclusively little-endian systems,
* with parts unconcerned with writing truecolor images discarded.
* -------------------------------------------------------------------------*/

#pragma once
#include <stdio.h>
#include <stdint.h>

#define BIT(index) (1 << (index))

/* Targa image and header fields -------------------------------------------*/
typedef struct
{
	/* Note that Targa is stored in little-endian order */
	uint8_t     image_id_length;

	uint8_t     color_map_type;
	/* color map = palette */
#define TGA_COLOR_MAP_ABSENT    0
#define TGA_COLOR_MAP_PRESENT   1

	uint8_t     image_type;
#define TGA_IMAGE_TYPE_NONE          0 /* no image data */
#define TGA_IMAGE_TYPE_COLORMAP      1 /* uncompressed, color-mapped */
#define TGA_IMAGE_TYPE_BGR           2 /* uncompressed, true-color */
#define TGA_IMAGE_TYPE_MONO          3 /* uncompressed, black and white */
#define TGA_IMAGE_TYPE_COLORMAP_RLE  9 /* run-length, color-mapped */
#define TGA_IMAGE_TYPE_BGR_RLE      10 /* run-length, true-color */
#define TGA_IMAGE_TYPE_MONO_RLE     11 /* run-length, black and white */

	/* color map specification */
	uint16_t    color_map_origin;   /* index of first entry */
	uint16_t    color_map_length;   /* number of entries included */
	uint8_t     color_map_depth;    /* number of bits per entry */

									/* image specification */
	uint16_t    origin_x;
	uint16_t    origin_y;
	uint16_t    width;
	uint16_t    height;
	uint8_t     pixel_depth;

	uint8_t     image_descriptor;
	/* bits 0,1,2,3 - attribute bits per pixel
	* bit  4       - set if image is stored right-to-left
	* bit  5       - set if image is stored top-to-bottom
	* bits 6,7     - unused (must be set to zero)
	*/
#define TGA_ATTRIB_BITS (uint8_t)(BIT(0)|BIT(1)|BIT(2)|BIT(3))
#define TGA_R_TO_L_BIT  (uint8_t)BIT(4)
#define TGA_T_TO_B_BIT  (uint8_t)BIT(5)
#define TGA_UNUSED_BITS (uint8_t)(BIT(6)|BIT(7))
	/* Note: right-to-left order is not honored by some Targa readers */

	uint8_t *image_id;
	/* The length of this field is given in image_id_length, it's read raw
	* from the file so it's not not guaranteed to be zero-terminated.  If
	* it's not NULL, it needs to be deallocated.  see: tga_free_buffers()
	*/

	uint8_t *color_map_data;
	/* See the "color map specification" fields above.  If not NULL, this
	* field needs to be deallocated.  see: tga_free_buffers()
	*/

	uint8_t *image_data;
	/* Follows image specification fields (see above) */

	/* Extension area and developer area are silently ignored.  The Targa 2.0
	* spec says we're not required to read or write them.
	*/

} tga_image;

/* Error handling ----------------------------------------------------------*/
typedef enum {
	TGA_NOERR,
	TGAERR_FOPEN,
	TGAERR_EOF,
	TGAERR_WRITE,
	TGAERR_CMAP_TYPE,
	TGAERR_IMG_TYPE,
	TGAERR_NO_IMG,
	TGAERR_CMAP_MISSING,
	TGAERR_CMAP_PRESENT,
	TGAERR_CMAP_LENGTH,
	TGAERR_CMAP_DEPTH,
	TGAERR_ZERO_SIZE,
	TGAERR_PIXEL_DEPTH,
	TGAERR_NO_MEM,
	TGAERR_NOT_CMAP,
	TGAERR_RLE,
	TGAERR_INDEX_RANGE,
	TGAERR_MONO
} tga_result;

const char *tga_error(const tga_result errcode);

/* Load/save ---------------------------------------------------------------*/
tga_result tga_write_to_FILE(FILE *file, const tga_image *src);

/* Convenient writing functions --------------------------------------------*/
tga_result tga_write_bgr_rle_FILE(FILE *file, uint8_t *image,
	const uint16_t width, const uint16_t height, const uint8_t depth);

void tga_free_buffers(tga_image *img);