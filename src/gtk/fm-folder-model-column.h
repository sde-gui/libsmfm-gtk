/*
 *      fm-folder-model-column.h
 *
 *      Copyright 2009 PCMan <pcman.tw@gmail.com>
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

#ifndef _FM_FOLDER_MODEL_COLUMN_H_
#define _FM_FOLDER_MODEL_COLUMN_H_

G_BEGIN_DECLS

/**
 * FmFolderModelCol:
 * @FM_FOLDER_MODEL_COL_GICON: (#GIcon *) icon image
 * @FM_FOLDER_MODEL_COL_ICON: (#FmIcon *) icon descriptor
 * @FM_FOLDER_MODEL_COL_NAME: (#gchar *) file display name
 * @FM_FOLDER_MODEL_COL_SIZE: (#gchar *) file size text
 * @FM_FOLDER_MODEL_COL_DESC: (#gchar *) file MIME description
 * @FM_FOLDER_MODEL_COL_PERM: (#gchar *) reserved, not implemented
 * @FM_FOLDER_MODEL_COL_OWNER: (#gchar *) reserved, not implemented
 * @FM_FOLDER_MODEL_COL_MTIME: (#gchar *) modification time text
 * @FM_FOLDER_MODEL_COL_INFO: (#FmFileInfo *) file info
 * @FM_FOLDER_MODEL_COL_DIRNAME: (#gchar *) path of dir containing the file
 *
 * Columns of folder view
 */
typedef enum {
    FM_FOLDER_MODEL_COL_GICON = 0,
    FM_FOLDER_MODEL_COL_ICON_NO_THUMBNAIL,
    FM_FOLDER_MODEL_COL_ICON_WITH_THUMBNAIL,
    FM_FOLDER_MODEL_COL_ICON_FORCE_THUMBNAIL,
    FM_FOLDER_MODEL_COL_NAME,
    FM_FOLDER_MODEL_COL_SIZE,
    FM_FOLDER_MODEL_COL_DESC,
    FM_FOLDER_MODEL_COL_PERM,
    FM_FOLDER_MODEL_COL_OWNER,
    FM_FOLDER_MODEL_COL_MTIME,
    FM_FOLDER_MODEL_COL_INFO,
    FM_FOLDER_MODEL_COL_DIRNAME,
    FM_FOLDER_MODEL_COL_COLOR,
    FM_FOLDER_MODEL_COL_COLOR_SET,
    /*< private >*/
    FM_FOLDER_MODEL_N_COLS
} FmFolderModelCol;

/**
 * FM_FOLDER_MODEL_COL_UNSORTED:
 *
 * for 'Unsorted' folder view use 'FileInfo' column which is ambiguous for sorting
 */
#define FM_FOLDER_MODEL_COL_UNSORTED FM_FOLDER_MODEL_COL_INFO

/**
 * FM_FOLDER_MODEL_COL_DEFAULT:
 *
 * value which means do not change sorting column.
 */
#define FM_FOLDER_MODEL_COL_DEFAULT ((FmFolderModelCol)-1)

#ifndef FM_DISABLE_DEPRECATED
/* for backward compatiblity, kept until soname 6 */
#define FmFolderModelViewCol    FmFolderModelCol
#define COL_FILE_GICON          FM_FOLDER_MODEL_COL_GICON
#define COL_FILE_ICON           FM_FOLDER_MODEL_COL_ICON
#define COL_FILE_NAME           FM_FOLDER_MODEL_COL_NAME
#define COL_FILE_SIZE           FM_FOLDER_MODEL_COL_SIZE
#define COL_FILE_DESC           FM_FOLDER_MODEL_COL_DESC
#define COL_FILE_PERM           FM_FOLDER_MODEL_COL_PERM
#define COL_FILE_OWNER          FM_FOLDER_MODEL_COL_OWNER
#define COL_FILE_MTIME          FM_FOLDER_MODEL_COL_MTIME
#define COL_FILE_INFO           FM_FOLDER_MODEL_COL_INFO
#define COL_FILE_UNSORTED       FM_FOLDER_MODEL_COL_UNSORTED
#define FM_FOLDER_MODEL_COL_IS_VALID(col) fm_folder_model_col_is_valid((guint)col)
#endif

/* APIs for FmFolderModelCol */

const char* fm_folder_model_col_get_title(FmFolderModel* model, FmFolderModelCol col_id);
gboolean fm_folder_model_col_is_sortable(FmFolderModel* model, FmFolderModelCol col_id);
const char* fm_folder_model_col_get_name(FmFolderModelCol col_id);
FmFolderModelCol fm_folder_model_get_col_by_name(const char* str);
gint fm_folder_model_col_get_default_width(FmFolderModel* model, FmFolderModelCol col_id);
gboolean fm_folder_model_col_is_valid(FmFolderModelCol col_id);

/* --- columns extensions --- */
typedef struct _FmFolderModelColumnInit FmFolderModelColumnInit;

/**
 * FmFolderModelColumnInit:
 * @title: column title
 * @default_width: default width for column (0 means auto)
 * @get_type: function to get #GType of column data
 * @get_value: function to retrieve column data
 * @compare: sorting routine (%NULL if column isn't sortable)
 */
struct _FmFolderModelColumnInit
{
    const char *title;
    gint default_width;
    GType (*get_type)(void);
    void (*get_value)(FmFileInfo *fi, GValue *value);
    gint (*compare)(FmFileInfo *fi1, FmFileInfo *fi2);
};

FmFolderModelCol fm_folder_model_add_custom_column(const char* name, FmFolderModelColumnInit* init);

G_END_DECLS

#endif
