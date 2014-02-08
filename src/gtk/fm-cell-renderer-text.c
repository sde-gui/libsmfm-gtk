/*
 *      fm-cell-renderer-text.c
 *
 *      Copyright 2009 PCMan <pcman.tw@gmail.com>
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
 * SECTION:fm-cell-renderer-text
 * @short_description: An implementation of cell text renderer.
 * @title: FmCellRendererText
 *
 * @include: libfm/fm-gtk.h
 *
 * The #FmCellRendererText can be used to render text in cell.
 */

#include "fm-cell-renderer-text.h"
#include <libsmfm-core/fm-config.h>

static void fm_cell_renderer_text_get_property(GObject *object, guint param_id,
                                               GValue *value, GParamSpec *psec);
static void fm_cell_renderer_text_set_property(GObject *object, guint param_id,
                                               const GValue *value, GParamSpec *psec);

enum
{
    PROP_HEIGHT = 1,
    N_PROPS
};

G_DEFINE_TYPE(FmCellRendererText, fm_cell_renderer_text, GTK_TYPE_CELL_RENDERER_TEXT);

static void fm_cell_renderer_text_get_size(GtkCellRenderer            *cell,
                                           GtkWidget                  *widget,
#if GTK_CHECK_VERSION(3, 0, 0)
                                           const
#endif
                                           GdkRectangle               *rectangle,
                                           gint                       *x_offset,
                                           gint                       *y_offset,
                                           gint                       *width,
                                           gint                       *height);

#if GTK_CHECK_VERSION(3, 0, 0)
static void fm_cell_renderer_text_render(GtkCellRenderer *cell,
                                         cairo_t *cr,
                                         GtkWidget *widget,
                                         const GdkRectangle *background_area,
                                         const GdkRectangle *cell_area,
                                         GtkCellRendererState flags);
#else
static void fm_cell_renderer_text_render(GtkCellRenderer *cell, 
                                         GdkDrawable *window,
                                         GtkWidget *widget,
                                         GdkRectangle *background_area,
                                         GdkRectangle *cell_area,
                                         GdkRectangle *expose_area,
                                         GtkCellRendererState  flags);
#endif

#if GTK_CHECK_VERSION(3, 0, 0)
static void fm_cell_renderer_text_get_preferred_width(GtkCellRenderer *cell,
                                                      GtkWidget *widget,
                                                      gint *minimum_size,
                                                      gint *natural_size);

static void fm_cell_renderer_text_get_preferred_height(GtkCellRenderer *cell,
                                                       GtkWidget *widget,
                                                       gint *minimum_size,
                                                       gint *natural_size);

static void fm_cell_renderer_text_get_preferred_height_for_width(GtkCellRenderer *cell,
                                                                 GtkWidget *widget,
                                                                 gint width,
                                                                 gint *minimum_height,
                                                                 gint *natural_height);
#endif

static void fm_cell_renderer_text_class_init(FmCellRendererTextClass *klass)
{
    GObjectClass *g_object_class = G_OBJECT_CLASS(klass);
    GtkCellRendererClass* render_class = GTK_CELL_RENDERER_CLASS(klass);

    g_object_class->get_property = fm_cell_renderer_text_get_property;
    g_object_class->set_property = fm_cell_renderer_text_set_property;

    render_class->render = fm_cell_renderer_text_render;
    render_class->get_size = fm_cell_renderer_text_get_size;
#if GTK_CHECK_VERSION(3, 0, 0)
    render_class->get_preferred_width = fm_cell_renderer_text_get_preferred_width;
    render_class->get_preferred_height = fm_cell_renderer_text_get_preferred_height;
    render_class->get_preferred_height_for_width = fm_cell_renderer_text_get_preferred_height_for_width;
#endif

    /**
     * FmCellRendererText:max-height:
     *
     * The #FmCellRendererText:max-height property is the maximum height
     * of text that will be rendered. If text doesn't fit in that height
     * then text will be ellipsized. The value of -1 means unlimited
     * height.
     *
     * Since: 1.0.2
     */
    g_object_class_install_property(g_object_class,
                                    PROP_HEIGHT,
                                    g_param_spec_int("max-height",
                                                     "Maximum_height",
                                                     "Maximum height",
                                                     -2048, 2048, 0,
                                                     G_PARAM_READWRITE));
}


static void fm_cell_renderer_text_init(FmCellRendererText *self)
{
    self->max_height = 0;
}

/**
 * fm_cell_renderer_text_new
 *
 * Creates new #GtkCellRenderer object for #FmCellRendererText.
 *
 * Returns: a new #GtkCellRenderer object.
 *
 * Since: 0.1.0
 */
GtkCellRenderer *fm_cell_renderer_text_new(void)
{
    return (GtkCellRenderer*)g_object_new(FM_CELL_RENDERER_TEXT_TYPE, NULL);
}

static void fm_cell_renderer_text_get_property(GObject *object, guint param_id,
                                               GValue *value, GParamSpec *psec)
{
    FmCellRendererText *self = FM_CELL_RENDERER_TEXT(object);
    switch(param_id)
    {
    case PROP_HEIGHT:
        g_value_set_int(value, self->max_height);
        break;
    default:
        G_OBJECT_WARN_INVALID_PROPERTY_ID(object, param_id, psec);
        break;
    }
}

static void fm_cell_renderer_text_set_property(GObject *object, guint param_id,
                                               const GValue *value, GParamSpec *psec)
{
    FmCellRendererText *self = FM_CELL_RENDERER_TEXT(object);
    switch(param_id)
    {
    case PROP_HEIGHT:
        self->max_height = g_value_get_int(value);
        break;
    default:
        G_OBJECT_WARN_INVALID_PROPERTY_ID(object, param_id, psec);
        break;
    }
}

static void add_attr(PangoAttrList * attr_list, PangoAttribute * attr)
{
  attr->start_index = 0;
  attr->end_index = G_MAXUINT;
  pango_attr_list_insert(attr_list, attr);
}

static PangoLayout * get_layout(FmCellRendererText * self, GtkWidget * widget, GdkRectangle * available_room)
{
    gchar * text;
    PangoWrapMode wrap_mode;
    gint wrap_width;
    PangoAlignment alignment;

    g_object_get(G_OBJECT(self),
                 "wrap-mode" , &wrap_mode,
                 "wrap-width", &wrap_width,
                 "alignment" , &alignment,
                 "text", &text,
                 NULL);

    PangoLayout * layout = gtk_widget_create_pango_layout(widget, text);

    pango_layout_set_alignment(layout, alignment);

    /* Setup the wrapping. */
    pango_layout_set_wrap(layout, wrap_mode);
    if (wrap_width < 0 && available_room && available_room->width < wrap_width)
        wrap_width = available_room->width;
    pango_layout_set_width(layout, wrap_width * PANGO_SCALE);

    pango_layout_set_ellipsize(layout, PANGO_ELLIPSIZE_END);
    if (self->max_height > 0)
    {
        pango_layout_set_height(layout, self->max_height * PANGO_SCALE);
    }
    else if (self->max_height < 0)
    {
        pango_layout_set_height(layout, self->max_height);
    }

    pango_layout_set_auto_dir(layout, TRUE);

    g_free(text);

    return layout;
}

#if GTK_CHECK_VERSION(3, 0, 0)
static void fm_cell_renderer_text_render(GtkCellRenderer *cell,
                                         cairo_t *cr,
                                         GtkWidget *widget,
                                         const GdkRectangle *background_area,
                                         const GdkRectangle *cell_area,
                                         GtkCellRendererState flags)
#else
static void fm_cell_renderer_text_render(GtkCellRenderer *cell,
                                         GdkDrawable *window,
                                         GtkWidget *widget,
                                         GdkRectangle *background_area,
                                         GdkRectangle *cell_area,
                                         GdkRectangle *expose_area,
                                         GtkCellRendererState flags)
#endif
{
    FmCellRendererText *self = FM_CELL_RENDERER_TEXT(cell);

#if GTK_CHECK_VERSION(3, 0, 0)
    GtkStyleContext* style;
    GtkStateFlags state = GTK_STATE_NORMAL;
    GdkRGBA * foreground_color = NULL;
#else
    GtkStyle* style;
    GtkStateType state = GTK_STATE_NORMAL;
    GdkColor * foreground_color = NULL;
#endif

    gboolean foreground_set = FALSE;
    gint x_offset;
    gint y_offset;
    GdkRectangle rect;
    gint xpad, ypad;

    g_object_get(G_OBJECT(cell),
                 "foreground-set", &foreground_set,
#if GTK_CHECK_VERSION(3, 0, 0)
                 "foreground-rgba", &foreground_color,
#else
                 "foreground-gdk", &foreground_color,
#endif
                 NULL);

    /* Calculate the real x and y offsets. */
/*
    gtk_cell_renderer_get_padding(cell, &xpad, &ypad);
    x_offset = ((gtk_widget_get_direction(widget) == GTK_TEXT_DIR_RTL) ? (1.0 - xalign) : xalign)
             * (cell_area->width - text_width - (2 * xpad));
    x_offset = MAX(x_offset, 0);

    y_offset = yalign * (cell_area->height - text_height - (2 * ypad));
    y_offset = MAX (y_offset, 0);
*/
    x_offset = 0;
    y_offset = 0;

    xpad = 0;
    ypad = 0;


    PangoLayout* layout = get_layout(self, widget, cell_area);

    if (foreground_set && foreground_color && (flags & GTK_CELL_RENDERER_SELECTED) == 0)
    {
        PangoAttrList * attr_list = pango_attr_list_new();
        add_attr(attr_list,
#if GTK_CHECK_VERSION(3, 0, 0)
            pango_attr_foreground_new(
                foreground_color->red * 65535,
                foreground_color->green * 65535,
                foreground_color->blue * 65535));
#else
            pango_attr_foreground_new(
                foreground_color->red,
                foreground_color->green,
                foreground_color->blue));
#endif
        pango_layout_set_attributes(layout, attr_list);
        pango_attr_list_unref(attr_list);
    }

    PangoRectangle layout_rect;
    pango_layout_get_pixel_extents(layout, NULL, &layout_rect);

    {
        rect.x = cell_area->x + x_offset;
        rect.y = cell_area->y + y_offset;
        rect.width = cell_area->width;
        rect.height = cell_area->height;
    }

#if GTK_CHECK_VERSION(3, 0, 0)
    style = gtk_widget_get_style_context(widget);

    if (flags & GTK_CELL_RENDERER_SELECTED)
    {
        if(flags & GTK_CELL_RENDERER_INSENSITIVE)
            state = GTK_STATE_FLAG_INSENSITIVE;
        else
            state = GTK_STATE_FLAG_SELECTED;

        GdkRGBA clr;
        gtk_style_context_get_background_color(style, state, &clr);

        gdk_cairo_rectangle(cr, &rect);
        cairo_set_source_rgb(cr, clr.red / 65535., clr.green / 65535., clr.blue / 65535.);
        cairo_fill (cr);
    }
#else

    style = gtk_widget_get_style(widget);

    if (flags & GTK_CELL_RENDERER_SELECTED)
    {
        if(flags & GTK_CELL_RENDERER_INSENSITIVE)
            state = GTK_STATE_INSENSITIVE;
        else
            state = GTK_STATE_SELECTED;

        if (!fm_config->exo_icon_draw_rectangle_around_selected_item) /* FIXME: Ugly hack! */
        {

            cairo_t *cr = gdk_cairo_create (window);

            if(expose_area)
            {
                gdk_cairo_rectangle(cr, expose_area);
                cairo_clip(cr);
            }

            GdkColor color = style->base[state];

            gdk_cairo_rectangle(cr, &rect);
            cairo_set_source_rgb(cr, color.red / 65535., color.green / 65535., color.blue / 65535.);
            cairo_fill(cr);

            cairo_destroy (cr);
        }
    }

#endif


    int x_align_offset = 0.5 * (cell_area->width  - layout_rect.width);
    int y_align_offset = 0.5 * (cell_area->height - layout_rect.height);

#if GTK_CHECK_VERSION(3, 0, 0)
    gtk_render_layout(style, cr,
                      cell_area->x + x_offset + xpad + x_align_offset - layout_rect.x,
                      cell_area->y + y_offset + ypad + y_align_offset - layout_rect.y, layout);
#else
    gtk_paint_layout(style, window, state, TRUE,
                     expose_area, widget, "cellrenderertext",
                     cell_area->x + x_offset + xpad + x_align_offset - layout_rect.x,
                     cell_area->y + y_offset + ypad + y_align_offset - layout_rect.y, layout);
#endif

    g_object_unref(layout);

    if(G_UNLIKELY( flags & GTK_CELL_RENDERER_FOCUSED) ) /* focused */
    {
#if GTK_CHECK_VERSION(3, 0, 0)
        gtk_render_focus(style, cr, rect.x, rect.y, rect.width, rect.height);
#else
        gtk_paint_focus(style, window, state, background_area,
                        widget, "cellrenderertext", rect.x, rect.y,
                        rect.width, rect.height);
#endif
    }

    gchar * text = NULL;
    if(flags & GTK_CELL_RENDERER_PRELIT) /* hovered */
        g_object_get(G_OBJECT(cell), "text", &text, NULL);
    g_object_set(G_OBJECT(widget), "tooltip-text", text, NULL);

    g_free(text);
}

static void fm_cell_renderer_text_get_size(GtkCellRenderer            *cell,
                                           GtkWidget                  *widget,
#if GTK_CHECK_VERSION(3, 0, 0)
                                           const
#endif
                                           GdkRectangle               *rectangle,
                                           gint                       *x_offset,
                                           gint                       *y_offset,
                                           gint                       *width,
                                           gint                       *height)
{
    FmCellRendererText *self = FM_CELL_RENDERER_TEXT(cell);
    PangoLayout * layout = NULL;

    int xpad = 2, ypad = 2;

    if (rectangle)
    {
        GdkRectangle available_room = *rectangle;
        if (available_room.width > xpad * 2)
            available_room.width -= xpad * 2;
        else
            available_room.width = 0;

        if (available_room.height > ypad * 2)
            available_room.height -= ypad * 2;
        else
            available_room.height = 0;
        layout = get_layout(self, widget, &available_room);
    }
    else
    {
        layout = get_layout(self, widget, NULL);
    }

    PangoRectangle layout_rect;
    pango_layout_get_pixel_extents(layout, NULL, &layout_rect);

    g_object_unref(layout);

    if (x_offset)
        *x_offset = layout_rect.x + xpad;

    if (y_offset)
        *y_offset = layout_rect.y + ypad;

    if (height)
        *height = layout_rect.height + ypad * 2;

    if (width)
        *width = layout_rect.width + xpad * 2;

    if (height)
        *height = layout_rect.height + ypad * 2;

}

#if GTK_CHECK_VERSION(3, 0, 0)

FIXME!
#if 0
static void fm_cell_renderer_text_get_preferred_width(GtkCellRenderer *cell,
                                                      GtkWidget *widget,
                                                      gint *minimum_size,
                                                      gint *natural_size)
{
    gint wrap_width;

    g_object_get(G_OBJECT(cell), "wrap-width", &wrap_width, NULL);
    if(wrap_width > 0)
    {
        if(minimum_size)
            *minimum_size = wrap_width;
        if(natural_size)
            *natural_size = wrap_width;
    }
    else
        GTK_CELL_RENDERER_CLASS(fm_cell_renderer_text_parent_class)->get_preferred_width(cell, widget, minimum_size, natural_size);
}

static void fm_cell_renderer_text_get_preferred_height(GtkCellRenderer *cell,
                                                       GtkWidget *widget,
                                                       gint *minimum_size,
                                                       gint *natural_size)
{
    FmCellRendererText *self = FM_CELL_RENDERER_TEXT(cell);

    GTK_CELL_RENDERER_CLASS(fm_cell_renderer_text_parent_class)->get_preferred_height(cell, widget, minimum_size, natural_size);
    if (self->max_height > 0)
    {
        if(natural_size && *natural_size > self->max_height)
            *natural_size = self->max_height;
        if(minimum_size && *minimum_size > self->max_height)
            *minimum_size = self->max_height;
    }
}

static void fm_cell_renderer_text_get_preferred_height_for_width(GtkCellRenderer *cell,
                                                                 GtkWidget *widget,
                                                                 gint width,
                                                                 gint *minimum_height,
                                                                 gint *natural_height)
{
    fm_cell_renderer_text_get_preferred_height(cell, widget, minimum_height, natural_height);
}

#endif

#endif
