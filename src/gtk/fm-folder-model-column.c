/*
 *      fm-folder-model-column.c
 *
 *      Copyright 2009 - 2012 Hong Jen Yee (PCMan) <pcman.tw@gmail.com>
 *      Copyright 2012 Andriy Grytsenko (LStranger) <andrej@rep.kiev.ua>
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
 * SECTION:fm-folder-model
 */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <libsmfm-core/fm-config.h>
#include "fm-folder-model.h"
#include <libsmfm-core/fm-file-info.h>
#include "fm-icon-pixbuf.h"
#include "fm-thumbnail.h"
#include "fm-gtk-marshal.h"

#include "glib-compat.h"

#include <gdk/gdk.h>
#include <glib/gi18n-lib.h>

#include <string.h>
#include <gio/gio.h>

#include "fm-folder-model-column-private.h"

guint column_infos_n = 0;
FmFolderModelInfo** column_infos = NULL;

static FmFolderModelInfo column_infos_raw[] = {
     /* columns visible to the users */
    { FM_FOLDER_MODEL_COL_NAME, 0, "name", N_("Name"), TRUE },
    { FM_FOLDER_MODEL_COL_DESC, 0, "desc", N_("Description"), TRUE },
    { FM_FOLDER_MODEL_COL_SIZE, 0, "size", N_("Size"), TRUE },
    { FM_FOLDER_MODEL_COL_PERM, 0, "perm", N_("Permissions"), FALSE },
    { FM_FOLDER_MODEL_COL_OWNER, 0, "owner", N_("Owner"), FALSE },
    { FM_FOLDER_MODEL_COL_MTIME, 0, "mtime", N_("Modified"), TRUE },
    { FM_FOLDER_MODEL_COL_DIRNAME, 0, "dirname", N_("Location"), TRUE },
    /* columns used internally */
    { FM_FOLDER_MODEL_COL_INFO, 0, "info", NULL, TRUE },
    { FM_FOLDER_MODEL_COL_ICON_NO_THUMBNAIL, 0, "icon", NULL, FALSE },
    { FM_FOLDER_MODEL_COL_ICON_WITH_THUMBNAIL, 0, "icon", NULL, FALSE },
    { FM_FOLDER_MODEL_COL_ICON_FORCE_THUMBNAIL, 0, "icon", NULL, FALSE },
    { FM_FOLDER_MODEL_COL_GICON, 0, "gicon", NULL, FALSE },
    { FM_FOLDER_MODEL_COL_COLOR, 0, "color", NULL, FALSE },
    { FM_FOLDER_MODEL_COL_COLOR_SET, 0, "color-set", NULL, FALSE }
};


void _fm_folder_model_col_init(void)
{
    guint i;

    if (column_infos)
        return;

    /* prepare column_infos table */
    column_infos_n = FM_FOLDER_MODEL_N_COLS;
    column_infos = g_new0(FmFolderModelInfo*, FM_FOLDER_MODEL_N_COLS);
    for(i = 0; i < G_N_ELEMENTS(column_infos_raw); i++)
    {
        FmFolderModelCol id = column_infos_raw[i].id;
        column_infos[id] = &column_infos_raw[i];
    }
    //fm_module_register_type("column", .......);

     /* GType value is actually generated at runtime by
      * calling _get_type() functions for every type.
      * So they should not be filled at compile-time.
      * Though G_TYPE_STRING and other fundimental type ids
      * are known at compile-time, this behavior is not 
      * guaranteed to be true in newer glib. */

    /* visible columns in the view */
    column_infos[FM_FOLDER_MODEL_COL_NAME]->type= G_TYPE_STRING;
    column_infos[FM_FOLDER_MODEL_COL_DESC]->type= G_TYPE_STRING;
    column_infos[FM_FOLDER_MODEL_COL_SIZE]->type= G_TYPE_STRING;
    column_infos[FM_FOLDER_MODEL_COL_PERM]->type= G_TYPE_STRING;
    column_infos[FM_FOLDER_MODEL_COL_OWNER]->type= G_TYPE_STRING;
    column_infos[FM_FOLDER_MODEL_COL_MTIME]->type= G_TYPE_STRING;
    column_infos[FM_FOLDER_MODEL_COL_DIRNAME]->type= G_TYPE_STRING;

    /* columns used internally */
    column_infos[FM_FOLDER_MODEL_COL_INFO]->type= G_TYPE_POINTER;
    column_infos[FM_FOLDER_MODEL_COL_ICON_NO_THUMBNAIL]->type= GDK_TYPE_PIXBUF;
    column_infos[FM_FOLDER_MODEL_COL_ICON_WITH_THUMBNAIL]->type= GDK_TYPE_PIXBUF;
    column_infos[FM_FOLDER_MODEL_COL_ICON_FORCE_THUMBNAIL]->type= GDK_TYPE_PIXBUF;
    column_infos[FM_FOLDER_MODEL_COL_GICON]->type= G_TYPE_ICON;
#if GTK_CHECK_VERSION(3, 0, 0)
    column_infos[FM_FOLDER_MODEL_COL_COLOR]->type= GDK_TYPE_RGBA;
#else
    column_infos[FM_FOLDER_MODEL_COL_COLOR]->type= GDK_TYPE_COLOR;
#endif
    column_infos[FM_FOLDER_MODEL_COL_COLOR_SET]->type= G_TYPE_BOOLEAN;
}


/* FmFolderModelCol APIs */

/**
 * fm_folder_model_col_get_title
 * @model: (allow-none): the folder model
 * @col_id: column id
 *
 * Retrieves the title of the column specified, or %NULL if the specified
 * @col_id is invalid. Returned data are owned by implementation and
 * should be not freed by caller.
 *
 * Returns: title of column in current locale.
 *
 * Since: 1.0.2
 */
const char* fm_folder_model_col_get_title(FmFolderModel* model, FmFolderModelCol col_id)
{
    if(G_UNLIKELY((guint)col_id >= column_infos_n
       || column_infos[col_id] == NULL)) /* invalid id */
        return NULL;
    return _(column_infos[col_id]->title);
}

/**
 * fm_folder_model_col_is_sortable
 * @model: (allow-none): model to check
 * @col_id: column id
 *
 * Checks if model can be sorted by @col_id.
 *
 * Returns: %TRUE if model can be sorted by @col_id.
 *
 * Since: 1.0.2
 */
gboolean fm_folder_model_col_is_sortable(FmFolderModel* model, FmFolderModelCol col_id)
{
    if(G_UNLIKELY((guint)col_id >= column_infos_n
       || column_infos[col_id] == NULL)) /* invalid id */
        return FALSE;
    return column_infos[col_id]->sortable;
}


/**
 * fm_folder_model_col_get_name
 * @col_id: column id
 *
 * Retrieves the name of the column specified, or %NULL if the specified
 * @col_id is invalid. The name of column may be used for config save or
 * another similar purpose. Returned data are owned by implementation and
 * should be not freed by caller.
 *
 * Returns: the name associated with column.
 *
 * Since: 1.0.2
 */
const char* fm_folder_model_col_get_name(FmFolderModelCol col_id)
{
    if(G_UNLIKELY((guint)col_id >= column_infos_n
       || column_infos[col_id] == NULL)) /* invalid id */
        return NULL;
    return column_infos[col_id]->name;
}

/**
 * fm_folder_model_get_col_by_name
 * @str: a column name
 *
 * Finds a column which has associated name equal to @str.
 *
 * Returns: column id or (FmFolderModelCol)-1 if no such column exists.
 *
 * Since: 1.0.2
 */
FmFolderModelCol fm_folder_model_get_col_by_name(const char* str)
{
    /* if further optimization is wanted, can use a sorted string array
     * and binary search here, but I think this micro-optimization is unnecessary. */
    if(G_LIKELY(str != NULL))
    {
        FmFolderModelCol i = 0;
        for(i = 0; i < column_infos_n; ++i)
        {
            if(column_infos[i] && strcmp(str, column_infos[i]->name) == 0)
                return i;
        }
    }
    return (FmFolderModelCol)-1;
}

/**
 * fm_folder_model_col_get_default_width
 * @model: (allow-none): model to check
 * @col_id: column id
 *
 * Retrieves preferred width for @col_id.
 *
 * Returns: default width.
 *
 * Since: 1.2.0
 */
gint fm_folder_model_col_get_default_width(FmFolderModel* model,
                                           FmFolderModelCol col_id)
{
    if(G_UNLIKELY((guint)col_id >= column_infos_n
       || column_infos[col_id] == NULL)) /* invalid id */
        return 0;
    return column_infos[col_id]->default_width;
}

/**
 * fm_folder_model_add_custom_column
 * @name: unique name of column
 * @init: setup data for column
 *
 * Registers custom columns in #FmFolderModel handlers.
 *
 * Returns: new column ID or FM_FOLDER_MODEL_COL_DEFAULT in case of failure.
 *
 * Since: 1.2.0
 */
FmFolderModelCol fm_folder_model_add_custom_column(const char* name,
                                                   FmFolderModelColumnInit* init)
{
    FmFolderModelInfo* info;
    guint i;

    g_return_val_if_fail(name && init && init->title && init->get_type &&
                         init->get_value, FM_FOLDER_MODEL_COL_DEFAULT);
    for(i = 0; i < column_infos_n; i++)
        if(strcmp(name, column_infos[i]->name) == 0) /* already exists */
            return FM_FOLDER_MODEL_COL_DEFAULT;
    column_infos = g_realloc(column_infos, sizeof(FmFolderModelInfo*) * (i+1));
    info = g_new0(FmFolderModelInfo, 1);
    column_infos[i] = info;
    column_infos_n = i+1;
    info->type = init->get_type();
    info->name = g_strdup(name);
    info->title = g_strdup(init->title);
    info->sortable = (init->compare != NULL);
    info->default_width = init->default_width;
    info->get_value = init->get_value;
    info->compare = init->compare;
    return i;
}

/**
 * fm_folder_model_col_is_valid
 * @col_id: column id
 *
 * Checks if @col_id can be handled by #FmFolderModel.
 * This API makes things similar to gtk_tree_model_get_n_columns() but it
 * doesn't operate the model instance.
 *
 * Returns: %TRUE if @col_id is valid.
 *
 * Since: 1.2.0
 */
gboolean fm_folder_model_col_is_valid(FmFolderModelCol col_id)
{
    return col_id < column_infos_n;
}
