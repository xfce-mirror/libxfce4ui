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

#ifndef __XFCE_LIST_VIEW_H__
#define __XFCE_LIST_VIEW_H__

#include <gtk/gtk.h>

G_BEGIN_DECLS

typedef struct _XfceListViewPrivate XfceListViewPrivate;

#define XFCE_TYPE_LIST_VIEW  (xfce_list_view_get_type ())

G_DECLARE_FINAL_TYPE (XfceListView, xfce_list_view, XFCE, LIST_VIEW, GtkTreeView)

struct _XfceListViewClass
{
  /*< private >*/
  GtkTreeViewClass __parent__;

  /*< private >*/
  void (*reserved1) (void);
  void (*reserved2) (void);
  void (*reserved3) (void);
  void (*reserved4) (void);
  void (*reserved5) (void);
  void (*reserved6) (void);
  void (*reserved7) (void);
  void (*reserved8) (void);
};

/**
 * XfceListView
 *
 **/
struct _XfceListView
{
  /*< private >*/
  GtkTreeView __parent__;

  /*< private >*/
  XfceListViewPrivate *priv;
};

GType           xfce_list_view_get_type                 (void) G_GNUC_CONST;

GtkWidget*      xfce_list_view_new                      (void) G_GNUC_MALLOC;

gboolean        xfce_list_view_get_single_click         (const XfceListView *list_view);

void            xfce_list_view_set_single_click         (XfceListView       *list_view,
                                                         gboolean            single_click);

guint           xfce_list_view_get_single_click_timeout (const XfceListView *list_view);

void            xfce_list_view_set_single_click_timeout (XfceListView       *list_view,
                                                         guint               single_click_timeout);

GtkTreeViewColumn* xfce_list_view_add_possible_column   (const XfceListView *list_view,
                                                         const gchar        *column_id,
                                                         const gchar        *column_title);

GtkTreeViewColumn* xfce_list_view_get_column            (XfceListView       *list_view,
                                                         gchar              *column_id);

void            xfce_list_view_set_column_visible       (XfceListView       *list_view,
                                                         const gchar        *column_id,
                                                         const gboolean      visible);

gboolean        xfce_list_view_get_column_visible       (const XfceListView *list_view,
                                                         const gchar        *column_id);

guint           xfce_list_view_get_column_position      (const XfceListView *list_view,
                                                         const gchar        *column_id);

void            xfce_list_view_insert_column_at_position(XfceListView       *list_view,
                                                         gchar              *column_id,
                                                         const guint         position);

void            xfce_list_view_render_text              (XfceListView       *list_view,
                                                         const gchar        *column_id,
                                                         gint                column);

void            xfce_list_view_render_text_with_func    (XfceListView       *list_view,
                                                         const gchar        *column_id,
                                                         GtkTreeCellDataFunc func);

void            xfce_list_view_render_pixbuf_text       (XfceListView       *list_view,
                                                         const gchar        *column_id,
                                                         gint                pixbuf_column,
                                                         gint                text_column);

void            xfce_list_view_bind_model               (XfceListView       *list_view,
                                                         GtkTreeModel       *model);

GtkTreeModel*   xfce_list_view_get_model                (XfceListView       *list_view);

gchar*          xfce_list_view_serialize_state          (XfceListView       *list_view);

void            xfce_list_view_set_state_from_string    (XfceListView       *list_view,
                                                         gchar              *state);

G_END_DECLS

#endif /* !__XFCE_LIST_VIEW_H__ */
