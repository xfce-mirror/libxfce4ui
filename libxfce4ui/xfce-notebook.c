/*-
 * Copyright (c) 2004-2006 Benedikt Meurer <benny@xfce.org>
 * Copyright (c) 2025 The XFCE Development Team
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

#include <libxfce4util/libxfce4util.h>

#include "libxfce4ui-private.h"
#include "xfce-gtk-extensions.h"
#include "xfce-notebook.h"
#include "libxfce4ui-visibility.h"

/**
 * SECTION: xfce-notebook
 * @title: XfceNotebook
 * @short_description: An improved version of #GtkNotebook
 * @include: libxfce4ui/libxfce4ui.h
 *
 * The #XfceNotebook class derives from #GtkNotebook and extends it with
 * the ability to scroll through tabs.
 **/



static void
xfce_notebook_finalize (GObject *object);

static gboolean
xfce_notebook_scroll_event (GtkWidget *widget,
                            GdkEventScroll *event);



/**
 * XfceNotebook:
 *
 * The #XfceNotebook struct contains only private fields and should
 * not be directly accessed.
 **/
struct _XfceNotebook
{
  GtkNotebook __parent__;
};



G_DEFINE_TYPE (XfceNotebook, xfce_notebook, GTK_TYPE_NOTEBOOK)

static void
xfce_notebook_class_init (XfceNotebookClass *klass)
{
  GtkWidgetClass *gtk_widget_class;
  GObjectClass *gobject_class;

  gobject_class = G_OBJECT_CLASS (klass);
  gobject_class->finalize = xfce_notebook_finalize;

  gtk_widget_class = GTK_WIDGET_CLASS (klass);
  gtk_widget_class->scroll_event = xfce_notebook_scroll_event;

  /* make sure to use the translations from libxfce4ui */
  bindtextdomain (GETTEXT_PACKAGE, PACKAGE_LOCALE_DIR);
#ifdef HAVE_BIND_TEXTDOMAIN_CODESET
  bind_textdomain_codeset (GETTEXT_PACKAGE, "UTF-8");
#endif
}

static void
xfce_notebook_init (XfceNotebook *notebook)
{
  gtk_widget_add_events (GTK_WIDGET (notebook), GDK_SCROLL_MASK);
}

static void
xfce_notebook_finalize (GObject *object)
{
  G_OBJECT_CLASS (xfce_notebook_parent_class)->finalize (object);
}

static gboolean
xfce_notebook_scroll_event (GtkWidget *notebook,
                            GdkEventScroll *event)
{
  g_return_val_if_fail (GTK_IS_NOTEBOOK (notebook), FALSE);

  if ((event->state & gtk_accelerator_get_default_mod_mask ()) != 0)
    return FALSE;

  switch (event->direction)
    {
    case GDK_SCROLL_RIGHT:
    case GDK_SCROLL_DOWN:
      gtk_notebook_next_page (GTK_NOTEBOOK (notebook));
      return TRUE;

    case GDK_SCROLL_LEFT:
    case GDK_SCROLL_UP:
      gtk_notebook_prev_page (GTK_NOTEBOOK (notebook));
      return TRUE;

    default: /* GDK_SCROLL_SMOOTH */
      switch (gtk_notebook_get_tab_pos (GTK_NOTEBOOK (notebook)))
        {
        case GTK_POS_LEFT:
        case GTK_POS_RIGHT:
          if (event->delta_y > 0)
            gtk_notebook_next_page (GTK_NOTEBOOK (notebook));
          else if (event->delta_y < 0)
            gtk_notebook_prev_page (GTK_NOTEBOOK (notebook));
          break;

        default: /* GTK_POS_TOP or GTK_POS_BOTTOM */
          if (event->delta_x > 0)
            gtk_notebook_next_page (GTK_NOTEBOOK (notebook));
          else if (event->delta_x < 0)
            gtk_notebook_prev_page (GTK_NOTEBOOK (notebook));
          break;
        }
      return TRUE;
    }
  /* don't chain-up to parent here: it is not necessary and the parent class method
   * is defined only from GTK 3.24.13 */
  return TRUE;
}



/**
 * xfce_notebook_new:
 *
 * Allocates a new #XfceNotebook instance.
 *
 * Returns: (transfer full): the newly allocated #XfceNotebook.
 *
 * Since: 4.21.1
 **/
GtkWidget *
xfce_notebook_new (void)
{
  return g_object_new (XFCE_TYPE_NOTEBOOK, NULL);
}



#define __XFCE_NOTEBOOK_C__
#include "libxfce4ui-visibility.c"
