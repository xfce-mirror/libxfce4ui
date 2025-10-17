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

#include "libxfce4ui-private.h"
#include "xfce-item-list-model.h"
#include "libxfce4ui-visibility.h"



/**
 * SECTION: xfce-item-list-model
 * @title: XfceItemListModel
 * @short_description: Model for #XfceItemListView
 * @include: libxfce4ui/libxfce4ui.h
 *
 * A model that already implements the complex parts of the #GtkTreeModel, and allows you to focus on your data.
 *
 * You don't need to implement the #GtkTreeModel, #GtkTreeDragSource, #GtkTreeDragDest methods yourself. You also
 * shouldn't call signals from #GtkTreeModel from overridden methods. However, if you define your new methods in a
 * descendant class, then you must call the #GtkTreeView signals yourself.
 *
 * Not all virtual functions are required to be implemented, it depends on the value you return from
 * xfce_item_list_model_get_list_flags().
 **/



static GType
xfce_item_list_model_get_list_column_type_default (XfceItemListModel *model,
                                                   gint column);

static XfceItemListModelFlags
xfce_item_list_model_get_list_flags_default (XfceItemListModel *model);

static void
xfce_item_list_model_tree_model_init (GtkTreeModelIface *iface);

static void
xfce_item_list_model_tree_drag_source_init (GtkTreeDragSourceIface *iface);

static void
xfce_item_list_model_tree_drag_dest_init (GtkTreeDragDestIface *iface);

static GType
xfce_item_list_model_tree_get_column_type (GtkTreeModel *tree_model,
                                           gint tree_column);

static GtkTreeModelFlags
xfce_item_list_model_tree_get_flags (GtkTreeModel *tree_model);

static gboolean
xfce_item_list_model_tree_get_iter (GtkTreeModel *tree_model,
                                    GtkTreeIter *iter,
                                    GtkTreePath *path);

static gint
xfce_item_list_model_tree_get_n_columns (GtkTreeModel *tree_model);

static GtkTreePath *
xfce_item_list_model_tree_get_path (GtkTreeModel *tree_model,
                                    GtkTreeIter *iter);

static void
xfce_item_list_model_tree_get_value (GtkTreeModel *tree_model,
                                     GtkTreeIter *iter,
                                     gint column,
                                     GValue *value);

static gboolean
xfce_item_list_model_tree_iter_children (GtkTreeModel *tree_model,
                                         GtkTreeIter *iter,
                                         GtkTreeIter *parent);

static gint
xfce_item_list_model_tree_iter_n_children (GtkTreeModel *tree_model,
                                           GtkTreeIter *iter);

static gboolean
xfce_item_list_model_tree_iter_next (GtkTreeModel *tree_model,
                                     GtkTreeIter *iter);

static gboolean
xfce_item_list_model_tree_iter_nth_child (GtkTreeModel *tree_model,
                                          GtkTreeIter *iter,
                                          GtkTreeIter *parent,
                                          gint n);

static gboolean
xfce_item_list_model_tree_iter_previous (GtkTreeModel *tree_model,
                                         GtkTreeIter *iter);

static gboolean
xfce_item_list_model_tree_drag_data_delete (GtkTreeDragSource *drag_source,
                                            GtkTreePath *path);

static gboolean
xfce_item_list_model_tree_drag_data_get (GtkTreeDragSource *drag_source,
                                         GtkTreePath *path,
                                         GtkSelectionData *selection_data);

static gboolean
xfce_item_list_model_row_draggable (GtkTreeDragSource *drag_source,
                                    GtkTreePath *path);

static gboolean
xfce_item_list_model_get_dnd_indexes (XfceItemListModel *model,
                                      GtkTreePath *dest,
                                      GtkSelectionData *selection_data,
                                      gint *p_source_index,
                                      gint *p_dest_index);

static gboolean
xfce_item_list_model_tree_drag_data_received (GtkTreeDragDest *drag_dest,
                                              GtkTreePath *dest,
                                              GtkSelectionData *selection_data);

static gboolean
xfce_item_list_model_tree_row_drop_possible (GtkTreeDragDest *drag_dest,
                                             GtkTreePath *dest,
                                             GtkSelectionData *selection_data);



G_DEFINE_TYPE_WITH_CODE (XfceItemListModel, xfce_item_list_model, G_TYPE_OBJECT,
                         G_IMPLEMENT_INTERFACE (GTK_TYPE_TREE_MODEL, xfce_item_list_model_tree_model_init)
                         G_IMPLEMENT_INTERFACE (GTK_TYPE_TREE_DRAG_SOURCE, xfce_item_list_model_tree_drag_source_init)
                         G_IMPLEMENT_INTERFACE (GTK_TYPE_TREE_DRAG_DEST, xfce_item_list_model_tree_drag_dest_init))



static void
xfce_item_list_model_class_init (XfceItemListModelClass *klass)
{
  klass->get_list_column_type = xfce_item_list_model_get_list_column_type_default;
  klass->get_list_flags = xfce_item_list_model_get_list_flags_default;
}



static void
xfce_item_list_model_init (XfceItemListModel *model)
{
}



static GType
xfce_item_list_model_get_list_column_type_default (XfceItemListModel *model,
                                                   gint column)
{
  switch (column)
    {
    case XFCE_ITEM_LIST_MODEL_COLUMN_ACTIVE:
      return G_TYPE_BOOLEAN;

    case XFCE_ITEM_LIST_MODEL_COLUMN_ACTIVABLE:
      return G_TYPE_BOOLEAN;

    case XFCE_ITEM_LIST_MODEL_COLUMN_ICON:
      return G_TYPE_ICON;

    case XFCE_ITEM_LIST_MODEL_COLUMN_NAME:
      return G_TYPE_STRING;

    case XFCE_ITEM_LIST_MODEL_COLUMN_TOOLTIP:
      return G_TYPE_STRING;

    case XFCE_ITEM_LIST_MODEL_COLUMN_EDITABLE:
      return G_TYPE_BOOLEAN;

    case XFCE_ITEM_LIST_MODEL_COLUMN_REMOVABLE:
      return G_TYPE_BOOLEAN;

    default:
      g_warn_if_reached ();
      return G_TYPE_NONE;
    }
}



static XfceItemListModelFlags
xfce_item_list_model_get_list_flags_default (XfceItemListModel *model)
{
  return XFCE_ITEM_LIST_MODEL_NONE;
}



static void
xfce_item_list_model_tree_model_init (GtkTreeModelIface *iface)
{
  iface->get_column_type = xfce_item_list_model_tree_get_column_type;
  iface->get_flags = xfce_item_list_model_tree_get_flags;
  iface->get_iter = xfce_item_list_model_tree_get_iter;
  iface->get_n_columns = xfce_item_list_model_tree_get_n_columns;
  iface->get_path = xfce_item_list_model_tree_get_path;
  iface->get_value = xfce_item_list_model_tree_get_value;
  iface->iter_children = xfce_item_list_model_tree_iter_children;
  iface->iter_n_children = xfce_item_list_model_tree_iter_n_children;
  iface->iter_next = xfce_item_list_model_tree_iter_next;
  iface->iter_nth_child = xfce_item_list_model_tree_iter_nth_child;
  iface->iter_previous = xfce_item_list_model_tree_iter_previous;
}



static void
xfce_item_list_model_tree_drag_source_init (GtkTreeDragSourceIface *iface)
{
  iface->drag_data_delete = xfce_item_list_model_tree_drag_data_delete;
  iface->drag_data_get = xfce_item_list_model_tree_drag_data_get;
  iface->row_draggable = xfce_item_list_model_row_draggable;
}



static void
xfce_item_list_model_tree_drag_dest_init (GtkTreeDragDestIface *iface)
{
  iface->drag_data_received = xfce_item_list_model_tree_drag_data_received;
  iface->row_drop_possible = xfce_item_list_model_tree_row_drop_possible;
}



static GType
xfce_item_list_model_tree_get_column_type (GtkTreeModel *tree_model,
                                           gint tree_column)
{
  XfceItemListModel *model = XFCE_ITEM_LIST_MODEL (tree_model);

  return xfce_item_list_model_get_list_column_type (model, tree_column);
}



static GtkTreeModelFlags
xfce_item_list_model_tree_get_flags (GtkTreeModel *tree_model)
{
  return GTK_TREE_MODEL_LIST_ONLY;
}



static gboolean
xfce_item_list_model_tree_get_iter (GtkTreeModel *tree_model,
                                    GtkTreeIter *iter,
                                    GtkTreePath *path)
{
  XfceItemListModel *model = XFCE_ITEM_LIST_MODEL (tree_model);

  g_return_val_if_fail (gtk_tree_path_get_depth (path) == 1, FALSE);
  gint index = *gtk_tree_path_get_indices (path);

  if (index >= 0 && index < xfce_item_list_model_get_n_items (model))
    {
      xfce_item_list_model_set_index (model, iter, index);
      return TRUE;
    }
  else
    {
      return FALSE;
    }
}



static gint
xfce_item_list_model_tree_get_n_columns (GtkTreeModel *tree_model)
{
  return XFCE_ITEM_LIST_MODEL_COLUMN_USER;
}



static GtkTreePath *
xfce_item_list_model_tree_get_path (GtkTreeModel *tree_model,
                                    GtkTreeIter *iter)
{
  XfceItemListModel *model = XFCE_ITEM_LIST_MODEL (tree_model);

  return gtk_tree_path_new_from_indices (xfce_item_list_model_get_index (model, iter), -1);
}



static void
xfce_item_list_model_tree_get_value (GtkTreeModel *tree_model,
                                     GtkTreeIter *iter,
                                     gint column,
                                     GValue *value)
{
  XfceItemListModel *model = XFCE_ITEM_LIST_MODEL (tree_model);

  xfce_item_list_model_get_item_value (model, xfce_item_list_model_get_index (model, iter), column, value);
}



static gboolean
xfce_item_list_model_tree_iter_children (GtkTreeModel *tree_model,
                                         GtkTreeIter *iter,
                                         GtkTreeIter *parent)
{
  XfceItemListModel *model = XFCE_ITEM_LIST_MODEL (tree_model);

  /* This is a list, so only the root element is supported */
  if (parent == NULL && xfce_item_list_model_get_n_items (model) > 0)
    {
      xfce_item_list_model_set_index (model, iter, 0);
      return TRUE;
    }
  else
    {
      return FALSE;
    }
}



static gint
xfce_item_list_model_tree_iter_n_children (GtkTreeModel *tree_model,
                                           GtkTreeIter *iter)
{
  XfceItemListModel *model = XFCE_ITEM_LIST_MODEL (tree_model);

  if (iter == NULL)
    return xfce_item_list_model_get_n_items (model);
  else
    return 0;
}



static gboolean
xfce_item_list_model_tree_iter_next (GtkTreeModel *tree_model,
                                     GtkTreeIter *iter)
{
  XfceItemListModel *model = XFCE_ITEM_LIST_MODEL (tree_model);
  /* The index may be out of date */
  gint index = GPOINTER_TO_INT (iter->user_data);

  if (index >= 0 && index + 1 < xfce_item_list_model_get_n_items (model))
    {
      xfce_item_list_model_set_index (model, iter, index + 1);
      return TRUE;
    }
  else
    {
      return FALSE;
    }
}



static gboolean
xfce_item_list_model_tree_iter_nth_child (GtkTreeModel *tree_model,
                                          GtkTreeIter *iter,
                                          GtkTreeIter *parent,
                                          gint n)
{
  XfceItemListModel *model = XFCE_ITEM_LIST_MODEL (tree_model);

  /* This is a list, so only the root element is supported */
  if (parent == NULL && n >= 0 && n < xfce_item_list_model_get_n_items (model))
    {
      xfce_item_list_model_set_index (model, iter, n);
      return TRUE;
    }
  else
    {
      return FALSE;
    }
}



static gboolean
xfce_item_list_model_tree_iter_previous (GtkTreeModel *tree_model,
                                         GtkTreeIter *iter)
{
  XfceItemListModel *model = XFCE_ITEM_LIST_MODEL (tree_model);
  /* The index may be out of date */
  gint index = GPOINTER_TO_INT (iter->user_data);

  if (index > 0 && index - 1 < xfce_item_list_model_get_n_items (model))
    {
      xfce_item_list_model_set_index (model, iter, index - 1);
      return TRUE;
    }
  else
    {
      return FALSE;
    }
}



static gboolean
xfce_item_list_model_tree_drag_data_delete (GtkTreeDragSource *drag_source,
                                            GtkTreePath *path)
{
  /* The row was not deleted, this model does not use remove for DnD, instead it uses xfce_item_list_model_move() */
  return FALSE;
}



static gboolean
xfce_item_list_model_tree_drag_data_get (GtkTreeDragSource *drag_source,
                                         GtkTreePath *path,
                                         GtkSelectionData *selection_data)
{
  return gtk_tree_set_row_drag_data (selection_data, GTK_TREE_MODEL (drag_source), path);
}



static gboolean
xfce_item_list_model_row_draggable (GtkTreeDragSource *drag_source,
                                    GtkTreePath *path)
{
  return TRUE;
}



static gboolean
xfce_item_list_model_get_dnd_indexes (XfceItemListModel *model,
                                      GtkTreePath *dest,
                                      GtkSelectionData *selection_data,
                                      gint *p_source_index,
                                      gint *p_dest_index)
{
  GtkTreePath *source = NULL;

  if (!gtk_tree_get_row_drag_data (selection_data, NULL, &source)
      || source == NULL
      || gtk_tree_path_get_depth (source) != 1)
    {
      gtk_tree_path_free (source);
      return FALSE;
    }
  gint source_index = *gtk_tree_path_get_indices (source);
  gtk_tree_path_free (source);

  /* Even though it's a list, Gtk can pass a double-depth path if the insertion occurs between items */
  if (gtk_tree_path_get_depth (dest) != 1 && gtk_tree_path_get_depth (dest) != 2)
    return FALSE;
  gint dest_index = *gtk_tree_path_get_indices (dest);

  /* Gtk assumes the source row has been removed */
  if (dest_index > source_index)
    dest_index = dest_index - 1;

  if (p_source_index != NULL)
    *p_source_index = source_index;
  if (p_dest_index != NULL)
    *p_dest_index = dest_index;

  gint n_items = xfce_item_list_model_get_n_items (model);
  return (dest_index >= 0 && dest_index < n_items) && (source_index >= 0 && source_index < n_items);
}



static gboolean
xfce_item_list_model_tree_drag_data_received (GtkTreeDragDest *drag_dest,
                                              GtkTreePath *dest,
                                              GtkSelectionData *selection_data)
{
  XfceItemListModel *model = XFCE_ITEM_LIST_MODEL (drag_dest);
  gint source_index, dest_index;

  if (xfce_item_list_model_get_dnd_indexes (model, dest, selection_data, &source_index, &dest_index))
    xfce_item_list_model_move (model, source_index, dest_index);

  /* Row was not inserted */
  return FALSE;
}



static gboolean
xfce_item_list_model_tree_row_drop_possible (GtkTreeDragDest *drag_dest,
                                             GtkTreePath *dest,
                                             GtkSelectionData *selection_data)
{
  XfceItemListModel *model = XFCE_ITEM_LIST_MODEL (drag_dest);

  return xfce_item_list_model_get_dnd_indexes (model, dest, selection_data, NULL, NULL);
}



/**
 * xfce_item_list_model_get_list_column_type:
 * @model: #XfceItemListModel
 * @column: Columns from #XfceItemListModelColumn, or custom columns after #XFCE_ITEM_LIST_MODEL_COLUMN_USER
 *
 * Returns: Column type
 *
 * Since: 4.21.2
 **/
GType
xfce_item_list_model_get_list_column_type (XfceItemListModel *model,
                                           gint column)
{
  XfceItemListModelClass *klass;

  g_return_val_if_fail (XFCE_IS_ITEM_LIST_MODEL (model), 0);
  klass = XFCE_ITEM_LIST_MODEL_GET_CLASS (model);

  g_return_val_if_fail (klass->get_list_column_type != NULL, 0);
  return klass->get_list_column_type (model, column);
}



/**
 * xfce_item_list_model_get_list_flags:
 * @model: #XfceItemListModel
 *
 * Returns: Supported model features
 *
 * Since: 4.21.2
 **/
XfceItemListModelFlags
xfce_item_list_model_get_list_flags (XfceItemListModel *model)
{
  XfceItemListModelClass *klass;

  g_return_val_if_fail (XFCE_IS_ITEM_LIST_MODEL (model), 0);
  klass = XFCE_ITEM_LIST_MODEL_GET_CLASS (model);

  g_return_val_if_fail (klass->get_list_flags != NULL, 0);
  return klass->get_list_flags (model);
}



/**
 * xfce_item_list_model_get_n_items:
 * @model: #XfceItemListModel
 *
 * Returns: Number of items
 *
 * Since: 4.21.2
 **/
gint
xfce_item_list_model_get_n_items (XfceItemListModel *model)
{
  XfceItemListModelClass *klass;

  g_return_val_if_fail (XFCE_IS_ITEM_LIST_MODEL (model), 0);
  klass = XFCE_ITEM_LIST_MODEL_GET_CLASS (model);

  g_return_val_if_fail (klass->get_n_items != NULL, 0);
  return klass->get_n_items (model);
}



/**
 * xfce_item_list_model_get_item_value:
 * @model: #XfceItemListModel
 * @index: Item index
 * @column: Columns from #XfceItemListModelColumn, or custom columns after #XFCE_ITEM_LIST_MODEL_COLUMN_USER
 * @value: (out) (transfer none): an empty #GValue to set
 *
 * Since: 4.21.2
 **/
void
xfce_item_list_model_get_item_value (XfceItemListModel *model,
                                     gint index,
                                     XfceItemListModelColumn column,
                                     GValue *value)
{
  XfceItemListModelClass *klass;

  g_return_if_fail (XFCE_IS_ITEM_LIST_MODEL (model));
  g_return_if_fail (index >= 0 && index < xfce_item_list_model_get_n_items (model));
  g_return_if_fail (column >= 0 && (gint) column < gtk_tree_model_get_n_columns (GTK_TREE_MODEL (model)));

  klass = XFCE_ITEM_LIST_MODEL_GET_CLASS (model);
  g_return_if_fail (klass->get_item_value != NULL);

  g_value_init (value, gtk_tree_model_get_column_type (GTK_TREE_MODEL (model), column));
  klass->get_item_value (model, index, column, value);
}



/**
 * xfce_item_list_model_move:
 * @model: #XfceItemListModel
 * @source_index: Index where the item will be taken from
 * @dest_index: Index where the item will be inserted
 *
 * Moves one item from the @source_index position to the @dest_index position
 *
 * Since: 4.21.2
 **/
void
xfce_item_list_model_move (XfceItemListModel *model,
                           gint source_index,
                           gint dest_index)
{
  XfceItemListModelClass *klass;

  g_return_if_fail (XFCE_IS_ITEM_LIST_MODEL (model));
  klass = XFCE_ITEM_LIST_MODEL_GET_CLASS (model);

  gint n_items = xfce_item_list_model_get_n_items (model);
  g_return_if_fail (source_index >= 0 && source_index < n_items);
  g_return_if_fail (dest_index >= 0 && dest_index < n_items);

  g_return_if_fail (klass->move != NULL);
  klass->move (model, source_index, dest_index);

  /* Signal for GtkTreeModel */
  gint *new_order = g_new (gint, n_items);
  for (gint i = 0, j = 0; i < n_items; ++i)
    {
      /* This loop does the same thing as:
       * new_order = order.copy()
       * tmp = new_order.remove(source_index)
       * new_order.insert(dest_index, tmp)
       */

      if (j == source_index)
        ++j;

      new_order[i] = i == dest_index ? source_index : j++;
    }
  GtkTreePath *tmp_path = gtk_tree_path_new ();
  gtk_tree_model_rows_reordered_with_length (GTK_TREE_MODEL (model), tmp_path, NULL, new_order, n_items);
  gtk_tree_path_free (tmp_path);
  g_free (new_order);
}



/**
 * xfce_item_list_model_set_activity:
 * @model: #XfceItemListModel
 * @index: Item index
 * @value: Activity value
 *
 * Since: 4.21.2
 **/
void
xfce_item_list_model_set_activity (XfceItemListModel *model,
                                   gint index,
                                   gboolean value)
{
  XfceItemListModelClass *klass;

  g_return_if_fail (XFCE_IS_ITEM_LIST_MODEL (model));
  g_return_if_fail (index >= 0 && index < xfce_item_list_model_get_n_items (model));
  klass = XFCE_ITEM_LIST_MODEL_GET_CLASS (model);

  g_return_if_fail (klass->set_activity != NULL);
  klass->set_activity (model, index, value);

  /* Signal for GtkTreeModel */
  GtkTreePath *path = gtk_tree_path_new_from_indices (index, -1);
  GtkTreeIter iter;
  xfce_item_list_model_set_index (model, &iter, index);
  gtk_tree_model_row_changed (GTK_TREE_MODEL (model), path, &iter);
  gtk_tree_path_free (path);
}



/**
 * xfce_item_list_model_edit:
 * @model: #XfceItemListModel
 * @index: Item index
 *
 * Since: 4.21.2
 **/
void
xfce_item_list_model_edit (XfceItemListModel *model,
                           gint index)
{
  XfceItemListModelClass *klass;

  g_return_if_fail (XFCE_IS_ITEM_LIST_MODEL (model));
  g_return_if_fail (index >= 0 && index < xfce_item_list_model_get_n_items (model));
  klass = XFCE_ITEM_LIST_MODEL_GET_CLASS (model);

  g_return_if_fail (klass->edit != NULL);
  klass->edit (model, index);

  /* Signal for GtkTreeModel */
  GtkTreePath *path = gtk_tree_path_new_from_indices (index, -1);
  GtkTreeIter iter;
  xfce_item_list_model_set_index (model, &iter, index);
  gtk_tree_model_row_changed (GTK_TREE_MODEL (model), path, &iter);
  gtk_tree_path_free (path);
}



/**
 * xfce_item_list_model_add:
 * @model: #XfceItemListModel
 *
 * Returns: If the item was inserted at the end, then returns TRUE
 *
 * Since: 4.21.2
 **/
gboolean
xfce_item_list_model_add (XfceItemListModel *model)
{
  XfceItemListModelClass *klass;

  g_return_val_if_fail (XFCE_IS_ITEM_LIST_MODEL (model), FALSE);
  klass = XFCE_ITEM_LIST_MODEL_GET_CLASS (model);

  g_return_val_if_fail (klass->add != NULL, FALSE);
  gint new_index = xfce_item_list_model_get_n_items (model);
  if (klass->add (model))
    {
      /* Signal for GtkTreeModel */
      GtkTreePath *path = gtk_tree_path_new_from_indices (new_index, -1);
      GtkTreeIter iter;
      xfce_item_list_model_set_index (model, &iter, new_index);
      gtk_tree_model_row_inserted (GTK_TREE_MODEL (model), path, &iter);
      gtk_tree_path_free (path);
      return TRUE;
    }
  else
    {
      return FALSE;
    }
}



/**
 * xfce_item_list_model_remove:
 * @model: #XfceItemListModel
 * @index: Item index
 *
 * Returns: If the item was removed then returns TRUE
 *
 * Since: 4.21.2
 **/
gboolean
xfce_item_list_model_remove (XfceItemListModel *model,
                             gint index)
{
  XfceItemListModelClass *klass;

  g_return_val_if_fail (XFCE_IS_ITEM_LIST_MODEL (model), FALSE);
  g_return_val_if_fail (index >= 0 && index < xfce_item_list_model_get_n_items (model), FALSE);
  klass = XFCE_ITEM_LIST_MODEL_GET_CLASS (model);

  g_return_val_if_fail (klass->remove != NULL, FALSE);
  if (klass->remove (model, index))
    {
      /* Signal for GtkTreeModel */
      GtkTreePath *path = gtk_tree_path_new_from_indices (index, -1);
      gtk_tree_model_row_deleted (GTK_TREE_MODEL (model), path);
      gtk_tree_path_free (path);
      return TRUE;
    }
  else
    {
      return FALSE;
    }
}



/**
 * xfce_item_list_model_reset:
 * @model: #XfceItemListModel
 *
 * Since: 4.21.2
 **/
void
xfce_item_list_model_reset (XfceItemListModel *model)
{
  XfceItemListModelClass *klass;

  g_return_if_fail (XFCE_IS_ITEM_LIST_MODEL (model));
  klass = XFCE_ITEM_LIST_MODEL_GET_CLASS (model);

  g_return_if_fail (klass->reset != NULL);
  klass->reset (model);

  /* Signal for GtkTreeModel */
  xfce_item_list_model_changed (model);
}



/**
 * xfce_item_list_model_set_index:
 * @model: #XfceItemListModel
 * @iter: Iterator that will be set to the specified index
 * @index: The index that will be set to the iterator
 *
 * Since: 4.21.2
 **/
void
xfce_item_list_model_set_index (XfceItemListModel *model,
                                GtkTreeIter *iter,
                                gint index)
{
  g_return_if_fail (XFCE_IS_ITEM_LIST_MODEL (model));
  g_return_if_fail (iter != NULL);
  g_return_if_fail (index >= 0 && index < xfce_item_list_model_get_n_items (model));

  iter->user_data = GINT_TO_POINTER (index);
}



/**
 * xfce_item_list_model_get_index:
 * @model: #XfceItemListModel
 * @iter: Iterator from which the index will be retrieved
 *
 * Returns: Index extracted from iterator
 *
 * Since: 4.21.2
 **/
gint
xfce_item_list_model_get_index (XfceItemListModel *model,
                                GtkTreeIter *iter)
{
  g_return_val_if_fail (XFCE_IS_ITEM_LIST_MODEL (model), -1);

  gint index = GPOINTER_TO_INT (iter->user_data);
  g_return_val_if_fail (index >= 0 && index < xfce_item_list_model_get_n_items (model), -1);

  return index;
}



/**
 * xfce_item_list_model_is_active:
 * @model: #XfceItemListModel
 * @index: Item index
 *
 * Returns: Item #XFCE_ITEM_LIST_MODEL_COLUMN_ACTIVE column value
 *
 * Since: 4.21.2
 **/
gboolean
xfce_item_list_model_is_active (XfceItemListModel *model,
                                gint index)
{
  g_return_val_if_fail (XFCE_IS_ITEM_LIST_MODEL (model), FALSE);
  g_return_val_if_fail (index >= 0 && index < xfce_item_list_model_get_n_items (model), FALSE);

  GValue value = G_VALUE_INIT;
  xfce_item_list_model_get_item_value (model, index, XFCE_ITEM_LIST_MODEL_COLUMN_ACTIVE, &value);
  return g_value_get_boolean (&value);
}



/**
 * xfce_item_list_model_is_activable:
 * @model: #XfceItemListModel
 * @index: Item index
 *
 * Returns: Item #XFCE_ITEM_LIST_MODEL_COLUMN_ACTIVABLE column value
 *
 * Since: 4.21.2
 **/
gboolean
xfce_item_list_model_is_activable (XfceItemListModel *model,
                                   gint index)
{
  g_return_val_if_fail (XFCE_IS_ITEM_LIST_MODEL (model), FALSE);
  g_return_val_if_fail (index >= 0 && index < xfce_item_list_model_get_n_items (model), FALSE);

  GValue value = G_VALUE_INIT;
  xfce_item_list_model_get_item_value (model, index, XFCE_ITEM_LIST_MODEL_COLUMN_ACTIVABLE, &value);
  return g_value_get_boolean (&value);
}



/**
 * xfce_item_list_model_is_editable:
 * @model: #XfceItemListModel
 * @index: Item index
 *
 * Returns: Item #XFCE_ITEM_LIST_MODEL_COLUMN_EDITABLE column value
 *
 * Since: 4.21.2
 **/
gboolean
xfce_item_list_model_is_editable (XfceItemListModel *model,
                                  gint index)
{
  g_return_val_if_fail (XFCE_IS_ITEM_LIST_MODEL (model), FALSE);
  g_return_val_if_fail (index >= 0 && index < xfce_item_list_model_get_n_items (model), FALSE);

  GValue value = G_VALUE_INIT;
  xfce_item_list_model_get_item_value (model, index, XFCE_ITEM_LIST_MODEL_COLUMN_EDITABLE, &value);
  return g_value_get_boolean (&value);
}



/**
 * xfce_item_list_model_is_removable:
 * @model: #XfceItemListModel
 * @index: Item index
 *
 * Returns: Item #XFCE_ITEM_LIST_MODEL_COLUMN_REMOVABLE column value
 *
 * Since: 4.21.2
 **/
gboolean
xfce_item_list_model_is_removable (XfceItemListModel *model,
                                   gint index)
{
  g_return_val_if_fail (XFCE_IS_ITEM_LIST_MODEL (model), FALSE);
  g_return_val_if_fail (index >= 0 && index < xfce_item_list_model_get_n_items (model), FALSE);

  GValue value = G_VALUE_INIT;
  xfce_item_list_model_get_item_value (model, index, XFCE_ITEM_LIST_MODEL_COLUMN_REMOVABLE, &value);
  return g_value_get_boolean (&value);
}



/**
 * xfce_item_list_model_changed:
 * @model: #XfceItemListModel
 *
 * Makes #GtkTreeView think that all items have changed their value
 *
 * Since: 4.21.2
 **/
void
xfce_item_list_model_changed (XfceItemListModel *model)
{
  g_return_if_fail (XFCE_IS_ITEM_LIST_MODEL (model));
  gint n_items = xfce_item_list_model_get_n_items (model);
  for (gint i = 0; i < n_items; ++i)
    {
      GtkTreePath *path = gtk_tree_path_new_from_indices (i, -1);
      GtkTreeIter iter;
      gtk_tree_model_get_iter (GTK_TREE_MODEL (model), &iter, path);
      gtk_tree_model_row_changed (GTK_TREE_MODEL (model), path, &iter);
      gtk_tree_path_free (path);
    }
}



#define __XFCE_ITEM_LIST_MODEL_C__
#include "libxfce4ui-visibility.c"
