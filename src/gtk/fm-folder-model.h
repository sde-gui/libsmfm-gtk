/*
 *      fm-folder-model.h
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

#ifndef _FM_FOLDER_MODEL_H_
#define _FM_FOLDER_MODEL_H_

#include <gtk/gtk.h>
#include <glib.h>
#include <glib-object.h>

#include <sys/types.h>

#include <libsmfm-core/fm-folder.h>
#include "fm-sortable.h"

G_BEGIN_DECLS

#define FM_TYPE_FOLDER_MODEL             (fm_folder_model_get_type())
#define FM_FOLDER_MODEL(obj)             (G_TYPE_CHECK_INSTANCE_CAST((obj),  FM_TYPE_FOLDER_MODEL, FmFolderModel))
#define FM_FOLDER_MODEL_CLASS(klass)     (G_TYPE_CHECK_CLASS_CAST ((klass),  FM_TYPE_FOLDER_MODEL, FmFolderModelClass))
#define FM_IS_FOLDER_MODEL(obj)          (G_TYPE_CHECK_INSTANCE_TYPE ((obj), FM_TYPE_FOLDER_MODEL))
#define FM_IS_FOLDER_MODEL_CLASS(klass)  (G_TYPE_CHECK_CLASS_TYPE ((klass),  FM_TYPE_FOLDER_MODEL))
#define FM_FOLDER_MODEL_GET_CLASS(obj)   (G_TYPE_INSTANCE_GET_CLASS ((obj),  FM_TYPE_FOLDER_MODEL, FmFolderModelClass))

typedef struct _FmFolderModel FmFolderModel;
typedef struct _FmFolderModelClass FmFolderModelClass;

#include "fm-folder-model-column.h"

/**
 * FmFolderModelClass:
 * @parent: the parent class
 * @row_deleting: the class closure for the #FmFolderModel::row-deleting signal
 * @filter_changed: the class closure for the #FmFolderModel::filter-changed signal
 */
struct _FmFolderModelClass
{
    GObjectClass parent;
    void (*row_deleting)(FmFolderModel* model, GtkTreePath* tp,
                         GtkTreeIter* iter, gpointer data);
    void (*filter_changed)(FmFolderModel* model);
    void (*filtering_changed)(FmFolderModel* model);
    /*< private >*/
    gpointer _reserved1;
    gpointer _reserved2;
};

typedef gboolean (*FmFolderModelFilterFunc)(FmFileInfo* file, gpointer user_data);

GType fm_folder_model_get_type (void);

FmFolderModel *fm_folder_model_new( FmFolder* dir, gboolean show_hidden );

void fm_folder_model_set_folder( FmFolderModel* model, FmFolder* dir );
FmFolder* fm_folder_model_get_folder(FmFolderModel* model);
FmPath* fm_folder_model_get_folder_path(FmFolderModel* model);

void fm_folder_model_set_item_userdata(FmFolderModel* model, GtkTreeIter* it,
                                       gpointer user_data);
gpointer fm_folder_model_get_item_userdata(FmFolderModel* model, GtkTreeIter* it);

gboolean fm_folder_model_get_show_hidden( FmFolderModel* model );

void fm_folder_model_set_show_hidden( FmFolderModel* model, gboolean show_hidden );

void fm_folder_model_file_created( FmFolderModel* model, FmFileInfo* file);

void fm_folder_model_file_deleted( FmFolderModel* model, FmFileInfo* file);

void fm_folder_model_file_changed( FmFolderModel* model, FmFileInfo* file);

gboolean fm_folder_model_find_iter_by_filename( FmFolderModel* model, GtkTreeIter* it, const char* name);

void fm_folder_model_set_icon_size(FmFolderModel* model, guint icon_size);
guint fm_folder_model_get_icon_size(FmFolderModel* model);

void fm_folder_model_add_filter(FmFolderModel* model, FmFolderModelFilterFunc func, gpointer user_data);
void fm_folder_model_remove_filter(FmFolderModel* model, FmFolderModelFilterFunc func, gpointer user_data);
void fm_folder_model_apply_filters(FmFolderModel* model);

void fm_folder_model_set_pattern(FmFolderModel* model, const char * pattern_str);
const char * fm_folder_model_get_pattern(FmFolderModel* model);

void fm_folder_model_set_sort(FmFolderModel* model, FmFolderModelCol col, FmSortMode mode);
gboolean fm_folder_model_get_sort(FmFolderModel* model, FmFolderModelCol *col, FmSortMode *mode);

/* void fm_folder_model_set_thumbnail_size(FmFolderModel* model, guint size); */

void fm_folder_model_set_use_custom_colors(FmFolderModel* model, gboolean use_custom_colors);
gboolean fm_folder_model_get_use_custom_colors(FmFolderModel* model);

int fm_folder_model_get_n_visible_items(FmFolderModel * model);
int fm_folder_model_get_n_hidden_items(FmFolderModel * model);
int fm_folder_model_get_n_incoming_items(FmFolderModel * model);

G_END_DECLS

#endif
