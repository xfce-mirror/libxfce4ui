/*-
 * Copyright (c) 2006 Benedikt Meurer <benny@xfce.org>
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

#ifndef __XFCE_THUMBNAIL_PREVIEW_H__
#define __XFCE_THUMBNAIL_PREVIEW_H__

#include <gtk/gtk.h>

G_BEGIN_DECLS

#define XFCE_TYPE_THUMBNAIL_PREVIEW (xfce_thumbnail_preview_get_type ())
G_DECLARE_FINAL_TYPE (XfceThumbnailPreview, xfce_thumbnail_preview, XFCE, THUMBNAIL_PREVIEW, GtkFrame)

G_GNUC_INTERNAL GtkWidget *
xfce_thumbnail_preview_new (void) G_GNUC_MALLOC G_GNUC_WARN_UNUSED_RESULT;

G_GNUC_INTERNAL void
xfce_thumbnail_preview_set_uri (XfceThumbnailPreview *preview,
                                const gchar *uri);

G_END_DECLS

#endif /* !__XFCE_THUMBNAIL_PREVIEW_H__ */
