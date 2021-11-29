/*
 *      fm-file-properties.c
 *
 *      Copyright 2009 PCMan <pcman.tw@gmail.com>
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
 * SECTION:fm-file-properties
 * @short_description: Dialog window for changing properties of file.
 * @title: File properties dialog
 *
 * @include: libfm/fm-gtk.h
 *
 * The file properties dialog is a window with few tabs and buttons "OK"
 * and "Cancel". Most of content of those tabs is handled by the widget
 * itself but there is a possibility to change its content for some file
 * type. See fm_file_properties_add_for_mime_type() for details.
 *
 * Default content of tabs of file properties dialog follows (each tab has
 * a GtkAlignment element, tab ids below meant of those):
 *
 * Tab 1: contains GtkTable (id general_table) with items:
 * - GtkImage (id icon)         : file icon, eventbox: id icon_eventbox
 * - GtkLabel (id file)         : reserved (hidden), hidden label: id file_label
 * - GtkEntry (id name)         : label: "Name"
 * - GtkLabel (id dir)          : label: "Location"
 * - GtkLabel (id target)       : label: "Target", id target_label
 * - GtkLabel (id type)         : label: "File type"
 * - GtkComboBox (id open_with) : label: "Open with", id open_with_label
 * - GtkLabel (id total_size)   : label: "Total Size of Files", id total_size_label
 * - GtkLabel (id size_on_disk) : label: "Size on Disk", id size_on_disk_label
 * - GtkLabel (id mtime)        : label: "Last Modification", id mtime_label
 * - GtkLabel (id atime)        : label: "Last Access", id atime_label
 *
 * Tab 2: id permissions_tab, contains items inside:
 * - GtkEntry (id owner)        : label: "Owner", id owner_label
 * - GtkEntry (id group)        : label: "Group", id group_label
 * - GtkComboBox (id read_perm) : label: "View content"
 * - GtkComboBox (id write_perm) : label: "Change content"
 * - GtkComboBox (id exec_perm) : label: "Execute", id exec_label
 * - GtkComboBox (id flags_set_file) : label: "Special bits", id flags_label
 * - GtkComboBox (id flags_set_dir) : share the place with flags_set_file
 *
 * Tab 3: id extra_tab (hidden), empty, label: id extra_tab_label
 *
 * Since gtk_table_get_size() is available only for GTK 2.22 ... GTK 3.4
 * it is not generally recommended to change size of GtkTable but be also
 * aware that gtk_table_attach() is marked deprecated in GTK 3.4 though.
 */

#include <config.h>
#include <glib/gi18n-lib.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <time.h>

#include <ctype.h>
#include <stdlib.h>
#include <pwd.h>
#include <grp.h>

#include <libsmfm-core/fm.h>
#include "fm-file-properties.h"

#include "fm-progress-dlg.h"
#include "fm-gtk-utils.h"
#include "fm-icon-pixbuf.h"

#include "fm-app-chooser-combo-box.h"

#define     UI_FILE             PACKAGE_UI_DIR"/file-prop.ui"
#define     GET_WIDGET(transform,name) data->name = transform(gtk_builder_get_object(builder, #name))

/* for 'Read' combo box */
enum {
    NO_CHANGE = 0,
    READ_USER,
    READ_GROUP,
    READ_ALL
};

/* for 'Write' and 'Exec'/'Enter' combo box */
enum {
    /* NO_CHANGE, */
    ACCESS_NOBODY = 1,
    ACCESS_USER,
    ACCESS_GROUP,
    ACCESS_ALL
};

/* for files-only 'Special' combo box */
enum {
    /* NO_CHANGE, */
    FILE_COMMON = 1,
    FILE_SUID,
    FILE_SGID,
    FILE_SUID_SGID
};

/* for directories-only 'Special' combo box */
enum {
    /* NO_CHANGE, */
    DIR_COMMON = 1,
    DIR_STICKY,
    DIR_SGID,
    DIR_STICKY_SGID
};

typedef struct _FmFilePropExt FmFilePropExt;
struct _FmFilePropExt
{
    FmFilePropExt *next; /* built-in GSList */
    FmMimeType *type;
    FmFilePropertiesExtensionInit cb; /* callbacks */
};

static FmFilePropExt *extensions = NULL;

typedef struct _FmFilePropData FmFilePropData;
struct _FmFilePropData
{
    GtkDialog* dlg;

    guint theme_changed_handler;
    FmIcon * fi_icon;

    /* General page */
    GtkTable* general_table;
    GtkImage* icon;
    GtkLabel* file_label;
    GtkLabel* file;
    GtkEntry* name;
    GtkLabel* dir;
    GtkLabel* target;
    GtkWidget* target_label;
    GtkLabel* type;
    GtkWidget* open_with_label;
    GtkComboBox* open_with;
    GtkLabel* total_size;
    GtkLabel* size_on_disk;
    GtkLabel* mtime;
    GtkLabel* atime;

    /* Permissions page */
    GtkWidget* permissions_tab;
    GtkEntry* owner;
    char* orig_owner;
    GtkEntry* group;
    char* orig_group;
    GtkComboBox* read_perm;
    int read_perm_sel;
    GtkComboBox* write_perm;
    int write_perm_sel;
    GtkLabel* exec_label;
    GtkComboBox* exec_perm;
    int exec_perm_sel;
    GtkLabel* flags_label;
    GtkComboBox* flags_set_file;
    GtkComboBox* flags_set_dir;
    int flags_set_sel;

    FmFileInfoList* files;
    FmFileInfo* fi;
    gboolean single_type;
    gboolean single_file;
    gboolean all_native;
    gboolean has_dir;
    gboolean all_dirs;
    FmMimeType* mime_type;

    gint32 uid;
    gint32 gid;

    guint timeout;
    FmDeepCountJob* dc_job;

    FmFilePropExt* ext;
    gpointer extdata;
};

static void label_set_size(GtkLabel * label, long long unsigned int size)
{
    char size_str1[256];
    fm_file_size_to_str(size_str1, sizeof(size_str1), size, fm_config->si_unit);

    gchar * size_str2 = g_strdup_printf("%'llu %s", size,
        g_dngettext(GETTEXT_PACKAGE, "byte", "bytes", size));

    if (strcmp(size_str1, size_str2) == 0)
    {
        gtk_label_set_text(label, size_str1);
    }
    else
    {
        gchar * str = g_strdup_printf("%s (%s)", size_str1, size_str2);
        gtk_label_set_text(label, str);
        g_free(str);
    }

    g_free(size_str2);
}

static gboolean on_timeout(gpointer user_data)
{
    FmFilePropData* data = (FmFilePropData*)user_data;
    FmDeepCountJob* dc;

    GDK_THREADS_ENTER();
    if(g_source_is_destroyed(g_main_current_source()))
    {
        GDK_THREADS_LEAVE();
        return FALSE;
    }
    dc = data->dc_job;
    if(G_LIKELY(dc && !fm_job_is_cancelled(FM_JOB(dc))))
    {
        label_set_size(data->total_size, dc->total_size);
        label_set_size(data->size_on_disk, dc->total_ondisk_size);
    }
    GDK_THREADS_LEAVE();
    return TRUE;
}

static void on_finished(FmDeepCountJob* job, FmFilePropData* data)
{
    on_timeout(data); /* update display */
    if(data->timeout)
    {
        g_source_remove(data->timeout);
        data->timeout = 0;
    }
    g_object_unref(data->dc_job);
    data->dc_job = NULL;
}

static void fm_file_prop_data_free(FmFilePropData* data)
{
    if (data->theme_changed_handler)
    {
        GtkIconTheme* theme = gtk_icon_theme_get_default();
        g_signal_handler_disconnect(theme, data->theme_changed_handler);
    }

    if (data->fi_icon)
        fm_icon_unref(data->fi_icon);

    g_free(data->orig_owner);
    g_free(data->orig_group);

    if(data->timeout)
        g_source_remove(data->timeout);
    if(data->dc_job) /* FIXME: check if it's running */
    {
        fm_job_cancel(FM_JOB(data->dc_job));
        g_signal_handlers_disconnect_by_func(data->dc_job, on_finished, data);
        g_object_unref(data->dc_job);
    }
    if(data->mime_type)
        fm_mime_type_unref(data->mime_type);
    fm_file_info_list_unref(data->files);
    g_slice_free(FmFilePropData, data);
}

static gboolean ensure_valid_owner(FmFilePropData* data)
{
    gboolean ret = TRUE;
    const char* tmp = gtk_entry_get_text(data->owner);

    data->uid = -1;
    if(tmp && *tmp)
    {
        if(data->all_native && !isdigit(tmp[0])) /* entering names instead of numbers is only allowed for local files. */
        {
            struct passwd* pw;
            if(!tmp || !*tmp || !(pw = getpwnam(tmp)))
                ret = FALSE;
            else
                data->uid = pw->pw_uid;
        }
        else
            data->uid = atoi(tmp);
    }
    else
        ret = FALSE;

    if(!ret)
    {
        fm_show_error(GTK_WINDOW(data->dlg), NULL, _("Please enter a valid user name or numeric id."));
        gtk_widget_grab_focus(GTK_WIDGET(data->owner));
    }

    return ret;
}

static gboolean ensure_valid_group(FmFilePropData* data)
{
    gboolean ret = TRUE;
    const char* tmp = gtk_entry_get_text(data->group);

    if(tmp && *tmp)
    {
        if(data->all_native && !isdigit(tmp[0])) /* entering names instead of numbers is only allowed for local files. */
        {
            struct group* gr;
            if(!tmp || !*tmp || !(gr = getgrnam(tmp)))
                ret = FALSE;
            else
                data->gid = gr->gr_gid;
        }
        else
            data->gid = atoi(tmp);
    }
    else
        ret = FALSE;

    if(!ret)
    {
        fm_show_error(GTK_WINDOW(data->dlg), NULL, _("Please enter a valid group name or numeric id."));
        gtk_widget_grab_focus(GTK_WIDGET(data->group));
    }

    return ret;
}


static void on_response(GtkDialog* dlg, int response, FmFilePropData* data)
{
    /* call the extension if it was set */
    if(data->ext != NULL)
        data->ext->cb.finish(data->extdata, response != GTK_RESPONSE_OK);

    if( response == GTK_RESPONSE_OK )
    {
        int sel;
        const char* new_owner = gtk_entry_get_text(data->owner);
        const char* new_group = gtk_entry_get_text(data->group);
        mode_t new_mode = 0, new_mode_mask = 0;

        if(!ensure_valid_owner(data) || !ensure_valid_group(data))
        {
            g_signal_stop_emission_by_name(dlg, "response");
            return;
        }

        /* FIXME: if all files are native, it's possible to check
         * if the names are legal user and group names on the local
         * machine prior to chown. */
        if(new_owner && *new_owner && g_strcmp0(data->orig_owner, new_owner))
        {
            /* change owner */
            g_debug("change owner to: %d", data->uid);
        }
        else
            data->uid = -1;

        if(new_group && *new_group && g_strcmp0(data->orig_group, new_group))
        {
            /* change group */
            g_debug("change group to: %d", data->gid);
        }
        else
            data->gid = -1;

        /* check if chmod is needed here. */
        sel = gtk_combo_box_get_active(data->read_perm);
        if( sel != NO_CHANGE ) /* requested to change read permissions */
        {
            g_debug("got selection for read: %d", sel);
            if(data->read_perm_sel != sel) /* new value is different from original */
            {
                new_mode_mask = (S_IRUSR|S_IRGRP|S_IROTH);
                data->read_perm_sel = sel;
                switch(sel)
                {
                case READ_ALL:
                    new_mode = S_IROTH;
                case READ_GROUP:
                    new_mode |= S_IRGRP;
                case READ_USER:
                default:
                    new_mode |= S_IRUSR;
                }
            }
            else /* otherwise, no change */
                data->read_perm_sel = NO_CHANGE;
        }
        else
            data->read_perm_sel = NO_CHANGE;

        sel = gtk_combo_box_get_active(data->write_perm);
        if( sel != NO_CHANGE ) /* requested to change write permissions */
        {
            g_debug("got selection for write: %d", sel);
            if(data->write_perm_sel != sel) /* new value is different from original */
            {
                new_mode_mask |= (S_IWUSR|S_IWGRP|S_IWOTH);
                data->write_perm_sel = sel;
                switch(sel)
                {
                case ACCESS_ALL:
                    new_mode |= S_IWOTH;
                case ACCESS_GROUP:
                    new_mode |= S_IWGRP;
                case ACCESS_USER:
                    new_mode |= S_IWUSR;
                case ACCESS_NOBODY: default: ;
                }
            }
            else /* otherwise, no change */
                data->write_perm_sel = NO_CHANGE;
        }
        else
            data->write_perm_sel = NO_CHANGE;

        sel = gtk_combo_box_get_active(data->exec_perm);
        if( sel != NO_CHANGE ) /* requested to change exec permissions */
        {
            g_debug("got selection for exec: %d", sel);
            if(data->exec_perm_sel != sel) /* new value is different from original */
            {
                new_mode_mask |= (S_IXUSR|S_IXGRP|S_IXOTH);
                data->exec_perm_sel = sel;
                switch(sel)
                {
                case ACCESS_ALL:
                    new_mode |= S_IXOTH;
                case ACCESS_GROUP:
                    new_mode |= S_IXGRP;
                case ACCESS_USER:
                    new_mode |= S_IXUSR;
                case ACCESS_NOBODY: default: ;
                }
            }
            else /* otherwise, no change */
                data->exec_perm_sel = NO_CHANGE;
        }
        else
            data->exec_perm_sel = NO_CHANGE;

        if(data->all_dirs)
            sel = gtk_combo_box_get_active(data->flags_set_dir);
        else if(!data->has_dir)
            sel = gtk_combo_box_get_active(data->flags_set_file);
        else
            sel = NO_CHANGE;
        if( sel != NO_CHANGE ) /* requested to change special bits */
        {
            g_debug("got selection for flags: %d", sel);
            if(data->flags_set_sel != sel) /* new value is different from original */
            {
                new_mode_mask |= (S_ISUID|S_ISGID|S_ISVTX);
                data->flags_set_sel = sel;
                if(data->all_dirs)
                {
                    switch(sel)
                    {
                    case DIR_STICKY:
                        new_mode |= S_ISVTX;
                        break;
                    case DIR_STICKY_SGID:
                        new_mode |= S_ISVTX;
                    case DIR_SGID:
                        new_mode |= S_ISGID;
                    case DIR_COMMON: default: ;
                    }
                }
                else
                {
                    switch(sel)
                    {
                    case FILE_SUID:
                        new_mode |= S_ISUID;
                        break;
                    case FILE_SUID_SGID:
                        new_mode |= S_ISUID;
                    case FILE_SGID:
                        new_mode |= S_ISGID;
                    case FILE_COMMON: default: ;
                    }
                }
            }
            else /* otherwise, no change */
                data->flags_set_sel = NO_CHANGE;
        }
        else
            data->flags_set_sel = NO_CHANGE;

        if(new_mode_mask || data->uid != -1 || data->gid != -1)
        {
            FmPathList* paths = fm_path_list_new_from_file_info_list(data->files);
            FmFileOpsJob* job = fm_file_ops_job_new(FM_FILE_OP_CHANGE_ATTR, paths);

            /* need to chown */
            if(data->uid != -1 || data->gid != -1)
                fm_file_ops_job_set_chown(job, data->uid, data->gid);

            /* need to do chmod */
            if(new_mode_mask) {
                g_debug("going to set mode bits %04o by mask %04o", new_mode, new_mode_mask);
                fm_file_ops_job_set_chmod(job, new_mode, new_mode_mask);
            }

            /* try recursion but don't recurse exec/sgid/sticky changes */
            if(data->has_dir && data->exec_perm_sel == NO_CHANGE &&
               data->flags_set_sel == NO_CHANGE)
            {
                gtk_combo_box_set_active(data->read_perm, data->read_perm_sel);
                gtk_combo_box_set_active(data->write_perm, data->write_perm_sel);
                gtk_combo_box_set_active(data->exec_perm, NO_CHANGE);
                gtk_combo_box_set_active(data->flags_set_dir, NO_CHANGE);
                /* FIXME: may special bits and exec flags still be messed up? */
                if(fm_yes_no(GTK_WINDOW(data->dlg), NULL, _( "Do you want to recursively apply these changes to all files and sub-folders?" ), TRUE))
                    fm_file_ops_job_set_recursive(job, TRUE);
            }

            /* show progress dialog */
            fm_file_ops_job_run_with_progress(GTK_WINDOW(dlg), job);
                                                        /* it eats reference! */
            fm_path_list_unref(paths);
        }

        /* change default application for the mime-type if needed */
        if(data->mime_type && fm_mime_type_get_type(data->mime_type) && data->open_with)
        {
            GAppInfo* app;
            gboolean default_app_changed = FALSE;
            GError* err = NULL;
            app = fm_app_chooser_combo_box_dup_selected_app(data->open_with, &default_app_changed);
            if(app)
            {
                if(default_app_changed)
                {
                    g_app_info_set_as_default_for_type(app, fm_mime_type_get_type(data->mime_type), &err);
                    if(err)
                    {
                        fm_show_error(GTK_WINDOW(dlg), NULL, err->message);
                        g_error_free(err);
                    }
                }
                g_object_unref(app);
            }
        }

        if(data->single_file) /* when only one file is shown */
        {
            /* if the user has changed its name */
            if(g_strcmp0(fm_file_info_get_disp_name(data->fi), gtk_entry_get_text(data->name)))
            {
                /* FIXME: rename the file or set display name for it. */
            }
        }
    }
    gtk_widget_destroy(GTK_WIDGET(dlg));
}

/* FIXME: this is too dirty. Need some refactor later. */
static void update_permissions(FmFilePropData* data)
{
    FmFileInfo* fi = fm_file_info_list_peek_head(data->files);
    GList *l;
    int sel;
    char* tmp;
    mode_t fi_mode = fm_file_info_get_mode(fi);
    mode_t read_perm = (fi_mode & (S_IRUSR|S_IRGRP|S_IROTH));
    mode_t write_perm = (fi_mode & (S_IWUSR|S_IWGRP|S_IWOTH));
    mode_t exec_perm = (fi_mode & (S_IXUSR|S_IXGRP|S_IXOTH));
    mode_t flags_set = (fi_mode & (S_ISUID|S_ISGID|S_ISVTX));
    gint32 uid = fm_file_info_get_uid(fi);
    gint32 gid = fm_file_info_get_gid(fi);
    gboolean mix_read = FALSE, mix_write = FALSE, mix_exec = FALSE;
    gboolean mix_flags = FALSE;
    struct group* grp = NULL;
    struct passwd* pw = NULL;
    char unamebuf[1024];
    struct group grpb;
    struct passwd pwb;

    data->all_native = fm_path_is_native(fm_file_info_get_path(fi));
    data->has_dir = (S_ISDIR(fi_mode) != FALSE);
    data->all_dirs = data->has_dir;

    if((fi_mode & ~S_IFDIR) == 0 && uid < 0) /* no permissions accessible */
    {
        gtk_widget_hide(data->permissions_tab);
        return;
    }

    for(l=fm_file_info_list_peek_head_link(data->files)->next; l; l=l->next)
    {
        FmFileInfo* fi = FM_FILE_INFO(l->data);

        if(data->all_native && !fm_path_is_native(fm_file_info_get_path(fi)))
            data->all_native = FALSE;

        fi_mode = fm_file_info_get_mode(fi);
        if((fi_mode & ~S_IFDIR) == 0) /* no permissions accessible */
        {
            gtk_widget_hide(data->permissions_tab);
            return;
        }
        if(S_ISDIR(fi_mode))
            data->has_dir = TRUE;
        else
            data->all_dirs = FALSE;

        if( uid >= 0 && uid != (gint32)fm_file_info_get_uid(fi) )
            uid = -1;
        if( gid >= 0 && gid != (gint32)fm_file_info_get_gid(fi) )
            gid = -1;

        if(!mix_read && read_perm != (fi_mode & (S_IRUSR|S_IRGRP|S_IROTH)))
            mix_read = TRUE;
        if(!mix_write && write_perm != (fi_mode & (S_IWUSR|S_IWGRP|S_IWOTH)))
            mix_write = TRUE;
        if(!mix_exec && exec_perm != (fi_mode & (S_IXUSR|S_IXGRP|S_IXOTH)))
            mix_exec = TRUE;
        if(!mix_flags && flags_set != (fi_mode & (S_ISUID|S_ISGID|S_ISVTX)))
            mix_flags = TRUE;
    }

    if( data->all_native )
    {
        if(uid >= 0)
        {
            getpwuid_r(uid, &pwb, unamebuf, sizeof(unamebuf), &pw);
            if(pw)
                gtk_entry_set_text(data->owner, pw->pw_name);
        }
        if(gid >= 0)
        {
            getgrgid_r(gid, &grpb, unamebuf, sizeof(unamebuf), &grp);
            if(grp)
                gtk_entry_set_text(data->group, grp->gr_name);
        }
    }

    if(uid >= 0 && !pw)
    {
        tmp = g_strdup_printf("%d", uid);
        gtk_entry_set_text(data->owner, tmp);
        g_free(tmp);
    }

    if(gid >= 0 && !grp)
    {
        tmp = g_strdup_printf("%d", gid);
        gtk_entry_set_text(data->group, tmp);
        g_free(tmp);
    }

    data->orig_owner = g_strdup(gtk_entry_get_text(data->owner));
    data->orig_group = g_strdup(gtk_entry_get_text(data->group));

    /* on local filesystems, only root can do chown. */
    if( data->all_native && geteuid() != 0 )
    {
        gtk_widget_set_sensitive(GTK_WIDGET(data->owner), FALSE);
        gtk_widget_set_sensitive(GTK_WIDGET(data->group), FALSE);
    }

    /* read access chooser */
    sel = NO_CHANGE;
    if(!mix_read)
    {
        if(read_perm & S_IROTH)
            sel = READ_ALL;
        else if(read_perm & S_IRGRP)
            sel = READ_GROUP;
        else
            sel = READ_USER;
    }
    gtk_combo_box_set_active(data->read_perm, sel);
    data->read_perm_sel = sel;

    /* write access chooser */
    sel = NO_CHANGE;
    if(!mix_write)
    {
        if(write_perm & S_IWOTH)
            sel = ACCESS_ALL;
        else if(write_perm & S_IWGRP)
            sel = ACCESS_GROUP;
        else if(write_perm & S_IWUSR)
            sel = ACCESS_USER;
        else
            sel = ACCESS_NOBODY;
    }
    gtk_combo_box_set_active(data->write_perm, sel);
    data->write_perm_sel = sel;

    /* disable exec and special bits for mixed selection and return */
    if(data->has_dir && !data->all_dirs) {
        gtk_widget_hide(GTK_WIDGET(data->exec_label));
        gtk_widget_hide(GTK_WIDGET(data->exec_perm));
        data->exec_perm_sel = NO_CHANGE;
        gtk_widget_hide(GTK_WIDGET(data->flags_label));
        gtk_widget_hide(GTK_WIDGET(data->flags_set_file));
        gtk_widget_hide(GTK_WIDGET(data->flags_set_dir));
        data->flags_set_sel = NO_CHANGE;
        return;
    }
    if(data->has_dir)
        gtk_label_set_label(data->exec_label, _("<b>_Access content:</b>"));
    if(!fm_config->advanced_mode)
    {
        gtk_widget_hide(GTK_WIDGET(data->flags_label));
        gtk_widget_hide(GTK_WIDGET(data->flags_set_file));
        gtk_widget_hide(GTK_WIDGET(data->flags_set_dir));
        data->flags_set_sel = NO_CHANGE;
    }
    else if(data->has_dir)
        gtk_widget_hide(GTK_WIDGET(data->flags_set_file));
    else
        gtk_widget_hide(GTK_WIDGET(data->flags_set_dir));

    /* exec access chooser */
    sel = NO_CHANGE;
    if(!mix_exec)
    {
        if(exec_perm & S_IXOTH)
            sel = ACCESS_ALL;
        else if(exec_perm & S_IXGRP)
            sel = ACCESS_GROUP;
        else if(exec_perm & S_IXUSR)
            sel = ACCESS_USER;
        else
            sel = ACCESS_NOBODY;
    }
    gtk_combo_box_set_active(data->exec_perm, sel);
    data->exec_perm_sel = sel;

    /* special bits chooser */
    sel = NO_CHANGE;
    if(data->has_dir)
    {
        if(!mix_flags)
        {
            if((flags_set & (S_ISGID|S_ISVTX)) == (S_ISGID|S_ISVTX))
                sel = DIR_STICKY_SGID;
            else if(flags_set & S_ISGID)
                sel = DIR_SGID;
            else if(flags_set & S_ISVTX)
                sel = DIR_STICKY;
            else
                sel = DIR_COMMON;
        }
        gtk_combo_box_set_active(data->flags_set_dir, sel);
    }
    else
    {
        if(!mix_flags)
        {
            if((flags_set & (S_ISUID|S_ISGID)) == (S_ISUID|S_ISGID))
                sel = FILE_SUID_SGID;
            else if(flags_set & S_ISUID)
                sel = FILE_SUID;
            else if(flags_set & S_ISGID)
                sel = FILE_SGID;
            else
                sel = FILE_COMMON;
        }
        gtk_combo_box_set_active(data->flags_set_file, sel);
    }
    data->flags_set_sel = sel;

    /* on local filesystems, only root or owner can change permissions. */
    if( data->all_native && geteuid() != 0 && geteuid() != uid)
    {
        gtk_widget_set_sensitive(GTK_WIDGET(data->read_perm), FALSE);
        gtk_widget_set_sensitive(GTK_WIDGET(data->write_perm), FALSE);
        gtk_widget_set_sensitive(GTK_WIDGET(data->exec_perm), FALSE);
        gtk_widget_set_sensitive(GTK_WIDGET(data->flags_set_file), FALSE);
        gtk_widget_set_sensitive(GTK_WIDGET(data->flags_set_dir), FALSE);
    }
}

static void set_time_interval(GtkLabel * label, time_t min_time, time_t max_time)
{
    struct tm tm;
    char buf1[256], buf2[256];

    localtime_r(&min_time, &tm);
    strftime(buf1, sizeof(buf1), "%x %R", &tm);

    if (min_time == 0)
    {
        gtk_label_set_text(label, _("N/A"));
    }
    else if (min_time == max_time)
    {
        gtk_label_set_text(label, buf1);
    }
    else
    {
        localtime_r(&max_time, &tm);
        strftime(buf2, sizeof(buf2), "%x %R", &tm);

        gchar * s = g_strdup_printf(_("%s ... %s"), buf1, buf2);
        gtk_label_set_text(label, s);
        g_free(s);
    }

}

static void on_icon_theme_changed(GtkIconTheme* theme, gpointer user_data)
{
    FmFilePropData * data = (FmFilePropData *) user_data;

    if (data->fi_icon)
    {
        GList * list = fm_pixbuf_list_from_icon(data->fi_icon);
        gtk_window_set_icon_list(GTK_WINDOW(data->dlg), list);
        g_list_free_full(list, g_object_unref);
    }
}


static void update_ui(FmFilePropData* data)
{
    GtkImage* img = data->icon;

    if( data->single_type ) /* all files are of the same mime-type */
    {
        /* FIXME: handle custom icons for some files */
        FmFilePropExt* ext;

        /* FIXME: display special property pages for special files or
         * some specified mime-types. */
        if (data->single_file) /* only one file is selected. */
        {
            FmFileInfo* fi = fm_file_info_list_peek_head(data->files);
            if (!data->fi_icon)
                data->fi_icon = fm_icon_ref(fm_file_info_get_icon(fi));
        }

        if (data->mime_type)
        {
            if (!data->fi_icon)
                data->fi_icon = fm_icon_ref(fm_mime_type_get_icon(data->mime_type));

            gchar * s = g_strdup_printf(_("%s (%s)"),
                fm_mime_type_get_desc(data->mime_type),
                fm_mime_type_get_type(data->mime_type));
            gtk_label_set_text(data->type, s);
            g_free(s);
        }

        if (data->fi_icon)
        {
            if (data->fi_icon->gicon)
                gtk_image_set_from_gicon(img, data->fi_icon->gicon, GTK_ICON_SIZE_DIALOG);

            GList * list = fm_pixbuf_list_from_icon(data->fi_icon);
            gtk_window_set_icon_list(GTK_WINDOW(data->dlg), list);
            g_list_free_full(list, g_object_unref);

            GtkIconTheme * theme = gtk_icon_theme_get_default();
            data->theme_changed_handler = g_signal_connect(theme, "changed", G_CALLBACK(on_icon_theme_changed), data);
        }

        if( data->single_file && fm_file_info_is_symlink(data->fi) )
        {
            gtk_widget_show(data->target_label);
            gtk_widget_show(GTK_WIDGET(data->target));
            gtk_label_set_text(data->target, fm_file_info_get_target(data->fi));
            // gtk_label_set_text(data->type, fm_mime_type_get_desc(data->mime_type));
        }
        else
        {
            gtk_widget_destroy(data->target_label);
            gtk_widget_destroy(GTK_WIDGET(data->target));
            gtk_table_set_row_spacing(data->general_table, 3, 0);
        }
        for(ext = extensions; ext; ext = ext->next)
            if(ext->type == data->mime_type)
            {
                data->ext = ext;
                break;
            }
        if(!data->ext)
            for(ext = extensions; ext; ext = ext->next)
                if(ext->type == NULL) /* fallback handler */
                {
                    data->ext = ext;
                    break;
                }
    }
    else
    {
        gtk_image_set_from_stock(img, GTK_STOCK_DND_MULTIPLE, GTK_ICON_SIZE_DIALOG);
        gtk_window_set_icon_name(GTK_WINDOW(data->dlg), GTK_STOCK_DND_MULTIPLE);

        gtk_widget_set_sensitive(GTK_WIDGET(data->name), FALSE);

        gtk_label_set_text(data->type, _("Files of different types"));

        gtk_widget_destroy(data->target_label);
        gtk_widget_destroy(GTK_WIDGET(data->target));

        gtk_widget_destroy(data->open_with_label);
        gtk_widget_destroy(GTK_WIDGET(data->open_with));
        data->open_with = NULL;
        data->open_with_label = NULL;
    }

    gtk_widget_hide(GTK_WIDGET(data->file_label));
    gtk_widget_hide(GTK_WIDGET(data->file));

    if( data->single_file )
    {
        const char * disp_name = fm_file_info_get_disp_name(data->fi);
        const char * name = fm_file_info_get_name(data->fi);

        if (strcmp(disp_name, name) != 0)
        {
            gtk_widget_show(GTK_WIDGET(data->file_label));
            gtk_widget_show(GTK_WIDGET(data->file));
            gtk_label_set_text(data->file, name);
        }

        gtk_entry_set_text(data->name, disp_name);

        /* FIXME: check if text fits in line */
        if (strlen(disp_name) > 16)
            gtk_widget_set_tooltip_text(GTK_WIDGET(data->name), disp_name);

        /* FIXME: changing file name isn't implemented yet, disable entry */
        gtk_widget_set_can_focus(GTK_WIDGET(data->name), FALSE);
        gtk_editable_set_editable(GTK_EDITABLE(data->name), FALSE);

        gchar * dialog_title = g_strdup_printf(_("%s - Properties"), disp_name);
        gtk_window_set_title(GTK_WINDOW(data->dlg), dialog_title);
        g_free(dialog_title);
    }
    else
    {
        gtk_entry_set_text(data->name, _("Multiple Files"));
        gtk_widget_set_sensitive(GTK_WIDGET(data->name), FALSE);


        size_t files_count = fm_file_info_list_get_length(data->files);
        gchar * dialog_title = g_strdup_printf(
            g_dngettext(GETTEXT_PACKAGE,
                "Properties - %u items",
                "Properties - %u items", files_count),
            files_count);
        gtk_window_set_title(GTK_WINDOW(data->dlg), dialog_title);
        g_free(dialog_title);
    }

    FmPath * common_parent = fm_path_get_parent(fm_file_info_get_path(data->fi));
    time_t min_atime = fm_file_info_get_atime(data->fi), max_atime = min_atime;
    time_t min_mtime = fm_file_info_get_mtime(data->fi), max_mtime = min_mtime;

    GList * l;
    for (l = fm_file_info_list_peek_head_link(data->files)->next; l; l = l->next)
    {
        FmFileInfo* fi = FM_FILE_INFO(l->data);

        if (!fm_path_equal(fm_path_get_parent(fm_file_info_get_path(fi)), common_parent))
            common_parent = NULL;

        time_t atime = fm_file_info_get_atime(fi);
        if (min_atime > atime)
            min_atime = atime;
        if (max_atime < atime)
            max_atime = atime;

        time_t mtime = fm_file_info_get_mtime(fi);
        if (min_mtime > mtime)
            min_mtime = mtime;
        if (max_mtime < mtime)
            max_mtime = mtime;
    }

    gchar * parent_str = common_parent ? fm_path_display_name(common_parent, TRUE) : NULL;
    if (parent_str)
    {
        gtk_label_set_text(data->dir, parent_str);
        /* FIXME: check if text fits in line */
        if (strlen(parent_str) > 16)
            gtk_widget_set_tooltip_text(GTK_WIDGET(data->dir), parent_str);
        g_free(parent_str);
    }
    else
    {
        gtk_label_set_text(data->dir, "");
    }

    set_time_interval(data->atime, min_atime, max_atime);
    set_time_interval(data->mtime, min_mtime, max_mtime);

    update_permissions(data);

    on_timeout(data);
}

static void init_application_list(FmFilePropData* data)
{
    if(data->single_type && data->mime_type)
    {
        /* The code from pcmanfm was: */
#if 0
        if(g_strcmp0(fm_mime_type_get_type(data->mime_type), "inode/directory"))
            fm_app_chooser_combo_box_setup_for_mime_type(data->open_with, data->mime_type);
        else /* shouldn't allow set file association for folders. */
        {
            gtk_widget_destroy(data->open_with_label);
            gtk_widget_destroy(GTK_WIDGET(data->open_with));
            data->open_with = NULL;
            data->open_with_label = NULL;
        }
#endif
        /* That's not the right way to handle the folder case.

           In case "inode/directory" associated to a wrong application,
           various 3rd party applications can be affected (well, even struuman-desktop is affected),
           and there will be absolutely no way to fix the things from GUI.

           So we do allow setting file associations for any mime types, including folders.
        */

        fm_app_chooser_combo_box_setup_for_mime_type(data->open_with, data->mime_type);
    }
}

/**
 * fm_file_properties_widget_new
 * @files: list of files
 * @toplevel: choose appearance of dialog
 *
 * Creates new dialog widget for change properties of @files.
 *
 * Returns: (transfer full): a new widget.
 *
 * Since: 0.1.0
 */
GtkDialog* fm_file_properties_widget_new(FmFileInfoList* files, gboolean toplevel)
{
    GtkBuilder* builder=gtk_builder_new();
    GtkDialog* dlg;
    FmFilePropData* data;
    FmPathList* paths;

    gtk_builder_set_translation_domain(builder, GETTEXT_PACKAGE);
    data = g_slice_new0(FmFilePropData);

    data->files = fm_file_info_list_ref(files);
    data->single_type = fm_file_info_list_is_same_type(files);
    data->single_file = (fm_file_info_list_get_length(files) == 1);
    data->fi = fm_file_info_list_peek_head(files);
    if(data->single_type)
        data->mime_type = fm_mime_type_ref(fm_file_info_get_mime_type(data->fi));
    paths = fm_path_list_new_from_file_info_list(files);
    data->dc_job = fm_deep_count_job_new(paths, FM_DC_JOB_DEFAULT);
    fm_path_list_unref(paths);
    data->ext = NULL; /* no extension by default */

    if(toplevel)
    {
        gtk_builder_add_from_file(builder, UI_FILE, NULL);
        GET_WIDGET(GTK_DIALOG,dlg);
        gtk_dialog_set_alternative_button_order(GTK_DIALOG(data->dlg), GTK_RESPONSE_OK, GTK_RESPONSE_CANCEL, -1);
    }
    else
    {
        /* FIXME: is this really useful? */
        char* names[]={"notebook", NULL};
        gtk_builder_add_objects_from_file(builder, UI_FILE, names, NULL);
        data->dlg = GTK_DIALOG(gtk_builder_get_object(builder, "notebook"));
    }

    dlg = data->dlg;

    GET_WIDGET(GTK_TABLE,general_table);
    GET_WIDGET(GTK_IMAGE,icon);
    GET_WIDGET(GTK_LABEL,file);
    GET_WIDGET(GTK_LABEL,file_label);
    GET_WIDGET(GTK_ENTRY,name);
    GET_WIDGET(GTK_LABEL,dir);
    GET_WIDGET(GTK_LABEL,target);
    GET_WIDGET(GTK_WIDGET,target_label);
    GET_WIDGET(GTK_LABEL,type);
    GET_WIDGET(GTK_WIDGET,open_with_label);
    GET_WIDGET(GTK_COMBO_BOX,open_with);
    GET_WIDGET(GTK_LABEL,total_size);
    GET_WIDGET(GTK_LABEL,size_on_disk);
    GET_WIDGET(GTK_LABEL,mtime);
    GET_WIDGET(GTK_LABEL,atime);

    GET_WIDGET(GTK_WIDGET,permissions_tab);
    GET_WIDGET(GTK_ENTRY,owner);
    GET_WIDGET(GTK_ENTRY,group);

    GET_WIDGET(GTK_COMBO_BOX,read_perm);
    GET_WIDGET(GTK_COMBO_BOX,write_perm);
    GET_WIDGET(GTK_LABEL,exec_label);
    GET_WIDGET(GTK_COMBO_BOX,exec_perm);
    GET_WIDGET(GTK_LABEL,flags_label);
    GET_WIDGET(GTK_COMBO_BOX,flags_set_file);
    GET_WIDGET(GTK_COMBO_BOX,flags_set_dir);

    init_application_list(data);

    data->timeout = g_timeout_add(600, on_timeout, data);
    g_signal_connect(dlg, "response", G_CALLBACK(on_response), data);
    g_signal_connect_swapped(dlg, "destroy", G_CALLBACK(fm_file_prop_data_free), data);
    g_signal_connect(data->dc_job, "finished", G_CALLBACK(on_finished), data);

    fm_job_run_async(FM_JOB(data->dc_job));

    update_ui(data);

    /* if we got some extension then activate it updating dialog window */
    if(data->ext)
        data->extdata = data->ext->cb.init(builder, data, data->files);

    g_object_unref(builder);

    return dlg;
}

/**
 * fm_show_file_properties
 * @parent: a window to put dialog over it
 * @files:list of files
 *
 * Creates and shows file properties dialog for @files.
 *
 * Returns: %TRUE.
 *
 * Since: 0.1.0
 */
gboolean fm_show_file_properties(GtkWindow* parent, FmFileInfoList* files)
{
    GtkDialog* dlg = fm_file_properties_widget_new(files, TRUE);
    if(parent)
        gtk_window_set_transient_for(GTK_WINDOW(dlg), parent);
    gtk_widget_show(GTK_WIDGET(dlg));
    return TRUE;
}

/**
 * fm_file_properties_add_for_mime_type
 * @mime_type: mime type to add handler for
 * @callbacks: table of handler callbacks
 *
 * Adds a handler for some mime type into file properties dialog. The
 * handler will be used if file properties dialog is opened for single
 * file or for few files of the same type to extend its functionality.
 * The value "*" of @mime_type has special meaning - the handler will
 * be used for file types where no other extension is applied. No
 * wildcards are allowed otherwise.
 *
 * Returns: %TRUE if handler was added successfully.
 *
 * Since: 1.2.0
 */
gboolean fm_file_properties_add_for_mime_type(const char *mime_type,
                                              FmFilePropertiesExtensionInit *callbacks)
{
    FmMimeType *type;
    FmFilePropExt *ext;

    /* validate input */
    if(!mime_type || !callbacks || !callbacks->init || !callbacks->finish)
        return FALSE;
    if(strcmp(mime_type, "*") == 0)
        type = NULL;
    else
        type = fm_mime_type_from_name(mime_type);
    for(ext = extensions; ext; ext = ext->next)
        if(ext->type == type)
        {
            g_warning("duplicate file properties handler for \"%s\" ignored",
                      mime_type);
            if(type)
                fm_mime_type_unref(type);
            return FALSE;
        }
    ext = g_slice_new(FmFilePropExt);
    ext->type = type;
    ext->next = extensions;
    ext->cb = *callbacks;
    extensions = ext;
    return TRUE;
}
