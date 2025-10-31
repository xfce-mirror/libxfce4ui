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

#include <gobject/gvaluecollector.h>

#include "xfce-item-list-store.h"
#include "libxfce4ui-visibility.h"



/**
 * SECTION: xfce-item-list-store
 * @title: XfceItemListStore
 * @short_description: Store for #XfceItemListModel
 * @include: libxfce4ui/libxfce4ui.h
 *
 * This class copies the #GtkListStore interface, it also supports the #GtkTreeModel interface,
 * but is intended to be used as an #XfceItemListModel.
 *
 * Since: 4.21.2
 **/



typedef struct _XfceItemListStoreItem XfceItemListStoreItem;

struct _XfceItemListStoreItem
{
  gint n_columns;
  GValue *columns;
};

struct _XfceItemListStore
{
  XfceItemListModel __parent__;

  gint n_columns;
  GType *column_types;
  GList *items;
};



static void
xfce_item_list_store_finalize (GObject *object);

static void
xfce_item_list_store_append_column (XfceItemListStore *store,
                                    GType column_type);

static XfceItemListStoreItem *
xfce_item_list_store_item_new (XfceItemListStore *store);

static void
xfce_item_list_store_item_free (XfceItemListStoreItem *item);

static gint
xfce_item_list_store_get_list_n_columns (XfceItemListModel *model);

static GType
xfce_item_list_store_get_list_column_type (XfceItemListModel *model,
                                           gint column);

static gint
xfce_item_list_store_get_n_items (XfceItemListModel *model);

static void
xfce_item_list_store_get_item_value (XfceItemListModel *model,
                                     gint index,
                                     gint column,
                                     GValue *value);

static void
xfce_item_list_store_move (XfceItemListModel *model,
                           gint source_index,
                           gint dest_index);

static void
xfce_item_list_store_set_activity (XfceItemListModel *model,
                                   gint index,
                                   gboolean value);

static gboolean
xfce_item_list_store_remove (XfceItemListModel *model,
                             gint index);

static void
xfce_item_list_store_set_va (XfceItemListStore *store,
                             gint index,
                             va_list arglist);


G_DEFINE_TYPE (XfceItemListStore, xfce_item_list_store, XFCE_TYPE_ITEM_LIST_MODEL)



static void
xfce_item_list_store_class_init (XfceItemListStoreClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);
  XfceItemListModelClass *model_class = XFCE_ITEM_LIST_MODEL_CLASS (klass);

  object_class->finalize = xfce_item_list_store_finalize;

  model_class->get_list_n_columns = xfce_item_list_store_get_list_n_columns;
  model_class->get_list_column_type = xfce_item_list_store_get_list_column_type;
  model_class->get_n_items = xfce_item_list_store_get_n_items;
  model_class->get_item_value = xfce_item_list_store_get_item_value;
  model_class->move = xfce_item_list_store_move;
  model_class->set_activity = xfce_item_list_store_set_activity;
  model_class->remove = xfce_item_list_store_remove;
}



static void
xfce_item_list_store_init (XfceItemListStore *store)
{
  store->n_columns = XFCE_ITEM_LIST_MODEL_COLUMN_USER;
  store->column_types = g_new (GType, store->n_columns);

  for (gint i = 0; i < store->n_columns; ++i)
    {
      store->column_types[i] = XFCE_ITEM_LIST_MODEL_CLASS (xfce_item_list_store_parent_class)
                                 ->get_list_column_type (XFCE_ITEM_LIST_MODEL (store), i);
    }
}



static void
xfce_item_list_store_finalize (GObject *object)
{
  XfceItemListStore *store = XFCE_ITEM_LIST_STORE (object);

  g_free (store->column_types);
  g_list_free_full (store->items, (GDestroyNotify) xfce_item_list_store_item_free);
  G_OBJECT_CLASS (xfce_item_list_store_parent_class)->finalize (object);
}



static void
xfce_item_list_store_append_column (XfceItemListStore *store,
                                    GType column_type)
{
  store->n_columns = store->n_columns + 1;
  store->column_types = g_renew (GType, store->column_types, store->n_columns);
  store->column_types[store->n_columns - 1] = column_type;
}



static XfceItemListStoreItem *
xfce_item_list_store_item_new (XfceItemListStore *store)
{
  XfceItemListModel *model = XFCE_ITEM_LIST_MODEL (store);
  XfceItemListStoreItem *item = g_new (XfceItemListStoreItem, 1);

  item->n_columns = store->n_columns;
  item->columns = g_new0 (GValue, item->n_columns);

  for (gint i = 0; i < item->n_columns; ++i)
    g_value_init (&item->columns[i], xfce_item_list_model_get_list_column_type (model, i));

  g_value_set_boolean (&item->columns[XFCE_ITEM_LIST_MODEL_COLUMN_ACTIVE], TRUE);
  g_value_set_boolean (&item->columns[XFCE_ITEM_LIST_MODEL_COLUMN_REMOVABLE], TRUE);

  return item;
}



static void
xfce_item_list_store_item_free (XfceItemListStoreItem *item)
{
  if (item == NULL)
    return;

  for (gint i = 0; i < item->n_columns; ++i)
    g_value_reset (&item->columns[i]);

  g_free (item->columns);
  g_free (item);
}



static gint
xfce_item_list_store_get_list_n_columns (XfceItemListModel *model)
{
  XfceItemListStore *store = XFCE_ITEM_LIST_STORE (model);

  return store->n_columns;
}



static GType
xfce_item_list_store_get_list_column_type (XfceItemListModel *model,
                                           gint column)
{
  XfceItemListStore *store = XFCE_ITEM_LIST_STORE (model);

  g_return_val_if_fail (column >= 0 && column < store->n_columns, G_TYPE_NONE);

  return store->column_types[column];
}



static gint
xfce_item_list_store_get_n_items (XfceItemListModel *model)
{
  XfceItemListStore *store = XFCE_ITEM_LIST_STORE (model);

  return g_list_length (store->items);
}



static void
xfce_item_list_store_get_item_value (XfceItemListModel *model,
                                     gint index,
                                     gint column,
                                     GValue *value)
{
  XfceItemListStore *store = XFCE_ITEM_LIST_STORE (model);
  XfceItemListStoreItem *item = g_list_nth_data (store->items, index);

  g_value_copy (&item->columns[column], value);
}



static void
xfce_item_list_store_move (XfceItemListModel *model,
                           gint source_index,
                           gint dest_index)
{
  XfceItemListStore *store = XFCE_ITEM_LIST_STORE (model);
  GList *link = g_list_nth (store->items, source_index);
  XfceItemListStoreItem *item = link->data;

  store->items = g_list_delete_link (store->items, link);
  store->items = g_list_insert (store->items, item, dest_index);
}



static void
xfce_item_list_store_set_activity (XfceItemListModel *model,
                                   gint index,
                                   gboolean value)
{
  XfceItemListStore *store = XFCE_ITEM_LIST_STORE (model);

  xfce_item_list_store_set (store, index, XFCE_ITEM_LIST_MODEL_COLUMN_ACTIVE, value, -1);
}



static gboolean
xfce_item_list_store_remove (XfceItemListModel *model,
                             gint index)
{
  XfceItemListStore *store = XFCE_ITEM_LIST_STORE (model);
  GList *link = g_list_nth (store->items, index);
  XfceItemListStoreItem *item = link->data;

  store->items = g_list_delete_link (store->items, link);
  xfce_item_list_store_item_free (item);

  return TRUE;
}



static void
xfce_item_list_store_set_va (XfceItemListStore *store,
                             gint index,
                             va_list arglist)
{
  XfceItemListStoreItem *item = g_list_nth_data (store->items, index);

  while (TRUE)
    {
      gint column_index = va_arg (arglist, gint);

      g_return_if_fail (column_index >= -1 && column_index < store->n_columns);

      if (column_index == -1)
        {
          break;
        }
      else
        {
          gchar *error_text = NULL;

          g_value_reset (&item->columns[column_index]);
          G_VALUE_COLLECT_INIT (&item->columns[column_index],
                                store->column_types[column_index],
                                arglist,
                                0,
                                &error_text);
          if (error_text != NULL)
            {
              g_warning ("column %d, %s", column_index, error_text);
              g_free (error_text);
            }
        }
    }

  /* GtkTreeModel signal */
  GtkTreePath *path = gtk_tree_path_new_from_indices (index, -1);
  GtkTreeIter iter;

  xfce_item_list_model_set_index (XFCE_ITEM_LIST_MODEL (store), &iter, index);
  gtk_tree_model_row_changed (GTK_TREE_MODEL (store), path, &iter);
  gtk_tree_path_free (path);
}



/**
 * xfce_item_list_store_new:
 * @n_columns: Number of columns in the model, if you use only standard ones, specify -1
 * @...: Column types
 *
 * Returns: (transfer full): #XfceItemListStore
 *
 * Since: 4.21.2
 **/
XfceItemListStore *
xfce_item_list_store_new (gint n_columns,
                          ...)
{
  XfceItemListStore *store = g_object_new (XFCE_TYPE_ITEM_LIST_STORE, NULL);

  if (n_columns == -1)
    n_columns = XFCE_ITEM_LIST_MODEL_COLUMN_USER;

  if (n_columns >= XFCE_ITEM_LIST_MODEL_COLUMN_USER)
    {
      va_list arglist;

      va_start (arglist, n_columns);
      for (gint i = XFCE_ITEM_LIST_MODEL_COLUMN_USER; i < n_columns; ++i)
        xfce_item_list_store_append_column (store, va_arg (arglist, GType));
      va_end (arglist);
    }
  else
    {
      g_warning ("Too few columns specified");
    }

  return store;
}



/**
 * xfce_item_list_store_clear:
 * @store: #XfceItemListStore
 *
 * Removes all items.
 *
 * Since: 4.21.2
 **/
void
xfce_item_list_store_clear (XfceItemListStore *store)
{
  g_return_if_fail (XFCE_IS_ITEM_LIST_STORE (store));

  gint prev_n_items = g_list_length (store->items);

  for (gint i = prev_n_items - 1; i >= 0; --i)
    xfce_item_list_model_remove (XFCE_ITEM_LIST_MODEL (store), i);
}



/**
 * xfce_item_list_store_insert:
 * @store: #XfceItemListStore
 * @index: Index to insert, or -1 to insert at the end
 *
 * Inserts an empty item at the specified index.
 *
 * Returns: The index at which the item was inserted
 *
 * Since: 4.21.2
 **/
gint
xfce_item_list_store_insert (XfceItemListStore *store,
                             gint index)
{
  g_return_val_if_fail (XFCE_IS_ITEM_LIST_STORE (store), -1);
  g_return_val_if_fail (index >= -1 && index <= (gint) g_list_length (store->items), -1);

  XfceItemListStoreItem *item = xfce_item_list_store_item_new (store);

  if (index == -1)
    index = g_list_length (store->items);
  store->items = g_list_insert (store->items, item, index);

  GtkTreePath *path = gtk_tree_path_new_from_indices (index, -1);
  GtkTreeIter iter;

  xfce_item_list_model_set_index (XFCE_ITEM_LIST_MODEL (store), &iter, index);
  gtk_tree_model_row_inserted (GTK_TREE_MODEL (store), path, &iter);
  gtk_tree_path_free (path);

  return index;
}



/**
 * xfce_item_list_store_set:
 * @store: #XfceItemListStore
 * @index: Item index
 * @...: Indexes and column values
 *
 * Sets the value of the item's columns at a specific index; the column index must be -1 at the end.
 *
 * Since: 4.21.2
 **/
void
xfce_item_list_store_set (XfceItemListStore *store,
                          gint index,
                          ...)
{
  g_return_if_fail (XFCE_IS_ITEM_LIST_STORE (store));
  g_return_if_fail (index >= 0 && index < (gint) g_list_length (store->items));

  va_list arglist;

  va_start (arglist, index);
  xfce_item_list_store_set_va (store, index, arglist);
  va_end (arglist);
}



/**
 * xfce_item_list_store_insert_with_values:
 * @store: #XfceItemListStore
 * @index: Index of the item to insert, or -1 to insert at the end
 * @...: Indexes and column values
 *
 * This is a function that combines #xfce_item_list_store_insert and #xfce_item_list_store_set.
 *
 * Returns: The index at which the new item was inserted
 *
 * Since: 4.21.2
 **/
gint
xfce_item_list_store_insert_with_values (XfceItemListStore *store,
                                         gint index,
                                         ...)
{
  g_return_val_if_fail (XFCE_IS_ITEM_LIST_STORE (store), -1);

  va_list arglist;

  va_start (arglist, index);
  index = xfce_item_list_store_insert (store, index);
  xfce_item_list_store_set_va (store, index, arglist);
  va_end (arglist);
  return index;
}



#define __XFCE_ITEM_LIST_STORE_C__
#include "libxfce4ui-visibility.c"
