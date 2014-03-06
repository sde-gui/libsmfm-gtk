/*
 *      fm-pixbuf-utils.c
 *
 *      Copyright 2014 vadim Ushakov <igeekless@gmail.com>
 *
 *      This program is free software; you can redistribute it and/or modify
 *      it under the terms of the GNU General Public License as published by
 *      the Free Software Foundation; either version 2 of the License, or
 *      (at your option) any later version.
 *
 *      This program is distributed in the hope that it will be useful,
 *      but WITHOUT ANY WARRANTY; without even the implied warranty of
 *      MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *      GNU General Public License for more details.
 *
 *      You should have received a copy of the GNU General Public License
 *      along with this program; if not, write to the Free Software
 *      Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston,
 *      MA 02110-1301, USA.
 */


#include "fm-pixbuf-utils.h"

/*****************************************************************************/

static GHashTable * surface_cache;

static void maintain_layout_cache(void)
{
    if (0 /* FIXME: should be value from config */)
    {
        if (surface_cache)
        {
            g_hash_table_unref(surface_cache);
            surface_cache = NULL;
        }
        return;
    }

    if (!surface_cache)
    {
        surface_cache = g_hash_table_new_full(
            (GHashFunc) g_direct_hash,
            (GEqualFunc) g_direct_equal,
            (GDestroyNotify) g_object_unref,
            (GDestroyNotify) cairo_surface_destroy);
    }
    else
    {
        if (g_hash_table_size(surface_cache) > 600)
            g_hash_table_remove_all(surface_cache);
    }
}

/*****************************************************************************/

GdkPixbuf * fm_pixbuf_create_colorized(GdkPixbuf * src, GdkColor * new_color)
{
    gint i, j;
    gint width, height, has_alpha, src_row_stride, dst_row_stride;
    gint red_value, green_value, blue_value;
    guchar *target_pixels;
    guchar *original_pixels;
    guchar *pixsrc;
    guchar *pixdest;
    GdkPixbuf *dest;

    red_value = new_color->red / 255.0;
    green_value = new_color->green / 255.0;
    blue_value = new_color->blue / 255.0;

    dest = gdk_pixbuf_new(
        gdk_pixbuf_get_colorspace (src),
        gdk_pixbuf_get_has_alpha (src),
        gdk_pixbuf_get_bits_per_sample (src),
        gdk_pixbuf_get_width (src),
        gdk_pixbuf_get_height (src)
    );

    has_alpha = gdk_pixbuf_get_has_alpha (src);
    width = gdk_pixbuf_get_width (src);
    height = gdk_pixbuf_get_height (src);
    src_row_stride = gdk_pixbuf_get_rowstride (src);
    dst_row_stride = gdk_pixbuf_get_rowstride (dest);
    target_pixels = gdk_pixbuf_get_pixels (dest);
    original_pixels = gdk_pixbuf_get_pixels (src);

    for (i = 0; i < height; i++)
    {
        pixdest = target_pixels + i * dst_row_stride;
        pixsrc = original_pixels + i * src_row_stride;
        for (j = 0; j < width; j++)
        {
            *pixdest++ = (*pixsrc++ * red_value) >> 8;
            *pixdest++ = (*pixsrc++ * green_value) >> 8;
            *pixdest++ = (*pixsrc++ * blue_value) >> 8;
            if (has_alpha)
            {
                *pixdest++ = *pixsrc++;
            }
        }
    }

    return dest;
}


cairo_surface_t * fm_cairo_surface_from_pixbuf(GdkPixbuf * pixbuf, gboolean allow_caching)
{
    if (!pixbuf)
        return NULL;

    gint width = gdk_pixbuf_get_width (pixbuf);
    gint height = gdk_pixbuf_get_height (pixbuf);

    if (allow_caching)
    {
        maintain_layout_cache();
        cairo_surface_t * cached_surface = (cairo_surface_t *) g_hash_table_lookup(surface_cache, pixbuf);
        if (cached_surface &&
            cairo_image_surface_get_width(cached_surface) == width &&
            cairo_image_surface_get_height(cached_surface) == height)
        {
            return cairo_surface_reference(cached_surface);
        }
    }

    guchar * gdk_pixels = gdk_pixbuf_get_pixels (pixbuf);
    int gdk_rowstride = gdk_pixbuf_get_rowstride (pixbuf);
    int n_channels = gdk_pixbuf_get_n_channels (pixbuf);

    cairo_surface_t * surface;
    int cairo_stride;
    guchar * cairo_pixels;
    cairo_format_t format;
    static const cairo_user_data_key_t key;
    int j;

    if (n_channels == 3)
        format = CAIRO_FORMAT_RGB24;
    else
        format = CAIRO_FORMAT_ARGB32;

    cairo_stride = cairo_format_stride_for_width(format, width);
    cairo_pixels = g_malloc(height * cairo_stride);
    surface = cairo_image_surface_create_for_data(
        (unsigned char *) cairo_pixels,
        format,
        width, height, cairo_stride
    );

    cairo_surface_set_user_data(surface, &key, cairo_pixels, (cairo_destroy_func_t) g_free);

    for (j = height; j; j--)
    {
        guchar * p = gdk_pixels;
        guchar * q = cairo_pixels;

        if (n_channels == 3)
        {
            guchar * end = p + 3 * width;

            while (p < end)
            {
#if G_BYTE_ORDER == G_LITTLE_ENDIAN
                q[0] = p[2];
                q[1] = p[1];
                q[2] = p[0];
#else
                q[1] = p[0];
                q[2] = p[1];
                q[3] = p[2];
#endif
                p += 3;
                q += 4;
            }
        }
        else
        {
            guchar * end = p + 4 * width;
            guint t1, t2, t3;

#define MULT(d,c,a,t) G_STMT_START { t = c * a + 0x7f; d = ((t >> 8) + t) >> 8; } G_STMT_END

            while (p < end)
            {
#if G_BYTE_ORDER == G_LITTLE_ENDIAN
                MULT(q[0], p[2], p[3], t1);
                MULT(q[1], p[1], p[3], t2);
                MULT(q[2], p[0], p[3], t3);
                q[3] = p[3];
#else
                q[0] = p[3];
                MULT(q[1], p[0], p[3], t1);
                MULT(q[2], p[1], p[3], t2);
                MULT(q[3], p[2], p[3], t3);
#endif
                p += 4;
                q += 4;
            }
#undef MULT
        }

        gdk_pixels += gdk_rowstride;
        cairo_pixels += cairo_stride;
    }

    if (allow_caching)
    {
        g_hash_table_insert(surface_cache, g_object_ref(pixbuf), cairo_surface_reference(surface));
    }

    return surface;
}


void fm_cairo_set_source_pixbuf(cairo_t         *cr,
                                GdkPixbuf       *pixbuf,
                                double           pixbuf_x,
                                double           pixbuf_y,
                                gboolean         allow_caching)
{
    if (!pixbuf || !cr)
        return;
    cairo_surface_t * surface = fm_cairo_surface_from_pixbuf(pixbuf, allow_caching);
    cairo_set_source_surface(cr, surface, pixbuf_x, pixbuf_y);
    cairo_surface_destroy(surface);
}

