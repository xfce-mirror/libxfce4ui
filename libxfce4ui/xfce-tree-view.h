/*-
 * Copyright (c) 2004-2006 Benedikt Meurer <benny@xfce.org>
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

#ifndef __XFCE_TREE_VIEW_H__
#define __XFCE_TREE_VIEW_H__

#include <gtk/gtk.h>

G_BEGIN_DECLS

#define XFCE_TYPE_TREE_VIEW (xfce_tree_view_get_type ())
G_DECLARE_FINAL_TYPE (XfceTreeView, xfce_tree_view, XFCE, TREE_VIEW, GtkTreeView)

GtkWidget *
xfce_tree_view_new (void) G_GNUC_MALLOC;

gboolean
xfce_tree_view_get_single_click (XfceTreeView *tree_view);
void
xfce_tree_view_set_single_click (XfceTreeView *tree_view,
                                 gboolean single_click);

guint
xfce_tree_view_get_single_click_timeout (XfceTreeView *tree_view);
void
xfce_tree_view_set_single_click_timeout (XfceTreeView *tree_view,
                                         guint single_click_timeout);

G_END_DECLS

#endif /* !__XFCE_TREE_VIEW_H__ */
