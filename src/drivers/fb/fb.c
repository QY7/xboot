/*
 * drivers/fb/fb.c
 *
 * Copyright (c) 2007-2009  jianjun jiang <jerryjianjun@gmail.com>
 * official site: http://xboot.org
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software Foundation,
 * Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 */

#include <configs.h>
#include <default.h>
#include <types.h>
#include <charset.h>
#include <malloc.h>
#include <xboot/chrdev.h>
#include <xboot/ioctl.h>
#include <console/console.h>
#include <fb/logo.h>
#include <fb/graphic.h>
#include <fb/fb.h>

extern x_bool fb_putcode(struct fb * fb, x_u32 code, x_u32 fc, x_u32 bc, x_u32 x, x_u32 y);

struct fbcon_cell
{
	/* code pointer */
	x_u32 cp;

	/* foreground color and background color */
	x_u32 fc, bc;

	/* the width of the cp */
	x_u32 w;
};

/*
 * defined the framebuffer console information
 */
struct fb_console_info
{
	/* the console name */
	char * name;

	/* framebuffer driver */
	struct fb * fb;

	/* console font width and height in pixel */
	x_s32 fw, fh;

	/* console width and height */
	x_s32 w, h;

	/* console current x, y */
	x_s32 x, y;

	/* console front color and background color */
	enum tcolor f, b;
	x_u32 fc, bc;

	/* cursor status, on or off */
	x_bool cursor;

	/* fb console's cell */
	struct fbcon_cell * cell;

	/* fb console cell's length */
	x_u32 clen;
};

static const x_u8 tcolor_to_rgba_table[256][4] = {
	/* 0x00 */	{ 0x00, 0x00, 0x00, 0xff },
	/* 0x01 */	{ 0xcd, 0x00, 0x00, 0xff },
	/* 0x02 */	{ 0x00, 0xcd, 0x00, 0xff },
	/* 0x03 */	{ 0xcd, 0xcd, 0x00, 0xff },
	/* 0x04 */	{ 0x00, 0x00, 0xee, 0xff },
	/* 0x05 */	{ 0xcd, 0x00, 0xcd, 0xff },
	/* 0x06 */	{ 0x00, 0xcd, 0xcd, 0xff },
	/* 0x07 */	{ 0xe5, 0xe5, 0xe5, 0xff },
	/* 0x08 */	{ 0x7f, 0x7f, 0x7f, 0xff },
	/* 0x09 */	{ 0xff, 0x00, 0x00, 0xff },
	/* 0x0a */	{ 0x00, 0xff, 0x00, 0xff },
	/* 0x0b */	{ 0xff, 0xff, 0x00, 0xff },
	/* 0x0c */	{ 0x5c, 0x5c, 0xff, 0xff },
	/* 0x0d */	{ 0xff, 0x00, 0xff, 0xff },
	/* 0x0e */	{ 0x00, 0xff, 0xff, 0xff },
	/* 0x0f */	{ 0xff, 0xff, 0xff, 0xff },
	/* 0x10 */	{ 0x00, 0x00, 0x00, 0xff },
	/* 0x11 */	{ 0x00, 0x00, 0x5f, 0xff },
	/* 0x12 */	{ 0x00, 0x00, 0x87, 0xff },
	/* 0x13 */	{ 0x00, 0x00, 0xaf, 0xff },
	/* 0x14 */	{ 0x00, 0x00, 0xd7, 0xff },
	/* 0x15 */	{ 0x00, 0x00, 0xff, 0xff },
	/* 0x16 */	{ 0x00, 0x5f, 0x00, 0xff },
	/* 0x17 */	{ 0x00, 0x5f, 0x5f, 0xff },
	/* 0x18 */	{ 0x00, 0x5f, 0x87, 0xff },
	/* 0x19 */	{ 0x00, 0x5f, 0xaf, 0xff },
	/* 0x1a */	{ 0x00, 0x5f, 0xd7, 0xff },
	/* 0x1b */	{ 0x00, 0x5f, 0xff, 0xff },
	/* 0x1c */	{ 0x00, 0x87, 0x00, 0xff },
	/* 0x1d */	{ 0x00, 0x87, 0x5f, 0xff },
	/* 0x1e */	{ 0x00, 0x87, 0x87, 0xff },
	/* 0x1f */	{ 0x00, 0x87, 0xaf, 0xff },
	/* 0x20 */	{ 0x00, 0x87, 0xd7, 0xff },
	/* 0x21 */	{ 0x00, 0x87, 0xff, 0xff },
	/* 0x22 */	{ 0x00, 0xaf, 0x00, 0xff },
	/* 0x23 */	{ 0x00, 0xaf, 0x5f, 0xff },
	/* 0x24 */	{ 0x00, 0xaf, 0x87, 0xff },
	/* 0x25 */	{ 0x00, 0xaf, 0xaf, 0xff },
	/* 0x26 */	{ 0x00, 0xaf, 0xd7, 0xff },
	/* 0x27 */	{ 0x00, 0xaf, 0xff, 0xff },
	/* 0x28 */	{ 0x00, 0xd7, 0x00, 0xff },
	/* 0x29 */	{ 0x00, 0xd7, 0x5f, 0xff },
	/* 0x2a */	{ 0x00, 0xd7, 0x87, 0xff },
	/* 0x2b */	{ 0x00, 0xd7, 0xaf, 0xff },
	/* 0x2c */	{ 0x00, 0xd7, 0xd7, 0xff },
	/* 0x2d */	{ 0x00, 0xd7, 0xff, 0xff },
	/* 0x2e */	{ 0x00, 0xff, 0x00, 0xff },
	/* 0x2f */	{ 0x00, 0xff, 0x5f, 0xff },
	/* 0x30 */	{ 0x00, 0xff, 0x87, 0xff },
	/* 0x31 */	{ 0x00, 0xff, 0xaf, 0xff },
	/* 0x32 */	{ 0x00, 0xff, 0xd7, 0xff },
	/* 0x33 */	{ 0x00, 0xff, 0xff, 0xff },
	/* 0x34 */	{ 0x5f, 0x00, 0x00, 0xff },
	/* 0x35 */	{ 0x5f, 0x00, 0x5f, 0xff },
	/* 0x36 */	{ 0x5f, 0x00, 0x87, 0xff },
	/* 0x37 */	{ 0x5f, 0x00, 0xaf, 0xff },
	/* 0x38 */	{ 0x5f, 0x00, 0xd7, 0xff },
	/* 0x39 */	{ 0x5f, 0x00, 0xff, 0xff },
	/* 0x3a */	{ 0x5f, 0x5f, 0x00, 0xff },
	/* 0x3b */	{ 0x5f, 0x5f, 0x5f, 0xff },
	/* 0x3c */	{ 0x5f, 0x5f, 0x87, 0xff },
	/* 0x3d */	{ 0x5f, 0x5f, 0xaf, 0xff },
	/* 0x3e */	{ 0x5f, 0x5f, 0xd7, 0xff },
	/* 0x3f */	{ 0x5f, 0x5f, 0xff, 0xff },
	/* 0x40 */	{ 0x5f, 0x87, 0x00, 0xff },
	/* 0x41 */	{ 0x5f, 0x87, 0x5f, 0xff },
	/* 0x42 */	{ 0x5f, 0x87, 0x87, 0xff },
	/* 0x43 */	{ 0x5f, 0x87, 0xaf, 0xff },
	/* 0x44 */	{ 0x5f, 0x87, 0xd7, 0xff },
	/* 0x45 */	{ 0x5f, 0x87, 0xff, 0xff },
	/* 0x46 */	{ 0x5f, 0xaf, 0x00, 0xff },
	/* 0x47 */	{ 0x5f, 0xaf, 0x5f, 0xff },
	/* 0x48 */	{ 0x5f, 0xaf, 0x87, 0xff },
	/* 0x49 */	{ 0x5f, 0xaf, 0xaf, 0xff },
	/* 0x4a */	{ 0x5f, 0xaf, 0xd7, 0xff },
	/* 0x4b */	{ 0x5f, 0xaf, 0xff, 0xff },
	/* 0x4c */	{ 0x5f, 0xd7, 0x00, 0xff },
	/* 0x4d */	{ 0x5f, 0xd7, 0x5f, 0xff },
	/* 0x4e */	{ 0x5f, 0xd7, 0x87, 0xff },
	/* 0x4f */	{ 0x5f, 0xd7, 0xaf, 0xff },
	/* 0x50 */	{ 0x5f, 0xd7, 0xd7, 0xff },
	/* 0x51 */	{ 0x5f, 0xd7, 0xff, 0xff },
	/* 0x52 */	{ 0x5f, 0xff, 0x00, 0xff },
	/* 0x53 */	{ 0x5f, 0xff, 0x5f, 0xff },
	/* 0x54 */	{ 0x5f, 0xff, 0x87, 0xff },
	/* 0x55 */	{ 0x5f, 0xff, 0xaf, 0xff },
	/* 0x56 */	{ 0x5f, 0xff, 0xd7, 0xff },
	/* 0x57 */	{ 0x5f, 0xff, 0xff, 0xff },
	/* 0x58 */	{ 0x87, 0x00, 0x00, 0xff },
	/* 0x59 */	{ 0x87, 0x00, 0x5f, 0xff },
	/* 0x5a */	{ 0x87, 0x00, 0x87, 0xff },
	/* 0x5b */	{ 0x87, 0x00, 0xaf, 0xff },
	/* 0x5c */	{ 0x87, 0x00, 0xd7, 0xff },
	/* 0x5d */	{ 0x87, 0x00, 0xff, 0xff },
	/* 0x5e */	{ 0x87, 0x5f, 0x00, 0xff },
	/* 0x5f */	{ 0x87, 0x5f, 0x5f, 0xff },
	/* 0x60 */	{ 0x87, 0x5f, 0x87, 0xff },
	/* 0x61 */	{ 0x87, 0x5f, 0xaf, 0xff },
	/* 0x62 */	{ 0x87, 0x5f, 0xd7, 0xff },
	/* 0x63 */	{ 0x87, 0x5f, 0xff, 0xff },
	/* 0x64 */	{ 0x87, 0x87, 0x00, 0xff },
	/* 0x65 */	{ 0x87, 0x87, 0x5f, 0xff },
	/* 0x66 */	{ 0x87, 0x87, 0x87, 0xff },
	/* 0x67 */	{ 0x87, 0x87, 0xaf, 0xff },
	/* 0x68 */	{ 0x87, 0x87, 0xd7, 0xff },
	/* 0x69 */	{ 0x87, 0x87, 0xff, 0xff },
	/* 0x6a */	{ 0x87, 0xaf, 0x00, 0xff },
	/* 0x6b */	{ 0x87, 0xaf, 0x5f, 0xff },
	/* 0x6c */	{ 0x87, 0xaf, 0x87, 0xff },
	/* 0x6d */	{ 0x87, 0xaf, 0xaf, 0xff },
	/* 0x6e */	{ 0x87, 0xaf, 0xd7, 0xff },
	/* 0x6f */	{ 0x87, 0xaf, 0xff, 0xff },
	/* 0x70 */	{ 0x87, 0xd7, 0x00, 0xff },
	/* 0x71 */	{ 0x87, 0xd7, 0x5f, 0xff },
	/* 0x72 */	{ 0x87, 0xd7, 0x87, 0xff },
	/* 0x73 */	{ 0x87, 0xd7, 0xaf, 0xff },
	/* 0x74 */	{ 0x87, 0xd7, 0xd7, 0xff },
	/* 0x75 */	{ 0x87, 0xd7, 0xff, 0xff },
	/* 0x76 */	{ 0x87, 0xff, 0x00, 0xff },
	/* 0x77 */	{ 0x87, 0xff, 0x5f, 0xff },
	/* 0x78 */	{ 0x87, 0xff, 0x87, 0xff },
	/* 0x79 */	{ 0x87, 0xff, 0xaf, 0xff },
	/* 0x7a */	{ 0x87, 0xff, 0xd7, 0xff },
	/* 0x7b */	{ 0x87, 0xff, 0xff, 0xff },
	/* 0x7c */	{ 0xaf, 0x00, 0x00, 0xff },
	/* 0x7d */	{ 0xaf, 0x00, 0x5f, 0xff },
	/* 0x7e */	{ 0xaf, 0x00, 0x87, 0xff },
	/* 0x7f */	{ 0xaf, 0x00, 0xaf, 0xff },
	/* 0x80 */	{ 0xaf, 0x00, 0xd7, 0xff },
	/* 0x81 */	{ 0xaf, 0x00, 0xff, 0xff },
	/* 0x82 */	{ 0xaf, 0x5f, 0x00, 0xff },
	/* 0x83 */	{ 0xaf, 0x5f, 0x5f, 0xff },
	/* 0x84 */	{ 0xaf, 0x5f, 0x87, 0xff },
	/* 0x85 */	{ 0xaf, 0x5f, 0xaf, 0xff },
	/* 0x86 */	{ 0xaf, 0x5f, 0xd7, 0xff },
	/* 0x87 */	{ 0xaf, 0x5f, 0xff, 0xff },
	/* 0x88 */	{ 0xaf, 0x87, 0x00, 0xff },
	/* 0x89 */	{ 0xaf, 0x87, 0x5f, 0xff },
	/* 0x8a */	{ 0xaf, 0x87, 0x87, 0xff },
	/* 0x8b */	{ 0xaf, 0x87, 0xaf, 0xff },
	/* 0x8c */	{ 0xaf, 0x87, 0xd7, 0xff },
	/* 0x8d */	{ 0xaf, 0x87, 0xff, 0xff },
	/* 0x8e */	{ 0xaf, 0xaf, 0x00, 0xff },
	/* 0x8f */	{ 0xaf, 0xaf, 0x5f, 0xff },
	/* 0x90 */	{ 0xaf, 0xaf, 0x87, 0xff },
	/* 0x91 */	{ 0xaf, 0xaf, 0xaf, 0xff },
	/* 0x92 */	{ 0xaf, 0xaf, 0xd7, 0xff },
	/* 0x93 */	{ 0xaf, 0xaf, 0xff, 0xff },
	/* 0x94 */	{ 0xaf, 0xd7, 0x00, 0xff },
	/* 0x95 */	{ 0xaf, 0xd7, 0x5f, 0xff },
	/* 0x96 */	{ 0xaf, 0xd7, 0x87, 0xff },
	/* 0x97 */	{ 0xaf, 0xd7, 0xaf, 0xff },
	/* 0x98 */	{ 0xaf, 0xd7, 0xd7, 0xff },
	/* 0x99 */	{ 0xaf, 0xd7, 0xff, 0xff },
	/* 0x9a */	{ 0xaf, 0xff, 0x00, 0xff },
	/* 0x9b */	{ 0xaf, 0xff, 0x5f, 0xff },
	/* 0x9c */	{ 0xaf, 0xff, 0x87, 0xff },
	/* 0x9d */	{ 0xaf, 0xff, 0xaf, 0xff },
	/* 0x9e */	{ 0xaf, 0xff, 0xd7, 0xff },
	/* 0x9f */	{ 0xaf, 0xff, 0xff, 0xff },
	/* 0xa0 */	{ 0xd7, 0x00, 0x00, 0xff },
	/* 0xa1 */	{ 0xd7, 0x00, 0x5f, 0xff },
	/* 0xa2 */	{ 0xd7, 0x00, 0x87, 0xff },
	/* 0xa3 */	{ 0xd7, 0x00, 0xaf, 0xff },
	/* 0xa4 */	{ 0xd7, 0x00, 0xd7, 0xff },
	/* 0xa5 */	{ 0xd7, 0x00, 0xff, 0xff },
	/* 0xa6 */	{ 0xd7, 0x5f, 0x00, 0xff },
	/* 0xa7 */	{ 0xd7, 0x5f, 0x5f, 0xff },
	/* 0xa8 */	{ 0xd7, 0x5f, 0x87, 0xff },
	/* 0xa9 */	{ 0xd7, 0x5f, 0xaf, 0xff },
	/* 0xaa */	{ 0xd7, 0x5f, 0xd7, 0xff },
	/* 0xab */	{ 0xd7, 0x5f, 0xff, 0xff },
	/* 0xac */	{ 0xd7, 0x87, 0x00, 0xff },
	/* 0xad */	{ 0xd7, 0x87, 0x5f, 0xff },
	/* 0xae */	{ 0xd7, 0x87, 0x87, 0xff },
	/* 0xaf */	{ 0xd7, 0x87, 0xaf, 0xff },
	/* 0xb0 */	{ 0xd7, 0x87, 0xd7, 0xff },
	/* 0xb1 */	{ 0xd7, 0x87, 0xff, 0xff },
	/* 0xb2 */	{ 0xd7, 0xaf, 0x00, 0xff },
	/* 0xb3 */	{ 0xd7, 0xaf, 0x5f, 0xff },
	/* 0xb4 */	{ 0xd7, 0xaf, 0x87, 0xff },
	/* 0xb5 */	{ 0xd7, 0xaf, 0xaf, 0xff },
	/* 0xb6 */	{ 0xd7, 0xaf, 0xd7, 0xff },
	/* 0xb7 */	{ 0xd7, 0xaf, 0xff, 0xff },
	/* 0xb8 */	{ 0xd7, 0xd7, 0x00, 0xff },
	/* 0xb9 */	{ 0xd7, 0xd7, 0x5f, 0xff },
	/* 0xba */	{ 0xd7, 0xd7, 0x87, 0xff },
	/* 0xbb */	{ 0xd7, 0xd7, 0xaf, 0xff },
	/* 0xbc */	{ 0xd7, 0xd7, 0xd7, 0xff },
	/* 0xbd */	{ 0xd7, 0xd7, 0xff, 0xff },
	/* 0xbe */	{ 0xd7, 0xff, 0x00, 0xff },
	/* 0xbf */	{ 0xd7, 0xff, 0x5f, 0xff },
	/* 0xc0 */	{ 0xd7, 0xff, 0x87, 0xff },
	/* 0xc1 */	{ 0xd7, 0xff, 0xaf, 0xff },
	/* 0xc2 */	{ 0xd7, 0xff, 0xd7, 0xff },
	/* 0xc3 */	{ 0xd7, 0xff, 0xff, 0xff },
	/* 0xc4 */	{ 0xff, 0x00, 0x00, 0xff },
	/* 0xc5 */	{ 0xff, 0x00, 0x5f, 0xff },
	/* 0xc6 */	{ 0xff, 0x00, 0x87, 0xff },
	/* 0xc7 */	{ 0xff, 0x00, 0xaf, 0xff },
	/* 0xc8 */	{ 0xff, 0x00, 0xd7, 0xff },
	/* 0xc9 */	{ 0xff, 0x00, 0xff, 0xff },
	/* 0xca */	{ 0xff, 0x5f, 0x00, 0xff },
	/* 0xcb */	{ 0xff, 0x5f, 0x5f, 0xff },
	/* 0xcc */	{ 0xff, 0x5f, 0x87, 0xff },
	/* 0xcd */	{ 0xff, 0x5f, 0xaf, 0xff },
	/* 0xce */	{ 0xff, 0x5f, 0xd7, 0xff },
	/* 0xcf */	{ 0xff, 0x5f, 0xff, 0xff },
	/* 0xd0 */	{ 0xff, 0x87, 0x00, 0xff },
	/* 0xd1 */	{ 0xff, 0x87, 0x5f, 0xff },
	/* 0xd2 */	{ 0xff, 0x87, 0x87, 0xff },
	/* 0xd3 */	{ 0xff, 0x87, 0xaf, 0xff },
	/* 0xd4 */	{ 0xff, 0x87, 0xd7, 0xff },
	/* 0xd5 */	{ 0xff, 0x87, 0xff, 0xff },
	/* 0xd6 */	{ 0xff, 0xaf, 0x00, 0xff },
	/* 0xd7 */	{ 0xff, 0xaf, 0x5f, 0xff },
	/* 0xd8 */	{ 0xff, 0xaf, 0x87, 0xff },
	/* 0xd9 */	{ 0xff, 0xaf, 0xaf, 0xff },
	/* 0xda */	{ 0xff, 0xaf, 0xd7, 0xff },
	/* 0xdb */	{ 0xff, 0xaf, 0xff, 0xff },
	/* 0xdc */	{ 0xff, 0xd7, 0x00, 0xff },
	/* 0xdd */	{ 0xff, 0xd7, 0x5f, 0xff },
	/* 0xde */	{ 0xff, 0xd7, 0x87, 0xff },
	/* 0xdf */	{ 0xff, 0xd7, 0xaf, 0xff },
	/* 0xe0 */	{ 0xff, 0xd7, 0xd7, 0xff },
	/* 0xe1 */	{ 0xff, 0xd7, 0xff, 0xff },
	/* 0xe2 */	{ 0xff, 0xff, 0x00, 0xff },
	/* 0xe3 */	{ 0xff, 0xff, 0x5f, 0xff },
	/* 0xe4 */	{ 0xff, 0xff, 0x87, 0xff },
	/* 0xe5 */	{ 0xff, 0xff, 0xaf, 0xff },
	/* 0xe6 */	{ 0xff, 0xff, 0xd7, 0xff },
	/* 0xe7 */	{ 0xff, 0xff, 0xff, 0xff },
	/* 0xe8 */	{ 0x08, 0x08, 0x08, 0xff },
	/* 0xe9 */	{ 0x12, 0x12, 0x12, 0xff },
	/* 0xea */	{ 0x1c, 0x1c, 0x1c, 0xff },
	/* 0xeb */	{ 0x26, 0x26, 0x26, 0xff },
	/* 0xec */	{ 0x30, 0x30, 0x30, 0xff },
	/* 0xed */	{ 0x3a, 0x3a, 0x3a, 0xff },
	/* 0xee */	{ 0x44, 0x44, 0x44, 0xff },
	/* 0xef */	{ 0x4e, 0x4e, 0x4e, 0xff },
	/* 0xf0 */	{ 0x58, 0x58, 0x58, 0xff },
	/* 0xf1 */	{ 0x62, 0x62, 0x62, 0xff },
	/* 0xf2 */	{ 0x6c, 0x6c, 0x6c, 0xff },
	/* 0xf3 */	{ 0x76, 0x76, 0x76, 0xff },
	/* 0xf4 */	{ 0x80, 0x80, 0x80, 0xff },
	/* 0xf5 */	{ 0x8a, 0x8a, 0x8a, 0xff },
	/* 0xf6 */	{ 0x94, 0x94, 0x94, 0xff },
	/* 0xf7 */	{ 0x9e, 0x9e, 0x9e, 0xff },
	/* 0xf8 */	{ 0xa8, 0xa8, 0xa8, 0xff },
	/* 0xf9 */	{ 0xb2, 0xb2, 0xb2, 0xff },
	/* 0xfa */	{ 0xbc, 0xbc, 0xbc, 0xff },
	/* 0xfb */	{ 0xc6, 0xc6, 0xc6, 0xff },
	/* 0xfc */	{ 0xd0, 0xd0, 0xd0, 0xff },
	/* 0xfd */	{ 0xda, 0xda, 0xda, 0xff },
	/* 0xfe */	{ 0xe4, 0xe4, 0xe4, 0xff },
	/* 0xff */	{ 0xee, 0xee, 0xee, 0xff }
};

static x_bool tcolor_to_rgba(enum tcolor c, x_u8 * r, x_u8 * g, x_u8 * b, x_u8 * a)
{
	x_u8 index = c;

	if (index > 0xff)
		index = 0;

	*r = tcolor_to_rgba_table[index][0];
	*g = tcolor_to_rgba_table[index][1];
	*b = tcolor_to_rgba_table[index][2];
	*a = tcolor_to_rgba_table[index][3];

	return TRUE;
}

/*
 * fb open
 */
static x_s32 fb_open(struct chrdev * dev)
{
	return 0;
}

/*
 * fb read
 */
static x_s32 fb_read(struct chrdev * dev, x_u8 * buf, x_s32 count)
{
	struct fb * fb = (struct fb *)(dev->driver);
	x_u8 * p = (x_u8 *)((x_u32)(fb->info->bitmap.data));
	x_s32 i;

	for(i = 0; i < count; i++)
	{
		buf[i] = p[i];
	}

	return i;
}

/*
 * fb write.
 */
static x_s32 fb_write(struct chrdev * dev, const x_u8 * buf, x_s32 count)
{
	struct fb * fb = (struct fb *)(dev->driver);
	x_u8 * p = (x_u8 *)((x_u32)(fb->info->bitmap.data));
	x_s32 i;

	for(i = 0; i < count; i++)
	{
		p[i] = buf[i];
	}

	return i;
}

/*
 * fb ioctl
 */
static x_s32 fb_ioctl(struct chrdev * dev, x_u32 cmd, void * arg)
{
	struct fb * fb = (struct fb *)(dev->driver);

	if(fb->ioctl)
		return ((fb->ioctl)(fb, cmd, arg));

	return -1;
}

/*
 * fb close
 */
static x_s32 fb_close(struct chrdev * dev)
{
	return 0;
}

/*
 * get console's width and height
 */
static x_bool fbcon_getwh(struct console * console, x_s32 * w, x_s32 * h)
{
	struct fb_console_info * info = console->priv;

	if(w)
		*w = info->w;

	if(h)
		*h = info->h;

	return TRUE;
}

/*
 * get cursor's position
 */
static x_bool fbcon_getxy(struct console * console, x_s32 * x, x_s32 * y)
{
	struct fb_console_info * info = console->priv;

	if(x)
		*x = info->x;

	if(y)
		*y = info->y;

	return TRUE;
}

/*
 * set cursor's position
 */
static x_bool fbcon_gotoxy(struct console * console, x_s32 x, x_s32 y)
{
	struct fb_console_info * info = console->priv;
	struct fbcon_cell * cell;
	x_s32 pos, px, py;

	if(x < 0)
		x = 0;
	if(y < 0)
		y = 0;

	if(x > info->w - 1)
		x = info->w - 1;
	if(y > info->h - 1)
		y = info->h - 1;

	if(info->cursor)
	{
		pos = info->w * info->y + info->x;
		cell = &(info->cell[pos]);
		px = (pos % info->w) * info->fw;
		py = (pos / info->w) * info->fh;
		fb_putcode(info->fb, cell->cp, cell->fc, cell->bc, px, py);

		pos = info->w * y + x;
		cell = &(info->cell[pos]);
		px = (pos % info->w) * info->fw;
		py = (pos / info->w) * info->fh;
		fb_putcode(info->fb, cell->cp, info->bc, info->fc, px, py);
	}

	info->x = x;
	info->y = y;

	return TRUE;
}

/*
 * turn on / off the cursor
 */
static x_bool fbcon_setcursor(struct console * console, x_bool on)
{
	struct fb_console_info * info = console->priv;
	struct fbcon_cell * cell;
	x_s32 pos, px, py;

	info->cursor = on;

	pos = info->w * info->y + info->x;
	cell = &(info->cell[pos]);
	px = (pos % info->w) * info->fw;
	py = (pos / info->w) * info->fh;

	if(info->cursor)
		fb_putcode(info->fb, cell->cp, info->bc, info->fc, px, py);
	else
		fb_putcode(info->fb, cell->cp, cell->fc, cell->bc, px, py);

	return TRUE;
}

/*
 * get cursor's status
 */
static x_bool fbcon_getcursor(struct console * console)
{
	struct fb_console_info * info = console->priv;

	return info->cursor;
}

/*
 * set console's front color and background color
 */
static x_bool fbcon_setcolor(struct console * console, enum tcolor f, enum tcolor b)
{
	struct fb_console_info * info = console->priv;
	x_u8 cr, cg, cb, ca;

	info->f = f;
	info->b = b;

	tcolor_to_rgba(f, &cr, &cg, &cb, &ca);
	info->fc = fb_map_color(info->fb, cr, cg, cb, ca);

	tcolor_to_rgba(b, &cr, &cg, &cb, &ca);
	info->bc = fb_map_color(info->fb, cr, cg, cb, ca);

	return TRUE;
}

/*
 * get console front color and background color
 */
static x_bool fbcon_getcolor(struct console * console, enum tcolor * f, enum tcolor * b)
{
	struct fb_console_info * info = console->priv;

	*f = info->f;
	*b = info->b;

	return TRUE;
}

/*
 * clear screen
 */
static x_bool fbcon_cls(struct console * console)
{
	struct fb_console_info * info = console->priv;
	struct fbcon_cell * cell = &(info->cell[0]);
	x_s32 i;

	for(i = 0; i < info->clen; i++)
	{
		cell->cp = UNICODE_SPACE;
		cell->fc = info->fc;
		cell->bc = info->bc;
		cell->w = 1;

		cell++;
	}

	fb_fill_rect(info->fb, info->bc, 0, 0, info->w * info->fw, info->h * info->fh);
	fbcon_gotoxy(console, 0, 0);

	return TRUE;
}

/*
 * scroll up
 */
static x_bool fbcon_scrollup(struct console * console)
{
	struct fb_console_info * info = console->priv;
	struct fbcon_cell * p, * q;
	x_s32 px, py;
	x_s32 m, l;
	x_s32 i, w;

	l = info->w;
	m = info->clen - l;
	p = &(info->cell[0]);
	q = &(info->cell[l]);

	for(i = 0, w = 1; i < m; i += w)
	{
		if( (p->cp != q->cp) || (p->fc != q->fc) || (p->bc != q->bc) || (p->w != q->w))
		{
			p->cp = q->cp;
			p->fc = q->fc;
			p->bc = q->bc;
			p->w = q->w;

			px = (i % info->w) * info->fw;
			py = (i / info->w) * info->fh;
			fb_putcode(info->fb, p->cp, p->fc, p->bc, px, py);
		}

		if( (w = q->w) < 1 )
			w = 1;
		p += w;
		q += w;
	}

	while( (l--) > 0 )
	{
		if( (p->w == 0) || (p->cp != UNICODE_SPACE) || (p->fc != info->fc) || (p->bc != info->bc) )
		{
			p->cp = UNICODE_SPACE;
			p->fc = info->fc;
			p->bc = info->bc;
			p->w = 1;

			px = (i % info->w) * info->fw;
			py = (i / info->w) * info->fh;
			fb_putcode(info->fb, p->cp, p->fc, p->bc, px, py);
		}

		p++;
		i++;
	}

	fbcon_gotoxy(console, info->x, info->y - 1);
	return TRUE;
}

/*
 * put a unicode character
 */
x_bool fbcon_putcode(struct console * console, x_u32 code)
{
	struct fb_console_info * info = console->priv;
	struct fbcon_cell * cell;
	x_s32 pos, px, py;
	x_s32 w, i;

	switch(code)
	{
	case UNICODE_BS:
		return TRUE;

	case UNICODE_TAB:
		i = 8 - (info->x % 8);
		if(i + info->x >= info->w)
			i = info->w - info->x - 1;

		while(i--)
		{
			pos = info->w * info->y + info->x;
			cell = &(info->cell[pos]);

			cell->cp = UNICODE_SPACE;
			cell->fc = info->fc;
			cell->bc = info->bc;
			cell->w = 1;

			px = (pos % info->w) * info->fw;
			py = (pos / info->w) * info->fh;
			fb_putcode(info->fb, cell->cp, cell->fc, cell->bc, px, py);
			info->x = info->x + 1;
		}
		fbcon_gotoxy(console, info->x, info->y);
		break;

	case UNICODE_LF:
		if(info->y + 1 >= info->h)
			fbcon_scrollup(console);
		fbcon_gotoxy(console, info->x, info->y + 1);
		break;

	case UNICODE_CR:
		fbcon_gotoxy(console, 0, info->y);
		break;

	default:
		w = ucs4_width(code);
		if(w <= 0)
			return TRUE;

		pos = info->w * info->y + info->x;
		cell = &(info->cell[pos]);

		cell->cp = code;
		cell->fc = info->fc;
		cell->bc = info->bc;
		cell->w = w;

		for(i = 1; i < w; i++)
		{
			((struct fbcon_cell *)(cell + i))->cp = UNICODE_SPACE;
			((struct fbcon_cell *)(cell + i))->fc = info->fc;
			((struct fbcon_cell *)(cell + i))->bc = info->bc;
			((struct fbcon_cell *)(cell + i))->w = 0;
		}

		px = (pos % info->w) * info->fw;
		py = (pos / info->w) * info->fh;
		fb_putcode(info->fb, cell->cp, cell->fc, cell->bc, px, py);

		if(info->x + w < info->w)
			fbcon_gotoxy(console, info->x + w, info->y);
		else
		{
			if(info->y + 1 >= info->h)
				fbcon_scrollup(console);
			fbcon_gotoxy(console, 0, info->y + 1);
		}
		break;
	}

	return TRUE;
}

/*
 * search framebuffer by name.
 */
struct fb * search_framebuffer(const char * name)
{
	struct fb * fb;
	struct chrdev * dev;

	dev = search_chrdev(name);
	if(!dev)
		return NULL;

	if(dev->type != CHR_DEV_FRAMEBUFFER)
		return NULL;

	fb = (struct fb *)dev->driver;

	return fb;
}

/*
 * register framebuffer driver.
 */
x_bool register_framebuffer(struct fb * fb)
{
	struct chrdev * dev;
	struct console * console;
	struct fb_console_info * info;
	x_u8 r, g, b, a;
	x_u8 brightness;

	if(!fb || !fb->info || !fb->info->name)
		return FALSE;

	dev = malloc(sizeof(struct chrdev));
	if(!dev)
		return FALSE;

	dev->name		= fb->info->name;
	dev->type		= CHR_DEV_FRAMEBUFFER;
	dev->open 		= fb_open;
	dev->read 		= fb_read;
	dev->write 		= fb_write;
	dev->ioctl 		= fb_ioctl;
	dev->close		= fb_close;
	dev->driver 	= fb;

	if(!register_chrdev(dev))
	{
		free(dev);
		return FALSE;
	}

	if(search_chrdev_with_type(dev->name, CHR_DEV_FRAMEBUFFER) == NULL)
	{
		unregister_chrdev(dev->name);
		free(dev);
		return FALSE;
	}

	if(fb->init)
		(fb->init)(fb);

	display_logo(fb);

	if(fb->ioctl)
	{
		brightness = 0xff;
		(fb->ioctl)(fb, IOCTL_SET_FB_BACKLIGHT, &brightness);
	}

	/*
	 * register a console
	 */
	console = malloc(sizeof(struct console));
	info = malloc(sizeof(struct fb_console_info));
	if(!console || !info)
	{
		unregister_chrdev(dev->name);
		free(dev);
		free(console);
		free(info);
		return FALSE;
	}

	info->name = (char *)fb->info->name;
	info->fb = fb;
	info->fw = 8;
	info->fh = 16;
	info->w = fb->info->bitmap.info.width / info->fw;
	info->h = fb->info->bitmap.info.height / info->fh;
	info->x = 0;
	info->y = 0;
	info->f = TCOLOR_WHITE;
	info->b = TCOLOR_BLACK;
	tcolor_to_rgba(info->f, &r, &g, &b, &a);
	info->fc = fb_map_color(fb, r, g, b, a);
	tcolor_to_rgba(info->b, &r, &g, &b, &a);
	info->bc = fb_map_color(fb, r, g, b, a);
	info->cursor = TRUE;
	info->clen = info->w * info->h;
	info->cell = malloc(info->clen * sizeof(struct fbcon_cell));
	if(!info->cell)
	{
		unregister_chrdev(dev->name);
		free(dev);
		free(console);
		free(info);
		return FALSE;
	}
	memset(info->cell, 0, info->clen * sizeof(struct fbcon_cell));

	console->name = info->name;
	console->getwh = fbcon_getwh;
	console->getxy = fbcon_getxy;
	console->gotoxy = fbcon_gotoxy;
	console->setcursor = fbcon_setcursor;
	console->getcursor = fbcon_getcursor;
	console->setcolor = fbcon_setcolor;
	console->getcolor = fbcon_getcolor;
	console->cls = fbcon_cls;
	console->getcode = NULL;
	console->putcode = fbcon_putcode;
	console->priv = info;

	if(!register_console(console))
	{
		unregister_chrdev(dev->name);
		free(dev);
		free(console);
		free(info->cell);
		free(info);
		return FALSE;
	}

	return TRUE;
}

/*
 * unregister framebuffer driver
 */
x_bool unregister_framebuffer(struct fb * fb)
{
	struct chrdev * dev;
	struct console * console;
	struct fb_console_info * info;
	struct fb * driver;
	x_u8 brightness;

	if(!fb || !fb->info || !fb->info->name)
		return FALSE;

	dev = search_chrdev_with_type(fb->info->name, CHR_DEV_FRAMEBUFFER);
	if(!dev)
		return FALSE;

	console = search_console((char *)fb->info->name);
	if(console)
		info = (struct fb_console_info *)console->priv;
	else
		return FALSE;

	driver = (struct fb *)(dev->driver);
	if(driver)
	{
		if(driver->ioctl)
		{
			brightness = 0x00;
			(driver->ioctl)(driver, IOCTL_SET_FB_BACKLIGHT, &brightness);
		}

		if(driver->exit)
			(driver->exit)(driver);
	}

	if(!unregister_console(console))
		return FALSE;

	if(!unregister_chrdev(dev->name))
		return FALSE;

	free(info->cell);
	free(info);
	free(console);
	free(dev);

	return TRUE;
}
