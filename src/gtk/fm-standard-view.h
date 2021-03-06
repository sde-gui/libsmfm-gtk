/*
 *      folder-view.h
 *
 *      Copyright 2009 - 2012 Hong Jen Yee (PCMan) <pcman.tw@gmail.com>
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


#ifndef __STANDARD_VIEW_H__
#define __STANDARD_VIEW_H__

#include <gtk/gtk.h>
#include "fm-folder-view.h"

G_BEGIN_DECLS

#define FM_STANDARD_VIEW_TYPE               (fm_standard_view_get_type())
#define FM_STANDARD_VIEW(obj)               (G_TYPE_CHECK_INSTANCE_CAST((obj),\
            FM_STANDARD_VIEW_TYPE, FmStandardView))
#define FM_STANDARD_VIEW_CLASS(klass)       (G_TYPE_CHECK_CLASS_CAST((klass),\
            FM_STANDARD_VIEW_TYPE, FmStandardViewClass))
#define FM_IS_STANDARD_VIEW(obj)            (G_TYPE_CHECK_INSTANCE_TYPE((obj),\
            FM_STANDARD_VIEW_TYPE))
#define FM_IS_STANDARD_VIEW_CLASS(klass)    (G_TYPE_CHECK_CLASS_TYPE((klass),\
            FM_STANDARD_VIEW_TYPE))


typedef enum _FmViewType {
    FmViewType_Icon,
    FmViewType_Tree
} FmViewType;

typedef struct _ModeSettings
{
    FmViewType type;

    long icon_size;
    const char * icon_size_id;

    long   item_width_min;
    double item_width_mult;

    long   wrap_width_min;
    double wrap_width_mult;

    long   row_spacing_min;
    double row_spacing_mult;

    long   col_spacing_min;
    double col_spacing_mult;

    long   padding_min;
    double padding_mult;

    long cell_spacing;

    PangoWrapMode wrap_mode;
    PangoAlignment text_alignment;
    long text_max_height;

    gboolean horizontal_orientation;

    /* FIXME: */
    /*ExoIconViewLayoutMode icon_layout_mode;*/
    unsigned icon_layout_mode;

    gboolean force_thumbnails;
} FmStandardViewModeSettings;


/**
 * FmStandardViewMode
 * @FM_FV_ICON_VIEW: standard icon view
 * @FM_FV_COMPACT_VIEW: view with small icons and text on right of them
 * @FM_FV_THUMBNAIL_VIEW: view with big icons/thumbnails
 * @FM_FV_LIST_VIEW: table-form view
 */
typedef enum
{
    FM_FV_ICON_VIEW,
    FM_FV_COMPACT_VIEW,
    FM_FV_THUMBNAIL_VIEW,
    FM_FV_LIST_VIEW,
    FM_FV_VIEW_MODE_COUNT
} FmStandardViewMode;

#ifndef FM_DISABLE_DEPRECATED
#define FM_FOLDER_VIEW_MODE_IS_VALID(mode) FM_STANDARD_VIEW_MODE_IS_VALID(mode)
#endif
#define FM_STANDARD_VIEW_MODE_IS_VALID(mode)  ((guint)mode <= FM_FV_LIST_VIEW)

typedef struct _FmStandardView             FmStandardView;
typedef struct _FmStandardViewClass        FmStandardViewClass;

GType       fm_standard_view_get_type(void);
FmStandardView* fm_standard_view_new(FmStandardViewMode mode,
                                     FmFolderViewUpdatePopup update_popup,
                                     FmLaunchFolderFunc open_folders);

void fm_standard_view_set_mode(FmStandardView* fv, FmStandardViewMode mode);
FmStandardViewMode fm_standard_view_get_mode(FmStandardView* fv);

const char* fm_standard_view_mode_to_str(FmStandardViewMode mode);
FmStandardViewMode fm_standard_view_mode_from_str(const char* str);

FmStandardViewModeSettings fm_standard_view_get_mode_settings(FmStandardView* fv);

G_END_DECLS

#endif /* __STANDARD_VIEW_H__ */
