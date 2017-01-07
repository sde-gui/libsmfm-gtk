/*
 *      fm-folder-model.c
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
 * @short_description: A model for folder view window.
 * @title: FmFolderModel
 *
 * @include: libfm/fm-gtk.h
 *
 * The #FmFolderModel is used by widgets such as #FmFolderView to arrange
 * items of folder.
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

struct _FmFolderModel
{
    GObject parent;
    FmFolder* folder;
    GSequence * items;
    GSequence * hidden; /* items hidden by filter */
    GSequence * incoming_items;

    guint incoming_items_handler;

    gboolean show_hidden;
    gboolean use_custom_colors;

    FmFolderModelCol sort_col;
    FmSortMode sort_mode;

    /* Random integer to check whether an iter belongs to our model */
    gint stamp;

    guint theme_change_handler;
    guint icon_size;

    guint thumbnail_max;
    GList* thumbnail_requests;
    GHashTable* items_hash;

    GSList* filters;

    GPatternSpec * pattern;
    char * pattern_str;

    GdkPixbuf * fallback_icon_pixbuf;
};

typedef struct _FmFolderItem FmFolderItem;
struct _FmFolderItem
{
    FmFileInfo * inf;
    gpointer userdata;

    struct {
        GdkPixbuf * pixbuf;
        gboolean loading : 1;
        gboolean failed : 1;
    } icon [2];

    gboolean color_initialized: 1;
    gboolean color_default : 1;
#if GTK_CHECK_VERSION(3, 0, 0)
    GdkRGBA color;
#else
    GdkColor color;
#endif
};

typedef struct _FmFolderModelFilterItem
{
    FmFolderModelFilterFunc func;
    gpointer user_data;
}FmFolderModelFilterItem;

enum ReloadFlags
{
    RELOAD_ICONS = 1 << 0,
    RELOAD_THUMBNAILS = 1 << 1,
    RELOAD_BOTH = (RELOAD_ICONS | RELOAD_THUMBNAILS)
};

static void fm_folder_model_tree_model_init(GtkTreeModelIface *iface);
static void fm_folder_model_tree_sortable_init(GtkTreeSortableIface *iface);
static void fm_folder_model_drag_source_init(GtkTreeDragSourceIface *iface);
static void fm_folder_model_drag_dest_init(GtkTreeDragDestIface *iface);

static void fm_folder_model_dispose(GObject *object);
G_DEFINE_TYPE_WITH_CODE( FmFolderModel, fm_folder_model, G_TYPE_OBJECT,
                        G_IMPLEMENT_INTERFACE(GTK_TYPE_TREE_MODEL, fm_folder_model_tree_model_init)
                        G_IMPLEMENT_INTERFACE(GTK_TYPE_TREE_SORTABLE, fm_folder_model_tree_sortable_init)
                        G_IMPLEMENT_INTERFACE(GTK_TYPE_TREE_DRAG_SOURCE, fm_folder_model_drag_source_init)
                        G_IMPLEMENT_INTERFACE(GTK_TYPE_TREE_DRAG_DEST, fm_folder_model_drag_dest_init) )

static GtkTreeModelFlags fm_folder_model_get_flags(GtkTreeModel *tree_model);
static gint fm_folder_model_get_n_columns(GtkTreeModel *tree_model);
static GType fm_folder_model_get_column_type(GtkTreeModel *tree_model,
                                             gint index);
static gboolean fm_folder_model_get_iter(GtkTreeModel *tree_model,
                                         GtkTreeIter *iter,
                                         GtkTreePath *path);
static GtkTreePath *fm_folder_model_get_path(GtkTreeModel *tree_model,
                                             GtkTreeIter *iter);
static void fm_folder_model_get_value(GtkTreeModel *tree_model,
                                      GtkTreeIter *iter,
                                      gint column,
                                      GValue *value);
static gboolean fm_folder_model_iter_next(GtkTreeModel *tree_model,
                                          GtkTreeIter *iter);
static gboolean fm_folder_model_iter_children(GtkTreeModel *tree_model,
                                              GtkTreeIter *iter,
                                              GtkTreeIter *parent);
static gboolean fm_folder_model_iter_has_child(GtkTreeModel *tree_model,
                                               GtkTreeIter *iter);
static gint fm_folder_model_iter_n_children(GtkTreeModel *tree_model,
                                            GtkTreeIter *iter);
static gboolean fm_folder_model_iter_nth_child(GtkTreeModel *tree_model,
                                               GtkTreeIter *iter,
                                               GtkTreeIter *parent,
                                               gint n);
static gboolean fm_folder_model_iter_parent(GtkTreeModel *tree_model,
                                            GtkTreeIter *iter,
                                            GtkTreeIter *child);
static gboolean fm_folder_model_get_sort_column_id(GtkTreeSortable* sortable,
                                                   gint* sort_column_id,
                                                   GtkSortType* order);
static void fm_folder_model_set_sort_column_id(GtkTreeSortable* sortable,
                                               gint sort_column_id,
                                               GtkSortType order);
static void fm_folder_model_set_sort_func(GtkTreeSortable *sortable,
                                          gint sort_column_id,
                                          GtkTreeIterCompareFunc sort_func,
                                          gpointer user_data,
                                          GDestroyNotify destroy);
static void fm_folder_model_set_default_sort_func(GtkTreeSortable *sortable,
                                                  GtkTreeIterCompareFunc sort_func,
                                                  gpointer user_data,
                                                  GDestroyNotify destroy);
static void fm_folder_model_do_sort(FmFolderModel* model);

static inline gboolean file_can_show(FmFolderModel* model, FmFileInfo* file);

static void fm_folder_model_thumbnail_request(FmFolderModel * model, FmFolderItem * item,
                                              FmThumbnailIconType icon_type, gboolean force_reload);
static void fm_folder_model_cancel_outdated_thumbnail_requests(FmFolderModel * model);


static void _fm_folder_model_add_file_real(FmFolderModel* model, FmFileInfo* file);
static void _fm_folder_model_add_file(FmFolderModel* model, FmFileInfo* file);

static gint fm_folder_model_compare(gconstpointer item1, gconstpointer item2, gpointer user_data);

/* signal handlers */

static void on_icon_theme_changed(GtkIconTheme* theme, FmFolderModel* model);

static void on_show_thumbnail_changed(FmConfig* cfg, gpointer user_data);

static void on_thumbnail_local_changed(FmConfig* cfg, gpointer user_data);

static void on_thumbnail_max_changed(FmConfig* cfg, gpointer user_data);

#include "fm-folder-model-column-private.h"

enum {
    ROW_DELETING,
    FILTER_CHANGED,
    FILTERING_CHANGED,
    N_SIGNALS
};

static guint signals[N_SIGNALS];

static void fm_folder_model_init(FmFolderModel* model)
{
    model->sort_mode = FM_SORT_ASCENDING;
    model->sort_col = FM_FOLDER_MODEL_COL_DEFAULT;
    /* Random int to check whether an iter belongs to our model */
    model->stamp = g_random_int();

    model->theme_change_handler = g_signal_connect(gtk_icon_theme_get_default(), "changed",
                                                   G_CALLBACK(on_icon_theme_changed), model);
    g_signal_connect(fm_config, "changed::show_thumbnail", G_CALLBACK(on_show_thumbnail_changed), model);
    g_signal_connect(fm_config, "changed::thumbnail_local", G_CALLBACK(on_thumbnail_local_changed), model);
    g_signal_connect(fm_config, "changed::thumbnail_max", G_CALLBACK(on_thumbnail_max_changed), model);

    model->thumbnail_max = fm_config->thumbnail_max << 10;
    model->items_hash = g_hash_table_new(g_direct_hash, g_direct_equal);
}

static void fm_folder_model_class_init(FmFolderModelClass *klass)
{
    GObjectClass * object_class;

    fm_folder_model_parent_class = (GObjectClass*)g_type_class_peek_parent(klass);
    object_class = (GObjectClass*)klass;
    object_class->dispose = fm_folder_model_dispose;

    /**
     * FmFolderModel::row-deleting:
     * @model: folder model instance that received the signal
     * @row:   path to row that is about to be deleted
     * @iter:  iterator of row that is about to be deleted
     * @data:  user data associated with the row
     *
     * This signal is emitted before row is deleted.
     *
     * It can be used if view has some data associated with the row so
     * those data can be freed safely.
     *
     * Since: 1.0.0
     */
    signals[ROW_DELETING] =
        g_signal_new("row-deleting",
                     G_TYPE_FROM_CLASS(klass),
                     G_SIGNAL_RUN_FIRST,
                     G_STRUCT_OFFSET(FmFolderModelClass, row_deleting),
                     NULL, NULL,
                     fm_marshal_VOID__BOXED_BOXED_POINTER,
                     G_TYPE_NONE, 3, GTK_TYPE_TREE_PATH, GTK_TYPE_TREE_ITER,
                     G_TYPE_POINTER);

    /**
     * FmFolderModel::filter-changed:
     * @model: folder model instance that received the signal
     *
     * This signal is emitted when model data were changed due to filter
     * changes.
     *
     * Since: 1.0.2
     */
    signals[FILTER_CHANGED] =
        g_signal_new("filter-changed",
                     G_TYPE_FROM_CLASS(klass),
                     G_SIGNAL_RUN_FIRST,
                     G_STRUCT_OFFSET(FmFolderModelClass, filter_changed),
                     NULL, NULL,
                     g_cclosure_marshal_VOID__VOID,
                     G_TYPE_NONE, 0);

    /**
     * FmFolderModel::filtering-changed:
     * @model: folder model instance that received the signal
     *
     * This signal is emitted when model data were changed due to filter
     * changes or changes in the linked folder (some files appeared or removed).
     *
     * Since: 1.4.0
     */
    signals[FILTERING_CHANGED] =
        g_signal_new("filtering-changed",
                     G_TYPE_FROM_CLASS(klass),
                     G_SIGNAL_RUN_FIRST,
                     G_STRUCT_OFFSET(FmFolderModelClass, filtering_changed),
                     NULL, NULL,
                     g_cclosure_marshal_VOID__VOID,
                     G_TYPE_NONE, 0);

}

static void fm_folder_model_tree_model_init(GtkTreeModelIface *iface)
{
    iface->get_flags = fm_folder_model_get_flags;
    iface->get_n_columns = fm_folder_model_get_n_columns;
    iface->get_column_type = fm_folder_model_get_column_type;
    iface->get_iter = fm_folder_model_get_iter;
    iface->get_path = fm_folder_model_get_path;
    iface->get_value = fm_folder_model_get_value;
    iface->iter_next = fm_folder_model_iter_next;
    iface->iter_children = fm_folder_model_iter_children;
    iface->iter_has_child = fm_folder_model_iter_has_child;
    iface->iter_n_children = fm_folder_model_iter_n_children;
    iface->iter_nth_child = fm_folder_model_iter_nth_child;
    iface->iter_parent = fm_folder_model_iter_parent;

    _fm_folder_model_col_init();
}

static void fm_folder_model_tree_sortable_init(GtkTreeSortableIface *iface)
{
    /* iface->sort_column_changed = fm_folder_model_sort_column_changed; */
    iface->get_sort_column_id = fm_folder_model_get_sort_column_id;
    iface->set_sort_column_id = fm_folder_model_set_sort_column_id;
    iface->set_sort_func = fm_folder_model_set_sort_func;
    iface->set_default_sort_func = fm_folder_model_set_default_sort_func;
    iface->has_default_sort_func = ( gboolean (*)(GtkTreeSortable *) )gtk_false;
}

static void fm_folder_model_drag_source_init(GtkTreeDragSourceIface *iface)
{
    /* FIXME: Unused. Will this cause any problem? */
}

static void fm_folder_model_drag_dest_init(GtkTreeDragDestIface *iface)
{
    /* FIXME: Unused. Will this cause any problem? */
}

static void fm_folder_model_filter_item_free(FmFolderModelFilterItem* item)
{
    g_slice_free(FmFolderModelFilterItem, item);
}

static void fm_folder_model_dispose(GObject *object)
{
    FmFolderModel* model = FM_FOLDER_MODEL(object);
    if(model->folder)
        fm_folder_model_set_folder(model, NULL);

    if (model->incoming_items_handler != 0)
        g_source_remove(model->incoming_items_handler);

    if(model->theme_change_handler)
    {
        g_signal_handler_disconnect(gtk_icon_theme_get_default(),
                                    model->theme_change_handler);
        model->theme_change_handler = 0;
    }

    g_signal_handlers_disconnect_by_func(fm_config, on_show_thumbnail_changed, model);
    g_signal_handlers_disconnect_by_func(fm_config, on_thumbnail_local_changed, model);
    g_signal_handlers_disconnect_by_func(fm_config, on_thumbnail_max_changed, model);

    if(model->thumbnail_requests)
    {
        g_list_foreach(model->thumbnail_requests, (GFunc)fm_thumbnail_request_cancel, NULL);
        g_list_free(model->thumbnail_requests);
        model->thumbnail_requests = NULL;
    }
    if(model->items_hash)
    {
        g_hash_table_destroy(model->items_hash);
        model->items_hash = NULL;
    }

    if(model->filters)
    {
        g_slist_free_full(model->filters, (GDestroyNotify)fm_folder_model_filter_item_free);
        model->filters = NULL;
    }

    if(model->pattern)
    {
        g_pattern_spec_free(model->pattern);
        model->pattern = NULL;
    }

    if(model->pattern_str)
    {
        g_free(model->pattern_str);
        model->pattern_str = NULL;
    }

    if (model->fallback_icon_pixbuf)
    {
        g_object_unref(model->fallback_icon_pixbuf);
        model->fallback_icon_pixbuf = NULL;
    }

    (*G_OBJECT_CLASS(fm_folder_model_parent_class)->dispose)(object);
}

/**
 * fm_folder_model_new
 * @dir: the folder to create model
 * @show_hidden: whether show hidden files initially or not
 *
 * Creates new folder model for the @dir.
 *
 * Returns: (transfer full): a new #FmFolderModel object.
 *
 * Since: 0.1.0
 */
FmFolderModel *fm_folder_model_new(FmFolder* dir, gboolean show_hidden)
{
    FmFolderModel* model;
    model = (FmFolderModel*)g_object_new(FM_TYPE_FOLDER_MODEL, NULL);
    model->items = NULL;
    model->hidden = NULL;
    model->incoming_items = NULL;
    model->show_hidden = show_hidden;
    fm_folder_model_set_folder(model, dir);
    return model;
}

static inline FmFolderItem* fm_folder_item_new(FmFileInfo* inf)
{
    FmFolderItem* item = g_slice_new0(FmFolderItem);
    item->inf = fm_file_info_ref(inf);
    return item;
}

static inline void fm_folder_item_free(gpointer data)
{
    FmFolderItem* item = (FmFolderItem*)data;
    if (item->icon[0].pixbuf)
        g_object_unref(item->icon[0].pixbuf);
    if (item->icon[1].pixbuf)
        g_object_unref(item->icon[1].pixbuf);
    fm_file_info_unref(item->inf);
    g_slice_free(FmFolderItem, item);
}

/*****************************************************************************/

static void _on_visible_item_inserted(FmFolderModel * model, GSequenceIter * item_it)
{
    FmFolderItem * item = (FmFolderItem *) g_sequence_get(item_it);
    g_hash_table_insert(model->items_hash, item->inf, item_it);

    GtkTreeIter it;
    GtkTreePath * path;
    it.stamp = model->stamp;
    it.user_data  = item_it;
    path = gtk_tree_path_new_from_indices(g_sequence_iter_get_position(item_it), -1);
    gtk_tree_model_row_inserted(GTK_TREE_MODEL(model), path, &it);
    gtk_tree_path_free(path);
}

static void _on_visible_item_removed(FmFolderModel * model, GSequenceIter * item_it)
{
    FmFolderItem * item = (FmFolderItem *) g_sequence_get(item_it);

    GtkTreeIter it;
    GtkTreePath* path;
    it.stamp = model->stamp;
    it.user_data = item_it;
    path = gtk_tree_path_new_from_indices(g_sequence_iter_get_position(item_it), -1);
    g_signal_emit(model, signals[ROW_DELETING], 0, path, &it, item->userdata);
    gtk_tree_model_row_deleted(GTK_TREE_MODEL(model), path);
    gtk_tree_path_free(path);
    g_hash_table_remove(model->items_hash, item->inf);
}

/*****************************************************************************/

static gboolean incoming_items_handler(gpointer user_data)
{
    FmFolderModel * model = (FmFolderModel *) user_data;

    long long start_time = g_get_monotonic_time();
    long long items_handled = 0;

    while (g_sequence_get_length(model->incoming_items))
    {
        GSequenceIter * item_it = g_sequence_get_iter_at_pos(model->incoming_items, 0);
        FmFolderItem * item = (FmFolderItem *) g_sequence_get(item_it);

        if (file_can_show(model, item->inf))
        {
            GSequenceIter * insert_item_it = g_sequence_search(model->items,
                item, fm_folder_model_compare, model);
            g_sequence_move(item_it, insert_item_it);
            _on_visible_item_inserted(model, item_it);
        }
        else
        {
            g_sequence_move(item_it, g_sequence_get_begin_iter(model->hidden));
        }

        items_handled++;

        if (g_get_monotonic_time() - start_time > G_USEC_PER_SEC * 0.2)
            break;
    }

    if (items_handled)
        g_signal_emit(model, signals[FILTERING_CHANGED], 0);

    g_debug("FmFolderModel: %s: %lld items handled in %lld µs", __FUNCTION__,
        items_handled, (long long) g_get_monotonic_time() - start_time);

    return g_sequence_get_length(model->incoming_items) != 0;
}

static void incoming_items_handler_destroy (gpointer user_data)
{
    FmFolderModel * model = (FmFolderModel *) user_data;
    model->incoming_items_handler = 0;
}

static void _fm_folder_model_add_file_to_incoming(FmFolderModel * model, FmFileInfo * file)
{
    g_sequence_append(model->incoming_items, fm_folder_item_new(file));
    if (!model->incoming_items_handler)
        model->incoming_items_handler =
            g_idle_add_full(G_PRIORITY_DEFAULT_IDLE, incoming_items_handler, model, incoming_items_handler_destroy);
}

/*****************************************************************************/

static void _fm_folder_model_files_changed(FmFolder* dir, GSList* files,
                                           FmFolderModel* model)
{
    GSList* l;
    for( l = files; l; l=l->next )
        fm_folder_model_file_changed(model, l->data);
}

static void _fm_folder_model_add_file_real(FmFolderModel* model, FmFileInfo* file)
{
    if(!file_can_show(model, file))
        g_sequence_append( model->hidden, fm_folder_item_new(file) );
    else
        fm_folder_model_file_created(model, file);
}

static void _fm_folder_model_add_file(FmFolderModel* model, FmFileInfo* file)
{
    if (g_sequence_get_length(model->items) < 1000)
        _fm_folder_model_add_file_real(model, file);
    else
        _fm_folder_model_add_file_to_incoming(model, file);
}

static void _fm_folder_model_files_added(FmFolder* dir, GSList* files,
                                         FmFolderModel* model)
{
    GSList* l;
    for( l = files; l; l=l->next )
    {
        FmFileInfo* fi = FM_FILE_INFO(l->data);
        _fm_folder_model_add_file(model, fi);
    }
    g_signal_emit(model, signals[FILTERING_CHANGED], 0);
}


static void _fm_folder_model_files_removed(FmFolder* dir, GSList* files,
                                           FmFolderModel* model)
{
    GSList* l;
    for( l = files; l; l=l->next )
        fm_folder_model_file_deleted(model, FM_FILE_INFO(l->data));
    g_signal_emit(model, signals[FILTERING_CHANGED], 0);
}

/**
 * fm_folder_model_get_folder
 * @model: the folder model instance
 *
 * Retrieves a folder that @model is created for. Returned data are owned
 * by the @model and should not be freed by caller.
 *
 * Returns: (transfer none): the folder descriptor.
 *
 * Since: 1.0.0
 */
FmFolder* fm_folder_model_get_folder(FmFolderModel* model)
{
    return model->folder;
}

/**
 * fm_folder_model_get_folder_path
 * @model: the folder model instance
 *
 * Retrieves path of folder that @model is created for. Returned data
 * are owned by the @model and should not be freed by caller.
 *
 * Returns: (transfer none): the path of the folder of the model.
 *
 * Since: 1.0.0
 */
FmPath* fm_folder_model_get_folder_path(FmFolderModel* model)
{
    return model->folder ? fm_folder_get_path(model->folder) : NULL;
}

/**
 * fm_folder_model_set_folder
 * @model: a folder model instance
 * @dir: a new folder for the model
 *
 * Changes folder which model handles. This call allows reusing the model
 * for different folder, in case, e.g. directory was changed.
 *
 * Since: 0.1.0
 */
void fm_folder_model_set_folder(FmFolderModel* model, FmFolder* dir)
{
    if(model->folder == dir)
        return;
    /* g_debug("fm_folder_model_set_folder(%p, %p)", model, dir); */

    /* free the old folder */
    if(model->folder)
    {
        guint row_deleted_signal = g_signal_lookup("row-deleted", GTK_TYPE_TREE_MODEL);
        g_signal_handlers_disconnect_by_func(model->folder,
                                             _fm_folder_model_files_added, model);
        g_signal_handlers_disconnect_by_func(model->folder,
                                             _fm_folder_model_files_removed, model);
        g_signal_handlers_disconnect_by_func(model->folder,
                                             _fm_folder_model_files_changed, model);

        /* Emit 'row-deleted' signals for all files */
        if(g_signal_has_handler_pending(model, row_deleted_signal, 0, TRUE))
        {
            GSequenceIter* item_it = g_sequence_get_begin_iter(model->items);
            while(!g_sequence_iter_is_end(item_it))
            {
                _on_visible_item_removed(model, item_it);
                item_it = g_sequence_iter_next(item_it);
            }
        }
        g_hash_table_remove_all(model->items_hash);
        g_sequence_free(model->items);
        g_sequence_free(model->hidden);
        g_sequence_free(model->incoming_items);
        g_object_unref(model->folder);
        model->folder = NULL;
    }
    if (!dir)
        return;
    model->items = g_sequence_new(fm_folder_item_free);
    model->hidden = g_sequence_new(fm_folder_item_free);
    model->incoming_items = g_sequence_new(fm_folder_item_free);
    model->folder = FM_FOLDER(g_object_ref(dir));

    g_signal_connect(model->folder, "files-added",
                     G_CALLBACK(_fm_folder_model_files_added),
                     model);
    g_signal_connect(model->folder, "files-removed",
                     G_CALLBACK(_fm_folder_model_files_removed),
                     model);
    g_signal_connect(model->folder, "files-changed",
                     G_CALLBACK(_fm_folder_model_files_changed),
                     model);

    if (fm_folder_is_loaded(model->folder) || fm_folder_is_incremental(model->folder)) /* if it's already loaded */
    {
        /* add existing files to the model */
        if (!fm_folder_is_empty(model->folder))
        {
            long long start_time = g_get_monotonic_time();
            long long time_slice_before_switching_to_incoming = G_USEC_PER_SEC * 1.2;
            long long items_handled = 0;
            long long items_added_to_incoming = 0;
            gboolean use_incoming = FALSE;
            GList *l;
            FmFileInfoList* files = fm_folder_get_files(model->folder);
            for (l = fm_file_info_list_peek_head_link(files); l; l = l->next)
            {
                items_handled++;
                if (use_incoming)
                {
                    _fm_folder_model_add_file_to_incoming(model, FM_FILE_INFO(l->data));
                    items_added_to_incoming++;
                }
                else
                {
                    _fm_folder_model_add_file_real(model, FM_FILE_INFO(l->data));
                    if ((g_get_monotonic_time() - start_time) > time_slice_before_switching_to_incoming)
                    {
                        use_incoming = TRUE;
                        g_debug(
                            "FmFolderModel: %s: %lld items handled in %lld µs before switching to the deferred incoming queue",
                            __FUNCTION__,
                            items_handled,
                            (long long) g_get_monotonic_time() - start_time
                        );
                    }
                }
            }

            if (use_incoming)
            {
                g_debug(
                    "FmFolderModel: %s: %lld items added to the deferred incoming queue",
                    __FUNCTION__,
                    items_added_to_incoming
                );
            }
            else
            {
                g_debug(
                    "FmFolderModel: %s: %lld items added directly to the model",
                    __FUNCTION__,
                    items_handled
                );
            }
        }
    }

    g_signal_emit(model, signals[FILTERING_CHANGED], 0);
}

static GtkTreeModelFlags fm_folder_model_get_flags(GtkTreeModel *tree_model)
{
    g_return_val_if_fail(FM_IS_FOLDER_MODEL(tree_model), (GtkTreeModelFlags)0);
    return (GTK_TREE_MODEL_LIST_ONLY | GTK_TREE_MODEL_ITERS_PERSIST);
}

static gint fm_folder_model_get_n_columns(GtkTreeModel *tree_model)
{
    return (gint)column_infos_n;
}

static GType fm_folder_model_get_column_type(GtkTreeModel *tree_model,
                                             gint index)
{
    g_return_val_if_fail(FM_IS_FOLDER_MODEL(tree_model), G_TYPE_INVALID);
    g_return_val_if_fail((guint)index < column_infos_n, G_TYPE_INVALID);
    g_return_val_if_fail(column_infos[index] != NULL, G_TYPE_INVALID);
    return column_infos[index]->type;
}

static gboolean fm_folder_model_get_iter(GtkTreeModel *tree_model,
                                         GtkTreeIter *iter,
                                         GtkTreePath *path)
{
    FmFolderModel* model;
    gint *indices, n, depth;
    GSequenceIter* items_it;

    g_assert(FM_IS_FOLDER_MODEL(tree_model));
    g_assert(path!=NULL);

    model = (FmFolderModel*)tree_model;

    indices = gtk_tree_path_get_indices(path);
    depth   = gtk_tree_path_get_depth(path);

    /* we do not allow children */
    g_assert(depth == 1); /* depth 1 = top level; a list only has top level nodes and no children */

    n = indices[0]; /* the n-th top level row */

    if( n >= g_sequence_get_length(model->items) || n < 0 )
        return FALSE;

    items_it = g_sequence_get_iter_at_pos(model->items, n);

    g_assert( items_it  != g_sequence_get_end_iter(model->items) );

    /* We simply store a pointer in the iter */
    iter->stamp = model->stamp;
    iter->user_data  = items_it;

    return TRUE;
}

static GtkTreePath *fm_folder_model_get_path(GtkTreeModel *tree_model,
                                             GtkTreeIter *iter)
{
    GtkTreePath* path;
    GSequenceIter* items_it;
    FmFolderModel* model = FM_FOLDER_MODEL(tree_model);

    g_return_val_if_fail(model, NULL);
    g_return_val_if_fail(iter != NULL, NULL);
    g_return_val_if_fail(iter->stamp == model->stamp, NULL);
    g_return_val_if_fail(iter->user_data != NULL, NULL);

    items_it = (GSequenceIter*)iter->user_data;
    path = gtk_tree_path_new();
    gtk_tree_path_append_index( path, g_sequence_iter_get_position(items_it) );
    return path;
}

static gboolean read_item_color(FmFolderModel * model, FmFolderItem * item)
{
    if (G_UNLIKELY(!item->color_initialized))
    {
        unsigned long color = fm_file_info_get_color(item->inf);
        item->color_default = (color == FILE_INFO_DEFAULT_COLOR);
        if (!item->color_default)
        {
#if GTK_CHECK_VERSION(3, 0, 0)
            item->color.red = ((color >> 16) & 0xFF) / 255.0;
            item->color.green = ((color >> 8) & 0xFF) / 255.0;
            item->color.blue = ((color) & 0xFF) / 255.0;
#else
            item->color.red = ((color >> 16) & 0xFF) * 257;
            item->color.green = ((color >> 8) & 0xFF) * 257;
            item->color.blue = ((color) & 0xFF) * 257;
#endif
        }
        item->color_initialized = TRUE;
    }

    return !item->color_default;
}

static void fm_folder_model_get_value(GtkTreeModel *tree_model,
                                      GtkTreeIter *iter,
                                      gint column,
                                      GValue *value)
{
    GSequenceIter* item_it;
    FmFolderModel* model = FM_FOLDER_MODEL(tree_model);

    g_return_if_fail(iter != NULL);
    g_return_if_fail((guint)column < column_infos_n && column_infos[column] != NULL);

    g_value_init(value, column_infos[column]->type);

    item_it = (GSequenceIter*)iter->user_data;
    g_return_if_fail(item_it != NULL);

    FmFolderItem* item = (FmFolderItem*)g_sequence_get(item_it);
    FmFileInfo* info = item->inf;

    if(column >= FM_FOLDER_MODEL_N_COLS) /* extension */
    {
        column_infos[column]->get_value(info, value);
    }
    else switch( (FmFolderModelCol)column )
    {
    case FM_FOLDER_MODEL_COL_GICON:
    {
        FmIcon * icon = fm_file_info_get_icon(info);
        if (G_LIKELY(icon))
            g_value_set_object(value, icon->gicon);
        break;
    }
    case FM_FOLDER_MODEL_COL_ICON_NO_THUMBNAIL:
    case FM_FOLDER_MODEL_COL_ICON_WITH_THUMBNAIL:
    case FM_FOLDER_MODEL_COL_ICON_FORCE_THUMBNAIL:
    {
        gboolean show_thumbnail;
        if (column == FM_FOLDER_MODEL_COL_ICON_NO_THUMBNAIL)
            show_thumbnail = FALSE;
        else if (column == FM_FOLDER_MODEL_COL_ICON_FORCE_THUMBNAIL)
            show_thumbnail = TRUE;
        else
            show_thumbnail = fm_config->show_thumbnail;

        if (show_thumbnail && item->icon[FM_THUMBNAIL_ICON_CONTENT].pixbuf)
        {
            g_value_set_object(value, item->icon[FM_THUMBNAIL_ICON_CONTENT].pixbuf);
        }
        else
        {
            if (G_UNLIKELY(!item->icon[FM_THUMBNAIL_ICON_SIMPLE].pixbuf))
            {
                if (fm_file_info_icon_loaded(info))
                {
                    item->icon[FM_THUMBNAIL_ICON_SIMPLE].pixbuf =
                        fm_pixbuf_from_icon(fm_file_info_get_icon(info), model->icon_size);
                    if (!item->icon[FM_THUMBNAIL_ICON_SIMPLE].pixbuf)
                        item->icon[FM_THUMBNAIL_ICON_SIMPLE].failed = TRUE;
                }
            }

            if (item->icon[FM_THUMBNAIL_ICON_SIMPLE].pixbuf)
                g_value_set_object(value, item->icon[FM_THUMBNAIL_ICON_SIMPLE].pixbuf);
            else
                g_value_set_object(value, model->fallback_icon_pixbuf);
        }

        fm_folder_model_thumbnail_request(model, item, FM_THUMBNAIL_ICON_SIMPLE, FALSE);

        if (show_thumbnail)
            fm_folder_model_thumbnail_request(model, item, FM_THUMBNAIL_ICON_CONTENT, FALSE);

        break;
    }
    case FM_FOLDER_MODEL_COL_NAME:
        g_value_set_string(value, fm_file_info_get_disp_name(info));
        break;
    case FM_FOLDER_MODEL_COL_SIZE:
        g_value_set_string( value, fm_file_info_get_disp_size(info) );
        break;
    case FM_FOLDER_MODEL_COL_DESC:
        g_value_set_string( value, fm_file_info_get_desc(info) );
        break;
    case FM_FOLDER_MODEL_COL_PERM:
//        g_value_set_string( value, fm_file_info_get_disp_perm(info) );
        break;
    case FM_FOLDER_MODEL_COL_OWNER:
//        g_value_set_string( value, fm_file_info_get_disp_owner(info) );
        break;
    case FM_FOLDER_MODEL_COL_MTIME:
        g_value_set_string( value, fm_file_info_get_disp_mtime(info) );
        break;
    case FM_FOLDER_MODEL_COL_INFO:
        g_value_set_pointer(value, info);
        break;
    case FM_FOLDER_MODEL_COL_COLOR_SET:
        g_value_set_boolean(value, model->use_custom_colors && read_item_color(model, item));
        break;
    case FM_FOLDER_MODEL_COL_COLOR:
        if (model->use_custom_colors && read_item_color(model, item))
            g_value_set_boxed(value, &item->color);
        else
            g_value_set_boxed(value, NULL);
        break;
    case FM_FOLDER_MODEL_COL_DIRNAME:
        {
            FmPath* path = fm_file_info_get_path(info);
            FmPath* dirpath = fm_path_get_parent(path);
            if(dirpath)
            {
                char* dirname = fm_path_display_name(dirpath, TRUE);
                /* get name of the folder containing the file. */
                g_value_set_string(value, dirname);
                g_free(dirname);
            }
            break;
        }
    case FM_FOLDER_MODEL_N_COLS: ; /* unused here */
    }
}

static gboolean fm_folder_model_iter_next(GtkTreeModel *tree_model,
                                          GtkTreeIter *iter)
{
    GSequenceIter* item_it, *next_item_it;
    FmFolderModel* model;

    g_return_val_if_fail(FM_IS_FOLDER_MODEL(tree_model), FALSE);

    if( iter == NULL || iter->user_data == NULL )
        return FALSE;

    model = (FmFolderModel*)tree_model;
    item_it = (GSequenceIter *)iter->user_data;

    /* Is this the last iter in the list? */
    next_item_it = g_sequence_iter_next(item_it);

    if( g_sequence_iter_is_end(next_item_it) )
        return FALSE;

    iter->stamp = model->stamp;
    iter->user_data = next_item_it;

    return TRUE;
}

static gboolean fm_folder_model_iter_children(GtkTreeModel *tree_model,
                                              GtkTreeIter *iter,
                                              GtkTreeIter *parent)
{
    FmFolderModel* model;
    GSequenceIter* items_it;
    g_return_val_if_fail(parent == NULL || parent->user_data != NULL, FALSE);

    /* this is a list, nodes have no children */
    if( parent )
        return FALSE;

    /* parent == NULL is a special case; we need to return the first top-level row */
    g_return_val_if_fail(FM_IS_FOLDER_MODEL(tree_model), FALSE);
    model = (FmFolderModel*)tree_model;

    /* No rows => no first row */
//    if ( model->folder->n_items == 0 )
//        return FALSE;

    /* Set iter to first item in list */
    items_it = g_sequence_get_begin_iter(model->items);
    iter->stamp = model->stamp;
    iter->user_data = items_it;
    return TRUE;
}

static gboolean fm_folder_model_iter_has_child(GtkTreeModel *tree_model,
                                               GtkTreeIter *iter)
{
    return FALSE;
}

static gint fm_folder_model_iter_n_children(GtkTreeModel *tree_model,
                                            GtkTreeIter *iter)
{
    FmFolderModel* model;
    g_return_val_if_fail(FM_IS_FOLDER_MODEL(tree_model), -1);
    g_return_val_if_fail(iter == NULL || iter->user_data != NULL, FALSE);
    model = (FmFolderModel*)tree_model;
    /* special case: if iter == NULL, return number of top-level rows */
    if( !iter )
        return g_sequence_get_length(model->items);
    return 0; /* otherwise, this is easy again for a list */
}

static gboolean fm_folder_model_iter_nth_child(GtkTreeModel *tree_model,
                                               GtkTreeIter *iter,
                                               GtkTreeIter *parent,
                                               gint n)
{
    GSequenceIter* items_it;
    FmFolderModel* model;

    g_return_val_if_fail(FM_IS_FOLDER_MODEL(tree_model), FALSE);
    model = (FmFolderModel*)tree_model;

    /* a list has only top-level rows */
    if( parent )
        return FALSE;

    /* special case: if parent == NULL, set iter to n-th top-level row */
    if( n >= g_sequence_get_length(model->items) || n < 0 )
        return FALSE;

    items_it = g_sequence_get_iter_at_pos(model->items, n);
    g_assert( items_it  != g_sequence_get_end_iter(model->items) );

    iter->stamp = model->stamp;
    iter->user_data  = items_it;

    return TRUE;
}

static gboolean fm_folder_model_iter_parent(GtkTreeModel *tree_model,
                                            GtkTreeIter *iter,
                                            GtkTreeIter *child)
{
    return FALSE;
}

static gboolean fm_folder_model_get_sort_column_id(GtkTreeSortable* sortable,
                                                   gint* sort_column_id,
                                                   GtkSortType* order)
{
    FmFolderModel* model = FM_FOLDER_MODEL(sortable);
    if( sort_column_id )
        *sort_column_id = model->sort_col;
    if( order )
        *order = FM_SORT_IS_ASCENDING(model->sort_mode) ? GTK_SORT_ASCENDING : GTK_SORT_DESCENDING;
    return TRUE;
}

static void fm_folder_model_set_sort_column_id(GtkTreeSortable* sortable,
                                               gint sort_column_id,
                                               GtkSortType order)
{
    FmFolderModel* model = FM_FOLDER_MODEL(sortable);
    FmSortMode mode = model->sort_mode;
    mode &= ~FM_SORT_ORDER_MASK;
    if(order == GTK_SORT_ASCENDING)
        mode |= FM_SORT_ASCENDING;
    else
        mode |= FM_SORT_DESCENDING;
    model->sort_col = sort_column_id;
    model->sort_mode = mode;
    gtk_tree_sortable_sort_column_changed(sortable);
    fm_folder_model_do_sort(model);
}

static void fm_folder_model_set_sort_func(GtkTreeSortable *sortable,
                                          gint sort_column_id,
                                          GtkTreeIterCompareFunc sort_func,
                                          gpointer user_data,
                                          GDestroyNotify destroy)
{
    g_warning("fm_folder_model_set_sort_func: Not supported\n");
}

static void fm_folder_model_set_default_sort_func(GtkTreeSortable *sortable,
                                                  GtkTreeIterCompareFunc sort_func,
                                                  gpointer user_data,
                                                  GDestroyNotify destroy)
{
    g_warning("fm_folder_model_set_default_sort_func: Not supported\n");
}

static gint fm_folder_model_compare(gconstpointer item1,
                                    gconstpointer item2,
                                    gpointer user_data)
{
    FmFolderModel* model = FM_FOLDER_MODEL(user_data);
    FmFileInfo* file1 = ((FmFolderItem*)item1)->inf;
    FmFileInfo* file2 = ((FmFolderItem*)item2)->inf;
    const char* key1;
    const char* key2;
    goffset diff;
    int ret = 0;

    /* put folders before files */
//    if(model->sort_mode & FM_SORT_FOLDER_FIRST)
    {
        ret = fm_file_info_is_directory(file2) - fm_file_info_is_directory(file1);
        if( ret )
            return ret;
    }

    if(model->sort_col >= FM_FOLDER_MODEL_N_COLS &&
       model->sort_col < column_infos_n &&
       column_infos[model->sort_col]->compare)
    {
        ret = column_infos[model->sort_col]->compare(file1, file2);
        if(ret == 0)
            goto _sort_by_name;
    }
    else switch( model->sort_col )
    {
    case FM_FOLDER_MODEL_COL_SIZE:
        /* to support files more than 2Gb */
        diff = fm_file_info_get_size(file1) - fm_file_info_get_size(file2);
        if(0 == diff)
            goto _sort_by_name;
        ret = diff > 0 ? 1 : -1;
        break;
    case FM_FOLDER_MODEL_COL_MTIME:
        ret = fm_file_info_get_mtime(file1) - fm_file_info_get_mtime(file2);
        if(0 == ret)
            goto _sort_by_name;
        break;
    case FM_FOLDER_MODEL_COL_DESC:
        /* FIXME: this is very slow */
        ret = g_utf8_collate(fm_file_info_get_desc(file1), fm_file_info_get_desc(file2));
        if(0 == ret)
            goto _sort_by_name;
        break;
    case FM_FOLDER_MODEL_COL_UNSORTED:
        return 0;
    case FM_FOLDER_MODEL_COL_DIRNAME:
        {
            /* get name of the folder containing the file. */
            FmPath* dirpath1 = fm_file_info_get_path(file1);
            FmPath* dirpath2 = fm_file_info_get_path(file2);
            dirpath1 = fm_path_get_parent(dirpath1);
            dirpath2 = fm_path_get_parent(dirpath2);

            /* FIXME: should we compare display names instead? */
            ret = fm_path_compare(dirpath1, dirpath2);
        }
        break;
    default:
_sort_by_name:
        if(model->sort_mode & FM_SORT_CASE_SENSITIVE)
        {
            key1 = fm_file_info_get_collate_key_nocasefold(file1);
            key2 = fm_file_info_get_collate_key_nocasefold(file2);
        }
        else
        {
            key1 = fm_file_info_get_collate_key(file1);
            key2 = fm_file_info_get_collate_key(file2);
            /*
            collate keys are already passed to g_utf8_casefold, no need to
            use strcasecmp here (and g_utf8_collate_key returns a string of
            which case cannot be ignored)
            */
        }
        ret = g_strcmp0(key1, key2);
        break;
    }
    return FM_SORT_IS_ASCENDING(model->sort_mode) ? ret : -ret;
}

static void fm_folder_model_do_sort(FmFolderModel* model)
{
    GHashTable* old_order;
    gint *new_order;
    GSequenceIter *items_it;
    GtkTreePath *path;

    /* if there is only one item */
    if( model->items == NULL || g_sequence_get_length(model->items) <= 1 )
        return;

    old_order = g_hash_table_new(g_direct_hash, g_direct_equal);
    /* save old order */
    items_it = g_sequence_get_begin_iter(model->items);
    while( !g_sequence_iter_is_end(items_it) )
    {
        int i = g_sequence_iter_get_position(items_it);
        g_hash_table_insert( old_order, items_it, GINT_TO_POINTER(i) );
        items_it = g_sequence_iter_next(items_it);
    }

    /* sort the list */
    g_sequence_sort(model->items, fm_folder_model_compare, model);

    /* save new order */
    new_order = g_new( int, g_sequence_get_length(model->items) );
    items_it = g_sequence_get_begin_iter(model->items);
    while( !g_sequence_iter_is_end(items_it) )
    {
        int i = g_sequence_iter_get_position(items_it);
        new_order[i] = GPOINTER_TO_INT(g_hash_table_lookup(old_order, items_it));
        items_it = g_sequence_iter_next(items_it);
    }
    g_hash_table_destroy(old_order);
    path = gtk_tree_path_new();
    gtk_tree_model_rows_reordered(GTK_TREE_MODEL(model),
                                  path, NULL, new_order);
    gtk_tree_path_free(path);
    g_free(new_order);
}

static void _fm_folder_model_insert_item(FmFolder* dir,
                                         FmFolderItem* new_item,
                                         FmFolderModel* model)
{
    GSequenceIter *item_it = g_sequence_insert_sorted(model->items, new_item, fm_folder_model_compare, model);
    _on_visible_item_inserted(model, item_it);
}

/**
 * fm_folder_model_file_created
 * @model: the folder model instance
 * @file: new file into
 *
 * Adds new created @file into @model.
 *
 * Since: 0.1.0
 */
void fm_folder_model_file_created(FmFolderModel* model, FmFileInfo* file)
{
    FmFolderItem* new_item = fm_folder_item_new(file);
    _fm_folder_model_insert_item(model->folder, new_item, model);
}

static inline GSequenceIter* info2iter(FmFolderModel* model, FmFileInfo* file)
{
    return (GSequenceIter*)g_hash_table_lookup(model->items_hash, file);
}

/**
 * fm_folder_model_file_deleted
 * @model: the folder model instance
 * @file: removed file info
 *
 * Removes a @file from @model.
 *
 * Since: 0.1.0
 */
void fm_folder_model_file_deleted(FmFolderModel* model, FmFileInfo* file)
{
    GSequenceIter *seq_it;
    FmFolderItem* item = NULL;

    seq_it = info2iter(model, file);
    if (seq_it)
    {
        _on_visible_item_removed(model, seq_it);
        g_sequence_remove(seq_it);
        return;
    }

    seq_it = g_sequence_get_begin_iter(model->incoming_items);
    while (!g_sequence_iter_is_end(seq_it))
    {
        item = (FmFolderItem*)g_sequence_get(seq_it);
        if (item->inf == file)
        {
            g_sequence_remove(seq_it);
            return;
        }
        seq_it = g_sequence_iter_next(seq_it);
    }


    if(!file_can_show(model, file)) /* if this is a hidden file */
    {
        seq_it = g_sequence_get_begin_iter(model->hidden);
        while( !g_sequence_iter_is_end(seq_it) )
        {
            item = (FmFolderItem*)g_sequence_get(seq_it);
            if( item->inf == file )
            {
                g_sequence_remove(seq_it);
                return;
            }
            seq_it = g_sequence_iter_next(seq_it);
        }
        return;
    }

    g_warning("%s: file not in model: %s", __FUNCTION__, fm_file_info_get_name(file));
    return;
}

/**
 * fm_folder_model_file_changed
 * @model: a folder model instance
 * @file: a file into
 *
 * Updates info for the @file in the @model.
 *
 * Since: 0.1.0
 */
void fm_folder_model_file_changed(FmFolderModel* model, FmFileInfo* file)
{
    FmFolderItem* item = NULL;
    GSequenceIter* items_it;
    GtkTreeIter it;
    GtkTreePath* path;
/*
    if(!file_can_show(model, file))
        return;
*/
    items_it = info2iter(model, file);

    if(!items_it)
        return;
    item = (FmFolderItem*)g_sequence_get(items_it);

    if (item->icon[FM_THUMBNAIL_ICON_SIMPLE].pixbuf)
    {
        g_object_unref(item->icon[FM_THUMBNAIL_ICON_SIMPLE].pixbuf);
        item->icon[FM_THUMBNAIL_ICON_SIMPLE].pixbuf = NULL;
    }

    if (item->icon[FM_THUMBNAIL_ICON_CONTENT].pixbuf)
    {
        g_object_unref(item->icon[FM_THUMBNAIL_ICON_CONTENT].pixbuf);
        item->icon[FM_THUMBNAIL_ICON_CONTENT].pixbuf = NULL;
    }

    it.stamp = model->stamp;
    it.user_data  = items_it;

    path = gtk_tree_path_new_from_indices(g_sequence_iter_get_position(items_it), -1);
    gtk_tree_model_row_changed(GTK_TREE_MODEL(model), path, &it);
    gtk_tree_path_free(path);
}


static inline gboolean file_can_show(FmFolderModel* model, FmFileInfo* file)
{
    if(!model->show_hidden && fm_file_info_is_hidden(file))
        return FALSE;

    if(model->pattern)
    {
        if (!g_pattern_match_string(model->pattern, fm_file_info_get_name(file)) && !fm_file_info_is_directory(file))
            return FALSE;
    }

    if(model->filters)
    {
        GSList* l;
        for(l = model->filters; l; l=l->next)
        {
            FmFolderModelFilterItem* item = (FmFolderModelFilterItem*)l->data;
            if(!item->func(file, item->user_data))
                return FALSE;
        }
    }
    return TRUE;
}

/**
 * fm_folder_model_get_show_hidden
 * @model: the folder model instance
 *
 * Retrieves info whether folder model includes hidden files.
 *
 * Returns: %TRUE if hidden files are visible within @model.
 *
 * Since: 0.1.0
 */
gboolean fm_folder_model_get_show_hidden(FmFolderModel* model)
{
    return model->show_hidden;
}

/**
 * fm_folder_model_set_show_hidden
 * @model: the folder model instance
 * @show_hidden: whether show hidden files or not
 *
 * Changes visibility of hodden files within @model.
 *
 * Since: 0.1.0
 */
void fm_folder_model_set_show_hidden(FmFolderModel* model, gboolean show_hidden)
{
    g_return_if_fail(model != NULL);
    if( model->show_hidden == show_hidden )
        return;
    model->show_hidden = show_hidden;
    fm_folder_model_apply_filters(model);
}

void fm_folder_model_set_pattern(FmFolderModel* model, const char * pattern_str)
{
    if (!pattern_str)
        pattern_str = "";

    if (model->pattern_str && strcmp(model->pattern_str, pattern_str) == 0)
        return;

    if (model->pattern_str)
        g_free(model->pattern_str);

    model->pattern_str = g_strdup(pattern_str);

    if (model->pattern)
    {
        g_pattern_spec_free(model->pattern);
        model->pattern = NULL;
    }

    if (*pattern_str)
        model->pattern = g_pattern_spec_new(pattern_str);

    fm_folder_model_apply_filters(model);
}

const char * fm_folder_model_get_pattern(FmFolderModel* model)
{
    return model->pattern_str ? model->pattern_str : "";
}

/**
 * fm_folder_model_find_iter_by_filename
 * @model: the folder model instance
 * @it: pointer to iterator to fill
 * @name: file name to search
 *
 * Searches @model for existance of some file in it. If file was found
 * then sets @it to match found file.
 *
 * Returns: %TRUE if file was found.
 *
 * Since: 0.1.0
 */
gboolean fm_folder_model_find_iter_by_filename(FmFolderModel* model, GtkTreeIter* it, const char* name)
{
    GSequenceIter *item_it = g_sequence_get_begin_iter(model->items);
    for( ; !g_sequence_iter_is_end(item_it); item_it = g_sequence_iter_next(item_it) )
    {
        FmFolderItem* item = (FmFolderItem*)g_sequence_get(item_it);
        FmPath* path = fm_file_info_get_path(item->inf);
        if( g_strcmp0(fm_path_get_basename(path), name) == 0 )
        {
            it->stamp = model->stamp;
            it->user_data  = item_it;
            return TRUE;
        }
    }
    return FALSE;
}

/*****************************************************************************/

static void update_fallback_icon(FmFolderModel* model)
{
    if (model->fallback_icon_pixbuf)
        g_object_unref(model->fallback_icon_pixbuf);

    FmIcon * fm_icon = fm_icon_from_name("unknown");
    model->fallback_icon_pixbuf = fm_pixbuf_from_icon(fm_icon, model->icon_size);
    fm_icon_unref(fm_icon);
}

static void fm_folder_model_reload_icons(FmFolderModel* model, enum ReloadFlags flags)
{
    update_fallback_icon(model);

    fm_folder_model_cancel_outdated_thumbnail_requests(model);

    GSequenceIter* it = g_sequence_get_begin_iter(model->items);
    GtkTreePath* tp = gtk_tree_path_new_from_indices(0, -1);

    for( ; !g_sequence_iter_is_end(it); it = g_sequence_iter_next(it) )
    {
        FmFolderItem* item = (FmFolderItem*)g_sequence_get(it);

        gboolean changed = FALSE;

        if (item->icon[FM_THUMBNAIL_ICON_CONTENT].pixbuf && (flags & RELOAD_THUMBNAILS))
        {
            g_object_unref(item->icon[FM_THUMBNAIL_ICON_CONTENT].pixbuf);
            item->icon[FM_THUMBNAIL_ICON_CONTENT].pixbuf = NULL;
            fm_folder_model_thumbnail_request(model, item, FM_THUMBNAIL_ICON_CONTENT, TRUE);
            changed = TRUE;
        }
        item->icon[FM_THUMBNAIL_ICON_CONTENT].loading = FALSE;

        if (item->icon[FM_THUMBNAIL_ICON_SIMPLE].pixbuf && (flags & RELOAD_ICONS))
        {
            g_object_unref(item->icon[FM_THUMBNAIL_ICON_SIMPLE].pixbuf);
            item->icon[FM_THUMBNAIL_ICON_SIMPLE].pixbuf = NULL;
            fm_folder_model_thumbnail_request(model, item, FM_THUMBNAIL_ICON_SIMPLE, TRUE);
            changed = TRUE;
        }
        item->icon[FM_THUMBNAIL_ICON_SIMPLE].loading = FALSE;

        if (changed)
        {
            GtkTreeIter tree_it;
            tree_it.stamp = model->stamp;
            tree_it.user_data = it;
            gtk_tree_model_row_changed(GTK_TREE_MODEL(model), tp, &tree_it);
        }
        gtk_tree_path_next(tp);
    }
    gtk_tree_path_free(tp);

    it = g_sequence_get_begin_iter(model->hidden);
    for( ; !g_sequence_iter_is_end(it); it = g_sequence_iter_next(it) )
    {
        FmFolderItem * item = (FmFolderItem*)g_sequence_get(it);

        if (item->icon[FM_THUMBNAIL_ICON_SIMPLE].pixbuf)
        {
            g_object_unref(item->icon[FM_THUMBNAIL_ICON_SIMPLE].pixbuf);
            item->icon[FM_THUMBNAIL_ICON_SIMPLE].pixbuf = NULL;
        }
        item->icon[FM_THUMBNAIL_ICON_SIMPLE].loading = FALSE;

        if (item->icon[FM_THUMBNAIL_ICON_CONTENT].pixbuf)
        {
            g_object_unref(item->icon[FM_THUMBNAIL_ICON_CONTENT].pixbuf);
            item->icon[FM_THUMBNAIL_ICON_CONTENT].pixbuf = NULL;
        }
        item->icon[FM_THUMBNAIL_ICON_CONTENT].loading = FALSE;
    }
}

static void on_icon_theme_changed(GtkIconTheme* theme, FmFolderModel* model)
{
    fm_folder_model_reload_icons(model, RELOAD_ICONS);
}

static gboolean _fm_file_info_can_thumbnail(FmFileInfo * fi, FmThumbnailIconType icon_type)
{
    if (icon_type == FM_THUMBNAIL_ICON_SIMPLE)
        return TRUE;

    if (fm_config->thumbnail_local && !fm_path_is_native_or_trash(fm_file_info_get_path(fi)))
        return FALSE;

    return fm_file_info_can_thumbnail(fi);
}

static FmFolderItem * _folder_item_for_file_info(FmFolderModel * model, FmFileInfo* fi, GSequenceIter ** _seq_it)
{
    GSequenceIter* seq_it = info2iter(model, fi);

    if (_seq_it)
        *_seq_it = seq_it;

    if (!seq_it)
        return NULL;

    return (FmFolderItem *) g_sequence_get(seq_it);
}

static void on_thumbnail_loaded(FmThumbnailRequest * req, gpointer user_data)
{
    FmFolderModel * model = (FmFolderModel *) user_data;

    model->thumbnail_requests = g_list_remove(model->thumbnail_requests, req);

    guint size = fm_thumbnail_request_get_size(req);
    FmFileInfo* fi = fm_thumbnail_request_get_file_info(req);
    GdkPixbuf* pix = fm_thumbnail_request_get_pixbuf(req);
    FmThumbnailIconType icon_type = fm_thumbnail_request_get_icon_type(req);

    if (icon_type != FM_THUMBNAIL_ICON_SIMPLE && icon_type != FM_THUMBNAIL_ICON_CONTENT)
        return;

    /*g_debug("%s: %s: %d %d", __FUNCTION__, fm_file_info_get_name(fi), size, icon_type);*/

    if (size != model->icon_size)
        return;

    GSequenceIter * seq_it;
    FmFolderItem * item = _folder_item_for_file_info(model, fi, &seq_it);
    if (!item)
        return;

    if (pix)
    {
        GtkTreeIter it;
        it.stamp = model->stamp;
        it.user_data = seq_it;
        GtkTreePath* tp = fm_folder_model_get_path(GTK_TREE_MODEL(model), &it);

        if (item->icon[icon_type].pixbuf)
            g_object_unref(item->icon[icon_type].pixbuf);
        item->icon[icon_type].pixbuf = g_object_ref(pix);

        gdk_threads_enter();
        gtk_tree_model_row_changed(GTK_TREE_MODEL(model), tp, &it);
        gtk_tree_path_free(tp);
        gdk_threads_leave();
    }
    else
    {
        item->icon[icon_type].failed = TRUE;
    }

    item->icon[icon_type].loading = FALSE;
}

static void fm_folder_model_thumbnail_request(FmFolderModel * model, FmFolderItem * item,
                                              FmThumbnailIconType icon_type, gboolean force_reload)
{
    if (!force_reload)
    {
        if (item->icon[icon_type].pixbuf ||
            item->icon[icon_type].failed ||
            item->icon[icon_type].loading)
        {
            return;
        }
    }

    gboolean can_thumbnail = _fm_file_info_can_thumbnail(item->inf, icon_type);

    if (can_thumbnail)
    {
        FmThumbnailRequest * req = fm_thumbnail_request(item->inf, model->icon_size, icon_type, on_thumbnail_loaded, model);
        if (req)
            model->thumbnail_requests = g_list_prepend(model->thumbnail_requests, req);
        else
            can_thumbnail = FALSE;
    }

    if (can_thumbnail)
        item->icon[icon_type].loading = TRUE;
    else
        item->icon[icon_type].failed = TRUE;
}

static void fm_folder_model_cancel_outdated_thumbnail_requests(FmFolderModel * model)
{
    GList * l;
    for (l = model->thumbnail_requests; l; )
    {
        GList * next = l->next;
        FmThumbnailRequest * req = (FmThumbnailRequest *) l->data;

        guint icon_size = fm_thumbnail_request_get_size(req);
        FmThumbnailIconType icon_type = fm_thumbnail_request_get_icon_type(req);
        FmFileInfo * fi = fm_thumbnail_request_get_file_info(req);

        if (icon_size != model->icon_size || _fm_file_info_can_thumbnail(fi, icon_type))
        {
            fm_thumbnail_request_cancel(req);
            model->thumbnail_requests = g_list_delete_link(model->thumbnail_requests, l);

            FmFolderItem * item = _folder_item_for_file_info(model, fi, NULL);
            if (item)
            {
                item->icon[icon_type].loading = FALSE;
            }
        }
        l = next;
    }
}

static void on_show_thumbnail_changed(FmConfig* cfg, gpointer user_data)
{
    FmFolderModel * model = (FmFolderModel *) user_data;
    fm_folder_model_reload_icons(model, RELOAD_THUMBNAILS);
}

static void on_thumbnail_local_changed(FmConfig* cfg, gpointer user_data)
{
    FmFolderModel * model = (FmFolderModel *) user_data;
    fm_folder_model_reload_icons(model, RELOAD_THUMBNAILS);
}

static void on_thumbnail_max_changed(FmConfig* cfg, gpointer user_data)
{
    FmFolderModel * model = (FmFolderModel *) user_data;
    fm_folder_model_reload_icons(model, RELOAD_THUMBNAILS);
}

/*****************************************************************************/

/**
 * fm_folder_model_set_icon_size
 * @model: the folder model instance
 * @icon_size: new size for icons in pixels
 *
 * Changes the size of icons in @model data.
 *
 * Since: 0.1.0
 */
void fm_folder_model_set_icon_size(FmFolderModel* model, guint icon_size)
{
    if (model->icon_size == icon_size)
        return;
    model->icon_size = icon_size;
    fm_folder_model_reload_icons(model, RELOAD_BOTH);
}

/**
 * fm_folder_model_get_icon_size
 * @model: the folder model instance
 *
 * Retrieves the size of icons in @model data.
 *
 * Returns: size of icons in pixels.
 *
 * Since: 0.1.0
 */
guint fm_folder_model_get_icon_size(FmFolderModel* model)
{
    return model->icon_size;
}

/*****************************************************************************/

/**
 * fm_folder_model_set_item_userdata
 * @model     : the folder model instance
 * @it        : iterator of row to set data
 * @user_data : user data that will be associated with the row
 *
 * Sets the data that can be retrieved by fm_folder_model_get_item_userdata().
 *
 * Since: 1.0.0
 */
void fm_folder_model_set_item_userdata(FmFolderModel* model, GtkTreeIter* it,
                                       gpointer user_data)
{
    GSequenceIter* item_it;
    FmFolderItem* item;

    g_return_if_fail(it != NULL);
    g_return_if_fail(model != NULL);
    g_return_if_fail(it->stamp == model->stamp);
    item_it = (GSequenceIter*)it->user_data;
    g_return_if_fail(item_it != NULL);
    item = (FmFolderItem*)g_sequence_get(item_it);
    item->userdata = user_data;
}

/**
 * fm_folder_model_get_item_userdata
 * @model:  the folder model instance
 * @it:     iterator of row to retrieve data
 *
 * Returns the data that was set by last call of
 * fm_folder_model_set_item_userdata() on that row.
 *
 * Return value: user data that was set on that row
 *
 * Since: 1.0.0
 */
gpointer fm_folder_model_get_item_userdata(FmFolderModel* model, GtkTreeIter* it)
{
    GSequenceIter* item_it;
    FmFolderItem* item;

    g_return_val_if_fail(it != NULL, NULL);
    g_return_val_if_fail(model != NULL, NULL);
    g_return_val_if_fail(it->stamp == model->stamp, NULL);
    item_it = (GSequenceIter*)it->user_data;
    g_return_val_if_fail(item_it != NULL, NULL);
    item = (FmFolderItem*)g_sequence_get(item_it);
    return item->userdata;
}

/**
 * fm_folder_model_add_filter
 * @model:  the folder model instance
 * @func:   a filter function
 * @user_data: user data passed to the filter function
 *
 * Install a filter function to filter out and hide some items.
 * This only install a filter function and does not update content of the model.
 * You need to call fm_folder_model_apply_filters() to refilter the model.
 *
 * Since: 1.0.2
 */
void fm_folder_model_add_filter(FmFolderModel* model, FmFolderModelFilterFunc func, gpointer user_data)
{
    FmFolderModelFilterItem* item = g_slice_new(FmFolderModelFilterItem);
    item->func = func;
    item->user_data = user_data;
    model->filters = g_slist_prepend(model->filters, item);
}

/**
 * fm_folder_model_remove_filter
 * @model:  the folder model instance
 * @func:   a filter function
 * @user_data: user data passed to the filter function
 *
 * Remove a filter function previously installed by fm_folder_model_add_filter()
 * This only remove the filter function and does not update content of the model.
 * You need to call fm_folder_model_apply_filters() to refilter the model.
 *
 * Since: 1.0.2
 */
void fm_folder_model_remove_filter(FmFolderModel* model, FmFolderModelFilterFunc func, gpointer user_data)
{
    GSList* l;
    for(l = model->filters; l; l=l->next)
    {
        FmFolderModelFilterItem* item = (FmFolderModelFilterItem*)l->data;
        if(item->func == func && item->user_data == user_data)
        {
            model->filters = g_slist_delete_link(model->filters, l);
            fm_folder_model_filter_item_free(item);
            break;
        }
    }
}

/**
 * fm_folder_model_apply_filters
 * @model:  the folder model instance
 *
 * After changing the filters by fm_folder_model_add_filter() or
 * fm_folder_model_remove_filter(), you have to call this function
 * to really apply the filter to the model content.
 * This is for performance reason.
 * You can add many filter functions and also remove some, and then
 * call fm_folder_model_apply_filters() to update the content of the model once.
 *
 * If you forgot to call fm_folder_model_apply_filters(), the content of the
 * model may be incorrect.
 *
 * Since: 1.0.2
 */
void fm_folder_model_apply_filters(FmFolderModel* model)
{
    FmFolderItem* item;
    GSList* items_to_show = NULL;
    GSequenceIter *item_it;

    /* make previously hidden items visible again if they can be shown */
    item_it = g_sequence_get_begin_iter(model->hidden);
    while(!g_sequence_iter_is_end(item_it)) /* iterate over the hidden list */
    {
        item = (FmFolderItem*)g_sequence_get(item_it);
        if(file_can_show(model, item->inf)) /* if the file should be shown */
        {
            /* we delay the real insertion operation and do it
             * after we finish hiding some currently visible items for
             * apparent performance reasons. */
            items_to_show = g_slist_prepend(items_to_show, item_it);
        }
        item_it = g_sequence_iter_next(item_it);
    }

    /* move currently visible items to hidden list if they should be hidden */
    item_it = g_sequence_get_begin_iter(model->items);
    while(!g_sequence_iter_is_end(item_it)) /* iterate over currently visible items */
    {
        GSequenceIter *next_item_it = g_sequence_iter_next(item_it); /* next item */
        item = (FmFolderItem*)g_sequence_get(item_it);
        if(!file_can_show(model, item->inf)) /* it should be hidden */
        {
            /* move the item from visible list to hidden list */
            _on_visible_item_removed(model, item_it);
            g_sequence_move(item_it, g_sequence_get_begin_iter(model->hidden));
        }
        item_it = next_item_it;
    }

    /* show items scheduled for showing */
    if(items_to_show)
    {
        GSList* l;
        for(l = items_to_show; l; l = l->next)
        {
            GSequenceIter *insert_item_it;
            item_it = (GSequenceIter*)l->data; /* this is a iterator from hidden list */
            item = (FmFolderItem*)g_sequence_get(item_it);

            /* find a nice position in visible item list to insert the item */
            insert_item_it = g_sequence_search(model->items, item,
                                                          fm_folder_model_compare, model);
            /* move the item from hidden items to visible items list */
            g_sequence_move(item_it, insert_item_it);
            _on_visible_item_inserted(model, item_it);
        }
        g_slist_free(items_to_show);
    }
    g_signal_emit(model, signals[FILTER_CHANGED], 0);
    g_signal_emit(model, signals[FILTERING_CHANGED], 0);
}

/**
 * fm_folder_model_set_sort
 * @model: model to apply
 * @col: new sorting column
 * @mode: new sorting mode
 *
 * Changes sorting of @model items in accordance to new @col and @mode.
 * If new parameters are not different from previous then nothing will
 * be changed (nor any signal emitted).
 *
 * Since: 1.0.2
 */
void fm_folder_model_set_sort(FmFolderModel* model, FmFolderModelCol col, FmSortMode mode)
{
    FmFolderModelCol old_col = model->sort_col;

    /* g_debug("fm_folder_model_set_sort: col %x mode %x", col, mode); */
    if((guint)col >= column_infos_n)
        col = old_col;
    if(mode == FM_SORT_DEFAULT)
        mode = model->sort_mode;
    if(model->sort_mode != mode || old_col != col)
    {
        model->sort_mode = mode;
        gtk_tree_sortable_set_sort_column_id(GTK_TREE_SORTABLE(model), col,
                FM_SORT_IS_ASCENDING(mode) ? GTK_SORT_ASCENDING : GTK_SORT_DESCENDING);
    }
}

/**
 * fm_folder_model_get_sort
 * @model: model to query
 * @col: (out) (allow-none): location to store sorting column
 * @mode: (out) (allow-none): location to store sorting mode
 *
 * Retrieves current sorting criteria for @model.
 *
 * Returns: %TRUE if @model is valid.
 *
 * Since: 1.0.2
 */
gboolean fm_folder_model_get_sort(FmFolderModel* model, FmFolderModelCol *col,
                                  FmSortMode *mode)
{
    if(!FM_IS_FOLDER_MODEL(model))
        return FALSE;
    /* g_debug("fm_folder_model_get_sort: col %x mode %x", model->sort_col, model->sort_mode); */
    if(col)
        *col = model->sort_col;
    if(mode)
        *mode = model->sort_mode;
    return TRUE;
}

/*****************************************************************************/

void fm_folder_model_set_use_custom_colors(FmFolderModel* model, gboolean use_custom_colors)
{
    model->use_custom_colors = use_custom_colors;
}

gboolean fm_folder_model_get_use_custom_colors(FmFolderModel* model)
{
    return model->use_custom_colors;
}

/*****************************************************************************/

int fm_folder_model_get_n_visible_items(FmFolderModel * model)
{
    return g_sequence_get_length(model->items);
}

int fm_folder_model_get_n_hidden_items(FmFolderModel * model)
{
    return g_sequence_get_length(model->hidden);
}

int fm_folder_model_get_n_incoming_items(FmFolderModel * model)
{
    return g_sequence_get_length(model->incoming_items);
}
