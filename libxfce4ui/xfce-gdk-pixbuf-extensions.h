/*-
 * Copyright (c) 2004-2006 os-cillation e.K.
 *
 * Written by Benedikt Meurer <benny@xfce.org>.
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

#if !defined(_LIBXFCE4UI_INSIDE_LIBXFCE4UI_H) && !defined(LIBXFCE4UI_COMPILATION)
#error "Only <libxfce4ui/libxfce4ui.h> can be included directly, this file is not part of the public API."
#endif

#ifndef __XFCE_GDK_PIXBUF_EXTENSIONS_H__
#define __XFCE_GDK_PIXBUF_EXTENSIONS_H__

#include <gdk-pixbuf/gdk-pixbuf.h>

G_BEGIN_DECLS

GdkPixbuf *
xfce_gdk_pixbuf_frame (const GdkPixbuf *source,
                       const GdkPixbuf *frame,
                       gint left_offset,
                       gint top_offset,
                       gint right_offset,
                       gint bottom_offset) G_GNUC_MALLOC G_GNUC_WARN_UNUSED_RESULT;

GdkPixbuf *
xfce_gdk_pixbuf_lucent (const GdkPixbuf *source,
                        guint percent) G_GNUC_MALLOC G_GNUC_WARN_UNUSED_RESULT;

GdkPixbuf *
xfce_gdk_pixbuf_scale_down (GdkPixbuf *source,
                            gboolean preserve_aspect_ratio,
                            gint dest_width,
                            gint dest_height) G_GNUC_WARN_UNUSED_RESULT;

GdkPixbuf *
xfce_gdk_pixbuf_scale_ratio (GdkPixbuf *source,
                             gint dest_size) G_GNUC_WARN_UNUSED_RESULT;

GdkPixbuf *
xfce_gdk_pixbuf_new_from_file_at_max_size (const gchar *filename,
                                           gint max_width,
                                           gint max_height,
                                           gboolean preserve_aspect_ratio,
                                           GError **error) G_GNUC_MALLOC G_GNUC_WARN_UNUSED_RESULT;

G_END_DECLS

#endif /* !__XFCE_GDK_PIXBUF_EXTENSIONS_H__ */
