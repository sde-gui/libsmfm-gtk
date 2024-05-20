/*
 *      fm-app-chooser-dlg.c
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

/**
 * SECTION:fm-app-utils
 * @short_description: Additional functions for handling GAppInfo
 * @title: GAppInfo utility functions
 *
 * @include: libfm/fm-gtk.h
 *
 */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include "fm-app-utils.h"
#include <gio/gio.h>
#include <gio/gdesktopappinfo.h>

#define ENABLE_CORRECTIONS 1
#define ENABLE_GUESSING    1

#define str_eq(s1, s2) (g_strcmp0((s1), (s2)) == 0)
#define str_contains(s1, s2) (s1 && strstr(s1, s2))
#define str_prefixed(s1, s2) (s1 && g_str_has_prefix(s1, s2))

static const char * guess_categories_for_app(GAppInfo* app)
{
    GDesktopAppInfo * desktop_app = G_DESKTOP_APP_INFO(app);
    if (!desktop_app)
        return NULL;

    const char * categories = NULL;

    char * mime_type = g_desktop_app_info_get_string(desktop_app, "MimeType");
    char * generic_name = g_desktop_app_info_get_string(desktop_app, "GenericName");

    if (mime_type && generic_name)
    {
        if (strstr(mime_type, "image/") && strstr(generic_name, "Image Viewer"))
        {
            categories = "Graphics;Viewer;";
        }
    }

    g_free(mime_type);
    g_free(generic_name);

    return categories;
}

/**
 * fm_app_utils_get_app_categories
 * @app: a GAppInfo
 *
 * Retrieves list of categories for @app, possibly with corrections made for
 * known to be incorrect values.
 *
 * This function is similar to g_desktop_app_info_get_categories(), except that
 * it can override the return value for some applications according to the
 * built-in list of corrections.
 *
 * Returns: (nullable): unparsed value of Categories from the .desktop file, if any.
 *
 * Since: 1.2.0
 */
const char * fm_app_utils_get_app_categories(GAppInfo* app)
{
    if (!app)
        return NULL;
    GDesktopAppInfo * desktop_app = G_DESKTOP_APP_INFO(app);
    if (!desktop_app)
        return NULL;

    const char * categories = g_desktop_app_info_get_categories(desktop_app);
    const char * filename = g_desktop_app_info_get_filename(desktop_app);
    const char * corrected_categories = NULL;

    g_debug("app: [%s], filename: [%s], categories: [%s]",
        g_app_info_get_name(app),
        filename ? filename : "(NULL)",
        categories ? categories : "(NULL)");

    if (categories != NULL && ENABLE_CORRECTIONS)
    {
        const char * id = g_app_info_get_id(app);
        char * mime_type = g_desktop_app_info_get_string(desktop_app, "MimeType");
        char * generic_name = g_desktop_app_info_get_string(desktop_app, "GenericName");

        /*
            Xfi reports "Utility;Viewer;", not "Graphics;Utility;Viewer;".
        */
        if (str_eq(categories, "Utility;Viewer;") &&
            str_contains(generic_name, "Image Viewer") &&
            str_contains(mime_type, "image/"))
            corrected_categories = "Graphics;Utility;Viewer;";
        /*
            KISS Player reports "Audio;Multimedia;Player;", which is contrary
            to the Desktop Menu Specification.
            1. The main category is missing. It should be AudioVideo.
            2. There's no Multimedia category.
        */
        else if (str_eq(categories, "Audio;Multimedia;Player;"))
            corrected_categories = "Audio;AudioVideo;Player;";
        /*
            MuPDF reports "Viewer;Graphics;" while most other PDF viewers include
            the "Office" category as well.
        */
        else if (str_prefixed(id, "mupdf") && str_eq(categories, "Viewer;Graphics;"))
            corrected_categories = "Viewer;Graphics;Office;";

        g_free(mime_type);
        g_free(generic_name);
    }

    if (corrected_categories)
    {
        g_debug("corrected categories: [%s]", corrected_categories);
        categories = corrected_categories;
    }

    if (categories == NULL && ENABLE_GUESSING)
    {
        categories = guess_categories_for_app(app);
        if (categories != NULL)
            g_debug("guessed categories: [%s]", categories);
    }

    return categories;
}
