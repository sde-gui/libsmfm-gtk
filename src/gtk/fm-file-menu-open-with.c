/******************************************************************************/

/* <Open With> Stuff */

/*
	The file is to be included by fm-file-menu.c
	A more elaborated solution is to be done.
*/

/******************************************************************************/

static gboolean check_app_in_list(GAppInfo* app, GList* apps2)
{
    GList *apps2_iter;
    for(apps2_iter = apps2; apps2_iter; apps2_iter = apps2_iter->next)
        if(g_app_info_equal(app, apps2_iter->data))
            break;
    return apps2_iter != NULL;
}

static GList* intersect_apps(GList* apps, GList* apps2)
{
    for(GList *apps_iter = apps; apps_iter; )
    {
        if(check_app_in_list(apps_iter->data, apps2))
        {
            /* app is in the both lists, so just jump to the next one */
            apps_iter = apps_iter->next;
        }
        else
        {
            /* Remove apps_iter from apps. */
            g_object_unref(apps_iter->data);
            GList *next = apps_iter->next;
            apps = g_list_delete_link(apps, apps_iter);
            apps_iter = next;
        }
    }
    return apps;
}

static GList* get_apps_for_mime_types(GList* mime_types)
{
    GList* apps = NULL;
    if (mime_types)
    {
        apps = g_app_info_get_all_for_type(fm_mime_type_get_type(mime_types->data));
        for(GList* mime_types_iter = mime_types->next; mime_types_iter; mime_types_iter = mime_types_iter->next)
        {
            if (!apps) /* fast way out, since intersecting with an empty set is an empty set */
                break;

            GList *apps2 = g_app_info_get_all_for_type(fm_mime_type_get_type(mime_types_iter->data));

            apps = intersect_apps(apps, apps2);

            g_list_foreach(apps2, (GFunc)g_object_unref, NULL);
            g_list_free(apps2);
        }
    }

    return apps;
}

static char * app_group_names__html[] = {
    "Network",
    "TextEditor",
    "Office",
    NULL
};

static char * app_group_names__text[] = {
    "TextEditor",
    "Office",
    NULL
};

static char * app_group_names__graphics[] = {
    "Graphics",
    "Network",
    NULL
};

static char * app_group_names__audio_video[] = {
    "AudioVideo",
    "Network",
    NULL
};

static char * app_group_names__directory[] = {
    "AudioVideo",
    "Graphics",
    NULL
};

static char * app_group_names__common[] = {
    "TextEditor",
    "Office",
    "Network",
    NULL
};

#define MIME_TYPE_CLASS_HTML        (1 << 0)
#define MIME_TYPE_CLASS_TEXT        (1 << 1)
#define MIME_TYPE_CLASS_GRAPHICS    (1 << 2)
#define MIME_TYPE_CLASS_AUDIO_VIDEO (1 << 3)
#define MIME_TYPE_CLASS_DIRECTORY   (1 << 4)

static guint get_class_for_mime_type(FmMimeType* mime_type)
{
    if (!mime_type)
        return 0;

    const char * type = fm_mime_type_get_type(mime_type);
    if (!type)
        return 0;

    guint cls = 0;

    if (strstr(type, "html") || strstr(type, "xml"))
        cls |= MIME_TYPE_CLASS_HTML;

    if (strcmp(type, "inode/directory") == 0)
        cls |= MIME_TYPE_CLASS_DIRECTORY;
    if (g_str_has_prefix(type, "text/") || strstr(type, "shellscript"))
        cls |= MIME_TYPE_CLASS_TEXT;
    else if(g_str_has_prefix(type, "image/"))
        cls |= MIME_TYPE_CLASS_GRAPHICS;
    else if(g_str_has_prefix(type, "audio/") || g_str_has_prefix(type, "video/"))
        cls |= MIME_TYPE_CLASS_AUDIO_VIDEO;

    return cls;
}

static char ** guess_app_group_names_from_mime_types(GList* mime_types)
{
    guint cls = (guint)(-1);
    for(GList* mime_types_iter = mime_types; mime_types_iter; mime_types_iter = mime_types_iter->next)
    {
        cls &= get_class_for_mime_type(mime_types_iter->data);
        if (cls == 0)
            goto return_common;
    }

    if (cls & MIME_TYPE_CLASS_HTML)
        return app_group_names__html;
    if (cls & MIME_TYPE_CLASS_TEXT)
        return app_group_names__text;
    if (cls & MIME_TYPE_CLASS_GRAPHICS)
        return app_group_names__graphics;
    if (cls & MIME_TYPE_CLASS_AUDIO_VIDEO)
        return app_group_names__audio_video;
    if (cls & MIME_TYPE_CLASS_DIRECTORY)
        return app_group_names__directory;

return_common:
    return app_group_names__common;
}

static guint get_group_index_for_app(char** app_group_names, GAppInfo* app)
{
    const char * categories = fm_app_utils_get_app_categories(app);

    guint group_index = 0;
    for (; app_group_names[group_index]; group_index++)
    {
        if (categories && strstr(categories, app_group_names[group_index]))
            break;
    }

    return group_index;
}

static GList** group_apps_by_categories(char** app_group_names, GList* apps)
{
    guint nr_groups = g_strv_length(app_group_names);
    GList** app_groups = g_new0(GList*, nr_groups + 2); /* +1 for "Misc" and +1 for the terminating NULL */

    for(GList *apps_iter = apps; apps_iter; apps_iter = apps_iter->next)
    {
        guint index = get_group_index_for_app(app_group_names, apps_iter->data);
        app_groups[index] = g_list_append(app_groups[index], apps_iter->data);
    }

    /* NULL is used to indicate end of vector,
       so add dummy list nodes, eliminating NULL from unused positions. */
    for (guint index = 0; index <= nr_groups; index++)
    {
        if (app_groups[index] == NULL)
            app_groups[index] = g_list_append(app_groups[index], NULL);
    }

    return app_groups;
}

static gboolean append_menu_item_for_app(FmFileMenu* data, GString* xml, GAppInfo* app)
{
    if (!app)
        return FALSE;

    /*g_debug("app %s, executable %s, command %s\n",
        g_app_info_get_name(app),
        g_app_info_get_executable(app),
        g_app_info_get_commandline(app));*/

    gchar * program_path = g_find_program_in_path(g_app_info_get_executable(app));
    if (!program_path)
        return FALSE;
    g_free(program_path);

    /* Create GtkAction for the menu item */
    GtkAction* act = gtk_action_new(
        g_app_info_get_id(app),
        g_app_info_get_name(app),
        g_app_info_get_description(app),
        NULL);
    g_signal_connect(act, "activate", G_CALLBACK(on_open_with_app), data);
    gtk_action_set_gicon(act, g_app_info_get_icon(app));
    gtk_action_group_add_action(data->action_group, act);
    /* associate the app info object with the action */
    g_object_set_qdata_full(G_OBJECT(act), fm_qdata_id, app, g_object_unref);

    /* Append to the menu */
    g_string_append_printf(xml, "<menuitem action='%s'/>", g_app_info_get_id(app));

    return TRUE;
}

static void append_menu_items_for_apps_grouped(FmFileMenu* data, GString* xml, GList* apps, GList* mime_types)
{
    char ** app_group_names = guess_app_group_names_from_mime_types(mime_types);
    GList** app_groups = group_apps_by_categories(app_group_names, apps);
    if (!app_groups)
        return; /* something went wrong */

    gboolean some_items_appended = FALSE;

    for (guint group_index = 0; app_groups[group_index]; group_index++)
    {
        /* Append separator, if needed */
        if (some_items_appended)
        {
            g_string_append(xml, "<separator/>");
            some_items_appended = FALSE;
        }

        /* Append items from the given group */
        for(GList* l = app_groups[group_index]; l; l = l->next)
        {
            GAppInfo* app = l->data;
            if (append_menu_item_for_app(data, xml, app))
                some_items_appended = TRUE;
        }

        /* Free the list; don't unref GAppInfos,
           since group_apps_by_categories() doesn't increment ref counters. */
        g_list_free(app_groups[group_index]);
    }

    g_free(app_groups);
}

static void append_menu_items_for_apps_plain(FmFileMenu* data, GString* xml, GList* apps, GList* mime_types)
{
    for(GList* l =  apps; l; l = l->next)
    {
        GAppInfo* app = l->data;
        append_menu_item_for_app(data, xml, app);
    }
}

static void append_menu_items_for_apps(FmFileMenu* data, GString* xml, GList* apps, GList* mime_types)
{
    if (fm_config->app_list_smart_grouping)
        append_menu_items_for_apps_grouped(data, xml, apps, mime_types);
    else
        append_menu_items_for_apps_plain(data, xml, apps, mime_types);
}

