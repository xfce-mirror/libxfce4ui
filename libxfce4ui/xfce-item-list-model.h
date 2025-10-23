/*-
 * Copyright (c) 2025 Dmitry Petrachkov <dmitry-petrachkov@outlook.com>
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

#ifndef __XFCE_ITEM_LIST_MODEL_H__
#define __XFCE_ITEM_LIST_MODEL_H__

#include <gtk/gtk.h>
#include <libxfce4ui/libxfce4ui-enums.h>

G_BEGIN_DECLS

#define XFCE_TYPE_ITEM_LIST_MODEL (xfce_item_list_model_get_type ())
G_DECLARE_DERIVABLE_TYPE (XfceItemListModel, xfce_item_list_model, XFCE, ITEM_LIST_MODEL, GObject)

/**
 * XfceItemListModelClass:
 * @get_list_n_columns: You can override this method to add more columns
 * @get_list_column_type: You can override this method to add your own columns
 * @get_n_items: Required for implementation
 * @get_item_value: Required for implementation
 * @move: Method must be implemented if the #XFCE_ITEM_LIST_MODEL_REORDERABLE flag is set
 * @set_activity: Method must be implemented if there is at least one item with column #XFCE_ITEM_LIST_MODEL_COLUMN_ACTIVABLE that is TRUE
 * @edit: Method must be implemented if the #XFCE_ITEM_LIST_MODEL_EDITABLE flag is set
 * @add: Method must be implemented if the #XFCE_ITEM_LIST_MODEL_ADDABLE flag is set
 * @remove: Method must be implemented if the #XFCE_ITEM_LIST_MODEL_REMOVABLE flag is set
 * @reset: Method must be implemented if the #XFCE_ITEM_LIST_MODEL_RESETTABLE flag is set
 **/
struct _XfceItemListModelClass
{
  /*< private >*/
  GObjectClass __parent__;

  /*< public >*/
  gint (*get_list_n_columns) (XfceItemListModel *model);

  GType (*get_list_column_type) (XfceItemListModel *model,
                                 gint column);

  gint (*get_n_items) (XfceItemListModel *model);

  void (*get_item_value) (XfceItemListModel *model,
                          gint index,
                          gint column,
                          GValue *value);

  void (*move) (XfceItemListModel *model,
                gint source_index,
                gint dest_index);

  void (*set_activity) (XfceItemListModel *model,
                        gint index,
                        gboolean value);

  gboolean (*remove) (XfceItemListModel *model,
                      gint index);

  void (*reset) (XfceItemListModel *model);
};

gint
xfce_item_list_model_get_list_n_columns (XfceItemListModel *model);

GType
xfce_item_list_model_get_list_column_type (XfceItemListModel *model,
                                           gint column);

XfceItemListModelFlags
xfce_item_list_model_get_list_flags (XfceItemListModel *model);

gint
xfce_item_list_model_get_n_items (XfceItemListModel *model);

void
xfce_item_list_model_get_item_value (XfceItemListModel *model,
                                     gint index,
                                     gint column,
                                     GValue *value);

void
xfce_item_list_model_move (XfceItemListModel *model,
                           gint source_index,
                           gint dest_index);

void
xfce_item_list_model_set_activity (XfceItemListModel *model,
                                   gint index,
                                   gboolean value);

gboolean
xfce_item_list_model_add (XfceItemListModel *model);

gboolean
xfce_item_list_model_remove (XfceItemListModel *model,
                             gint index);

void
xfce_item_list_model_reset (XfceItemListModel *model);

void
xfce_item_list_model_set_index (XfceItemListModel *model,
                                GtkTreeIter *iter,
                                gint index);

gint
xfce_item_list_model_get_index (XfceItemListModel *model,
                                GtkTreeIter *iter);

gboolean
xfce_item_list_model_test (XfceItemListModel *model,
                           gint index,
                           gint column);

gboolean
xfce_item_list_model_test_any (XfceItemListModel *model,
                               const gint *indexes,
                               gint n_indexes,
                               gint column);

gboolean
xfce_item_list_model_test_all (XfceItemListModel *model,
                               const gint *indexes,
                               gint n_indexes,
                               gint column);

void
xfce_item_list_model_changed (XfceItemListModel *model);

void
xfce_item_list_model_reloaded (XfceItemListModel *model,
                               gint prev_n_items);

G_END_DECLS

#endif /* !__XFCE_ITEM_LIST_MODEL_H__ */
