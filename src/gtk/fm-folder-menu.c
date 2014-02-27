/*
 *      fm-folder-menu.c
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

/**
 * SECTION:fm-folder-view
 */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif
#include "gtk-compat.h"

#include <glib/gi18n-lib.h>
#include <stdlib.h>
#include <gdk/gdkkeysyms.h>

#include "fm-folder-view.h"
#include "fm-file-properties.h"
#include "fm-clipboard.h"
#include "fm-gtk-file-launcher.h"
#include "fm-file-menu.h"
#include "fm-gtk-utils.h"
#include <libsmfm-core/fm-templates.h>
#include <libsmfm-core/fm-config.h>

static const char folder_popup_xml[] =
"<popup>"
  "<menu action='CreateNew'>"
    "<menuitem action='NewFolder'/>"
    /* placeholder for ~/Templates support */
    "<placeholder name='ph1'/>"
    "<separator/>"
    "<menuitem action='NewBlank'/>"
  "</menu>"
  "<separator/>"
  "<menuitem action='Paste'/>"
  "<menuitem action='Cut'/>"
  "<menuitem action='Copy'/>"
  "<menuitem action='Del'/>"
  "<menuitem action='Remove'/>"
  "<menuitem action='FileProp'/>"
  "<separator/>"
  "<menuitem action='SetPattern'/>"
  "<separator/>"
  "<menuitem action='SelAll'/>"
  "<menuitem action='InvSel'/>"
  "<separator/>"
  "<menu action='Sort'>"
    "<menuitem action='Asc'/>"
    "<menuitem action='Desc'/>"
    "<separator/>"
    "<menuitem action='ByName'/>"
    "<menuitem action='ByMTime'/>"
    "<menuitem action='BySize'/>"
    "<menuitem action='ByType'/>"
    "<separator/>"
    /* "<menuitem action='MingleDirs'/>"
    "<menuitem action='SortIgnoreCase'/>" */
    /* placeholder for custom sort options */
    "<placeholder name='CustomSortOps'/>"
    /* "<separator/>"
    "<menuitem action='SortPerFolder'/>" */
  "</menu>"
  "<menuitem action='ShowHidden'/>"
  "<menuitem action='Rename'/>"
  /* placeholder for custom folder operations */
  "<placeholder name='CustomFolderOps'/>"
  "<separator/>"
  /* placeholder for custom application operations such as view mode changing */
  "<placeholder name='CustomCommonOps'/>"
  "<separator/>"
  "<menuitem action='Prop'/>"
"</popup>"
"<accelerator action='NewFolder2'/>"
"<accelerator action='NewFolder3'/>"
"<accelerator action='Copy2'/>"
"<accelerator action='Paste2'/>"
"<accelerator action='Del2'/>"
"<accelerator action='Remove2'/>"
"<accelerator action='FileProp2'/>"
"<accelerator action='FileProp3'/>";

static void on_create_new(GtkAction* act, FmFolderView* fv);
static void on_cut(GtkAction* act, FmFolderView* fv);
static void on_copy(GtkAction* act, FmFolderView* fv);
static void on_paste(GtkAction* act, FmFolderView* fv);
static void on_trash(GtkAction* act, FmFolderView* fv);
static void on_rm(GtkAction* act, FmFolderView* fv);
static void on_set_pattern(GtkAction* act, FmFolderView* fv);
static void on_select_all(GtkAction* act, FmFolderView* fv);
static void on_invert_select(GtkAction* act, FmFolderView* fv);
static void on_rename(GtkAction* act, FmFolderView* fv);
static void on_prop(GtkAction* act, FmFolderView* fv);
static void on_file_prop(GtkAction* act, FmFolderView* fv);
static void on_show_hidden(GtkToggleAction* act, FmFolderView* fv);

static const GtkActionEntry folder_popup_actions[]=
{
    {"CreateNew", NULL, N_("Create _New..."), NULL, NULL, NULL},
    {"NewFolder", "folder", N_("Folder"), "<Ctrl><Shift>N", NULL, G_CALLBACK(on_create_new)},
    {"NewFolder2", NULL, NULL, "Insert", NULL, G_CALLBACK(on_create_new)},
    {"NewFolder3", NULL, NULL, "KP_Insert", NULL, G_CALLBACK(on_create_new)},
    {"NewBlank", NULL, N_("Empty File"), NULL, NULL, G_CALLBACK(on_create_new)},
    /* {"NewShortcut", "system-run", N_("Shortcut"), NULL, NULL, G_CALLBACK(on_create_new)}, */
    {"Cut", GTK_STOCK_CUT, NULL, "<Ctrl>X", NULL, G_CALLBACK(on_cut)},
    {"Copy", GTK_STOCK_COPY, NULL, "<Ctrl>C", NULL, G_CALLBACK(on_copy)},
    {"Copy2", NULL, NULL, "<Ctrl>Insert", NULL, G_CALLBACK(on_copy)},
    {"Paste", GTK_STOCK_PASTE, NULL, "<Ctrl>V", NULL, G_CALLBACK(on_paste)},
    {"Paste2", NULL, NULL, "<Shift>Insert", NULL, G_CALLBACK(on_paste)},
    {"Del", GTK_STOCK_DELETE, NULL, "Delete", NULL, G_CALLBACK(on_trash)},
    {"Del2", NULL, NULL, "KP_Delete", NULL, G_CALLBACK(on_trash)},
    {"Remove", GTK_STOCK_REMOVE, NULL, "<Shift>Delete", NULL, G_CALLBACK(on_rm)},
    {"Remove2", NULL, NULL, "<Shift>KP_Delete", NULL, G_CALLBACK(on_rm)},
    {"SetPattern", NULL, "_Filter...", NULL, NULL, G_CALLBACK(on_set_pattern)},
    {"SelAll", GTK_STOCK_SELECT_ALL, NULL, "<Ctrl>A", NULL, G_CALLBACK(on_select_all)},
    {"InvSel", NULL, N_("_Invert Selection"), "<Ctrl>I", NULL, G_CALLBACK(on_invert_select)},
    {"Sort", NULL, N_("_Sort Files"), NULL, NULL, NULL},
    {"Rename", NULL, N_("_Rename Folder"), NULL, NULL, G_CALLBACK(on_rename)},
    {"Prop", GTK_STOCK_PROPERTIES, N_("Folder Prop_erties"), "", NULL, G_CALLBACK(on_prop)},
    {"FileProp", GTK_STOCK_PROPERTIES, N_("Prop_erties"), "<Alt>Return", NULL, G_CALLBACK(on_file_prop)},
    {"FileProp2", NULL, NULL, "<Alt>KP_Enter", NULL, G_CALLBACK(on_file_prop)},
    {"FileProp3", NULL, NULL, "<Alt>ISO_Enter", NULL, G_CALLBACK(on_file_prop)}
};

static GtkToggleActionEntry folder_toggle_actions[]=
{
    {"ShowHidden", NULL, N_("Show _Hidden"), "<Ctrl>H", NULL, G_CALLBACK(on_show_hidden), FALSE},
    {"SortPerFolder", NULL, N_("_Only for this folder"), NULL,
                N_("Check to remember sort as folder setting rather than global one"), NULL, FALSE},
    {"MingleDirs", NULL, N_("Mingle _files and folders"), NULL, NULL, NULL, FALSE},
    {"SortIgnoreCase", NULL, N_("_Ignore name case"), NULL, NULL, NULL, TRUE}
};

static const GtkRadioActionEntry folder_sort_type_actions[]=
{
    {"Asc", GTK_STOCK_SORT_ASCENDING, NULL, NULL, NULL, GTK_SORT_ASCENDING},
    {"Desc", GTK_STOCK_SORT_DESCENDING, NULL, NULL, NULL, GTK_SORT_DESCENDING},
};

static const GtkRadioActionEntry folder_sort_by_actions[]=
{
    {"ByName", NULL, N_("By _Name"), NULL, NULL, FM_FOLDER_MODEL_COL_NAME},
    {"ByMTime", NULL, N_("By _Modification Time"), NULL, NULL, FM_FOLDER_MODEL_COL_MTIME},
    {"BySize", NULL, N_("By _Size"), NULL, NULL, FM_FOLDER_MODEL_COL_SIZE},
    {"ByType", NULL, N_("By File _Type"), NULL, NULL, FM_FOLDER_MODEL_COL_DESC}
};

static GQuark ui_quark;
static GQuark popup_quark;
static GQuark templates_quark;

static void _init_quarks(void)
{
    if (ui_quark)
        return;

    ui_quark = g_quark_from_static_string("popup-ui");
    popup_quark = g_quark_from_static_string("popup-menu");
    templates_quark = g_quark_from_static_string("templates-list");
}

static void on_run_app_toggled(GtkToggleButton *button, gboolean *run)
{
    *run = gtk_toggle_button_get_active(button);
}

static void on_create_new(GtkAction* act, FmFolderView* fv)
{
    _init_quarks();

    const char* name = gtk_action_get_name(act);
    GtkMenu *popup = g_object_get_qdata(G_OBJECT(fv), popup_quark);
    GtkWidget *win = gtk_menu_get_attach_widget(popup);
    GtkUIManager *ui = g_object_get_qdata(G_OBJECT(fv), ui_quark);
    GList *templates = g_object_get_qdata(G_OBJECT(ui), templates_quark);
    FmTemplate *templ;
    FmMimeType *mime_type;
    const char *prompt, *name_template, *label;
    char *_prompt = NULL, *header, *basename;
    FmPath *dest;
    GFile *gf;
    GError *error = NULL;
    GtkWidget *run_button, *sub_button;
    gboolean new_folder = FALSE, run_app;
    gint n;

    if(strncmp(name, "NewFolder", 9) == 0)
    {
        templ = NULL;
        prompt = _("Enter a name for the newly created folder:");
        header = _("Creating new folder");
        new_folder = TRUE;
    }
    else if(G_LIKELY(strncmp(name, "NewFile", 7) == 0))
    {
        n = atoi(&name[7]);
        if(n < 0 || (templ = g_list_nth_data(templates, n)) == NULL)
            return; /* invalid action name, is it possible? */
    }
    /* special option 'NewBlank' */
    else if(G_LIKELY(strcmp(name, "NewBlank") == 0))
    {
        templ = NULL;
        prompt = _("Enter a name for empty file:");
        header = _("Creating ...");
    }
    else /* invalid action name, is it possible? */
        return;
    if(templ == NULL) /* new folder */
    {
        name_template = _("New");
        n = -1;
        run_app = FALSE;
        run_button = NULL;
    }
    else
    {
        mime_type = fm_template_get_mime_type(templ);
        prompt = fm_template_get_prompt(templ);
        if(!prompt)
            prompt = _prompt = g_strdup_printf(_("Enter a name for the new %s:"),
                                               fm_mime_type_get_desc(mime_type));
        label = fm_template_get_label(templ);
        header = g_strdup_printf(_("Creating %s"), label ? label :
                                             fm_mime_type_get_desc(mime_type));
        name_template = fm_template_get_name(templ, &n);
        run_app = fm_config->template_run_app;
        sub_button = gtk_check_button_new_with_mnemonic(_("_Run default application on file after creation"));
        gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(sub_button), run_app);
        g_signal_connect(sub_button, "toggled", G_CALLBACK(on_run_app_toggled), &run_app);
        run_button = gtk_alignment_new(0, 0, 1, 1);
        gtk_alignment_set_padding(GTK_ALIGNMENT(run_button), 0, 0, 16, 0);
        gtk_container_add(GTK_CONTAINER(run_button), sub_button);
    }
    basename = fm_get_user_input_n(GTK_WINDOW(win), header, prompt,
                                   name_template, n, run_button);
    if(templ)
    {
        g_free(_prompt);
        g_free(header);
    }
    if(!basename)
        return;
    dest = fm_path_new_child(fm_folder_view_get_cwd(fv), basename);
    g_free(basename);
    gf = fm_path_to_gfile(dest);
    fm_path_unref(dest);
    if(templ)
        fm_template_create_file(templ, gf, &error, run_app);
    else if(new_folder)
        g_file_make_directory(gf, NULL, &error);
    /* specail option 'NewBlank' */
    else
    {
        GFileOutputStream *f = g_file_create(gf, G_FILE_CREATE_NONE, NULL, &error);
        if(f)
            g_object_unref(f);
    }
    if(error)
    {
        fm_show_error(GTK_WINDOW(win), NULL, error->message);
        g_error_free(error);
    }
    g_object_unref(gf);
}

static void on_cut(GtkAction* act, FmFolderView* fv)
{
    _init_quarks();

    GtkMenu *popup = g_object_get_qdata(G_OBJECT(fv), popup_quark);
    GtkWidget *win = gtk_menu_get_attach_widget(popup);
    GtkWidget *focus = gtk_window_get_focus(GTK_WINDOW(win));

    /* check if we cut inside the view; for desktop focus will be NULL */
    if(focus == NULL || gtk_widget_is_ancestor(focus, GTK_WIDGET(fv)))
    {
        FmPathList* files = fm_folder_view_dup_selected_file_paths(fv);
        if(files)
        {
            fm_clipboard_cut_files(win, files);
            fm_path_list_unref(files);
        }
    }
    else if(GTK_IS_EDITABLE(focus) && /* fallback for editables */
            gtk_editable_get_selection_bounds((GtkEditable*)focus, NULL, NULL))
        gtk_editable_cut_clipboard((GtkEditable*)focus);
}

static void on_copy(GtkAction* act, FmFolderView* fv)
{
    _init_quarks();

    GtkMenu *popup = g_object_get_qdata(G_OBJECT(fv), popup_quark);
    GtkWidget *win = gtk_menu_get_attach_widget(popup);
    GtkWidget *focus = gtk_window_get_focus(GTK_WINDOW(win));

    /* check if we copy inside the view; for desktop focus will be NULL */
    if(focus == NULL || gtk_widget_is_ancestor(focus, GTK_WIDGET(fv)))
    {
        FmPathList* files = fm_folder_view_dup_selected_file_paths(fv);
        if(files)
        {
            fm_clipboard_copy_files(win, files);
            fm_path_list_unref(files);
        }
    }
    else if(GTK_IS_EDITABLE(focus) && /* fallback for editables */
            gtk_editable_get_selection_bounds((GtkEditable*)focus, NULL, NULL))
        gtk_editable_copy_clipboard((GtkEditable*)focus);
}

static void on_paste(GtkAction* act, FmFolderView* fv)
{
    _init_quarks();

    GtkMenu *popup = g_object_get_qdata(G_OBJECT(fv), popup_quark);
    GtkWidget *win = gtk_menu_get_attach_widget(popup);
    GtkWidget *focus = gtk_window_get_focus(GTK_WINDOW(win));

    /* check if we paste inside the view; for desktop focus will be NULL */
    if(focus == NULL || gtk_widget_is_ancestor(focus, GTK_WIDGET(fv)))
    {
        FmPath* path = fm_folder_view_get_cwd(fv);
        fm_clipboard_paste_files(GTK_WIDGET(fv), path);
    }
    else if(GTK_IS_EDITABLE(focus)) /* fallback for editables */
        gtk_editable_paste_clipboard((GtkEditable*)focus);
}

static void on_trash(GtkAction* act, FmFolderView* fv)
{
    _init_quarks();

    GtkMenu *popup = g_object_get_qdata(G_OBJECT(fv), popup_quark);
    GtkWidget *win = gtk_menu_get_attach_widget(popup);
    GtkWidget *focus = gtk_window_get_focus(GTK_WINDOW(win));

    /* check if user pressed 'Del' inside the view; for desktop focus will be NULL */
    if(focus == NULL || gtk_widget_is_ancestor(focus, GTK_WIDGET(fv)))
    {
        FmPathList* files = fm_folder_view_dup_selected_file_paths(fv);
        if(files)
        {
            fm_trash_or_delete_files(GTK_WINDOW(win), files);
            fm_path_list_unref(files);
        }
    }
    else if(GTK_IS_EDITABLE(focus)) /* fallback for editables */
    {
        if(!gtk_editable_get_selection_bounds((GtkEditable*)focus, NULL, NULL))
        {
            gint pos = gtk_editable_get_position((GtkEditable*)focus);
            /* if no text selected then delete character next to cursor */
            gtk_editable_select_region((GtkEditable*)focus, pos, pos + 1);
        }
        gtk_editable_delete_selection((GtkEditable*)focus);
    }
}

static void on_rm(GtkAction* act, FmFolderView* fv)
{
    _init_quarks();

    GtkMenu *popup = g_object_get_qdata(G_OBJECT(fv), popup_quark);
    GtkWidget *win = gtk_menu_get_attach_widget(popup);
    GtkWidget *focus = gtk_window_get_focus(GTK_WINDOW(win));

    /* check if user pressed 'Shift+Del' inside the view */
    if(focus == NULL || gtk_widget_is_ancestor(focus, GTK_WIDGET(fv)))
    {
        FmPathList* files = fm_folder_view_dup_selected_file_paths(fv);
        if(files)
        {
            fm_delete_files(GTK_WINDOW(win), files);
            fm_path_list_unref(files);
        }
    }
    /* for editables 'Shift+Del' means 'Cut' */
    else if(GTK_IS_EDITABLE(focus))
        gtk_editable_cut_clipboard((GtkEditable*)focus);
}

static void on_set_pattern(GtkAction* act, FmFolderView* fv)
{
    g_return_if_fail(FM_IS_FOLDER_VIEW(fv));

    _init_quarks();

    GtkMenu *popup = g_object_get_qdata(G_OBJECT(fv), popup_quark);
    GtkWidget *win = gtk_menu_get_attach_widget(popup);

    FmFolderViewInterface* iface = FM_FOLDER_VIEW_GET_IFACE(fv);

    const char * old_pattern = NULL;
    if (iface && iface->get_pattern)
        old_pattern = iface->get_pattern(fv);
    if (!old_pattern)
        old_pattern = "";

    gchar * new_pattern = fm_get_user_input(GTK_WINDOW(win), "Select pattern", "Enter a pattern to filter displayed files", old_pattern);

    if (iface && iface->set_pattern)
        iface->set_pattern(fv, new_pattern);

    g_free(new_pattern);
}

static void on_select_all(GtkAction* act, FmFolderView* fv)
{
    _init_quarks();

    GtkMenu *popup = g_object_get_qdata(G_OBJECT(fv), popup_quark);
    GtkWidget *win = gtk_menu_get_attach_widget(popup);
    GtkWidget *focus = gtk_window_get_focus(GTK_WINDOW(win));

    /* check if we are inside the view; for desktop focus will be NULL */
    if(focus == NULL || gtk_widget_is_ancestor(focus, GTK_WIDGET(fv)))
        fm_folder_view_select_all(fv);
    else if(GTK_IS_EDITABLE(focus)) /* fallback for editables */
        gtk_editable_select_region((GtkEditable*)focus, 0, -1);
}

static void on_invert_select(GtkAction* act, FmFolderView* fv)
{
    fm_folder_view_select_invert(fv);
}

static void on_rename(GtkAction* act, FmFolderView* fv)
{
    _init_quarks();

    GtkMenu *popup = g_object_get_qdata(G_OBJECT(fv), popup_quark);
    GtkWidget *win = gtk_menu_get_attach_widget(popup);

    /* FIXME: is it OK to rename folder itself? */
    fm_rename_file(GTK_WINDOW(win), fm_folder_view_get_cwd(fv));
}

static void on_prop(GtkAction* act, FmFolderView* fv)
{
    _init_quarks();

    FmFolder* folder = fm_folder_view_get_folder(fv);

    if(folder && fm_folder_is_valid(folder))
    {
        GtkMenu *popup = g_object_get_qdata(G_OBJECT(fv), popup_quark);
        GtkWidget *win = gtk_menu_get_attach_widget(popup);
        FmFileInfo* fi = fm_folder_get_info(folder);
        FmFileInfoList* files = fm_file_info_list_new();
        fm_file_info_list_push_tail(files, fi);
        fm_show_file_properties(GTK_WINDOW(win), files);
        fm_file_info_list_unref(files);
    }
}

static void on_file_prop(GtkAction* act, FmFolderView* fv)
{
    _init_quarks();

    GtkMenu *popup = g_object_get_qdata(G_OBJECT(fv), popup_quark);
    GtkWidget *win = gtk_menu_get_attach_widget(popup);
    GtkWidget *focus = gtk_window_get_focus(GTK_WINDOW(win));

    /* check if it is inside the view; for desktop focus will be NULL */
    if(focus == NULL || gtk_widget_is_ancestor(focus, GTK_WIDGET(fv)))
    {
        FmFileInfoList* files = fm_folder_view_dup_selected_files(fv);
        if(files)
        {
            fm_show_file_properties(GTK_WINDOW(win), files);
            fm_file_info_list_unref(files);
        }
        else
        {
            on_prop(NULL, fv);
        }
    }
}

static void popup_position_func(GtkMenu *menu, gint *x, gint *y,
                                gboolean *push_in, gpointer user_data)
{
    GtkWidget *widget = GTK_WIDGET(user_data);
    GdkWindow *parent_window;
    GtkAllocation a, ma;
    gint x2, y2;
    gboolean rtl = (gtk_widget_get_direction(widget) == GTK_TEXT_DIR_RTL);

    /* realize menu so we get actual size of it */
    gtk_widget_realize(GTK_WIDGET(menu));
    /* get all the relative coordinates */
    gtk_widget_get_allocation(widget, &a);
    gtk_widget_get_pointer(widget, &x2, &y2);
    gtk_widget_get_allocation(GTK_WIDGET(menu), &ma);
    /* position menu inside widget */
    if(rtl) /* RTL */
        x2 = CLAMP(x2, 1, ma.width + a.width - 1);
    else /* LTR */
        x2 = CLAMP(x2, 1 - ma.width, a.width - 1);
    y2 = CLAMP(y2, 1 - ma.height, a.height - 1);
    /* get absolute coordinate of parent window - we got coords relative to it */
    parent_window = gtk_widget_get_parent_window(widget);
    if(parent_window)
        gdk_window_get_origin(parent_window, x, y);
    else
        /* desktop has no parent window so parent coords will be 0;0 */
        *x = *y = 0;
    /* calculate desired position for menu */
    *x += a.x + x2;
    *y += a.y + y2;
    /* limit coordinates so menu will be not positioned outside of screen */
    /* FIXME: honor monitor */
    if(rtl) /* RTL */
    {
        x2 = gdk_screen_width();
        *x = CLAMP(*x, MIN(ma.width, x2), x2);
    }
    else /* LTR */
    {
        *x = CLAMP(*x, 0, MAX(0, gdk_screen_width() - ma.width));
    }
    *y = CLAMP(*y, 0, MAX(0, gdk_screen_height() - ma.height));
}

void fm_folder_view_show_popup(FmFolderView* fv)
{
    _init_quarks();

    GtkUIManager *ui = g_object_get_qdata(G_OBJECT(fv), ui_quark);
    GtkMenu *popup = g_object_get_qdata(G_OBJECT(fv), popup_quark);
    FmFolderViewInterface *iface = FM_FOLDER_VIEW_GET_IFACE(fv);
    FmFolderModel *model;
    GtkActionGroup *act_grp;
    GList *templates;
    gboolean show_hidden;
    FmSortMode mode;
    GtkSortType type = GTK_SORT_ASCENDING;
    FmFolderModelCol by;
    GtkAction * act;

    /* update actions */
    model = iface->get_model(fv);
    if(fm_folder_model_get_sort(model, &by, &mode))
        type = FM_SORT_IS_ASCENDING(mode) ? GTK_SORT_ASCENDING : GTK_SORT_DESCENDING;
    act = gtk_ui_manager_get_action(ui, "/popup/Sort/Asc");
    /* g_debug("set /popup/Sort/Asc: %u", type); */
    gtk_radio_action_set_current_value(GTK_RADIO_ACTION(act), type);
    act = gtk_ui_manager_get_action(ui, "/popup/Sort/ByName");
    if(by == FM_FOLDER_MODEL_COL_DEFAULT)
        by = FM_FOLDER_MODEL_COL_NAME;
    /* g_debug("set /popup/Sort/ByName: %u", by); */
    gtk_radio_action_set_current_value(GTK_RADIO_ACTION(act), by);
    show_hidden = iface->get_show_hidden(fv);
    act = gtk_ui_manager_get_action(ui, "/popup/ShowHidden");
    gtk_toggle_action_set_active(GTK_TOGGLE_ACTION(act), show_hidden);
    /* shadow 'Paste' if clipboard is empty and unshadow if not */
    act = gtk_ui_manager_get_action(ui, "/popup/Paste");
    gtk_action_set_sensitive(act, fm_clipboard_have_files(GTK_WIDGET(fv)));
    /* prepare templates list */
    templates = g_object_get_qdata(G_OBJECT(ui), templates_quark);
    /* FIXME: updating context menu is not lightweight here - we should
       remember all actions and ui we added, remove them, and add again.
       That will take some time, memory and may be error-prone as well.
       For simplicity we create it once here but if users will find
       any inconveniences this behavior should be changed later. */
    if(!templates)
    {
        templates = fm_template_list_all(fm_config->only_user_templates);
        if(templates)
        {
            FmTemplate *templ;
            FmMimeType *mime_type;
            FmIcon *icon;
            const gchar *label;
            GList *l;
            GString *xml;
            GtkActionEntry actent;
            guint i;
            char act_name[16];

            l = gtk_ui_manager_get_action_groups(ui);
            act_grp = l->data; /* our action group if first one */
            actent.callback = G_CALLBACK(on_create_new);
            actent.accelerator = NULL;
            actent.tooltip = NULL;
            actent.name = act_name;
            actent.stock_id = NULL;
            xml = g_string_new("<popup><menu action='CreateNew'><placeholder name='ph1'>");
            for(l = templates, i = 0; l; l = l->next, i++)
            {
                templ = l->data;
                /* we support directories differently */
                if(fm_template_is_directory(templ))
                    continue;
                mime_type = fm_template_get_mime_type(templ);
                label = fm_template_get_label(templ);
                snprintf(act_name, sizeof(act_name), "NewFile%u", i);
                g_string_append_printf(xml, "<menuitem action='%s'/>", act_name);
                icon = fm_template_get_icon(templ);
                if(!icon)
                    icon = fm_mime_type_get_icon(mime_type);
                /* create and insert new action */
                actent.label = label ? label : fm_mime_type_get_desc(mime_type);
                gtk_action_group_add_actions(act_grp, &actent, 1, fv);
                if(icon)
                {
                    act = gtk_action_group_get_action(act_grp, act_name);
                    gtk_action_set_gicon(act, icon->gicon);
                }
            }
            g_string_append(xml, "</placeholder></menu></popup>");
            gtk_ui_manager_add_ui_from_string(ui, xml->str, -1, NULL);
            g_string_free(xml, TRUE);
        }
        g_object_set_qdata(G_OBJECT(ui), templates_quark, templates);
    }

    /* open popup */
    gtk_menu_popup(popup, NULL, NULL, popup_position_func, fv, 3,
                   gtk_get_current_event_time());
}

/* handle 'Menu' and 'Shift+F10' here */
static gboolean on_key_press(GtkWidget *widget, GdkEventKey *evt, FmFolderView* fv)
{
    int modifier = (evt->state & gtk_accelerator_get_default_mod_mask());
    if((evt->keyval == GDK_KEY_Menu && !modifier) ||
       (evt->keyval == GDK_KEY_F10 && modifier == GDK_SHIFT_MASK))
    {
        fm_folder_view_show_popup_for_selected_files(fv);
        return TRUE;
    }
    else if((evt->keyval == GDK_KEY_Menu && modifier == GDK_CONTROL_MASK) ||
       (evt->keyval == GDK_KEY_F10 && modifier == GDK_CONTROL_MASK))
    {
        fm_folder_view_show_popup(fv);
        return TRUE;
    }
    return FALSE;
}

GtkMenu * fm_folder_view_get_popup_for_selected_files(FmFolderView* fv)
{
    _init_quarks();

    FmFolderViewInterface * iface = FM_FOLDER_VIEW_GET_IFACE(fv);
    GtkMenu * popup = NULL;

    if (iface->count_selected_files(fv) > 0)
    {
        GtkMenu * folder_popup = g_object_get_qdata(G_OBJECT(fv), popup_quark);
        if (folder_popup == NULL) /* no fm_folder_view_add_popup() was called before */
            return NULL;

        FmFileInfoList * files = iface->dup_selected_files(fv);

        GtkWindow * window = GTK_WINDOW(gtk_menu_get_attach_widget(folder_popup));

        FmFileMenu* menu = fm_file_menu_new_for_files(window, files, fm_folder_view_get_cwd(fv), TRUE);

        FmFolderViewUpdatePopup update_popup = NULL;
        FmLaunchFolderFunc open_folders = NULL;
        if (iface->get_custom_menu_callbacks)
            iface->get_custom_menu_callbacks(fv, &update_popup, &open_folders);

        fm_file_menu_set_folder_func(menu, open_folders, window);

        /* TODO: add info message on selection if enabled in config */
        /* merge some specific menu items */
        if (update_popup)
            update_popup(fv, window, fm_file_menu_get_ui(menu),
                         fm_file_menu_get_action_group(menu), files);

        fm_file_info_list_unref(files);

        popup = fm_file_menu_get_menu(menu);
    }

    return popup;
}

void fm_folder_view_show_popup_for_selected_files(FmFolderView* fv)
{
    _init_quarks();

    FmFolderViewInterface *iface = FM_FOLDER_VIEW_GET_IFACE(fv);

    if(iface->count_selected_files(fv) > 0)
    {
        GtkMenu * popup = fm_folder_view_get_popup_for_selected_files(fv);
        if (popup)
            gtk_menu_popup(popup, NULL, NULL, popup_position_func, fv, 3, gtk_get_current_event_time());
    }
    else
    {
        fm_folder_view_show_popup(fv);
    }
}

static void on_show_hidden(GtkToggleAction* act, FmFolderView* fv)
{
    gboolean active = gtk_toggle_action_get_active(act);
    fm_folder_view_set_show_hidden(fv, active);
}

static void on_change_by(GtkRadioAction* act, GtkRadioAction* cur, FmFolderView* fv)
{
    guint val = gtk_radio_action_get_current_value(cur);
    FmFolderModel *model = fm_folder_view_get_model(fv);

    /* g_debug("on_change_by"); */
    if(model)
        fm_folder_model_set_sort(model, val, FM_SORT_DEFAULT);
}

static void on_change_type(GtkRadioAction* act, GtkRadioAction* cur, FmFolderView* fv)
{
    guint val = gtk_radio_action_get_current_value(cur);
    FmFolderModel *model = fm_folder_view_get_model(fv);
    FmSortMode mode;

    /* g_debug("on_change_type"); */
    if(model)
    {
        fm_folder_model_get_sort(model, NULL, &mode);
        mode &= ~FM_SORT_ORDER_MASK;
        mode |= (val == GTK_SORT_ASCENDING) ? FM_SORT_ASCENDING : FM_SORT_DESCENDING;
        fm_folder_model_set_sort(model, FM_FOLDER_MODEL_COL_DEFAULT, mode);
    }
}

static void on_ui_destroy(gpointer ui_ptr)
{
    _init_quarks();

    GtkUIManager* ui = (GtkUIManager*)ui_ptr;
    GtkMenu* popup = GTK_MENU(gtk_ui_manager_get_widget(ui, "/popup"));
    GtkWindow* win = GTK_WINDOW(gtk_menu_get_attach_widget(popup));
    GtkAccelGroup* accel_grp = gtk_ui_manager_get_accel_group(ui);
    GList *templates = g_object_get_qdata(G_OBJECT(ui), templates_quark);

    if(!gtk_accel_group_get_is_locked(accel_grp))
        gtk_window_remove_accel_group(win, accel_grp);
    g_list_foreach(templates, (GFunc)g_object_unref, NULL);
    g_list_free(templates);
    g_object_set_qdata(G_OBJECT(ui), templates_quark, NULL);
    gtk_widget_destroy(GTK_WIDGET(popup));
    g_object_unref(ui);
}

/**
 * fm_folder_view_add_popup
 * @fv: a widget to apply
 * @parent: parent window of @fv
 * @update_popup: function to extend popup menu for folder
 *
 * Adds popup menu to window @parent associated with widget @fv. This
 * includes hotkeys for popup menu items. Popup will be destroyed and
 * hotkeys will be removed from @parent when @fv is finalized.
 *
 * Returns: (transfer none): a new created widget.
 *
 * Since: 1.0.1
 */
GtkMenu* fm_folder_view_add_popup(FmFolderView* fv, GtkWindow* parent,
                                  FmFolderViewUpdatePopup update_popup)
{
    _init_quarks();

    FmFolderViewInterface* iface;
    FmFolderModel* model;
    GtkUIManager* ui;
    GtkActionGroup* act_grp;
    GtkMenu* popup;
    GtkAction* act;
    GtkAccelGroup* accel_grp;
    gboolean show_hidden;
    FmSortMode mode;
    GtkSortType type = (GtkSortType)-1;
    FmFolderModelCol by = (FmFolderModelCol)-1;

    iface = FM_FOLDER_VIEW_GET_IFACE(fv);
    show_hidden = iface->get_show_hidden(fv);
    model = iface->get_model(fv);
    if(fm_folder_model_get_sort(model, &by, &mode))
        type = FM_SORT_IS_ASCENDING(mode) ? GTK_SORT_ASCENDING : GTK_SORT_DESCENDING;

    /* init popup from XML string */
    ui = gtk_ui_manager_new();
    act_grp = gtk_action_group_new("Folder");
    gtk_action_group_set_translation_domain(act_grp, GETTEXT_PACKAGE);
    gtk_action_group_add_actions(act_grp, folder_popup_actions,
                                 G_N_ELEMENTS(folder_popup_actions), fv);
    gtk_action_group_add_toggle_actions(act_grp, folder_toggle_actions,
                                        G_N_ELEMENTS(folder_toggle_actions), fv);
    gtk_action_group_add_radio_actions(act_grp, folder_sort_type_actions,
                                       G_N_ELEMENTS(folder_sort_type_actions),
                                       type, G_CALLBACK(on_change_type), fv);
    gtk_action_group_add_radio_actions(act_grp, folder_sort_by_actions,
                                       G_N_ELEMENTS(folder_sort_by_actions),
                                       by, G_CALLBACK(on_change_by), fv);
    gtk_ui_manager_insert_action_group(ui, act_grp, 0);
    gtk_ui_manager_add_ui_from_string(ui, folder_popup_xml, -1, NULL);
    act = gtk_ui_manager_get_action(ui, "/popup/ShowHidden");
    gtk_toggle_action_set_active(GTK_TOGGLE_ACTION(act), show_hidden);
    act = gtk_ui_manager_get_action(ui, "/popup/Cut");
    gtk_action_set_visible(act, FALSE);
    act = gtk_ui_manager_get_action(ui, "/popup/Copy");
    gtk_action_set_visible(act, FALSE);
    act = gtk_ui_manager_get_action(ui, "/popup/Del");
    gtk_action_set_visible(act, FALSE);
    act = gtk_ui_manager_get_action(ui, "/popup/Remove");
    gtk_action_set_visible(act, FALSE);
    act = gtk_ui_manager_get_action(ui, "/popup/FileProp");
    gtk_action_set_visible(act, FALSE);
    if(update_popup)
        update_popup(fv, parent, ui, act_grp, NULL);
    popup = GTK_MENU(gtk_ui_manager_get_widget(ui, "/popup"));
    accel_grp = gtk_ui_manager_get_accel_group(ui);
    gtk_window_add_accel_group(parent, accel_grp);
    gtk_menu_attach_to_widget(popup, GTK_WIDGET(parent), NULL);
    g_object_unref(act_grp);
    g_object_set_qdata_full(G_OBJECT(fv), ui_quark, ui, on_ui_destroy);
    g_object_set_qdata(G_OBJECT(fv), popup_quark, popup);
    /* special handling for 'Menu' key */
    g_signal_connect(G_OBJECT(fv), "key-press-event", G_CALLBACK(on_key_press), fv);
    return popup;
}

/**
 * fm_folder_view_bounce_action
 * @act: an action to execute
 * @fv: a widget to apply
 *
 * Executes the action with the same name as @act in popup menu of @fv.
 * The popup menu should be created with fm_folder_view_add_popup()
 * before this call.
 *
 * Implemented actions are:
 * - Cut       : cut files (or text from editable) into clipboard
 * - Copy      : copy files (or text from editable) into clipboard
 * - Paste     : paste files (or text from editable) from clipboard
 * - Del       : move files into trash bin (or delete text from editable)
 * - Remove    : delete files from filesystem (for editable does Cut)
 * - SelAll    : select all
 * - InvSel    : invert selection
 * - Rename    : rename the folder
 * - Prop      : folder properties dialog
 * - FileProp  : file properties dialog
 * - NewFolder : create new folder here
 * - NewBlank  : create an empty file here
 *
 * Actions 'Cut', 'Copy', 'Paste', 'Del', 'Remove', 'SelAll' do nothing
 * if current keyboard focus is neither on @fv nor on some #GtkEditable.
 *
 * See also: fm_folder_view_add_popup().
 *
 * Since: 1.0.1
 */
void fm_folder_view_bounce_action(GtkAction* act, FmFolderView* fv)
{
    _init_quarks();

    const gchar *name;
    GtkUIManager *ui;
    GList *groups;

    g_return_if_fail(FM_IS_FOLDER_VIEW(fv));
    g_return_if_fail(act != NULL);

    ui = g_object_get_qdata(G_OBJECT(fv), ui_quark);
    g_return_if_fail(ui != NULL && GTK_IS_UI_MANAGER(ui));

    groups = gtk_ui_manager_get_action_groups(ui);
    g_return_if_fail(groups != NULL);

    name = gtk_action_get_name(act);
    act = gtk_action_group_get_action((GtkActionGroup*)groups->data, name);
    if(act)
        gtk_action_activate(act);
    else
        g_debug("requested action %s wasn't found in popup", name);
}

/**
 * fm_folder_view_set_active
 * @fv: the folder view widget to apply
 * @set: state of accelerators to be set
 *
 * If @set is %TRUE then activates accelerators on the @fv that were
 * created with fm_folder_view_add_popup() before. If @set is %FALSE
 * then deactivates accelerators on the @fv. This API is useful if the
 * application window contains more than one folder view so gestures
 * will be used only on active view. This API has no effect in no popup
 * menu was created with fm_folder_view_add_popup() before this call.
 *
 * See also: fm_folder_view_add_popup().
 *
 * Since: 1.0.1
 */
void fm_folder_view_set_active(FmFolderView* fv, gboolean set)
{
    _init_quarks();

    GtkUIManager *ui;
    GtkMenu *popup;
    GtkWindow* win;
    GtkAccelGroup* accel_grp;
    gboolean locked;

    g_return_if_fail(FM_IS_FOLDER_VIEW(fv));

    ui = g_object_get_qdata(G_OBJECT(fv), ui_quark);
    popup = g_object_get_qdata(G_OBJECT(fv), popup_quark);

    g_return_if_fail(ui != NULL && GTK_IS_UI_MANAGER(ui));
    g_return_if_fail(popup != NULL && GTK_IS_MENU(popup));

    win = GTK_WINDOW(gtk_menu_get_attach_widget(popup));
    accel_grp = gtk_ui_manager_get_accel_group(ui);
    locked = gtk_accel_group_get_is_locked(accel_grp);

    if(set && locked)
    {
        gtk_window_add_accel_group(win, accel_grp);
        gtk_accel_group_unlock(accel_grp);
    }
    else if(!set && !locked)
    {
        gtk_window_remove_accel_group(win, accel_grp);
        gtk_accel_group_lock(accel_grp);
    }
}


void fm_folder_view_files_clicked(FmFolderView* fv, FmFileInfo * fi, FmFileInfoList* files,
                                 FmFolderViewClickType type)
{
    g_return_if_fail(FM_IS_FOLDER_VIEW(fv));

    FmFolderViewInterface * iface = FM_FOLDER_VIEW_GET_IFACE(fv);
    g_return_if_fail(iface);

    _init_quarks();

    GtkMenu * popup = g_object_get_qdata(G_OBJECT(fv), popup_quark);
    if (popup == NULL) /* no fm_folder_view_add_popup() was called before */
        return;

    GtkWindow * win = GTK_WINDOW(gtk_menu_get_attach_widget(popup));

    FmFolderViewUpdatePopup update_popup = NULL;
    FmLaunchFolderFunc open_folders = NULL;
    if (iface->get_custom_menu_callbacks)
        iface->get_custom_menu_callbacks(fv, &update_popup, &open_folders);

    switch(type)
    {
    case FM_FV_ACTIVATED: /* file activated */
        fm_launch_file_simple(win, NULL, fi, open_folders, win);
        break;
    case FM_FV_CONTEXT_MENU:
    {
        if (files)
        {
            FmFileMenu* menu = fm_file_menu_new_for_files(win, files, fm_folder_view_get_cwd(fv), TRUE);
            fm_file_menu_set_folder_func(menu, open_folders, win);

            /* TODO: add info message on selection if enabled in config */
            /* merge some specific menu items */
            if (update_popup)
                update_popup(fv, win, fm_file_menu_get_ui(menu),
                             fm_file_menu_get_action_group(menu), files);
            popup = fm_file_menu_get_menu(menu);
            gtk_menu_popup(popup, NULL, NULL, NULL, NULL, 3, gtk_get_current_event_time());
        }
        else
        {
            fm_folder_view_show_popup(fv);
        }
        break;
    }
    default: ;
    }

}

