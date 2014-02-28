/*
 *      fm-icon-pixbuf.c
 *      
 *      Copyright 2009 Hong Jen Yee (PCMan) <pcman.tw@gmail.com>
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
 * SECTION:fm-icon-pixbuf
 * @short_description: An icon image creator.
 * @title: Icon image
 *
 * @include: libfm/fm-gtk.h
 *
 */

#include "fm-icon-pixbuf.h"

//static gboolean init = FALSE;
static guint changed_handler = 0;

typedef struct _PixEntry
{
    int size;
    GdkPixbuf* pix;
}PixEntry;

static void destroy_pixbufs(gpointer data)
{
    GSList* pixs = (GSList*)data;
    GSList* l;
    gdk_threads_enter(); /* FIXME: is this needed? */
    for(l = pixs; l; l=l->next)
    {
        PixEntry* ent = (PixEntry*)l->data;
        if(G_LIKELY(ent->pix))
            g_object_unref(ent->pix);
        g_slice_free(PixEntry, ent);
    }
    gdk_threads_leave();
    g_slist_free(pixs);
}


static gint * _fm_icon_get_sizes(FmIcon * icon)
{
    gint * sizes = NULL;

    if (icon && icon->gicon && G_IS_THEMED_ICON(icon->gicon))
    {
        const gchar * const * names = g_themed_icon_get_names(G_THEMED_ICON(icon->gicon));

        size_t sizes_size = 32;
        sizes = g_malloc0(sizeof(gint) * sizes_size);

        size_t i;
        for (i = 0; names[i]; i++)
        {
            const gchar * icon_name = names[i];
            gint * sizes_for_name = gtk_icon_theme_get_icon_sizes(gtk_icon_theme_get_default(), icon_name);

            size_t j1, j2;
            for (j1 = 0; sizes_for_name[j1]; j1++)
            {
                for (j2 = 0; j2 < sizes_size - 1; j2++)
                {
                    if (sizes[j2] == 0)
                        sizes[j2] = sizes_for_name[j1];

                    if (sizes[j2] == sizes_for_name[j1])
                        break;
                }
            }

            g_free(sizes_for_name);
        }
    }

    if (sizes && *sizes)
        return sizes;

    sizes = g_realloc(sizes, sizeof(gint) * 4);
    sizes[0] = 16;
    sizes[1] = 32;
    sizes[2] = 48;
    sizes[3] = 0;

    return sizes;
}



/**
 * fm_pixbuf_from_icon
 * @icon: icon descriptor
 * @size: size in pixels
 *
 * Creates a #GdkPixbuf and draws icon there.
 *
 * Before 1.0.0 this call had name fm_icon_get_pixbuf.
 *
 * Returns: (transfer full): an image.
 *
 * Since: 0.1.0
 */
GdkPixbuf* fm_pixbuf_from_icon(FmIcon* icon, int size)
{
    GtkIconInfo* ii;
    GdkPixbuf* pix;
    GSList *pixs, *l;
    PixEntry* ent;

    if (!icon)
        return NULL;

    pixs = (GSList*)fm_icon_get_user_data(icon);
    for( l = pixs; l; l=l->next )
    {
        ent = (PixEntry*)l->data;
        if(ent->size == size) /* cached pixbuf is found! */
        {
            return ent->pix ? GDK_PIXBUF(g_object_ref(ent->pix)) : NULL;
        }
    }

    /* not found! load the icon from disk */
    ii = gtk_icon_theme_lookup_by_gicon(gtk_icon_theme_get_default(), icon->gicon, size, GTK_ICON_LOOKUP_FORCE_SIZE);
    if(ii)
    {
        pix = gtk_icon_info_load_icon(ii, NULL);
        gtk_icon_info_free(ii);

        /* increase ref_count to keep this pixbuf in memory
           even when no one is using it. */
        if(pix)
            g_object_ref(pix);
    }
    else
    {
        char* str = g_icon_to_string(icon->gicon);
        g_debug("unable to load icon %s", str);

        const char * alternative_name = NULL;
        if (g_strrstr(str, "user-home"))
            alternative_name = "gtk-home";
        else if (g_strrstr(str, "folder") || g_strrstr(str, "directory"))
            alternative_name = "gtk-directory";
        else
            alternative_name = "gtk-file";

        if (alternative_name)
        {
            g_debug("alternative name %s", alternative_name);
            pix = gtk_icon_theme_load_icon(gtk_icon_theme_get_default(), alternative_name,
                    size, GTK_ICON_LOOKUP_USE_BUILTIN|GTK_ICON_LOOKUP_FORCE_SIZE, NULL);
            if (!pix)
            {
                GtkIconSet * icon_set = gtk_icon_factory_lookup_default(alternative_name);
                if (icon_set)
                {
                    GtkStyle * style = gtk_style_new();
                    pix = gtk_icon_set_render_icon(icon_set, style,
                        GTK_TEXT_DIR_NONE, GTK_STATE_NORMAL, GTK_ICON_SIZE_MENU, NULL, NULL);
                    gtk_style_unref(style);
                }
            }
        }

        if (pix)
            g_object_ref(pix);

        g_free(str);
    }

    /* cache this! */
    ent = g_slice_new(PixEntry);
    ent->size = size;
    ent->pix = pix;

    /* FIXME: maybe we should unload icons that nobody is using to reduce memory usage. */
    /* g_object_weak_ref(); */
    pixs = g_slist_prepend(pixs, ent);
    fm_icon_set_user_data(icon, pixs);

    return pix;
}

/**
 * fm_pixbuf_list_from_icon
 * @icon: icon descriptor
 *
 * Creates a list of #GdkPixbuf's of different sizes from an icon descriptor.
 *
 * Returns: (transfer full): an list of #GdkPixbuf's.
 *
 * Since: 1.5.0
 */
GList * fm_pixbuf_list_from_icon(FmIcon * icon)
{
    GList * list = NULL;
    gint * sizes = _fm_icon_get_sizes(icon);

    size_t i;
    for (i = 0; sizes[i]; i++)
    {
        gint size = sizes[i];
        if (size < 0)
            size = 48;
        GdkPixbuf * pixbuf = fm_pixbuf_from_icon(icon, size);
        if (pixbuf)
            list = g_list_append(list, pixbuf);
    }

    g_free(sizes);

    return list;
}


static void on_icon_theme_changed(GtkIconTheme* theme, gpointer user_data)
{
    g_debug("icon theme changed!");
    /* unload pixbufs cached in FmIcon's hash table. */
    fm_icon_unload_user_data_cache();
}

void _fm_icon_pixbuf_init()
{
    /* FIXME: GtkIconTheme object is different on different GdkScreen */
    GtkIconTheme* theme = gtk_icon_theme_get_default();
    changed_handler = g_signal_connect(theme, "changed", G_CALLBACK(on_icon_theme_changed), NULL);

    fm_icon_set_user_data_destroy(destroy_pixbufs);
}

void _fm_icon_pixbuf_finalize()
{
    GtkIconTheme* theme = gtk_icon_theme_get_default();
    g_signal_handler_disconnect(theme, changed_handler);
}
