/*-
 * Copyright (c) 2004-2006  os-cillation e.K.
 * Copyright (c) 2002,2004  Anders Carlsson <andersca@gnu.org>
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

#ifndef __XFCE_ICON_VIEW_H__
#define __XFCE_ICON_VIEW_H__

#include <gtk/gtk.h>

#include "libxfce4ui-enums.h"

G_BEGIN_DECLS

#define XFCE_TYPE_ICON_VIEW (xfce_icon_view_get_type ())
G_DECLARE_DERIVABLE_TYPE (XfceIconView, xfce_icon_view, XFCE, ICON_VIEW, GtkContainer)

/**
 * XfceIconViewForeachFunc:
 * @icon_view : an #XfceIconView.
 * @path      : the current path.
 * @user_data : the user data supplied to xfce_icon_view_selected_foreach().
 *
 * Callback function prototype, invoked for every selected path in the
 * @icon_view. See xfce_icon_view_selected_foreach() for details.
 *
 * Since: 4.21.0
 **/
typedef void (*XfceIconViewForeachFunc) (XfceIconView *icon_view,
                                         GtkTreePath *path,
                                         gpointer user_data);

/**
 * XfceIconViewSearchEqualFunc:
 * @model       : the #GtkTreeModel being searched.
 * @column      : the search column set by xfce_icon_view_set_search_column().
 * @key         : the key string to compare with.
 * @iter        : the #GtkTreeIter of the current item.
 * @search_data : user data from xfce_icon_view_set_search_equal_func().
 *
 * A function used for checking whether a row in @model matches a search key string
 * entered by the user. Note the return value is reversed from what you would normally
 * expect, though it has some similarity to strcmp() returning 0 for equal strings.
 *
 * Returns: %FALSE if the row matches, %TRUE otherwise.
 *
 * Since: 4.21.0
 **/
typedef gboolean (*XfceIconViewSearchEqualFunc) (GtkTreeModel *model,
                                                 gint column,
                                                 const gchar *key,
                                                 GtkTreeIter *iter,
                                                 gpointer search_data);

/**
 * XfceIconViewSearchPositionFunc:
 * @icon_view     : an #XfceIconView.
 * @search_dialog : the search dialog window to place.
 * @user_data     : user data from xfce_icon_view_set_search_position_func().
 *
 * A function used to place the @search_dialog for the @icon_view.
 *
 * Since: 4.21.0
 **/
typedef void (*XfceIconViewSearchPositionFunc) (XfceIconView *icon_view,
                                                GtkWidget *search_dialog,
                                                gpointer user_data);

/**
 * XfceIconView:
 *
 * #XfceIconView provides an alternative view on a list model.
 * It displays the model as a grid of icons with labels. Like
 * #GtkTreeView, it allows to select one or multiple items
 * (depending on the selection mode, see xfce_icon_view_set_selection_mode()).
 * In addition to selection with the arrow keys, #XfceIconView supports
 * rubberband selection, which is controlled by dragging the pointer.
 *
 * Since: 4.21.0
 **/
struct _XfceIconViewClass
{
  GtkContainerClass __parent__;

  /* virtual methods */
  void (*set_scroll_adjustments) (XfceIconView *icon_view,
                                  GtkAdjustment *hadjustment,
                                  GtkAdjustment *vadjustment);

  /* signals */
  void (*item_activated) (XfceIconView *icon_view,
                          GtkTreePath *path);
  void (*selection_changed) (XfceIconView *icon_view);

  /* Key binding signals */
  void (*select_all) (XfceIconView *icon_view);
  void (*unselect_all) (XfceIconView *icon_view);
  void (*select_cursor_item) (XfceIconView *icon_view);
  void (*toggle_cursor_item) (XfceIconView *icon_view);
  gboolean (*move_cursor) (XfceIconView *icon_view,
                           GtkMovementStep step,
                           gint count);
  gboolean (*activate_cursor_item) (XfceIconView *icon_view);
  gboolean (*start_interactive_search) (XfceIconView *icon_view);

  /*< private >*/
  void (*reserved0) (void);
  void (*reserved1) (void);
  void (*reserved2) (void);
  void (*reserved3) (void);
  void (*reserved4) (void);
  void (*reserved5) (void);
  void (*reserved6) (void);
  void (*reserved7) (void);
  void (*reserved8) (void);
  void (*reserved9) (void);
};

GtkWidget *
xfce_icon_view_new (void);
GtkWidget *
xfce_icon_view_new_with_model (GtkTreeModel *model);

GtkTreeModel *
xfce_icon_view_get_model (XfceIconView *icon_view);
void
xfce_icon_view_set_model (XfceIconView *icon_view,
                          GtkTreeModel *model);

GtkOrientation
xfce_icon_view_get_orientation (XfceIconView *icon_view);
void
xfce_icon_view_set_orientation (XfceIconView *icon_view,
                                GtkOrientation orientation);

gint
xfce_icon_view_get_columns (XfceIconView *icon_view);
void
xfce_icon_view_set_columns (XfceIconView *icon_view,
                            gint columns);

gint
xfce_icon_view_get_item_width (XfceIconView *icon_view);
void
xfce_icon_view_set_item_width (XfceIconView *icon_view,
                               gint item_width);

gint
xfce_icon_view_get_spacing (XfceIconView *icon_view);
void
xfce_icon_view_set_spacing (XfceIconView *icon_view,
                            gint spacing);

gint
xfce_icon_view_get_row_spacing (XfceIconView *icon_view);
void
xfce_icon_view_set_row_spacing (XfceIconView *icon_view,
                                gint row_spacing);

gint
xfce_icon_view_get_column_spacing (XfceIconView *icon_view);
void
xfce_icon_view_set_column_spacing (XfceIconView *icon_view,
                                   gint column_spacing);

gint
xfce_icon_view_get_margin (XfceIconView *icon_view);
void
xfce_icon_view_set_margin (XfceIconView *icon_view,
                           gint margin);

GtkSelectionMode
xfce_icon_view_get_selection_mode (XfceIconView *icon_view);
void
xfce_icon_view_set_selection_mode (XfceIconView *icon_view,
                                   GtkSelectionMode mode);

XfceIconViewLayoutMode
xfce_icon_view_get_layout_mode (XfceIconView *icon_view);
void
xfce_icon_view_set_layout_mode (XfceIconView *icon_view,
                                XfceIconViewLayoutMode layout_mode);

gboolean
xfce_icon_view_get_single_click (XfceIconView *icon_view);
void
xfce_icon_view_set_single_click (XfceIconView *icon_view,
                                 gboolean single_click);

guint
xfce_icon_view_get_single_click_timeout (XfceIconView *icon_view);
void
xfce_icon_view_set_single_click_timeout (XfceIconView *icon_view,
                                         guint single_click_timeout);

void
xfce_icon_view_widget_to_icon_coords (XfceIconView *icon_view,
                                      gint wx,
                                      gint wy,
                                      gint *ix,
                                      gint *iy);
void
xfce_icon_view_icon_to_widget_coords (XfceIconView *icon_view,
                                      gint ix,
                                      gint iy,
                                      gint *wx,
                                      gint *wy);

GtkTreePath *
xfce_icon_view_get_path_at_pos (XfceIconView *icon_view,
                                gint x,
                                gint y);
gboolean
xfce_icon_view_get_item_at_pos (XfceIconView *icon_view,
                                gint x,
                                gint y,
                                GtkTreePath **path,
                                GtkCellRenderer **cell);

gboolean
xfce_icon_view_get_visible_range (XfceIconView *icon_view,
                                  GtkTreePath **start_path,
                                  GtkTreePath **end_path);

void
xfce_icon_view_selected_foreach (XfceIconView *icon_view,
                                 XfceIconViewForeachFunc func,
                                 gpointer data);
void
xfce_icon_view_select_path (XfceIconView *icon_view,
                            GtkTreePath *path);
void
xfce_icon_view_unselect_path (XfceIconView *icon_view,
                              GtkTreePath *path);
gboolean
xfce_icon_view_path_is_selected (XfceIconView *icon_view,
                                 GtkTreePath *path);
GList *
xfce_icon_view_get_selected_items (XfceIconView *icon_view);
void
xfce_icon_view_select_all (XfceIconView *icon_view);
void
xfce_icon_view_unselect_all (XfceIconView *icon_view);
void
xfce_icon_view_selection_invert (XfceIconView *icon_view);
void
xfce_icon_view_item_activated (XfceIconView *icon_view,
                               GtkTreePath *path);

gint
xfce_icon_view_get_item_column (XfceIconView *icon_view,
                                GtkTreePath *path);
gint
xfce_icon_view_get_item_row (XfceIconView *icon_view,
                             GtkTreePath *path);

gboolean
xfce_icon_view_get_cursor (XfceIconView *icon_view,
                           GtkTreePath **path,
                           GtkCellRenderer **cell);
void
xfce_icon_view_set_cursor (XfceIconView *icon_view,
                           GtkTreePath *path,
                           GtkCellRenderer *cell,
                           gboolean start_editing);

void
xfce_icon_view_scroll_to_path (XfceIconView *icon_view,
                               GtkTreePath *path,
                               gboolean use_align,
                               gfloat row_align,
                               gfloat col_align);

/* Drag-and-Drop support */
void
xfce_icon_view_enable_model_drag_source (XfceIconView *icon_view,
                                         GdkModifierType start_button_mask,
                                         const GtkTargetEntry *targets,
                                         gint n_targets,
                                         GdkDragAction actions);
void
xfce_icon_view_enable_model_drag_dest (XfceIconView *icon_view,
                                       const GtkTargetEntry *targets,
                                       gint n_targets,
                                       GdkDragAction actions);
void
xfce_icon_view_unset_model_drag_source (XfceIconView *icon_view);
void
xfce_icon_view_unset_model_drag_dest (XfceIconView *icon_view);
void
xfce_icon_view_set_reorderable (XfceIconView *icon_view,
                                gboolean reorderable);
gboolean
xfce_icon_view_get_reorderable (XfceIconView *icon_view);


/* These are useful to implement your own custom stuff. */
void
xfce_icon_view_set_drag_dest_item (XfceIconView *icon_view,
                                   GtkTreePath *path,
                                   XfceIconViewDropPosition pos);
void
xfce_icon_view_get_drag_dest_item (XfceIconView *icon_view,
                                   GtkTreePath **path,
                                   XfceIconViewDropPosition *pos);
gboolean
xfce_icon_view_get_dest_item_at_pos (XfceIconView *icon_view,
                                     gint drag_x,
                                     gint drag_y,
                                     GtkTreePath **path,
                                     XfceIconViewDropPosition *pos);
cairo_surface_t *
xfce_icon_view_create_drag_icon (XfceIconView *icon_view,
                                 GtkTreePath *path);


/* Interactive search support */
gboolean
xfce_icon_view_get_enable_search (XfceIconView *icon_view);
void
xfce_icon_view_set_enable_search (XfceIconView *icon_view,
                                  gboolean enable_search);
gint
xfce_icon_view_get_search_column (XfceIconView *icon_view);
void
xfce_icon_view_set_search_column (XfceIconView *icon_view,
                                  gint search_column);
XfceIconViewSearchEqualFunc
xfce_icon_view_get_search_equal_func (XfceIconView *icon_view);
void
xfce_icon_view_set_search_equal_func (XfceIconView *icon_view,
                                      XfceIconViewSearchEqualFunc search_equal_func,
                                      gpointer search_equal_data,
                                      GDestroyNotify search_equal_destroy);
XfceIconViewSearchPositionFunc
xfce_icon_view_get_search_position_func (XfceIconView *icon_view);
void
xfce_icon_view_set_search_position_func (XfceIconView *icon_view,
                                         XfceIconViewSearchPositionFunc search_position_func,
                                         gpointer search_position_data,
                                         GDestroyNotify search_position_destroy);

G_END_DECLS

#endif /* __XFCE_ICON_VIEW_H__ */
