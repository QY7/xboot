/*
 * kernel/graphic/software/sw_fill_rects.c
 *
 * Copyright (c) 2007-2011  jianjun jiang <jerryjianjun@gmail.com>
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

#include <graphic/software.h>

static void surface_fill_rect_1byte(struct surface_t * surface,
		const struct rect_t * rect, u32_t c)
{
	u8_t * p, *q;
	u8_t * t;
	u32_t len, skip;
	u32_t i;
	u8_t fill = (u8_t) (c & 0xff);

	u32_t x = rect->x;
	u32_t y = rect->y;
	u32_t w = rect->w;
	u32_t h = rect->h;

	len = surface->info.bytes_per_pixel * w;
	skip = surface->pitch;
	t = (u8_t *) (surface->pixels + y * surface->pitch + x
			* surface->info.bytes_per_pixel);
	p = q = (u8_t *) t;

	for (i = 0; i < w; i++)
		*t++ = fill;

	for (i = 1; i < h; i++)
	{
		q += skip;
		memcpy(q, p, len);
	}
}

static void surface_fill_rect_2byte(struct surface_t * surface,
		const struct rect_t * rect, u32_t c)
{
	u8_t * p, *q;
	u16_t * t;
	u32_t len, skip;
	u32_t i;
	u16_t fill = (u16_t)(c & 0xffff);

	u32_t x = rect->x;
	u32_t y = rect->y;
	u32_t w = rect->w;
	u32_t h = rect->h;

	len = surface->info.bytes_per_pixel * w;
	skip = surface->pitch;
	t = (u16_t *) (surface->pixels + y * surface->pitch + x
			* surface->info.bytes_per_pixel);
	p = q = (u8_t *) t;

	for (i = 0; i < w; i++)
		*t++ = fill;

	for (i = 1; i < h; i++)
	{
		q += skip;
		memcpy(q, p, len);
	}
}

static void surface_fill_rect_3byte(struct surface_t * surface,
		const struct rect_t * rect, u32_t c)
{
	u8_t * p, *q;
	u8_t * t;
	u32_t len, skip;
	u32_t i;
	u8_t fill0 = (u8_t) ((c >> 0) & 0xff);
	u8_t fill1 = (u8_t) ((c >> 8) & 0xff);
	u8_t fill2 = (u8_t) ((c >> 16) & 0xff);

	u32_t x = rect->x;
	u32_t y = rect->y;
	u32_t w = rect->w;
	u32_t h = rect->h;

	len = surface->info.bytes_per_pixel * w;
	skip = surface->pitch;
	t = (u8_t *) (surface->pixels + y * surface->pitch + x
			* surface->info.bytes_per_pixel);
	p = q = (u8_t *) t;

	for (i = 0; i < w; i++)
	{
#if (__BYTE_ORDER == __BIG_ENDIAN)
		*t++ = fill2;
		*t++ = fill1;
		*t++ = fill0;
#else
		*t++ = fill0;
		*t++ = fill1;
		*t++ = fill2;
#endif
	}

	for (i = 1; i < h; i++)
	{
		q += skip;
		memcpy(q, p, len);
	}
}

static void surface_fill_rect_4byte(struct surface_t * surface,
		const struct rect_t * rect, u32_t c)
{
	u8_t * p, *q;
	u32_t * t;
	u32_t len, skip;
	u32_t i;

	u32_t x = rect->x;
	u32_t y = rect->y;
	u32_t w = rect->w;
	u32_t h = rect->h;

	len = surface->info.bytes_per_pixel * w;
	skip = surface->pitch;
	t = (u32_t *) (surface->pixels + y * surface->pitch + x
			* surface->info.bytes_per_pixel);
	p = q = (u8_t *) t;

	for (i = 0; i < w; i++)
		*t++ = c;

	for (i = 1; i < h; i++)
	{
		q += skip;
		memcpy(q, p, len);
	}
}

bool_t software_fill_rects(struct surface_t * surface,
		const struct rect_t * rects, u32_t count, u32_t c)
{
	struct rect_t clipped;
	u32_t i;

	if (!surface)
		return FALSE;

	if (!surface->pixels)
		return FALSE;

	if (surface->info.bits_per_pixel < 8)
		return FALSE;

	if (!rects)
		return FALSE;

	for (i = 0; i < count; i++)
	{
		if (rect_intersect(&rects[i], &surface->clip, &clipped))
		{
			switch (surface->info.bytes_per_pixel)
			{
			case 1:
				surface_fill_rect_1byte(surface, &clipped, c);
				break;

			case 2:
				surface_fill_rect_2byte(surface, &clipped, c);
				break;

			case 3:
				surface_fill_rect_3byte(surface, &clipped, c);
				break;

			case 4:
				surface_fill_rect_4byte(surface, &clipped, c);
				break;

			default:
				break;
			}
		}
	}

	return TRUE;
}
