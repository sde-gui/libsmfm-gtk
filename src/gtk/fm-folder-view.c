/*
 *      fm-folder-view.c
 *
 *      Copyright 2012-2013 Andriy Grytsenko (LStranger) <andrej@rep.kiev.ua>
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

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif
#include "gtk-compat.h"

#include <glib/gi18n-lib.h>

#include "fm-folder-view.h"

G_DEFINE_INTERFACE(FmFolderView, fm_folder_view, GTK_TYPE_WIDGET);

enum
{
    CLICKED,
    SEL_CHANGED,
    SORT_CHANGED,
    FILTER_CHANGED,
    COLUMNS_CHANGED,
    //CHDIR,
    N_SIGNALS
};

static guint signals[N_SIGNALS];

static void fm_folder_view_default_init(FmFolderViewInterface *iface)
{
    /* properties and signals */
    /**
     * FmFolderView::clicked:
     * @view: the widget that emitted the signal
     * @type: (#FmFolderViewClickType) type of click
     * @file: (#FmFileInfo *) file on which cursor is
     *
     * The #FmFolderView::clicked signal is emitted when user clicked
     * somewhere in the folder area. If click was on free folder area
     * then @file is %NULL.
     *
     * Since: 0.1.0
     */
    signals[CLICKED]=
        g_signal_new("clicked",
                     FM_TYPE_FOLDER_VIEW,
                     G_SIGNAL_RUN_FIRST,
                     G_STRUCT_OFFSET(FmFolderViewInterface, clicked),
                     NULL, NULL,
                     g_cclosure_marshal_VOID__UINT_POINTER,
                     G_TYPE_NONE, 2, G_TYPE_UINT, G_TYPE_POINTER);

    /**
     * FmFolderView::sel-changed:
     * @view: the widget that emitted the signal
     * @n_sel: number of files currently selected in the folder
     *
     * The #FmFolderView::sel-changed signal is emitted when
     * selection of the view got changed.
     *
     * Before 1.0.0 parameter was list of currently selected files.
     *
     * Since: 0.1.0
     */
    signals[SEL_CHANGED]=
        g_signal_new("sel-changed",
                     FM_TYPE_FOLDER_VIEW,
                     G_SIGNAL_RUN_FIRST,
                     G_STRUCT_OFFSET(FmFolderViewInterface, sel_changed),
                     NULL, NULL,
                     g_cclosure_marshal_VOID__INT,
                     G_TYPE_NONE, 1, G_TYPE_INT);

    /**
     * FmFolderView::sort-changed:
     * @view: the widget that emitted the signal
     *
     * The #FmFolderView::sort-changed signal is emitted when sorting
     * of the view got changed.
     *
     * Since: 0.1.10
     */
    signals[SORT_CHANGED]=
        g_signal_new("sort-changed",
                     FM_TYPE_FOLDER_VIEW,
                     G_SIGNAL_RUN_FIRST,
                     G_STRUCT_OFFSET(FmFolderViewInterface, sort_changed),
                     NULL, NULL,
                     g_cclosure_marshal_VOID__VOID,
                     G_TYPE_NONE, 0);

    /**
     * FmFolderView::filter-changed:
     * @view: the widget that emitted the signal
     *
     * The #FmFolderView::filter-changed signal is emitted when filter
     * of the view model got changed. It's just bouncer for the same
     * signal of #FmFolderModel.
     *
     * Since: 1.0.2
     */
    signals[FILTER_CHANGED]=
        g_signal_new("filter-changed",
                     FM_TYPE_FOLDER_VIEW,
                     G_SIGNAL_RUN_FIRST,
                     G_STRUCT_OFFSET(FmFolderViewInterface, filter_changed),
                     NULL, NULL,
                     g_cclosure_marshal_VOID__VOID,
                     G_TYPE_NONE, 0);

    /**
     * FmFolderView::columns-changed:
     * @view: the widget that emitted the signal
     *
     * The #FmFolderView::columns-changed signal is emitted when layout
     * of #FmFolderView instance is changed, i.e. some column is added,
     * deleted, or changed its size.
     *
     * Since: 1.2.0
     */
    signals[COLUMNS_CHANGED]=
        g_signal_new("columns-changed",
                     FM_TYPE_FOLDER_VIEW,
                     G_SIGNAL_RUN_FIRST,
                     G_STRUCT_OFFSET(FmFolderViewInterface, columns_changed),
                     NULL, NULL,
                     g_cclosure_marshal_VOID__VOID,
                     G_TYPE_NONE, 0);

    /* FIXME: add "chdir" so main-win can connect to it and sync side-pane */
}

static void on_sort_col_changed(GtkTreeSortable* sortable, FmFolderView* fv)
{
    if(fm_folder_model_get_sort(FM_FOLDER_MODEL(sortable), NULL, NULL))
    {
        g_signal_emit(fv, signals[SORT_CHANGED], 0);
    }
}

static void on_filter_changed(FmFolderModel* model, FmFolderView* fv)
{
    g_signal_emit(fv, signals[FILTER_CHANGED], 0);
}

/**
 * fm_folder_view_set_selection_mode
 * @fv: a widget to apply
 * @mode: new mode of selection in @fv.
 *
 * Changes selection mode in @fv.
 *
 * Since: 0.1.0
 */
void fm_folder_view_set_selection_mode(FmFolderView* fv, GtkSelectionMode mode)
{
    g_return_if_fail(FM_IS_FOLDER_VIEW(fv));

    FM_FOLDER_VIEW_GET_IFACE(fv)->set_sel_mode(fv, mode);
}

/**
 * fm_folder_view_get_selection_mode
 * @fv: a widget to inspect
 *
 * Retrieves current selection mode in @fv.
 *
 * Returns: current selection mode.
 *
 * Since: 0.1.0
 */
GtkSelectionMode fm_folder_view_get_selection_mode(FmFolderView* fv)
{
    g_return_val_if_fail(FM_IS_FOLDER_VIEW(fv), GTK_SELECTION_NONE);

    return (*FM_FOLDER_VIEW_GET_IFACE(fv)->get_sel_mode)(fv);
}

/**
 * fm_folder_view_sort
 * @fv: a widget to apply
 * @type: new mode of sorting (ascending or descending)
 * @by: criteria of sorting
 *
 * Changes sorting in the view. Invalid values for @type or @by are
 * ignored (will not change sorting).
 *
 * Since 1.0.2 values passed to this API aren't remembered in the @fv
 * object. If @fv has no model then this API has no effect.
 * After the model is removed from @fv (calling fm_folder_view_set_model()
 * with NULL) there is no possibility to recover last settings and any
 * model added to @fv later will get defaults: FM_FOLDER_MODEL_COL_DEFAULT
 * and FM_SORT_DEFAULT.
 *
 * Since: 0.1.0
 *
 * Deprecated: 1.0.2: Use fm_folder_model_set_sort() instead.
 */
void fm_folder_view_sort(FmFolderView* fv, GtkSortType type, FmFolderModelCol by)
{
    FmFolderViewInterface* iface;
    FmFolderModel* model;
    FmSortMode mode;

    g_return_if_fail(FM_IS_FOLDER_VIEW(fv));

    iface = FM_FOLDER_VIEW_GET_IFACE(fv);
    model = iface->get_model(fv);
    if(model)
    {
        if(type == GTK_SORT_ASCENDING || type == GTK_SORT_DESCENDING)
        {
            fm_folder_model_get_sort(model, NULL, &mode);
            mode &= ~FM_SORT_ORDER_MASK;
            mode |= (type == GTK_SORT_ASCENDING) ? FM_SORT_ASCENDING : FM_SORT_DESCENDING;
        }
        else
            mode = FM_SORT_DEFAULT;
        fm_folder_model_set_sort(model, by, mode);
    }
    /* model will generate signal to update config if changed */
}

/**
 * fm_folder_view_get_sort_type
 * @fv: a widget to inspect
 *
 * Retrieves current sorting type in @fv.
 *
 * Returns: mode of sorting (ascending or descending)
 *
 * Since: 0.1.0
 *
 * Deprecated: 1.0.2: use fm_folder_model_get_sort() instead.
 */
GtkSortType fm_folder_view_get_sort_type(FmFolderView* fv)
{
    FmFolderViewInterface* iface;
    FmFolderModel* model;
    FmSortMode mode;

    g_return_val_if_fail(FM_IS_FOLDER_VIEW(fv), GTK_SORT_ASCENDING);

    iface = FM_FOLDER_VIEW_GET_IFACE(fv);
    model = iface->get_model(fv);
    if(model == NULL || !fm_folder_model_get_sort(model, NULL, &mode))
        return GTK_SORT_ASCENDING;
    return FM_SORT_IS_ASCENDING(mode) ? GTK_SORT_ASCENDING : GTK_SORT_DESCENDING;
}

/**
 * fm_folder_view_get_sort_by
 * @fv: a widget to inspect
 *
 * Retrieves current criteria of sorting in @fv (e.g. by name).
 *
 * Returns: criteria of sorting.
 *
 * Since: 0.1.0
 *
 * Deprecated: 1.0.2: use fm_folder_model_get_sort() instead.
 */
FmFolderModelCol fm_folder_view_get_sort_by(FmFolderView* fv)
{
    FmFolderViewInterface* iface;
    FmFolderModel* model;
    FmFolderModelCol by;

    g_return_val_if_fail(FM_IS_FOLDER_VIEW(fv), FM_FOLDER_MODEL_COL_DEFAULT);

    iface = FM_FOLDER_VIEW_GET_IFACE(fv);
    model = iface->get_model(fv);
    if(model == NULL || !fm_folder_model_get_sort(model, &by, NULL))
        return FM_FOLDER_MODEL_COL_DEFAULT;
    return by;
}

/**
 * fm_folder_view_set_show_hidden
 * @fv: a widget to apply
 * @show: new setting
 *
 * Changes whether hidden files in folder shown in @fv should be visible
 * or not.
 *
 * See also: fm_folder_view_get_show_hidden().
 *
 * Since: 0.1.0
 */
void fm_folder_view_set_show_hidden(FmFolderView* fv, gboolean show)
{
    FmFolderViewInterface* iface;

    g_return_if_fail(FM_IS_FOLDER_VIEW(fv));

    iface = FM_FOLDER_VIEW_GET_IFACE(fv);
    if(iface->get_show_hidden(fv) != show)
    {
        FmFolderModel* model;
        iface->set_show_hidden(fv, show);
        model = iface->get_model(fv);
        if(G_LIKELY(model))
            fm_folder_model_set_show_hidden(model, show);
        /* FIXME: signal to update config */
    }
}

/**
 * fm_folder_view_get_show_hidden
 * @fv: a widget to inspect
 *
 * Retrieves setting whether hidden files in folder shown in @fv should
 * be visible or not.
 *
 * See also: fm_folder_view_set_show_hidden().
 *
 * Returns: %TRUE if hidden files are visible.
 *
 * Since: 0.1.0
 */
gboolean fm_folder_view_get_show_hidden(FmFolderView* fv)
{
    g_return_val_if_fail(FM_IS_FOLDER_VIEW(fv), FALSE);

    return (*FM_FOLDER_VIEW_GET_IFACE(fv)->get_show_hidden)(fv);
}

/**
 * fm_folder_view_get_folder
 * @fv: a widget to inspect
 *
 * Retrieves the folder shown by @fv. Returned data are owned by @fv and
 * should not be freed by caller.
 *
 * Returns: (transfer none): the folder of view.
 *
 * Since: 1.0.0
 */
FmFolder* fm_folder_view_get_folder(FmFolderView* fv)
{
    g_return_val_if_fail(FM_IS_FOLDER_VIEW(fv), NULL);

    return (*FM_FOLDER_VIEW_GET_IFACE(fv)->get_folder)(fv);
}

/**
 * fm_folder_view_get_cwd
 * @fv: a widget to inspect
 *
 * Retrieves file path of the folder shown by @fv. Returned data are
 * owned by @fv and should not be freed by caller.
 *
 * Returns: (transfer none): file path of the folder.
 *
 * Since: 0.1.0
 */
FmPath* fm_folder_view_get_cwd(FmFolderView* fv)
{
    FmFolder* folder;

    g_return_val_if_fail(FM_IS_FOLDER_VIEW(fv), NULL);

    folder = FM_FOLDER_VIEW_GET_IFACE(fv)->get_folder(fv);
    return folder ? fm_folder_get_path(folder) : NULL;
}

/**
 * fm_folder_view_get_cwd_info
 * @fv: a widget to inspect
 *
 * Retrieves file info of the folder shown by @fv. Returned data are
 * owned by @fv and should not be freed by caller.
 *
 * Returns: (transfer none): file info descriptor of the folder.
 *
 * Since: 0.1.0
 */
FmFileInfo* fm_folder_view_get_cwd_info(FmFolderView* fv)
{
    FmFolder* folder;

    g_return_val_if_fail(FM_IS_FOLDER_VIEW(fv), NULL);

    folder = FM_FOLDER_VIEW_GET_IFACE(fv)->get_folder(fv);
    return folder ? fm_folder_get_info(folder) : NULL;
}

/**
 * fm_folder_view_get_model
 * @fv: a widget to inspect
 *
 * Retrieves the model used by @fv. Returned data are owned by @fv and
 * should not be freed by caller.
 *
 * Returns: (transfer none): the model of view.
 *
 * Since: 0.1.16
 */
FmFolderModel* fm_folder_view_get_model(FmFolderView* fv)
{
    g_return_val_if_fail(FM_IS_FOLDER_VIEW(fv), NULL);

    return (*FM_FOLDER_VIEW_GET_IFACE(fv)->get_model)(fv);
}

static void unset_model(FmFolderView* fv, FmFolderModel* model)
{
    g_signal_handlers_disconnect_by_func(model, on_sort_col_changed, fv);
    g_signal_handlers_disconnect_by_func(model, on_filter_changed, fv);
}

/**
 * fm_folder_view_set_model
 * @fv: a widget to apply
 * @model: (allow-none): new view model
 *
 * Changes model for the @fv.
 *
 * Since: 1.0.0
 */
void fm_folder_view_set_model(FmFolderView* fv, FmFolderModel* model)
{
    FmFolderViewInterface* iface;
    FmFolderModel* old_model;
    FmFolderModelCol by = FM_FOLDER_MODEL_COL_DEFAULT;
    FmSortMode mode = FM_SORT_ASCENDING;

    g_return_if_fail(FM_IS_FOLDER_VIEW(fv));

    iface = FM_FOLDER_VIEW_GET_IFACE(fv);
    old_model = iface->get_model(fv);
    if(old_model)
    {
        fm_folder_model_get_sort(old_model, &by, &mode);
        unset_model(fv, old_model);
    }
    /* FIXME: which setting to apply if this is first model? */
    iface->set_model(fv, model);
    if(model)
    {
        fm_folder_model_set_sort(model, by, mode);
        g_signal_connect(model, "sort-column-changed", G_CALLBACK(on_sort_col_changed), fv);
        g_signal_connect(model, "filter-changed", G_CALLBACK(on_filter_changed), fv);
    }
}

void fm_folder_view_disconnect_model_with_delay(FmFolderView* fv)
{
    g_return_if_fail(FM_IS_FOLDER_VIEW(fv));
    FM_FOLDER_VIEW_GET_IFACE(fv)->disconnect_model_with_delay(fv);
}

/**
 * fm_folder_view_get_n_selected_files
 * @fv: a widget to inspect
 *
 * Retrieves number of the currently selected files.
 *
 * Returns: number of files selected.
 *
 * Since: 1.0.1
 */
gint fm_folder_view_get_n_selected_files(FmFolderView* fv)
{
    g_return_val_if_fail(FM_IS_FOLDER_VIEW(fv), 0);

    return (*FM_FOLDER_VIEW_GET_IFACE(fv)->count_selected_files)(fv);
}

/**
 * fm_folder_view_dup_selected_files
 * @fv: a FmFolderView object
 *
 * Retrieves a list of
 * the currently selected files. The list should be freed after usage
 * with fm_file_info_list_unref&lpar;). If there are no files selected then
 * returns %NULL.
 *
 * Before 1.0.0 this API had name fm_folder_view_get_selected_files.
 *
 * Returns: (transfer full) (element-type FmFileInfo): list of selected file infos.
 *
 * Since: 0.1.0
 */
FmFileInfoList* fm_folder_view_dup_selected_files(FmFolderView* fv)
{
    g_return_val_if_fail(FM_IS_FOLDER_VIEW(fv), NULL);

    return (*FM_FOLDER_VIEW_GET_IFACE(fv)->dup_selected_files)(fv);
}

/**
 * fm_folder_view_dup_selected_file_paths
 * @fv: a FmFolderView object
 *
 * Retrieves a list of
 * the currently selected files. The list should be freed after usage
 * with fm_path_list_unref&lpar;). If there are no files selected then returns
 * %NULL.
 *
 * Before 1.0.0 this API had name fm_folder_view_get_selected_file_paths.
 *
 * Returns: (transfer full) (element-type FmPath): list of selected file paths.
 *
 * Since: 0.1.0
 */
FmPathList* fm_folder_view_dup_selected_file_paths(FmFolderView* fv)
{
    g_return_val_if_fail(FM_IS_FOLDER_VIEW(fv), NULL);

    return (*FM_FOLDER_VIEW_GET_IFACE(fv)->dup_selected_file_paths)(fv);
}

/**
 * fm_folder_view_select_all
 * @fv: a widget to apply
 *
 * Selects all files in folder.
 *
 * Since: 0.1.0
 */
void fm_folder_view_select_all(FmFolderView* fv)
{
    g_return_if_fail(FM_IS_FOLDER_VIEW(fv));

    FM_FOLDER_VIEW_GET_IFACE(fv)->select_all(fv);
}

/**
 * fm_folder_view_unselect_all
 * @fv: a widget to apply
 *
 * Unselects all files in folder.
 *
 * Since: 1.0.1
 */
void fm_folder_view_unselect_all(FmFolderView* fv)
{
    g_return_if_fail(FM_IS_FOLDER_VIEW(fv));

    FM_FOLDER_VIEW_GET_IFACE(fv)->unselect_all(fv);
}

/**
 * fm_folder_view_select_invert
 * @fv: a widget to apply
 *
 * Selects all unselected files in @fv but unselects all selected.
 *
 * Since: 0.1.0
 */
void fm_folder_view_select_invert(FmFolderView* fv)
{
    g_return_if_fail(FM_IS_FOLDER_VIEW(fv));

    FM_FOLDER_VIEW_GET_IFACE(fv)->select_invert(fv);
}

/**
 * fm_folder_view_select_file_path
 * @fv: a widget to apply
 * @path: a file path to select
 *
 * Selects a file in the folder.
 *
 * Since: 0.1.0
 */
void fm_folder_view_select_file_path(FmFolderView* fv, FmPath* path)
{
    g_return_if_fail(FM_IS_FOLDER_VIEW(fv));

    FM_FOLDER_VIEW_GET_IFACE(fv)->select_file_path(fv, path);
}

/**
 * fm_folder_view_select_file_paths
 * @fv: a widget to apply
 * @paths: list of files to select
 *
 * Selects few files in the folder.
 *
 * Since: 0.1.0
 */
void fm_folder_view_select_file_paths(FmFolderView* fv, FmPathList* paths)
{
    GList* l;
    FmFolderViewInterface* iface;

    g_return_if_fail(FM_IS_FOLDER_VIEW(fv));

    iface = FM_FOLDER_VIEW_GET_IFACE(fv);
    for(l = fm_path_list_peek_head_link(paths);l; l=l->next)
    {
        FmPath* path = FM_PATH(l->data);
        iface->select_file_path(fv, path);
    }
}


/**
 * fm_folder_view_item_clicked
 * @fv: the folder view widget
 * @path: (allow-none): path to current pointed item
 * @type: what click was received
 *
 * Handles left click and right click in folder area. If some item was
 * left-clicked then fm_folder_view_item_clicked() tries to launch it.
 * If some item was right-clicked then opens file menu (applying the
 * update_popup returned by get_custom_menu_callbacks interface function
 * before opening it if it's not %NULL). If it was right-click on empty
 * space of folder view (so @path is %NULL) then opens folder popup
 * menu that was created by fm_folder_view_add_popup(). After that
 * emits the #FmFolderView::clicked signal.
 *
 * If open_folders callback from interface function get_custom_menu_callbacks
 * is %NULL then assume it was old API call so click will be not handled
 * by this function and signal handler will handle it instead.
 *
 * This API is internal for #FmFolderView and should be used only in
 * class implementations.
 *
 * Since: 1.0.1
 */
void fm_folder_view_item_clicked(FmFolderView* fv, GtkTreePath* path,
                                 FmFolderViewClickType type)
{
    g_return_if_fail(FM_IS_FOLDER_VIEW(fv));

    FmFolderViewInterface * iface = FM_FOLDER_VIEW_GET_IFACE(fv);
    g_return_if_fail(iface);

    FmFileInfo * fi = NULL;
    if (path)
    {
        GtkTreeModel* model = GTK_TREE_MODEL(iface->get_model(fv));
        GtkTreeIter it;
        if(gtk_tree_model_get_iter(model, &it, path))
            gtk_tree_model_get(model, &it, FM_FOLDER_MODEL_COL_INFO, &fi, -1);
    }

    FmFileInfoList* files = iface->dup_selected_files(fv);

    fm_folder_view_files_clicked(fv, fi, files, type);

    fm_file_info_list_unref(files);

    g_signal_emit(fv, signals[CLICKED], 0, type, fi);
}

/**
 * fm_folder_view_sel_changed
 * @obj: some object; unused
 * @fv: the folder view widget to apply
 *
 * Emits the #FmFolderView::sel-changed signal.
 *
 * This API is internal for #FmFolderView and should be used only in
 * class implementations.
 *
 * Since: 1.0.1
 */
void fm_folder_view_sel_changed(GObject* obj, FmFolderView* fv)
{
    g_return_if_fail(FM_IS_FOLDER_VIEW(fv));

    /* if someone is connected to our "sel-changed" signal. */
    if(g_signal_has_handler_pending(fv, signals[SEL_CHANGED], 0, TRUE))
    {
        FmFolderViewInterface* iface = FM_FOLDER_VIEW_GET_IFACE(fv);
        gint files = iface->count_selected_files(fv);

        /* emit a selection changed notification to the world. */
        g_signal_emit(fv, signals[SEL_CHANGED], 0, files);
    }
}

#if 0
/**
 * fm_folder_view_chdir
 * @fv: the folder view widget to apply
 *
 * Emits the #FmFolderView::chdir signal.
 *
 * This API is internal for #FmFolderView and should be used only in
 * class implementations.
 *
 * Since: 1.0.2
 */
void fm_folder_view_chdir(FmFolderView* fv, FmPath* path)
{
    g_return_if_fail(FM_IS_FOLDER_VIEW(fv));

    g_signal_emit(fv, signals[CHDIR], 0, path);
}
#endif

/**
 * fm_folder_view_set_columns
 * @fv: the folder view widget to apply
 * @cols: (element-type FmFolderViewColumnInfo): new list of column infos
 *
 * Changes composition (rendering) of folder view @fv in accordance to
 * new list of column infos.
 *
 * Returns: %TRUE in case of success.
 *
 * Since: 1.0.2
 */
gboolean fm_folder_view_set_columns(FmFolderView* fv, const GSList* cols)
{
    FmFolderViewInterface* iface;

    g_return_val_if_fail(FM_IS_FOLDER_VIEW(fv), FALSE);

    iface = FM_FOLDER_VIEW_GET_IFACE(fv);

    if(iface->set_columns)
        return iface->set_columns(fv, cols);
    return FALSE;
}

/**
 * fm_folder_view_get_columns
 * @fv: the folder view widget to query
 *
 * Retrieves current composition of @fv as list of column infos. Returned
 * list should be freed with g_slist_free() after usage.
 *
 * Returns: (transfer container) (element-type FmFolderViewColumnInfo): list of column infos.
 *
 * Since: 1.0.2
 */
GSList* fm_folder_view_get_columns(FmFolderView* fv)
{
    FmFolderViewInterface* iface;

    g_return_val_if_fail(FM_IS_FOLDER_VIEW(fv), NULL);

    iface = FM_FOLDER_VIEW_GET_IFACE(fv);

    if(iface->get_columns)
        return iface->get_columns(fv);
    return NULL;
}

/**
 * fm_folder_view_columns_changed
 * @fv: the folder view widget to apply
 *
 * Emits the #FmFolderView::columns-changed signal.
 *
 * This API is internal for #FmFolderView and should be used only in
 * class implementations.
 *
 * Since: 1.2.0
 */
void fm_folder_view_columns_changed(FmFolderView* fv)
{
    g_return_if_fail(FM_IS_FOLDER_VIEW(fv));

    g_signal_emit(fv, signals[COLUMNS_CHANGED], 0);
}
