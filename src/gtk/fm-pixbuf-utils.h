/*
 *      fm-pixbuf-utils.h
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


#ifndef __FM_PIXBUF_UTILS_H__
#define __FM_PIXBUF_UTILS_H__

#include <gtk/gtk.h>

G_BEGIN_DECLS

GdkPixbuf * fm_pixbuf_create_colorized(GdkPixbuf * src, GdkColor * new_color);

G_END_DECLS

#endif
