/*-
 * Copyright (c) 2006 Benedikt Meurer <benny@xfce.org>.
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

#include <gtk/gtk.h>
#include <libxfce4util/libxfce4util.h>

#include "xfce-die-command-model.h"



/* Signal identifiers */
enum
{
  LOADED,
  LAST_SIGNAL,
};



static void
xfce_die_command_model_tree_model_init (GtkTreeModelIface *iface);
static void
xfce_die_command_model_finalize (GObject *object);
static GtkTreeModelFlags
xfce_die_command_model_get_flags (GtkTreeModel *tree_model);
static gint
xfce_die_command_model_get_n_columns (GtkTreeModel *tree_model);
static GType
xfce_die_command_model_get_column_type (GtkTreeModel *tree_model,
                                        gint column);
static gboolean
xfce_die_command_model_get_iter (GtkTreeModel *tree_model,
                                 GtkTreeIter *iter,
                                 GtkTreePath *path);
static GtkTreePath *
xfce_die_command_model_get_path (GtkTreeModel *tree_model,
                                 GtkTreeIter *iter);
static void
xfce_die_command_model_get_value (GtkTreeModel *tree_model,
                                  GtkTreeIter *iter,
                                  gint column,
                                  GValue *value);
static gboolean
xfce_die_command_model_iter_next (GtkTreeModel *tree_model,
                                  GtkTreeIter *iter);
static gboolean
xfce_die_command_model_iter_children (GtkTreeModel *tree_model,
                                      GtkTreeIter *iter,
                                      GtkTreeIter *parent);
static gboolean
xfce_die_command_model_iter_has_child (GtkTreeModel *tree_model,
                                       GtkTreeIter *iter);
static gint
xfce_die_command_model_iter_n_children (GtkTreeModel *tree_model,
                                        GtkTreeIter *iter);
static gboolean
xfce_die_command_model_iter_nth_child (GtkTreeModel *tree_model,
                                       GtkTreeIter *iter,
                                       GtkTreeIter *parent,
                                       gint n);
static gboolean
xfce_die_command_model_iter_parent (GtkTreeModel *tree_model,
                                    GtkTreeIter *iter,
                                    GtkTreeIter *child);
static gboolean
xfce_die_command_model_collect_idle (gpointer user_data);
static void
xfce_die_command_model_collect_idle_destroy (gpointer user_data);
static gpointer
xfce_die_command_model_collect_thread (gpointer user_data);



struct _XfceDieCommandModel
{
  GObject __parent__;

  gint stamp;
  GSList *items;

  gint collect_idle_id;
  GSList *collect_items;
  GThread *collect_thread;
  volatile gboolean collect_cancelled;
};



static guint command_model_signals[LAST_SIGNAL];



G_DEFINE_TYPE_WITH_CODE (XfceDieCommandModel, xfce_die_command_model, G_TYPE_OBJECT,
                         G_IMPLEMENT_INTERFACE (GTK_TYPE_TREE_MODEL, xfce_die_command_model_tree_model_init))



static void
xfce_die_command_model_class_init (XfceDieCommandModelClass *klass)
{
  GObjectClass *gobject_class;

  gobject_class = G_OBJECT_CLASS (klass);
  gobject_class->finalize = xfce_die_command_model_finalize;

  /**
   * XfceDieCommandModel::loaded:
   * @command_model : an #XfceDieCommandModel.
   *
   * Emitted by the @command_model once the completion
   * data is loaded from the disk.
   **/
  command_model_signals[LOADED] =
    g_signal_new (I_ ("loaded"),
                  G_TYPE_FROM_CLASS (klass),
                  G_SIGNAL_RUN_LAST,
                  0, NULL, NULL,
                  g_cclosure_marshal_VOID__VOID,
                  G_TYPE_NONE, 0);
}



static void
xfce_die_command_model_tree_model_init (GtkTreeModelIface *iface)
{
  iface->get_flags = xfce_die_command_model_get_flags;
  iface->get_n_columns = xfce_die_command_model_get_n_columns;
  iface->get_column_type = xfce_die_command_model_get_column_type;
  iface->get_iter = xfce_die_command_model_get_iter;
  iface->get_path = xfce_die_command_model_get_path;
  iface->get_value = xfce_die_command_model_get_value;
  iface->iter_next = xfce_die_command_model_iter_next;
  iface->iter_children = xfce_die_command_model_iter_children;
  iface->iter_has_child = xfce_die_command_model_iter_has_child;
  iface->iter_n_children = xfce_die_command_model_iter_n_children;
  iface->iter_nth_child = xfce_die_command_model_iter_nth_child;
  iface->iter_parent = xfce_die_command_model_iter_parent;
}



static void
xfce_die_command_model_init (XfceDieCommandModel *command_model)
{
  /* generate a unique stamp */
  command_model->stamp = g_random_int ();

  /* spawn the collector thread */
  command_model->collect_thread =
    g_thread_new ("XfceCommandCollect", xfce_die_command_model_collect_thread, command_model);
}



static void
xfce_die_command_model_finalize (GObject *object)
{
  XfceDieCommandModel *command_model = XFCE_DIE_COMMAND_MODEL (object);

  /* join the collector thread */
  command_model->collect_cancelled = TRUE;
  g_thread_join (command_model->collect_thread);

  /* cancel any pending collect idle source */
  if (G_UNLIKELY (command_model->collect_idle_id > 0))
    g_source_remove (command_model->collect_idle_id);

  /* release collected items (if any) */
  g_slist_free_full (command_model->collect_items, g_free);

  /* release all items */
  g_slist_free_full (command_model->items, g_free);

  G_OBJECT_CLASS (xfce_die_command_model_parent_class)->finalize (object);
}



static GtkTreeModelFlags
xfce_die_command_model_get_flags (GtkTreeModel *tree_model)
{
  return GTK_TREE_MODEL_ITERS_PERSIST | GTK_TREE_MODEL_LIST_ONLY;
}



static gint
xfce_die_command_model_get_n_columns (GtkTreeModel *tree_model)
{
  return XFCE_DIE_COMMAND_MODEL_N_COLUMNS;
}



static GType
xfce_die_command_model_get_column_type (GtkTreeModel *tree_model,
                                        gint column)
{
  switch (column)
    {
    case XFCE_DIE_COMMAND_MODEL_COLUMN_NAME:
      return G_TYPE_STRING;

    default:
      g_assert_not_reached ();
      return G_TYPE_INVALID;
    }
}



static gboolean
xfce_die_command_model_get_iter (GtkTreeModel *tree_model,
                                 GtkTreeIter *iter,
                                 GtkTreePath *path)
{
  XfceDieCommandModel *command_model = XFCE_DIE_COMMAND_MODEL (tree_model);

  g_return_val_if_fail (XFCE_IS_DIE_COMMAND_MODEL (command_model), FALSE);
  g_return_val_if_fail (gtk_tree_path_get_depth (path) > 0, FALSE);

  iter->stamp = command_model->stamp;
  iter->user_data = g_slist_nth (command_model->items, gtk_tree_path_get_indices (path)[0]);

  return (iter->user_data != NULL);
}



static GtkTreePath *
xfce_die_command_model_get_path (GtkTreeModel *tree_model,
                                 GtkTreeIter *iter)
{
  XfceDieCommandModel *command_model = XFCE_DIE_COMMAND_MODEL (tree_model);
  gint idx;

  g_return_val_if_fail (XFCE_IS_DIE_COMMAND_MODEL (command_model), NULL);
  g_return_val_if_fail (iter->stamp == command_model->stamp, NULL);

  /* determine the index of the iter */
  idx = g_slist_position (command_model->items, iter->user_data);
  if (G_UNLIKELY (idx < 0))
    return NULL;

  return gtk_tree_path_new_from_indices (idx, -1);
}



static void
xfce_die_command_model_get_value (GtkTreeModel *tree_model,
                                  GtkTreeIter *iter,
                                  gint column,
                                  GValue *value)
{
  XfceDieCommandModel *command_model = XFCE_DIE_COMMAND_MODEL (tree_model);

  g_return_if_fail (XFCE_IS_DIE_COMMAND_MODEL (command_model));
  g_return_if_fail (iter->stamp == command_model->stamp);

  switch (column)
    {
    case XFCE_DIE_COMMAND_MODEL_COLUMN_NAME:
      g_value_init (value, G_TYPE_STRING);
      g_value_set_static_string (value, g_slist_nth_data (iter->user_data, 0));
      break;

    default:
      g_assert_not_reached ();
      break;
    }
}



static gboolean
xfce_die_command_model_iter_next (GtkTreeModel *tree_model,
                                  GtkTreeIter *iter)
{
  g_return_val_if_fail (XFCE_IS_DIE_COMMAND_MODEL (tree_model), FALSE);
  g_return_val_if_fail (iter->stamp == XFCE_DIE_COMMAND_MODEL (tree_model)->stamp, FALSE);

  iter->user_data = g_slist_next (iter->user_data);
  return (iter->user_data != NULL);
}



static gboolean
xfce_die_command_model_iter_children (GtkTreeModel *tree_model,
                                      GtkTreeIter *iter,
                                      GtkTreeIter *parent)
{
  XfceDieCommandModel *command_model = XFCE_DIE_COMMAND_MODEL (tree_model);

  g_return_val_if_fail (XFCE_IS_DIE_COMMAND_MODEL (command_model), FALSE);

  if (G_LIKELY (parent == NULL && command_model->items != NULL))
    {
      iter->stamp = command_model->stamp;
      iter->user_data = command_model->items;
      return TRUE;
    }

  return FALSE;
}



static gboolean
xfce_die_command_model_iter_has_child (GtkTreeModel *tree_model,
                                       GtkTreeIter *iter)
{
  return FALSE;
}



static gint
xfce_die_command_model_iter_n_children (GtkTreeModel *tree_model,
                                        GtkTreeIter *iter)
{
  XfceDieCommandModel *command_model = XFCE_DIE_COMMAND_MODEL (tree_model);

  g_return_val_if_fail (XFCE_IS_DIE_COMMAND_MODEL (command_model), 0);

  return (iter == NULL) ? g_slist_length (command_model->items) : 0;
}



static gboolean
xfce_die_command_model_iter_nth_child (GtkTreeModel *tree_model,
                                       GtkTreeIter *iter,
                                       GtkTreeIter *parent,
                                       gint n)
{
  XfceDieCommandModel *command_model = XFCE_DIE_COMMAND_MODEL (tree_model);

  g_return_val_if_fail (XFCE_IS_DIE_COMMAND_MODEL (command_model), FALSE);

  if (G_LIKELY (parent != NULL))
    {
      iter->stamp = command_model->stamp;
      iter->user_data = g_slist_nth (command_model->items, n);
      return (iter->user_data != NULL);
    }

  return FALSE;
}



static gboolean
xfce_die_command_model_iter_parent (GtkTreeModel *tree_model,
                                    GtkTreeIter *iter,
                                    GtkTreeIter *child)
{
  return FALSE;
}



static gboolean
xfce_die_command_model_collect_idle (gpointer user_data)
{
  XfceDieCommandModel *command_model = XFCE_DIE_COMMAND_MODEL (user_data);
  GtkTreePath *path;
  GtkTreeIter iter;
  GSList *lp;
  GSList *np;

  g_return_val_if_fail (XFCE_IS_DIE_COMMAND_MODEL (command_model), FALSE);
  g_return_val_if_fail (command_model->items == NULL, FALSE);

  /* move the collected items "online" */
  command_model->items = command_model->collect_items;
  command_model->collect_items = NULL;

  /* emit notifications for all new items */
  path = gtk_tree_path_new_first ();
  for (lp = command_model->items; lp != NULL; lp = lp->next)
    {
      /* remember the next item */
      np = lp->next;
      lp->next = NULL;

      /* generate the iterator */
      iter.stamp = command_model->stamp;
      iter.user_data = lp;

      /* emit the "row-inserted" signal */
      gtk_tree_model_row_inserted (GTK_TREE_MODEL (command_model), path, &iter);

      /* advance the path */
      gtk_tree_path_next (path);

      /* reset the next item */
      lp->next = np;
    }
  gtk_tree_path_free (path);

  /* tell the consumer that we are loaded */
  g_signal_emit (G_OBJECT (command_model), command_model_signals[LOADED], 0);

  return FALSE;
}



static void
xfce_die_command_model_collect_idle_destroy (gpointer user_data)
{
  XFCE_DIE_COMMAND_MODEL (user_data)->collect_idle_id = 0;
}



static gpointer
xfce_die_command_model_collect_thread (gpointer user_data)
{
  XfceDieCommandModel *command_model = XFCE_DIE_COMMAND_MODEL (user_data);
  const gchar *name;
  GSList *items = NULL;
  GSList *lp;
  gchar *filename;
  gchar **paths;
  gchar *path;
  guint n;
  GDir *dp;

  /* split the $PATH */
  paths = g_strsplit (g_getenv ("PATH"), G_SEARCHPATH_SEPARATOR_S, -1);
  if (G_UNLIKELY (paths == NULL))
    return NULL;

  /* process all directories in $PATH */
  for (n = 0; !command_model->collect_cancelled && paths[n] != NULL; ++n)
    {
      /* try to open the directory */
      dp = g_dir_open (paths[n], 0, NULL);
      if (G_UNLIKELY (dp == NULL))
        continue;

      /* process the directory */
      while (!command_model->collect_cancelled)
        {
          /* read the next file name */
          name = g_dir_read_name (dp);
          if (G_UNLIKELY (name == NULL))
            break;

          /* convert the name to valid UTF-8 */
          filename = g_filename_display_name (name);

          /* test if we already have that item */
          for (lp = items; lp != NULL; lp = lp->next)
            if (g_ascii_strcasecmp (lp->data, filename) == 0)
              break;

          /* determine info if we don't have it already */
          if (G_LIKELY (lp == NULL))
            {
              /* determine the absolute path to the file */
              path = g_build_filename (paths[n], name, NULL);

              /* check if the path refers to an executable */
              if (g_file_test (path, G_FILE_TEST_IS_EXECUTABLE))
                {
                  /* insert the file into the item list */
                  items = g_slist_insert_sorted (items, filename, (GCompareFunc) g_ascii_strcasecmp);

                  /* the filename is now owned by the list */
                  filename = NULL;
                }

              /* release absolute path */
              g_free (path);
            }

          /* release filename */
          g_free (filename);
        }

      /* close the directory */
      g_dir_close (dp);
    }

  /* check if we're still active */
  if (G_LIKELY (!command_model->collect_cancelled && items != NULL))
    {
      /* tell the model about the items */
      command_model->collect_items = items;

      /* and schedule an idle source */
      command_model->collect_idle_id =
        gdk_threads_add_idle_full (G_PRIORITY_LOW, xfce_die_command_model_collect_idle,
                                   command_model, xfce_die_command_model_collect_idle_destroy);
    }
  else
    {
      /* release the collected items */
      g_slist_free_full (items, g_free);
    }

  /* cleanup */
  g_strfreev (paths);

  return NULL;
}



/**
 * xfce_die_command_model_new:
 *
 * Allocates a new #XfceDieCommandModel instance.
 *
 * Return value: the newly allocated #XfceDieCommandModel.
 **/
XfceDieCommandModel *
xfce_die_command_model_new (void)
{
  return g_object_new (XFCE_TYPE_DIE_COMMAND_MODEL, NULL);
}
