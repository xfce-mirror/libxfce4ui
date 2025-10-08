/*-
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

#include <libxfce4util/libxfce4util.h>

#include "libxfce4ui-private.h"
#include "xfce-item-list-view.h"
#include "libxfce4ui-visibility.h"



/**
 * SECTION: xfce-item-list-view
 * @title: XfceItemListView
 * @short_description: #GtkTreeView with toolbar and ability to move elements
 * @include: libxfce4ui/libxfce4ui.h
 **/



struct _XfceItemListView
{
  GtkBoxClass __parent__;

  XfceItemListModel *model;
  GMenu *menu;
  GtkWidget *tree_view;

  GtkWidget *vbuttons;
  GtkWidget *hbuttons;

  GSimpleAction *up_action;
  GSimpleAction *down_action;
  GSimpleAction *edit_action;
  GSimpleAction *add_action;
  GSimpleAction *remove_action;
};

enum
{
  PROP_0,
  PROP_MODEL,
  PROP_MENU,
  PROP_TREE_VIEW,
};



static void
xfce_item_list_view_finalize (GObject *object);

static void
xfce_item_list_view_set_property (GObject *object,
                                  guint prop_id,
                                  const GValue *value,
                                  GParamSpec *pspec);

static void
xfce_item_list_view_get_property (GObject *object,
                                  guint prop_id,
                                  GValue *value,
                                  GParamSpec *pspec);

static void
xfce_item_list_view_set_model (XfceItemListView *view,
                               XfceItemListModel *model);

static void
xfce_item_list_view_render_buttons (XfceItemListView *view);

static void
xfce_item_list_view_update_actions (XfceItemListView *view);

static void
xfce_item_list_view_item_up (XfceItemListView *view);

static void
xfce_item_list_view_item_down (XfceItemListView *view);

static void
xfce_item_list_view_toggle_item (XfceItemListView *view,
                                 const gchar *path_string);

static void
xfce_item_list_view_edit_item (XfceItemListView *view);

static void
xfce_item_list_view_add_item (XfceItemListView *view);

static void
xfce_item_list_view_remove_item (XfceItemListView *view);

static gboolean
xfce_item_list_view_tree_pressed (XfceItemListView *view,
                                  GdkEventButton *event);

static void
xfce_item_list_view_row_activate (XfceItemListView *view);



G_DEFINE_FINAL_TYPE (XfceItemListView, xfce_item_list_view, GTK_TYPE_BOX)



static void
xfce_item_list_view_class_init (XfceItemListViewClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

  object_class->finalize = xfce_item_list_view_finalize;
  object_class->set_property = xfce_item_list_view_set_property;
  object_class->get_property = xfce_item_list_view_get_property;

  g_object_class_install_property (object_class,
                                   PROP_MODEL,
                                   g_param_spec_object ("model", NULL, NULL,
                                                        XFCE_TYPE_ITEM_LIST_MODEL,
                                                        G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));

  g_object_class_install_property (object_class,
                                   PROP_MENU,
                                   g_param_spec_object ("menu", NULL, NULL,
                                                        G_TYPE_MENU,
                                                        G_PARAM_READABLE | G_PARAM_STATIC_STRINGS));

  g_object_class_install_property (object_class,
                                   PROP_TREE_VIEW,
                                   g_param_spec_object ("tree-view", NULL, NULL,
                                                        GTK_TYPE_TREE_VIEW,
                                                        G_PARAM_READABLE | G_PARAM_STATIC_STRINGS));
}



static void
xfce_item_list_view_init (XfceItemListView *view)
{
  GtkWidget *vbox, *scrwin;
  GtkTreeSelection *selection;
  GtkCellRenderer *renderer;
  GSimpleActionGroup *group;

  g_object_set (view, "orientation", GTK_ORIENTATION_HORIZONTAL, "spacing", 6, NULL);

  vbox = gtk_box_new (GTK_ORIENTATION_VERTICAL, 0);
  gtk_box_pack_start (GTK_BOX (view), vbox, TRUE, TRUE, 0);
  gtk_widget_show (vbox);

  scrwin = gtk_scrolled_window_new (NULL, NULL);
  gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (scrwin), GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
  gtk_scrolled_window_set_shadow_type (GTK_SCROLLED_WINDOW (scrwin), GTK_SHADOW_IN);
  gtk_widget_set_hexpand (scrwin, TRUE);
  gtk_widget_set_vexpand (scrwin, TRUE);
  gtk_box_pack_start (GTK_BOX (vbox), scrwin, TRUE, TRUE, 0);
  gtk_widget_show (scrwin);

  view->tree_view = gtk_tree_view_new ();
  g_signal_connect_swapped (view->tree_view, "button-press-event", G_CALLBACK (xfce_item_list_view_tree_pressed), view);
  g_signal_connect_swapped (view->tree_view, "row-activated", G_CALLBACK (xfce_item_list_view_row_activate), view);
  gtk_tree_view_set_headers_visible (GTK_TREE_VIEW (view->tree_view), FALSE);
  gtk_tree_view_set_search_column (GTK_TREE_VIEW (view->tree_view), XFCE_ITEM_LIST_MODEL_COLUMN_NAME);
  gtk_tree_view_set_tooltip_column (GTK_TREE_VIEW (view->tree_view), XFCE_ITEM_LIST_MODEL_COLUMN_TOOLTIP);
  gtk_container_add (GTK_CONTAINER (scrwin), view->tree_view);
  gtk_widget_show (view->tree_view);

  selection = gtk_tree_view_get_selection (GTK_TREE_VIEW (view->tree_view));
  g_signal_connect_swapped (selection, "changed", G_CALLBACK (xfce_item_list_view_update_actions), view);

  renderer = gtk_cell_renderer_toggle_new ();
  g_signal_connect_swapped (G_OBJECT (renderer), "toggled", G_CALLBACK (xfce_item_list_view_toggle_item), view);
  gtk_tree_view_insert_column_with_attributes (GTK_TREE_VIEW (view->tree_view), -1, "active", renderer,
                                               "active", XFCE_ITEM_LIST_MODEL_COLUMN_ACTIVE,
                                               "visible", XFCE_ITEM_LIST_MODEL_COLUMN_ACTIVABLE,
                                               NULL);

  renderer = gtk_cell_renderer_pixbuf_new ();
  gtk_tree_view_insert_column_with_attributes (GTK_TREE_VIEW (view->tree_view), -1, "icon", renderer,
                                               "gicon", XFCE_ITEM_LIST_MODEL_COLUMN_ICON,
                                               NULL);

  renderer = gtk_cell_renderer_text_new ();
  gtk_tree_view_insert_column_with_attributes (GTK_TREE_VIEW (view->tree_view), -1, "name", renderer,
                                               "markup", XFCE_ITEM_LIST_MODEL_COLUMN_NAME,
                                               "sensitive", XFCE_ITEM_LIST_MODEL_COLUMN_ACTIVE,
                                               NULL);

  view->vbuttons = gtk_box_new (GTK_ORIENTATION_VERTICAL, 0);
  gtk_style_context_add_class (gtk_widget_get_style_context (view->vbuttons), GTK_STYLE_CLASS_LINKED);
  gtk_box_pack_start (GTK_BOX (view), view->vbuttons, FALSE, FALSE, 0);
  gtk_widget_set_vexpand (view->vbuttons, FALSE);
  gtk_widget_show (view->vbuttons);

  view->hbuttons = gtk_button_box_new (GTK_ORIENTATION_HORIZONTAL);
  gtk_button_box_set_layout (GTK_BUTTON_BOX (view->hbuttons), GTK_BUTTONBOX_START);
  gtk_style_context_add_class (gtk_widget_get_style_context (view->hbuttons), GTK_STYLE_CLASS_INLINE_TOOLBAR);
  gtk_box_set_homogeneous (GTK_BOX (view->hbuttons), FALSE);
  gtk_box_pack_start (GTK_BOX (vbox), view->hbuttons, FALSE, FALSE, 0);
  gtk_widget_show (view->hbuttons);

  group = g_simple_action_group_new ();

  view->up_action = g_simple_action_new ("move-item-up", NULL);
  g_signal_connect_swapped (view->up_action, "activate", G_CALLBACK (xfce_item_list_view_item_up), view);
  g_action_map_add_action (G_ACTION_MAP (group), G_ACTION (view->up_action));

  view->down_action = g_simple_action_new ("move-item-down", NULL);
  g_signal_connect_swapped (view->down_action, "activate", G_CALLBACK (xfce_item_list_view_item_down), view);
  g_action_map_add_action (G_ACTION_MAP (group), G_ACTION (view->down_action));

  view->edit_action = g_simple_action_new ("edit-item", NULL);
  g_signal_connect_swapped (view->edit_action, "activate", G_CALLBACK (xfce_item_list_view_edit_item), view);
  g_action_map_add_action (G_ACTION_MAP (group), G_ACTION (view->edit_action));

  view->add_action = g_simple_action_new ("add-item", NULL);
  g_signal_connect_swapped (view->add_action, "activate", G_CALLBACK (xfce_item_list_view_add_item), view);
  g_action_map_add_action (G_ACTION_MAP (group), G_ACTION (view->add_action));

  view->remove_action = g_simple_action_new ("remove-item", NULL);
  g_signal_connect_swapped (view->remove_action, "activate", G_CALLBACK (xfce_item_list_view_remove_item), view);
  g_action_map_add_action (G_ACTION_MAP (group), G_ACTION (view->remove_action));

  gtk_widget_insert_action_group (GTK_WIDGET (view), "xfce", G_ACTION_GROUP (group));

  view->menu = g_menu_new ();
  g_signal_connect_swapped (view->menu, "items-changed", G_CALLBACK (xfce_item_list_view_render_buttons), view);
}



static void
xfce_item_list_view_finalize (GObject *object)
{
  XfceItemListView *view = XFCE_ITEM_LIST_VIEW (object);

  g_clear_object (&view->menu);
  G_OBJECT_CLASS (xfce_item_list_view_parent_class)->finalize (object);
}



static void
xfce_item_list_view_set_property (GObject *object,
                                  guint prop_id,
                                  const GValue *value,
                                  GParamSpec *pspec)
{
  XfceItemListView *view = XFCE_ITEM_LIST_VIEW (object);

  switch (prop_id)
    {
    case PROP_MODEL:
      xfce_item_list_view_set_model (view, g_value_get_object (value));
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
    }
}



static void
xfce_item_list_view_get_property (GObject *object,
                                  guint prop_id,
                                  GValue *value,
                                  GParamSpec *pspec)
{
  XfceItemListView *view = XFCE_ITEM_LIST_VIEW (object);

  switch (prop_id)
    {
    case PROP_MODEL:
      g_value_set_object (value, view->model);
      break;

    case PROP_MENU:
      g_value_set_object (value, view->menu);
      break;

    case PROP_TREE_VIEW:
      g_value_set_object (value, view->tree_view);
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
    }
}



static void
xfce_item_list_view_set_model (XfceItemListView *view,
                               XfceItemListModel *model)
{
  const char *actions[] = { "xfce.move-item-up", "xfce.move-item-down", "xfce.edit-item", "xfce.add-item", "xfce.remove-item" };
  gint n_items = g_menu_model_get_n_items (G_MENU_MODEL (view->menu));
  GVariant *action;
  GMenuItem *item;
  XfceItemListModelFlags flags;
  gint i, j, index;

  _libxfce4ui_return_if_fail (model == NULL || XFCE_IS_ITEM_LIST_MODEL (model));

  if (view->model != NULL)
    g_signal_handlers_disconnect_by_data (view->model, view);

  gtk_tree_view_set_model (GTK_TREE_VIEW (view->tree_view), GTK_TREE_MODEL (model));
  view->model = model;

  for (i = 0; i < n_items; ++i)
    {
      for (j = 0; j < (int) G_N_ELEMENTS (actions); ++j)
        {
          action = g_menu_model_get_item_attribute_value (G_MENU_MODEL (view->menu), i, G_MENU_ATTRIBUTE_ACTION, G_VARIANT_TYPE_STRING);
          if (action != NULL && g_strcmp0 (g_variant_get_string (action, NULL), actions[j]) == 0)
            {
              g_menu_remove (view->menu, i);
              --i;
            }
          g_clear_pointer (&action, g_variant_unref);
        }
    }

  if (model != NULL)
    {
      g_signal_connect_swapped (model, "row-changed", G_CALLBACK (xfce_item_list_view_update_actions), view);
      g_signal_connect_swapped (model, "row-deleted", G_CALLBACK (xfce_item_list_view_update_actions), view);
      g_signal_connect_swapped (model, "row-inserted", G_CALLBACK (xfce_item_list_view_update_actions), view);
      g_signal_connect_swapped (model, "rows-reordered", G_CALLBACK (xfce_item_list_view_update_actions), view);
    }

  if (model != NULL)
    flags = xfce_item_list_model_get_list_flags (model);
  else
    flags = 0;

  index = 0;
  if (flags & XFCE_ITEM_LIST_MODEL_REORDERABLE)
    {
      gtk_tree_view_set_reorderable (GTK_TREE_VIEW (view->tree_view), TRUE);

      item = g_menu_item_new (_("Move item up"), "xfce.move-item-up");
      g_menu_item_set_icon (item, g_themed_icon_new ("go-up-symbolic"));
      g_menu_item_set_attribute_value (item, XFCE_MENU_ATTRIBUTE_MOVEMENT, g_variant_new_boolean (TRUE));
      g_menu_insert_item (view->menu, index++, item);

      item = g_menu_item_new (_("Move item down"), "xfce.move-item-down");
      g_menu_item_set_icon (item, g_themed_icon_new ("go-down-symbolic"));
      g_menu_item_set_attribute_value (item, XFCE_MENU_ATTRIBUTE_MOVEMENT, g_variant_new_boolean (TRUE));
      g_menu_insert_item (view->menu, index++, item);
    }
  else
    {
      gtk_tree_view_set_reorderable (GTK_TREE_VIEW (view->tree_view), FALSE);
    }

  if (flags & XFCE_ITEM_LIST_MODEL_EDITABLE)
    {
      item = g_menu_item_new (_("Edit"), "xfce.edit-item");
      g_menu_item_set_icon (item, g_themed_icon_new ("document-edit-symbolic"));
      g_menu_item_set_attribute_value (item, XFCE_MENU_ATTRIBUTE_MNEMONIC, g_variant_new_string ("_Edit"));
      g_menu_insert_item (view->menu, index++, item);
    }

  if (flags & XFCE_ITEM_LIST_MODEL_FILLABLE)
    {
      item = g_menu_item_new (_("Add"), "xfce.add-item");
      g_menu_item_set_icon (item, g_themed_icon_new ("list-add-symbolic"));
      g_menu_item_set_attribute_value (item, XFCE_MENU_ATTRIBUTE_MNEMONIC, g_variant_new_string ("_Add"));
      g_menu_insert_item (view->menu, index++, item);

      item = g_menu_item_new (_("Remove"), "xfce.remove-item");
      g_menu_item_set_icon (item, g_themed_icon_new ("list-remove-symbolic"));
      g_menu_item_set_attribute_value (item, XFCE_MENU_ATTRIBUTE_MNEMONIC, g_variant_new_string ("_Remove"));
      g_menu_insert_item (view->menu, index++, item);
    }
}



static void
xfce_item_list_view_render_buttons (XfceItemListView *view)
{
  GtkWidget *button, *image;
  GVariant *action, *target, *label, *icon, *mnemonic, *movement, *tooltip;
  GList *children, *l;
  gint n_items = g_menu_model_get_n_items (G_MENU_MODEL (view->menu));
  gint i, n_vbuttons, n_hbuttons;

  children = gtk_container_get_children (GTK_CONTAINER (view->vbuttons));
  for (l = children; l != NULL; l = l->next)
    gtk_container_remove (GTK_CONTAINER (view->vbuttons), GTK_WIDGET (l->data));
  g_list_free (children);

  children = gtk_container_get_children (GTK_CONTAINER (view->hbuttons));
  for (l = children; l != NULL; l = l->next)
    gtk_container_remove (GTK_CONTAINER (view->hbuttons), GTK_WIDGET (l->data));
  g_list_free (children);

  n_vbuttons = 0;
  n_hbuttons = 0;
  for (i = 0; i < n_items; ++i)
    {
      action = g_menu_model_get_item_attribute_value (G_MENU_MODEL (view->menu), i, G_MENU_ATTRIBUTE_ACTION, G_VARIANT_TYPE_STRING);
      target = g_menu_model_get_item_attribute_value (G_MENU_MODEL (view->menu), i, G_MENU_ATTRIBUTE_TARGET, NULL);
      label = g_menu_model_get_item_attribute_value (G_MENU_MODEL (view->menu), i, G_MENU_ATTRIBUTE_LABEL, G_VARIANT_TYPE_STRING);
      icon = g_menu_model_get_item_attribute_value (G_MENU_MODEL (view->menu), i, G_MENU_ATTRIBUTE_ICON, NULL);
      mnemonic = g_menu_model_get_item_attribute_value (G_MENU_MODEL (view->menu), i, XFCE_MENU_ATTRIBUTE_MNEMONIC, G_VARIANT_TYPE_STRING);
      movement = g_menu_model_get_item_attribute_value (G_MENU_MODEL (view->menu), i, XFCE_MENU_ATTRIBUTE_MOVEMENT, G_VARIANT_TYPE_BOOLEAN);
      tooltip = g_menu_model_get_item_attribute_value (G_MENU_MODEL (view->menu), i, XFCE_MENU_ATTRIBUTE_TOOLTIP, G_VARIANT_TYPE_STRING);

      if (movement != NULL && g_variant_get_boolean (movement))
        {
          ++n_vbuttons;

          button = gtk_button_new ();
          gtk_box_pack_start (GTK_BOX (view->vbuttons), button, FALSE, FALSE, 0);
        }
      else
        {
          ++n_hbuttons;

          if (mnemonic != NULL)
            button = gtk_button_new_with_mnemonic (g_variant_get_string (mnemonic, NULL));
          else
            button = gtk_button_new_with_label (g_variant_get_string (label, NULL));

          gtk_box_pack_start (GTK_BOX (view->hbuttons), button, FALSE, FALSE, 0);
          gtk_button_box_set_child_non_homogeneous (GTK_BUTTON_BOX (view->hbuttons), button, TRUE);
        }

      gtk_actionable_set_action_name (GTK_ACTIONABLE (button), g_variant_get_string (action, NULL));
      gtk_actionable_set_action_target_value (GTK_ACTIONABLE (button), target);
      gtk_widget_show (button);

      if (icon != NULL)
        {
          image = gtk_image_new_from_gicon (g_icon_deserialize (icon), GTK_ICON_SIZE_BUTTON);
          gtk_button_set_always_show_image (GTK_BUTTON (button), TRUE);
          gtk_button_set_image (GTK_BUTTON (button), image);
          gtk_widget_show (image);
        }

      if (tooltip != NULL)
        gtk_widget_set_tooltip_text (button, g_variant_get_string (tooltip, NULL));

      g_clear_pointer (&action, g_variant_unref);
      g_clear_pointer (&target, g_variant_unref);
      g_clear_pointer (&label, g_variant_unref);
      g_clear_pointer (&icon, g_variant_unref);
      g_clear_pointer (&mnemonic, g_variant_unref);
      g_clear_pointer (&movement, g_variant_unref);
      g_clear_pointer (&tooltip, g_variant_unref);
    }

  gtk_widget_set_visible (view->vbuttons, n_vbuttons > 0);
  gtk_widget_set_visible (view->hbuttons, n_hbuttons > 0);
}



static void
xfce_item_list_view_update_actions (XfceItemListView *view)
{
  GtkTreeSelection *selection = gtk_tree_view_get_selection (GTK_TREE_VIEW (view->tree_view));
  GtkTreeIter iter, tmp;
  gint index;

  if (gtk_tree_selection_get_selected (selection, NULL, &iter))
    {
      tmp = iter;
      g_simple_action_set_enabled (view->up_action, gtk_tree_model_iter_previous (GTK_TREE_MODEL (view->model), &tmp));

      tmp = iter;
      g_simple_action_set_enabled (view->down_action, gtk_tree_model_iter_next (GTK_TREE_MODEL (view->model), &tmp));

      index = xfce_item_list_model_get_index (view->model, &iter);
      g_simple_action_set_enabled (view->edit_action, xfce_item_list_model_is_editable (view->model, index));
      g_simple_action_set_enabled (view->remove_action, xfce_item_list_model_is_removable (view->model, index));
    }
  else
    {
      g_simple_action_set_enabled (view->up_action, FALSE);
      g_simple_action_set_enabled (view->down_action, FALSE);
      g_simple_action_set_enabled (view->edit_action, FALSE);
      g_simple_action_set_enabled (view->remove_action, FALSE);
    }
}



static void
xfce_item_list_view_item_up (XfceItemListView *view)
{
  GtkTreeSelection *selection = gtk_tree_view_get_selection (GTK_TREE_VIEW (view->tree_view));
  GtkTreeIter iter;
  gint index;

  if (gtk_tree_selection_get_selected (selection, NULL, &iter))
    {
      index = xfce_item_list_model_get_index (view->model, &iter);
      xfce_item_list_model_move (view->model, index, index - 1);
    }
}



static void
xfce_item_list_view_item_down (XfceItemListView *view)
{
  GtkTreeSelection *selection = gtk_tree_view_get_selection (GTK_TREE_VIEW (view->tree_view));
  GtkTreeIter iter;
  gint index;

  if (gtk_tree_selection_get_selected (selection, NULL, &iter))
    {
      index = xfce_item_list_model_get_index (view->model, &iter);
      xfce_item_list_model_move (view->model, index, index + 1);
    }
}



static void
xfce_item_list_view_toggle_item (XfceItemListView *view,
                                 const gchar *path_string)
{
  GtkTreePath *path = gtk_tree_path_new_from_string (path_string);
  GtkTreeIter iter;
  gint index;

  if (gtk_tree_model_get_iter (GTK_TREE_MODEL (view->model), &iter, path))
    {
      index = xfce_item_list_model_get_index (view->model, &iter);
      xfce_item_list_model_set_activity (view->model, index, !xfce_item_list_model_is_active (view->model, index));
    }
}



static void
xfce_item_list_view_edit_item (XfceItemListView *view)
{
  GtkTreeSelection *selection = gtk_tree_view_get_selection (GTK_TREE_VIEW (view->tree_view));
  GtkTreeIter iter;
  gint index;

  if (gtk_tree_selection_get_selected (selection, NULL, &iter))
    {
      index = xfce_item_list_model_get_index (view->model, &iter);
      xfce_item_list_model_edit (view->model, index);
    }
}



static void
xfce_item_list_view_add_item (XfceItemListView *view)
{
  xfce_item_list_model_add (view->model);
}



static void
xfce_item_list_view_remove_item (XfceItemListView *view)
{
  GtkTreeSelection *selection = gtk_tree_view_get_selection (GTK_TREE_VIEW (view->tree_view));
  GtkTreeIter iter;
  gint index;

  if (gtk_tree_selection_get_selected (selection, NULL, &iter))
    {
      index = xfce_item_list_model_get_index (view->model, &iter);
      xfce_item_list_model_remove (view->model, index);
    }
}


static gboolean
xfce_item_list_view_tree_pressed (XfceItemListView *view,
                                  GdkEventButton *event)
{
  GtkWidget *context_menu;

  if (event->button == GDK_BUTTON_SECONDARY)
    {
      context_menu = gtk_menu_new_from_model (G_MENU_MODEL (view->menu));
      gtk_menu_attach_to_widget (GTK_MENU (context_menu), view->tree_view, NULL);
      gtk_widget_show_all (context_menu);
      gtk_menu_popup_at_pointer (GTK_MENU (context_menu), (GdkEvent *) event);
    }

  return FALSE;
}


static void
xfce_item_list_view_row_activate (XfceItemListView *view)
{
  if (g_action_get_enabled (G_ACTION (view->edit_action)))
    g_action_activate (G_ACTION (view->edit_action), NULL);
}



/**
 * xfce_item_list_view_new:
 *
 * Since: 4.21.2
 **/
GtkWidget *
xfce_item_list_view_new (XfceItemListModel *model)
{
  return gtk_widget_new (XFCE_TYPE_ITEM_LIST_VIEW, "model", model, NULL);
}



/**
 * xfce_item_list_view_get_menu:
 *
 * Returns: (transfer none): Model responsible for buttons and context menu
 * Since: 4.21.2
 **/
GMenu *
xfce_item_list_view_get_menu (XfceItemListView *view)
{
  _libxfce4ui_return_val_if_fail (XFCE_IS_ITEM_LIST_VIEW (view), NULL);

  return view->menu;
}



/**
 * xfce_item_list_view_get_tree_view:
 *
 * Returns: (transfer none): Internal #GtkTreeView
 * Since: 4.21.2
 **/
GtkWidget *
xfce_item_list_view_get_tree_view (XfceItemListView *view)
{
  _libxfce4ui_return_val_if_fail (XFCE_IS_ITEM_LIST_VIEW (view), NULL);

  return view->tree_view;
}



#define __XFCE_ITEM_LIST_VIEW_C__
#include "libxfce4ui-visibility.c"
