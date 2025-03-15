/*-
 * Copyright (c) 2005-2006 Benedikt Meurer <benny@xfce.org>.
 * Copyright (c) 2009 Jannis Pohlmann <jannis@xfce.org>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston,
 * MA 02110-1301 USA
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <gio/gio.h>
#include <libxfce4util/libxfce4util.h>

#include "libxfce4ui-private.h"
#include "xfce-cell-renderer-icon.h"
#include "xfce-gdk-pixbuf-extensions.h"
#include "xfce-thumbnail.h"
#include "libxfce4ui-visibility.h"

/**
 * SECTION: xfce-cell-renderer-icon
 * @title: XfceCellRendererIcon
 * @short_description: Renders an icon in a cell
 * @include: libxfce4ui/libxfce4ui.h
 * @see_also: #XfceIconView
 *
 * An #XfceCellRendererIcon can be used to render an icon in a cell. It
 * allows to render either a named icon, which is looked up using the
 * #GtkIconTheme, or an image file loaded from the file system. The icon
 * name or absolute path to the image file is set via the
 * XfceCellRendererIcon:icon property.
 *
 * To support the #XfceIconView (and #GtkIconView) #XfceCellRendererIcon supports
 * rendering icons based on the state of the view if the
 * XfceCellRendererIcon:follow-state property is set.
 *
 * Since: 4.21.0
 **/

/* Property identifiers */
enum
{
  PROP_0,
  PROP_FOLLOW_STATE,
  PROP_ICON,
  PROP_GICON,
  PROP_SIZE,
};



static void
xfce_cell_renderer_icon_finalize (GObject *object);
static void
xfce_cell_renderer_icon_get_property (GObject *object,
                                      guint prop_id,
                                      GValue *value,
                                      GParamSpec *pspec);
static void
xfce_cell_renderer_icon_set_property (GObject *object,
                                      guint prop_id,
                                      const GValue *value,
                                      GParamSpec *pspec);
static void
xfce_cell_renderer_icon_get_size (GtkCellRenderer *renderer,
                                  GtkWidget *widget,
                                  const GdkRectangle *cell_area,
                                  gint *x_offset,
                                  gint *y_offset,
                                  gint *width,
                                  gint *height);
static void
xfce_cell_renderer_icon_render (GtkCellRenderer *renderer,
                                cairo_t *cr,
                                GtkWidget *widget,
                                const GdkRectangle *background_area,
                                const GdkRectangle *cell_area,
                                GtkCellRendererState flags);



/**
 * XfceCellRendererIcon:
 *
 * The #XfceCellRendererIcon struct contains only private fields and
 * should not be directly accessed.
 *
 * Since: 4.21.0
 **/
struct _XfceCellRendererIcon
{
  GtkCellRenderer __parent__;

  guint follow_state : 1;
  guint icon_static : 1;
  gchar *icon;
  GIcon *gicon;
  gint size;
};



G_DEFINE_TYPE (XfceCellRendererIcon, xfce_cell_renderer_icon, GTK_TYPE_CELL_RENDERER)



static void
xfce_cell_renderer_icon_class_init (XfceCellRendererIconClass *klass)
{
  GtkCellRendererClass *gtkcell_renderer_class;
  GObjectClass *gobject_class;

  gobject_class = G_OBJECT_CLASS (klass);
  gobject_class->finalize = xfce_cell_renderer_icon_finalize;
  gobject_class->get_property = xfce_cell_renderer_icon_get_property;
  gobject_class->set_property = xfce_cell_renderer_icon_set_property;

  gtkcell_renderer_class = GTK_CELL_RENDERER_CLASS (klass);
  gtkcell_renderer_class->get_size = xfce_cell_renderer_icon_get_size;
  gtkcell_renderer_class->render = xfce_cell_renderer_icon_render;

  /* make sure to use the translations from libxfce4ui */
  bindtextdomain (GETTEXT_PACKAGE, PACKAGE_LOCALE_DIR);
#ifdef HAVE_BIND_TEXTDOMAIN_CODESET
  bind_textdomain_codeset (GETTEXT_PACKAGE, "UTF-8");
#endif

  /**
   * XfceCellRendererIcon:follow-state:
   *
   * Specifies whether the icon renderer should render icon based on the
   * selection state of the items. This is necessary for #XfceIconView,
   * which doesn't draw any item state indicators itself.
   *
   * Since: 4.21.0
   **/
  g_object_class_install_property (gobject_class,
                                   PROP_FOLLOW_STATE,
                                   g_param_spec_boolean ("follow-state",
                                                         "Follow state",
                                                         "Render differently based on the selection state.",
                                                         TRUE,
                                                         G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS | G_PARAM_CONSTRUCT));

  /**
   * XfceCellRendererIcon:icon:
   *
   * The name of the themed icon to render or an absolute path to an image file
   * to render. May also be %NULL in which case no icon will be rendered for the
   * cell.
   *
   * Image files are loaded via the thumbnail database, creating a thumbnail
   * as necessary. The thumbnail database is also used to load scalable icons
   * in the icon theme, because loading scalable icons is quite expensive
   * these days.
   *
   * Since: 4.21.0
   **/
  g_object_class_install_property (gobject_class,
                                   PROP_ICON,
                                   g_param_spec_string ("icon",
                                                        "Icon",
                                                        "The icon to render.",
                                                        NULL,
                                                        G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));

  /**
   * XfceCellRendererIcon:gicon:
   *
   * The #GIcon to render. May also be %NULL in which case no icon will be
   * rendered for the cell.
   *
   * Currently only #GThemedIcon<!---->s are supported which are loaded
   * using the current icon theme.
   *
   * Since: 4.21.0
   **/
  g_object_class_install_property (gobject_class,
                                   PROP_GICON,
                                   g_param_spec_object ("gicon",
                                                        "GIcon",
                                                        "The GIcon to render.",
                                                        G_TYPE_ICON,
                                                        G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));

  /**
   * XfceCellRendererIcon:size:
   *
   * The size in pixel at which to render the icon. This is also the fixed
   * size that the renderer will request no matter if the actual icons are
   * smaller than this size.
   *
   * This improves the performance of the layouting in the icon and tree
   * view, because during the layouting phase no icons will need to be
   * loaded, but the icons will only be loaded when they need to be rendered,
   * i.e. the view scrolls to the cell.
   *
   * Since: 4.21.0
   **/
  g_object_class_install_property (gobject_class,
                                   PROP_SIZE,
                                   g_param_spec_int ("size",
                                                     "size",
                                                     "The size of the icon to render in pixels.",
                                                     1, G_MAXINT, 48,
                                                     G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS | G_PARAM_CONSTRUCT));
}



static void
xfce_cell_renderer_icon_init (XfceCellRendererIcon *icon_undocked)
{
}



static void
xfce_cell_renderer_icon_finalize (GObject *object)
{
  XfceCellRendererIcon *renderer_icon = XFCE_CELL_RENDERER_ICON (object);

  /* free the icon if not static */
  if (!renderer_icon->icon_static)
    g_free (renderer_icon->icon);

  /* free the GICon */
  if (renderer_icon->gicon != NULL)
    g_object_unref (renderer_icon->gicon);

  G_OBJECT_CLASS (xfce_cell_renderer_icon_parent_class)->finalize (object);
}



static void
xfce_cell_renderer_icon_get_property (GObject *object,
                                      guint prop_id,
                                      GValue *value,
                                      GParamSpec *pspec)
{
  XfceCellRendererIcon *renderer_icon = XFCE_CELL_RENDERER_ICON (object);

  switch (prop_id)
    {
    case PROP_FOLLOW_STATE:
      g_value_set_boolean (value, renderer_icon->follow_state);
      break;

    case PROP_ICON:
      g_value_set_string (value, renderer_icon->icon);
      break;

    case PROP_GICON:
      g_value_set_object (value, renderer_icon->gicon);
      break;

    case PROP_SIZE:
      g_value_set_int (value, renderer_icon->size);
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
    }
}



static void
xfce_cell_renderer_icon_set_property (GObject *object,
                                      guint prop_id,
                                      const GValue *value,
                                      GParamSpec *pspec)
{
  XfceCellRendererIcon *renderer_icon = XFCE_CELL_RENDERER_ICON (object);
  const gchar *icon;

  switch (prop_id)
    {
    case PROP_FOLLOW_STATE:
      renderer_icon->follow_state = g_value_get_boolean (value);
      break;

    case PROP_ICON:
      /* release the previous icon (if not static) */
      if (!renderer_icon->icon_static)
        g_free (renderer_icon->icon);
      icon = g_value_get_string (value);
      renderer_icon->icon_static = (value->data[1].v_uint & G_VALUE_NOCOPY_CONTENTS);
      renderer_icon->icon = (gchar *) ((icon == NULL) ? "" : icon);
      if (!renderer_icon->icon_static)
        renderer_icon->icon = g_strdup (renderer_icon->icon);
      break;

    case PROP_GICON:
      if (renderer_icon->gicon != NULL)
        g_object_unref (renderer_icon->gicon);
      renderer_icon->gicon = g_value_dup_object (value);
      break;

    case PROP_SIZE:
      renderer_icon->size = g_value_get_int (value);
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
    }
}



static void
xfce_cell_renderer_icon_get_size (GtkCellRenderer *renderer,
                                  GtkWidget *widget,
                                  const GdkRectangle *cell_area,
                                  gint *x_offset,
                                  gint *y_offset,
                                  gint *width,
                                  gint *height)
{
  XfceCellRendererIcon *renderer_icon = XFCE_CELL_RENDERER_ICON (renderer);
  gfloat xalign, yalign;
  gint xpad, ypad;

  gtk_cell_renderer_get_alignment (renderer, &xalign, &yalign);
  gtk_cell_renderer_get_padding (renderer, &xpad, &ypad);

  if (cell_area != NULL)
    {
      if (x_offset != NULL)
        {
          *x_offset = ((gtk_widget_get_direction (widget) == GTK_TEXT_DIR_RTL) ? 1.0 - xalign : xalign)
                      * (cell_area->width - renderer_icon->size);
          *x_offset = MAX (*x_offset, 0) + xpad;
        }

      if (y_offset != NULL)
        {
          *y_offset = yalign * (cell_area->height - renderer_icon->size);
          *y_offset = MAX (*y_offset, 0) + ypad;
        }
    }
  else
    {
      if (x_offset != NULL)
        *x_offset = 0;

      if (y_offset != NULL)
        *y_offset = 0;
    }

  if (G_LIKELY (width != NULL))
    *width = (gint) xpad * 2 + renderer_icon->size;

  if (G_LIKELY (height != NULL))
    *height = (gint) ypad * 2 + renderer_icon->size;
}


static void
xfce_cell_renderer_icon_render (GtkCellRenderer *renderer,
                                cairo_t *cr,
                                GtkWidget *widget,
                                const GdkRectangle *background_area,
                                const GdkRectangle *cell_area,
                                GtkCellRendererState flags)
{
  GdkRectangle clip_area;
  GdkRectangle *expose_area = &clip_area;
  GdkRGBA *color_rgba;
  GdkColor color_gdk;
  GtkStyleContext *style_context;
  XfceCellRendererIcon *renderer_icon = XFCE_CELL_RENDERER_ICON (renderer);
  GtkIconTheme *icon_theme;
  GdkRectangle icon_area;
  GdkRectangle draw_area;
  const gchar *filename;
  GtkIconInfo *icon_info = NULL;
  GdkPixbuf *icon = NULL;
  GdkPixbuf *temp;
  cairo_surface_t *surface;
  GError *err = NULL;
  gchar *display_name = NULL;
  gint scaled_icon_size;
  gint scale_factor;

  gdk_cairo_get_clip_rectangle (cr, expose_area);

  /* verify that we have an icon */
  if (G_UNLIKELY (renderer_icon->icon == NULL && renderer_icon->gicon == NULL))
    return;

  scale_factor = gtk_widget_get_scale_factor (widget);
  scaled_icon_size = renderer_icon->size * scale_factor;

  /* icon may be either an image file or a named icon */
  if (renderer_icon->icon != NULL && g_path_is_absolute (renderer_icon->icon))
    {
      /* load the icon via the thumbnail database */
      icon = xfce_thumbnail_get_for_file (
        renderer_icon->icon,
        (scaled_icon_size > 128) ? XFCE_THUMBNAIL_SIZE_LARGE : XFCE_THUMBNAIL_SIZE_NORMAL,
        &err);
    }
  else if (renderer_icon->icon != NULL || renderer_icon->gicon != NULL)
    {
      /* determine the best icon size (GtkIconTheme is somewhat messy scaling up small icons) */
      icon_theme = gtk_icon_theme_get_for_screen (gtk_widget_get_screen (widget));

      if (renderer_icon->icon != NULL)
        {
          /* lookup the icon in the icon theme */
          icon_info = gtk_icon_theme_lookup_icon_for_scale (icon_theme,
                                                            renderer_icon->icon,
                                                            renderer_icon->size,
                                                            scale_factor,
                                                            GTK_ICON_LOOKUP_FORCE_SIZE);
        }
      else if (renderer_icon->gicon != NULL)
        {
          icon_info = gtk_icon_theme_lookup_by_gicon_for_scale (icon_theme,
                                                                renderer_icon->gicon,
                                                                renderer_icon->size,
                                                                scale_factor,
                                                                GTK_ICON_LOOKUP_USE_BUILTIN | GTK_ICON_LOOKUP_FORCE_SIZE);
        }

      if (G_UNLIKELY (icon_info == NULL))
        return;

      /* check if we have an SVG icon here */
      filename = gtk_icon_info_get_filename (icon_info);
      if (filename != NULL && g_str_has_suffix (filename, ".svg"))
        {
          /* loading SVG icons is terribly slow, so we try to use thumbnail instead, and we use the
           * real available cell area directly here, because loading thumbnails involves scaling anyway
           * and this way we need to the thumbnail pixbuf scale only once.
           */
          icon = xfce_thumbnail_get_for_file (
            filename,
            (scaled_icon_size > 128) ? XFCE_THUMBNAIL_SIZE_LARGE : XFCE_THUMBNAIL_SIZE_NORMAL,
            &err);
        }
      else
        {
          /* regularly load the icon from the theme */
          icon = gtk_icon_info_load_icon (icon_info, &err);
        }
      g_object_unref (icon_info);
    }

  /* check if we failed */
  if (G_UNLIKELY (icon == NULL))
    {
      /* better let the user know whats going on, might be surprising otherwise */
      if (G_LIKELY (renderer_icon->icon != NULL))
        {
          display_name = g_filename_display_name (renderer_icon->icon);
        }
      else if (G_UNLIKELY (renderer_icon->gicon != NULL
                           && g_object_class_find_property (G_OBJECT_GET_CLASS (renderer_icon->gicon), "name")))
        {
          g_object_get (renderer_icon->gicon, "name", &display_name, NULL);
        }

      if (display_name != NULL)
        {
          g_warning ("Failed to load \"%s\": %s", display_name, err->message);
          g_free (display_name);
        }

      g_error_free (err);
      return;
    }

  /* determine the real icon size */
  icon_area.width = gdk_pixbuf_get_width (icon) / scale_factor;
  icon_area.height = gdk_pixbuf_get_height (icon) / scale_factor;

  /* scale down the icon on-demand */
  if (G_UNLIKELY (icon_area.width > cell_area->width || icon_area.height > cell_area->height))
    {
      /* scale down to fit */
      temp = xfce_gdk_pixbuf_scale_down (icon, TRUE,
                                         cell_area->width * scale_factor,
                                         cell_area->height * scale_factor);
      g_object_unref (G_OBJECT (icon));
      icon = temp;

      /* determine the icon dimensions again */
      icon_area.width = gdk_pixbuf_get_width (icon) / scale_factor;
      icon_area.height = gdk_pixbuf_get_height (icon) / scale_factor;
    }

  icon_area.x = cell_area->x + (cell_area->width - icon_area.width) / 2;
  icon_area.y = cell_area->y + (cell_area->height - icon_area.height) / 2;

  /* Gtk3: we don't have any expose rectangle and just draw everything */
  if (gdk_rectangle_intersect (expose_area, &icon_area, &draw_area))
    {
      /* colorize the icon if we should follow the selection state */
      if ((flags & (GTK_CELL_RENDERER_SELECTED | GTK_CELL_RENDERER_PRELIT)) != 0 && renderer_icon->follow_state)
        {
          if ((flags & GTK_CELL_RENDERER_SELECTED) != 0)
            {
              style_context = gtk_widget_get_style_context (widget);
              gtk_style_context_get (style_context,
                                     gtk_widget_has_focus (widget) ? GTK_STATE_FLAG_SELECTED : GTK_STATE_FLAG_ACTIVE,
                                     GTK_STYLE_PROPERTY_BACKGROUND_COLOR, &color_rgba,
                                     NULL);

              color_gdk.pixel = 0;
              color_gdk.red = color_rgba->red * 65535;
              color_gdk.blue = color_rgba->blue * 65535;
              color_gdk.green = color_rgba->green * 65535;
              gdk_rgba_free (color_rgba);
              temp = xfce_gdk_pixbuf_colorize (icon, &color_gdk);
              g_object_unref (G_OBJECT (icon));
              icon = temp;
            }

          if ((flags & GTK_CELL_RENDERER_PRELIT) != 0)
            {
              temp = xfce_gdk_pixbuf_spotlight (icon);
              g_object_unref (G_OBJECT (icon));
              icon = temp;
            }
        }

      /* check if we should render an insensitive icon */
      if (G_UNLIKELY (gtk_widget_get_state_flags (widget) & GTK_STATE_INSENSITIVE
                      || !gtk_cell_renderer_get_sensitive (renderer)))
        {
          style_context = gtk_widget_get_style_context (widget);
          gtk_style_context_get (style_context, GTK_STATE_FLAG_INSENSITIVE,
                                 GTK_STYLE_PROPERTY_COLOR, &color_rgba,
                                 NULL);

          color_gdk.pixel = 0;
          color_gdk.red = color_rgba->red * 65535;
          color_gdk.blue = color_rgba->blue * 65535;
          color_gdk.green = color_rgba->green * 65535;
          gdk_rgba_free (color_rgba);
          temp = xfce_gdk_pixbuf_colorize (icon, &color_gdk);

          g_object_unref (G_OBJECT (icon));
          icon = temp;
        }

      /* render the invalid parts of the icon */
      surface = gdk_cairo_surface_create_from_pixbuf (icon, scale_factor, gtk_widget_get_window (widget));
      cairo_set_source_surface (cr, surface, icon_area.x, icon_area.y);
      cairo_rectangle (cr, draw_area.x, draw_area.y, draw_area.width, draw_area.height);
      cairo_fill (cr);

      cairo_surface_destroy (surface);
    }

  /* release the file's icon */
  g_object_unref (G_OBJECT (icon));
}



/**
 * xfce_cell_renderer_icon_new:
 *
 * Creates a new #XfceCellRendererIcon. Adjust rendering parameters using object properties,
 * which can be set globally via g_object_set(). Also, with #GtkCellLayout and
 * #GtkTreeViewColumn, you can bind a property to a value in a #GtkTreeModel. For example
 * you can bind the XfceCellRendererIcon:icon property on the
 * cell renderer to an icon name in the model, thus rendering a different icon in each row
 * of the #GtkTreeView.
 *
 * Returns: (transfer full): the newly allocated #XfceCellRendererIcon.
 *
 * Since: 4.21.0
 **/
GtkCellRenderer *
xfce_cell_renderer_icon_new (void)
{
  return g_object_new (XFCE_TYPE_CELL_RENDERER_ICON, NULL);
}

#define __XFCE_CELL_RENDERER_ICON_C__
#include "libxfce4ui-visibility.c"
