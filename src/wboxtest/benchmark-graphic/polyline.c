/*
 * wboxtest/benchmark-graphic/polyline.c
 */

#include <wboxtest.h>

struct wbt_polyline_pdata_t
{
	struct window_t * w;

	ktime_t t1;
	ktime_t t2;
	int calls;
};

static void * polyline_setup(struct wboxtest_t * wbt)
{
	struct wbt_polyline_pdata_t * pdat;

	pdat = malloc(sizeof(struct wbt_polyline_pdata_t));
	if(!pdat)
		return NULL;

	pdat->w = window_alloc(NULL, NULL);
	if(!pdat->w)
	{
		free(pdat);
		return NULL;
	}
	return pdat;
}

static void polyline_clean(struct wboxtest_t * wbt, void * data)
{
	struct wbt_polyline_pdata_t * pdat = (struct wbt_polyline_pdata_t *)data;

	if(pdat)
	{
		window_free(pdat->w);
		free(pdat);
	}
}

static void polyline_run(struct wboxtest_t * wbt, void * data)
{
	struct wbt_polyline_pdata_t * pdat = (struct wbt_polyline_pdata_t *)data;
	struct surface_t * s = pdat->w->s;

	if(pdat)
	{
		srand(0);
		pdat->calls = 0;
		pdat->t2 = pdat->t1 = ktime_get();
		do {
			int x[10], y[10];
			int n = wboxtest_random_int(2, 10);
			for(int i = 0; i < n; i++)
			{
				x[i] = wboxtest_random_int(0, surface_get_width(s));
				y[i] = wboxtest_random_int(0, surface_get_height(s));
			}
			struct color_t c;
			color_init(&c, rand() & 0xff, rand() & 0xff, rand() & 0xff, 255);
			int thickness = wboxtest_random_int(0, 50);
			surface_shape_save(s);
			surface_shape_move_to(s, x[0], y[0]);
			for(int i = 1; i < n; i++)
				surface_shape_line_to(s, x[i], y[i]);
			surface_shape_set_source_color(s, &c);
			if(thickness > 0)
			{
				surface_shape_set_line_width(s, thickness);
				surface_shape_stroke(s);
			}
			else
			{
				surface_shape_fill(s);
			}
			surface_shape_restore(s);
			pdat->calls++;
			pdat->t2 = ktime_get();
		} while(ktime_before(pdat->t2, ktime_add_ms(pdat->t1, 2000)));
		wboxtest_print(" Counts: %g\r\n", (double)(pdat->calls * 1000.0) / ktime_ms_delta(pdat->t2, pdat->t1));
	}
}

static struct wboxtest_t wbt_polyline = {
	.group	= "benchmark-graphic",
	.name	= "polyline",
	.setup	= polyline_setup,
	.clean	= polyline_clean,
	.run	= polyline_run,
};

static __init void polyline_wbt_init(void)
{
	register_wboxtest(&wbt_polyline);
}

static __exit void polyline_wbt_exit(void)
{
	unregister_wboxtest(&wbt_polyline);
}

wboxtest_initcall(polyline_wbt_init);
wboxtest_exitcall(polyline_wbt_exit);
