/*
 * Copyright (c) 2007 The Xfce Development Team
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
 * You should have received a copy of the GNU Library General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston,
 * MA 02110-1301 USA
 */

#ifndef __LIBXFCE4UI_PRIVATE_H__
#define __LIBXFCE4UI_PRIVATE_H__

#include <gtk/gtk.h>

G_BEGIN_DECLS

G_GNUC_INTERNAL GdkPixbuf *
xfce_gdk_pixbuf_colorize (const GdkPixbuf *source,
                          const GdkColor *color) G_GNUC_MALLOC G_GNUC_WARN_UNUSED_RESULT;

G_GNUC_INTERNAL GdkPixbuf *
xfce_gdk_pixbuf_spotlight (const GdkPixbuf *source) G_GNUC_MALLOC G_GNUC_WARN_UNUSED_RESULT;

G_GNUC_INTERNAL void
xfce_gtk_position_search_box (GtkWidget *view,
                              GtkWidget *search_dialog,
                              gpointer user_data);

G_END_DECLS

#endif /* !__LIBXFCE4UI_PRIVATE_H__ */
