/* vi:set expandtab sw=2 sts=2: */
/*
 * Copyright (c) 2022 Sergios - Anestis Kefalidis <sergioskefalidis@gmail.com>
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
 * License along with this library; if not, write to the
 * Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 * Boston, MA 02110-1301, USA.
 */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <gdk/gdkkeysyms.h>
#include <gdk/gdk.h>
#include <gdk/gdkx.h>
#include <gtk/gtk.h>

#include <libxfce4util/libxfce4util.h>
#include <libxfce4ui/libxfce4ui.h>
#include <libxfce4ui/xfce-settings-cell-renderer.h>

#include <xfconf/xfconf.h>


enum
{
    PROP_COLUMN_NAME,
    PROP_COLUMN_TYPE_NAME,
    PROP_COLUMN_VALUE,
    PROP_COLUMN_LOCKED,
    N_PROP_COLUMNS
};



static void     xfce_settings_editor_finalize                 (GObject              *object);
static void     xfce_settings_editor_create_contents          (XfceSettingsEditor   *editor);
static void     xfce_settings_editor_load_properties          (XfceSettingsEditor   *editor);

static const gchar *xfce_settings_editor_box_type_name        (const GValue         *value);
static void         xfce_settings_editor_box_value_changed    (GtkCellRenderer      *renderer,
                                                               const gchar          *str_path,
                                                               const GValue         *new_value,
                                                               XfceSettingsEditor   *self);



struct _XfceSettingsEditorClass
{
  GtkVBoxClass __parent__;
};

struct _XfceSettingsEditor
{
  GtkVBox       __parent__;

  XfconfChannel  *channel;
  gchar         **properties;

  GtkWidget      *treeview;
  GtkTreeStore   *model;
};



G_DEFINE_TYPE (XfceSettingsEditor, xfce_settings_editor, GTK_TYPE_BOX)



static void
xfce_settings_editor_class_init (XfceSettingsEditorClass *klass)
{
  GObjectClass *gobject_class;

  /* Make sure to use the translations from libxfce4ui */
  bindtextdomain (GETTEXT_PACKAGE, PACKAGE_LOCALE_DIR);
#ifdef HAVE_BIND_TEXTDOMAIN_CODESET
  bind_textdomain_codeset (GETTEXT_PACKAGE, "UTF-8");
#endif

  gobject_class = G_OBJECT_CLASS (klass);
  gobject_class->finalize = xfce_settings_editor_finalize;
}



static void
xfce_settings_editor_init (XfceSettingsEditor *editor)
{
  ;
}



static void
xfce_settings_editor_finalize (GObject *object)
{
  ;
}



/**
 * xfce_shortcuts_editor_new:
 * @argument_count : #int, the number of arguments, including this one.
 *
 * A variable arguments version of xfce_shortcuts_editor_new_variadic.
 *
 * Since: 4.17.2
 **/
GtkWidget*
xfce_settings_editor_new (gchar *channel,
                          int    argument_count,
                          ...)
{
  GtkWidget *editor;
  va_list    argument_list;

  va_start (argument_list, argument_count);

  editor = xfce_settings_editor_new_variadic (channel, argument_count, argument_list);

  va_end (argument_list);

  return editor;
}



/**
 * xfce_shortcuts_editor_new_variadic:
 * @argument_count : #int, the number of arguments, including this one.
 * @argument_list : a #va_list containing the arguments
 *
 * Create a new Settings Editor.
 *
 * Since: 4.17.2
 **/
GtkWidget*
xfce_settings_editor_new_variadic (gchar   *channel,
                                   int      argument_count,
                                   va_list  argument_list)
{
  XfceSettingsEditor *editor;

  editor = g_object_new (XFCE_TYPE_SETTINGS_EDITOR, NULL);

  editor->channel = xfconf_channel_get (channel);
  editor->properties = g_malloc (sizeof (char*) * (argument_count + 1));
  editor->properties[argument_count] = NULL;

  for (int i = 0; i < argument_count; i++)
    editor->properties[i] = g_strdup (va_arg (argument_list, gchar*));

  xfce_settings_editor_create_contents (editor);

  return GTK_WIDGET (editor);
}



static void
xfce_settings_editor_create_contents (XfceSettingsEditor *editor)
{
  GtkAdjustment     *adjustment;
  GtkWidget         *vbox;
  GtkWidget         *frame;
  GtkWidget         *swin;
  GtkCellRenderer   *render;
  GtkTreeViewColumn *column;

  vbox = GTK_WIDGET (editor);

  frame = g_object_new (GTK_TYPE_FRAME, "border-width", 0, "shadow-type", GTK_SHADOW_NONE, NULL);
  gtk_box_pack_start (GTK_BOX (vbox), frame, FALSE, TRUE, 0);
  gtk_widget_show (frame);

  swin = gtk_scrolled_window_new (NULL, NULL);
  gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (swin), GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
  gtk_widget_set_hexpand (swin, TRUE);
  gtk_widget_set_vexpand (swin, TRUE);
  gtk_container_add (GTK_CONTAINER (frame), swin);
  gtk_widget_show (swin);

  adjustment = gtk_scrolled_window_get_hadjustment (GTK_SCROLLED_WINDOW (swin));
  gtk_adjustment_set_value (adjustment, 0);
  gtk_scrolled_window_set_hadjustment (GTK_SCROLLED_WINDOW (swin), adjustment);

  editor->model = gtk_tree_store_new (N_PROP_COLUMNS,
                                      G_TYPE_STRING,
                                      G_TYPE_STRING,
                                      G_TYPE_VALUE,
                                      G_TYPE_BOOLEAN);
  gtk_tree_sortable_set_sort_column_id (GTK_TREE_SORTABLE (editor->model),
                                        PROP_COLUMN_NAME, GTK_SORT_ASCENDING);

  editor->treeview = gtk_tree_view_new_with_model (GTK_TREE_MODEL (editor->model));
  gtk_tree_view_set_headers_visible (GTK_TREE_VIEW (editor->treeview), TRUE);
  gtk_tree_view_set_headers_clickable (GTK_TREE_VIEW (editor->treeview), FALSE);
  gtk_container_add (GTK_CONTAINER (swin), editor->treeview);
  gtk_widget_show (editor->treeview);

  render = gtk_cell_renderer_text_new ();
  column = gtk_tree_view_column_new_with_attributes (_("Property"), render,
                                                     "text", PROP_COLUMN_NAME,
                                                     NULL);
  gtk_tree_view_column_set_sizing (column, GTK_TREE_VIEW_COLUMN_AUTOSIZE);
  gtk_tree_view_append_column (GTK_TREE_VIEW (editor->treeview), column);

  render = gtk_cell_renderer_text_new ();
  column = gtk_tree_view_column_new_with_attributes (_("Type"), render,
                                                     "text", PROP_COLUMN_TYPE_NAME,
                                                     NULL);
  gtk_tree_view_column_set_sizing (column, GTK_TREE_VIEW_COLUMN_AUTOSIZE);
  gtk_tree_view_append_column (GTK_TREE_VIEW (editor->treeview), column);

  render = xfce_settings_cell_renderer_new ();
  column = gtk_tree_view_column_new_with_attributes (_("Value"), render,
                                                     "value", PROP_COLUMN_VALUE,
                                                     "locked", PROP_COLUMN_LOCKED,
                                                     NULL);
  gtk_tree_view_column_set_sizing (column, GTK_TREE_VIEW_COLUMN_AUTOSIZE);
  gtk_tree_view_append_column (GTK_TREE_VIEW (editor->treeview), column);

  g_signal_connect (G_OBJECT (render), "value-changed", G_CALLBACK (xfce_settings_editor_box_value_changed), editor);

  gtk_tree_view_set_enable_search (GTK_TREE_VIEW (editor->treeview), TRUE);
  gtk_tree_view_set_search_column (GTK_TREE_VIEW (editor->treeview), PROP_COLUMN_NAME);

  xfce_settings_editor_load_properties (editor);
}



static void
xfce_settings_editor_load_properties (XfceSettingsEditor *editor)
{
  GtkTreeIter iter;

  g_signal_handlers_disconnect_matched (G_OBJECT (editor->channel), G_SIGNAL_MATCH_FUNC, 0, 0, NULL, xfce_settings_editor_load_properties, NULL);

  gtk_tree_store_clear (editor->model);

  for (guint i = 0; i < g_strv_length (editor->properties); i++)
    {
      GValue value = G_VALUE_INIT;

      xfconf_channel_get_property (editor->channel, editor->properties[i], &value);
      gtk_tree_store_append (editor->model, &iter, NULL);
      gtk_tree_store_set (editor->model, &iter,
                          PROP_COLUMN_NAME, g_strdup (editor->properties[i]),
                          PROP_COLUMN_TYPE_NAME, xfce_settings_editor_box_type_name (&value),
                          PROP_COLUMN_VALUE, &value,
                          PROP_COLUMN_LOCKED, FALSE,
                          -1);
    }

  g_signal_connect_swapped (G_OBJECT (editor->channel), "property-changed", G_CALLBACK (xfce_settings_editor_load_properties), editor);
}



static const gchar *
xfce_settings_editor_box_type_name (const GValue *value)
{
  if (G_UNLIKELY (value == NULL))
    return _("Empty");

  if (G_UNLIKELY (G_VALUE_TYPE (value) == xfce_settings_array_type ()))
    return _("Array");

  switch (G_VALUE_TYPE (value))
    {
      case G_TYPE_STRING:
        return _("String");

      /* show non-technical name here, the tooltip
       * contains the full type name */
      case G_TYPE_INT:
      case G_TYPE_UINT:
      case G_TYPE_INT64:
      case G_TYPE_UINT64:
        return _("Integer");

      case G_TYPE_BOOLEAN:
        return _("Boolean");

      case G_TYPE_DOUBLE:
        return _("Double");

      default:
        return G_VALUE_TYPE_NAME (value);
    }
}



static void
xfce_settings_editor_box_value_changed (GtkCellRenderer          *renderer,
                                        const gchar              *str_path,
                                        const GValue             *new_value,
                                        XfceSettingsEditor       *self)
{
  GtkTreeModel     *model = GTK_TREE_MODEL (self->model);
  GtkTreePath      *path;
  GtkTreeIter       iter;
  gchar            *property;
  GtkTreeSelection *selection;

  g_return_if_fail (G_IS_VALUE (new_value));
  g_return_if_fail (XFCE_IS_SETTINGS_EDITOR (self));

  /* only change values on selected paths, this to avoid miss clicking */
  path = gtk_tree_path_new_from_string (str_path);
  selection = gtk_tree_view_get_selection (GTK_TREE_VIEW (self->treeview));
  if (gtk_tree_selection_path_is_selected (selection, path)
      && gtk_tree_model_get_iter (model, &iter, path))
    {
      gtk_tree_model_get (model, &iter, PROP_COLUMN_NAME, &property, -1);
      if (G_LIKELY (property != NULL))
        {
          if (!xfconf_channel_is_property_locked (self->channel, property))
            xfconf_channel_set_property (self->channel, property, new_value);
          g_free (property);
        }
    }
  gtk_tree_path_free (path);
}
