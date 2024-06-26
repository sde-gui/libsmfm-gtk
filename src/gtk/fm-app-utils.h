/*
 *      fm-app-utils.h
 *
 *      Copyright 2024 Vadim Ushakov <wandrien.dev@gmail.com>
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


#ifndef __FM_APP_UTILS_H__
#define __FM_APP_UTILS_H__

#include <gio/gio.h>

G_BEGIN_DECLS

const char * fm_app_utils_get_app_categories(GAppInfo * app);

G_END_DECLS

#endif /* __FM_APP_UTILS_H__ */
