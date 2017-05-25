#pragma once

/*
 * Pretty obvious stuff for modeling colors and increasing understandability of code tailored for this domain
 */
typedef unsigned char channel;

/*
 * Models a 24-bit RGB color.
 * Strict struct member alignment is unnecessary.
 */
typedef struct rgb {
	channel r;
	channel g;
	channel b;
} rgb;