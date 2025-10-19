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

  /* Model used for TreeView display and modification */
  XfceItemListModel *model;

  /* Menu model for buttons and context menu */
  GMenu *menu;

  GtkWidget *tree_view;

  GtkWidget *vbox;
  GtkWidget *buttons_vbox;
  GtkWidget *buttons_hbox;

  /* Standard actions */
  GSimpleAction *up_action;
  GSimpleAction *down_action;
  GSimpleAction *edit_action;
  GSimpleAction *add_action;
  GSimpleAction *remove_action;
  GSimpleAction *reset_action;
};

enum
{
  PROP_0,
  PROP_MODEL,
  PROP_MENU,
  PROP_TREE_VIEW,
};

enum
{
  REMOVE_ITEMS,
  RESET_ITEMS,
  EDIT_ITEM,
  ADD_ITEM,
  N_SIGNALS
};

static gint signals[N_SIGNALS];



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
xfce_item_list_view_add_menu_item (XfceItemListView *view,
                                   gint index,
                                   gboolean movement,
                                   gboolean hide_in_context_menu,
                                   const gchar *mnemonic,
                                   const gchar *label,
                                   const gchar *icon_name,
                                   const gchar *action);

static void
xfce_item_list_view_add_button (XfceItemListView *view,
                                gboolean movement,
                                const gchar *mnemonic,
                                const gchar *label,
                                const gchar *tooltip,
                                GIcon *icon,
                                const gchar *action,
                                GVariant *target);

static void
xfce_item_list_view_recreate_buttons (XfceItemListView *view);

static gint
xfce_item_list_view_get_index_by_path (XfceItemListView *view,
                                       GtkTreePath *path);

static void
xfce_item_list_view_update_actions (XfceItemListView *view);

static void
xfce_item_list_view_move_item (XfceItemListView *view,
                               gint direction);

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

static void
xfce_item_list_view_reset (XfceItemListView *view);

static gboolean
xfce_item_list_view_tree_button_pressed (XfceItemListView *view,
                                         GdkEventButton *event);

static gboolean
xfce_item_list_view_tree_key_released (XfceItemListView *view,
                                       GdkEventKey *event);

static void
xfce_item_list_view_row_activate (XfceItemListView *view);

static GMenu *
xfce_item_list_view_create_context_menu_model (XfceItemListView *view);



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

  /**
   * XfceItemListView::edit-item:
   * @view: #XfceItemListView
   * @item: Item index
   *
   * Returns: %TRUE to stop other handlers from being invoked for the event. %FALSE to propagate the event further.
   **/
  signals[EDIT_ITEM] = g_signal_new ("edit-item",
                                     G_TYPE_FROM_CLASS (object_class),
                                     G_SIGNAL_RUN_LAST,
                                     0,
                                     NULL, NULL,
                                     NULL,
                                     G_TYPE_BOOLEAN, 1, G_TYPE_INT);

  /**
   * XfceItemListView::add-item:
   * @view: #XfceItemListView
   *
   * Returns: %TRUE to stop other handlers from being invoked for the event. %FALSE to propagate the event further.
   **/
  signals[ADD_ITEM] = g_signal_new ("add-item",
                                    G_TYPE_FROM_CLASS (object_class),
                                    G_SIGNAL_RUN_LAST,
                                    0,
                                    NULL, NULL,
                                    NULL,
                                    G_TYPE_BOOLEAN, 0);

  /**
   * XfceItemListView::remove-items:
   * @view: #XfceItemListView
   * @items: (array length=n_items) (element-type int) (in): Item indexes
   * @n_items: Number of indexes
   *
   * Returns: %TRUE to stop other handlers from being invoked for the event. %FALSE to propagate the event further.
   **/
  signals[REMOVE_ITEMS] = g_signal_new ("remove-items",
                                        G_TYPE_FROM_CLASS (object_class),
                                        G_SIGNAL_RUN_LAST,
                                        0,
                                        NULL, NULL,
                                        NULL,
                                        G_TYPE_BOOLEAN, 2, G_TYPE_POINTER, G_TYPE_INT);

  /**
   * XfceItemListView::reset-items:
   * @view: #XfceItemListView
   *
   * Returns: %TRUE to stop other handlers from being invoked for the event. %FALSE to propagate the event further.
   **/
  signals[RESET_ITEMS] = g_signal_new ("reset-items",
                                       G_TYPE_FROM_CLASS (object_class),
                                       G_SIGNAL_RUN_LAST,
                                       0,
                                       NULL, NULL,
                                       NULL,
                                       G_TYPE_BOOLEAN, 0);
}



static void
xfce_item_list_view_init (XfceItemListView *view)
{
  g_object_set (view, "orientation", GTK_ORIENTATION_HORIZONTAL, "spacing", 6, NULL);

  view->vbox = gtk_box_new (GTK_ORIENTATION_VERTICAL, 0);
  gtk_box_pack_start (GTK_BOX (view), view->vbox, TRUE, TRUE, 0);
  gtk_widget_show (view->vbox);

  GtkWidget *scrwin = gtk_scrolled_window_new (NULL, NULL);
  gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (scrwin), GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
  gtk_scrolled_window_set_shadow_type (GTK_SCROLLED_WINDOW (scrwin), GTK_SHADOW_IN);
  gtk_widget_set_hexpand (scrwin, TRUE);
  gtk_widget_set_vexpand (scrwin, TRUE);
  gtk_box_pack_start (GTK_BOX (view->vbox), scrwin, TRUE, TRUE, 0);
  gtk_widget_show (scrwin);

  view->tree_view = gtk_tree_view_new ();
  g_signal_connect_swapped (view->tree_view, "button-press-event", G_CALLBACK (xfce_item_list_view_tree_button_pressed), view);
  g_signal_connect_swapped (view->tree_view, "key-release-event", G_CALLBACK (xfce_item_list_view_tree_key_released), view);
  g_signal_connect_swapped (view->tree_view, "row-activated", G_CALLBACK (xfce_item_list_view_row_activate), view);
  gtk_tree_view_set_headers_visible (GTK_TREE_VIEW (view->tree_view), FALSE);
  gtk_tree_view_set_search_column (GTK_TREE_VIEW (view->tree_view), XFCE_ITEM_LIST_MODEL_COLUMN_NAME);
  gtk_tree_view_set_tooltip_column (GTK_TREE_VIEW (view->tree_view), XFCE_ITEM_LIST_MODEL_COLUMN_TOOLTIP);
  gtk_container_add (GTK_CONTAINER (scrwin), view->tree_view);
  gtk_widget_show (view->tree_view);

  GtkTreeSelection *selection = gtk_tree_view_get_selection (GTK_TREE_VIEW (view->tree_view));
  g_signal_connect_swapped (selection, "changed", G_CALLBACK (xfce_item_list_view_update_actions), view);

  GtkCellRenderer *renderer_active = gtk_cell_renderer_toggle_new ();
  g_signal_connect_swapped (G_OBJECT (renderer_active), "toggled", G_CALLBACK (xfce_item_list_view_toggle_item), view);
  gtk_tree_view_insert_column_with_attributes (GTK_TREE_VIEW (view->tree_view),
                                               XFCE_ITEM_LIST_VIEW_COLUMN_ACTIVE,
                                               _("Active"),
                                               renderer_active,
                                               "active", XFCE_ITEM_LIST_MODEL_COLUMN_ACTIVE,
                                               "visible", XFCE_ITEM_LIST_MODEL_COLUMN_ACTIVABLE,
                                               NULL);

  GtkCellRenderer *renderer_icon = gtk_cell_renderer_pixbuf_new ();
  gtk_tree_view_insert_column_with_attributes (GTK_TREE_VIEW (view->tree_view),
                                               XFCE_ITEM_LIST_VIEW_COLUMN_ICON,
                                               NULL,
                                               renderer_icon,
                                               "gicon", XFCE_ITEM_LIST_MODEL_COLUMN_ICON,
                                               NULL);

  GtkCellRenderer *renderer_name = gtk_cell_renderer_text_new ();
  gtk_tree_view_insert_column_with_attributes (GTK_TREE_VIEW (view->tree_view),
                                               XFCE_ITEM_LIST_VIEW_COLUMN_NAME,
                                               _("Name"),
                                               renderer_name,
                                               "markup", XFCE_ITEM_LIST_MODEL_COLUMN_NAME,
                                               "sensitive", XFCE_ITEM_LIST_MODEL_COLUMN_ACTIVE,
                                               NULL);

  GSimpleActionGroup *group = g_simple_action_group_new ();

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

  view->reset_action = g_simple_action_new ("reset", NULL);
  g_signal_connect_swapped (view->reset_action, "activate", G_CALLBACK (xfce_item_list_view_reset), view);
  g_action_map_add_action (G_ACTION_MAP (group), G_ACTION (view->reset_action));

  gtk_widget_insert_action_group (GTK_WIDGET (view), "xfce", G_ACTION_GROUP (group));

  view->menu = g_menu_new ();
  g_signal_connect_swapped (view->menu, "items-changed", G_CALLBACK (xfce_item_list_view_recreate_buttons), view);
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
xfce_item_list_view_add_menu_item (XfceItemListView *view,
                                   gint index,
                                   gboolean movement,
                                   gboolean hide_in_context_menu,
                                   const gchar *mnemonic,
                                   const gchar *label,
                                   const gchar *icon_name,
                                   const gchar *action)
{
  GMenuItem *item = g_menu_item_new (label, action);
  GIcon *icon = g_themed_icon_new (icon_name);

  g_menu_item_set_icon (item, icon);
  g_object_unref (icon);

  if (movement)
    g_menu_item_set_attribute_value (item, XFCE_MENU_ATTRIBUTE_MOVEMENT, g_variant_new_boolean (movement));

  if (mnemonic != NULL)
    g_menu_item_set_attribute_value (item, XFCE_MENU_ATTRIBUTE_MNEMONIC, g_variant_new_string (mnemonic));

  if (hide_in_context_menu)
    g_menu_item_set_attribute_value (item, XFCE_MENU_ATTRIBUTE_HIDE_IN_CONTEXT_MENU, g_variant_new_boolean (hide_in_context_menu));

  g_menu_insert_item (view->menu, index, item);
  g_object_unref (item);
}



static void
xfce_item_list_view_add_button (XfceItemListView *view,
                                gboolean movement,
                                const gchar *mnemonic,
                                const gchar *label,
                                const gchar *tooltip,
                                GIcon *icon,
                                const gchar *action,
                                GVariant *target)
{
  GtkWidget *button = NULL;

  if (movement)
    {
      if (view->buttons_vbox == NULL)
        {
          view->buttons_vbox = gtk_box_new (GTK_ORIENTATION_VERTICAL, 0);
          gtk_style_context_add_class (gtk_widget_get_style_context (view->buttons_vbox), GTK_STYLE_CLASS_LINKED);
          gtk_box_pack_start (GTK_BOX (view), view->buttons_vbox, FALSE, FALSE, 0);
          gtk_widget_set_vexpand (view->buttons_vbox, FALSE);
          gtk_widget_show (view->buttons_vbox);
        }

      button = gtk_button_new ();
      gtk_box_pack_start (GTK_BOX (view->buttons_vbox), button, FALSE, FALSE, 0);
    }
  else
    {
      if (view->buttons_hbox == NULL)
        {
          view->buttons_hbox = gtk_button_box_new (GTK_ORIENTATION_HORIZONTAL);
          gtk_button_box_set_layout (GTK_BUTTON_BOX (view->buttons_hbox), GTK_BUTTONBOX_START);
          gtk_style_context_add_class (gtk_widget_get_style_context (view->buttons_hbox), GTK_STYLE_CLASS_INLINE_TOOLBAR);
          gtk_box_set_homogeneous (GTK_BOX (view->buttons_hbox), FALSE);
          gtk_box_pack_start (GTK_BOX (view->vbox), view->buttons_hbox, FALSE, FALSE, 0);
          gtk_widget_show (view->buttons_hbox);
        }

      if (mnemonic != NULL)
        button = gtk_button_new_with_mnemonic (mnemonic);
      else
        button = gtk_button_new_with_label (label);

      gtk_box_pack_start (GTK_BOX (view->buttons_hbox), button, FALSE, FALSE, 0);
      gtk_button_box_set_child_non_homogeneous (GTK_BUTTON_BOX (view->buttons_hbox), button, TRUE);
    }

  if (tooltip != NULL)
    gtk_widget_set_tooltip_text (button, tooltip);

  if (icon != NULL)
    {
      GtkWidget *image = gtk_image_new_from_gicon (icon, GTK_ICON_SIZE_BUTTON);
      gtk_button_set_always_show_image (GTK_BUTTON (button), TRUE);
      gtk_button_set_image (GTK_BUTTON (button), image);
      gtk_widget_show (image);
    }

  gtk_actionable_set_action_name (GTK_ACTIONABLE (button), action);
  gtk_actionable_set_action_target_value (GTK_ACTIONABLE (button), target);
  gtk_widget_show (button);
}



static void
xfce_item_list_view_recreate_buttons (XfceItemListView *view)
{
  /* Removing button containers, they will be created later if they are needed */
  g_clear_pointer (&view->buttons_vbox, gtk_widget_destroy);
  g_clear_pointer (&view->buttons_hbox, gtk_widget_destroy);

  /* Creating only the necessary buttons according to the menu model */
  gint n_items = g_menu_model_get_n_items (G_MENU_MODEL (view->menu));
  for (gint i = 0; i < n_items; ++i)
    {
      GVariant *action = g_menu_model_get_item_attribute_value (G_MENU_MODEL (view->menu), i, G_MENU_ATTRIBUTE_ACTION, G_VARIANT_TYPE_STRING);
      GVariant *target = g_menu_model_get_item_attribute_value (G_MENU_MODEL (view->menu), i, G_MENU_ATTRIBUTE_TARGET, NULL);
      GVariant *label = g_menu_model_get_item_attribute_value (G_MENU_MODEL (view->menu), i, G_MENU_ATTRIBUTE_LABEL, G_VARIANT_TYPE_STRING);
      GVariant *icon = g_menu_model_get_item_attribute_value (G_MENU_MODEL (view->menu), i, G_MENU_ATTRIBUTE_ICON, NULL);
      GVariant *mnemonic = g_menu_model_get_item_attribute_value (G_MENU_MODEL (view->menu), i, XFCE_MENU_ATTRIBUTE_MNEMONIC, G_VARIANT_TYPE_STRING);
      GVariant *movement = g_menu_model_get_item_attribute_value (G_MENU_MODEL (view->menu), i, XFCE_MENU_ATTRIBUTE_MOVEMENT, G_VARIANT_TYPE_BOOLEAN);
      GVariant *tooltip = g_menu_model_get_item_attribute_value (G_MENU_MODEL (view->menu), i, XFCE_MENU_ATTRIBUTE_TOOLTIP, G_VARIANT_TYPE_STRING);
      GIcon *gicon = icon != NULL ? g_icon_deserialize (icon) : NULL;

      if (action != NULL)
        {
          xfce_item_list_view_add_button (view,
                                          movement != NULL ? g_variant_get_boolean (movement) : FALSE,
                                          mnemonic != NULL ? g_variant_get_string (mnemonic, NULL) : NULL,
                                          label != NULL ? g_variant_get_string (label, NULL) : NULL,
                                          tooltip != NULL ? g_variant_get_string (tooltip, NULL) : NULL,
                                          gicon,
                                          action != NULL ? g_variant_get_string (action, NULL) : NULL,
                                          target);
        }
      g_object_unref (gicon);

      g_clear_pointer (&action, g_variant_unref);
      g_clear_pointer (&target, g_variant_unref);
      g_clear_pointer (&label, g_variant_unref);
      g_clear_pointer (&icon, g_variant_unref);
      g_clear_pointer (&mnemonic, g_variant_unref);
      g_clear_pointer (&movement, g_variant_unref);
      g_clear_pointer (&tooltip, g_variant_unref);
    }
}



static gint
xfce_item_list_view_get_index_by_path (XfceItemListView *view,
                                       GtkTreePath *path)
{
  GtkTreeIter iter;

  gtk_tree_model_get_iter (GTK_TREE_MODEL (view->model), &iter, path);
  return xfce_item_list_model_get_index (view->model, &iter);
}



static void
xfce_item_list_view_update_actions (XfceItemListView *view)
{
  gint *sel_items = NULL;
  gint n_sel_items = xfce_item_list_view_get_selected_items (view, &sel_items);

  g_simple_action_set_enabled (view->up_action, n_sel_items == 1 && sel_items[0] > 0);
  g_simple_action_set_enabled (view->down_action, n_sel_items == 1 && sel_items[0] + 1 < xfce_item_list_model_get_n_items (view->model));
  g_simple_action_set_enabled (view->edit_action, n_sel_items == 1 && xfce_item_list_model_test (view->model, sel_items[0], XFCE_ITEM_LIST_MODEL_COLUMN_EDITABLE));
  g_simple_action_set_enabled (view->remove_action, n_sel_items > 0 && xfce_item_list_model_test_all (view->model, sel_items, n_sel_items, XFCE_ITEM_LIST_MODEL_COLUMN_REMOVABLE));
  g_free (sel_items);
}



static void
xfce_item_list_view_move_item (XfceItemListView *view,
                               gint direction)
{
  gint *sel_items = NULL;
  gint n_sel_items = xfce_item_list_view_get_selected_items (view, &sel_items);

  if (n_sel_items == 1)
    {
      gint new_index = direction > 0 ? sel_items[0] + 1 : sel_items[0] - 1;
      xfce_item_list_model_move (view->model, sel_items[0], new_index);

      /* make the new selected position visible if moved out of area */
      GtkTreePath *path = gtk_tree_path_new_from_indices (new_index, -1);
      gtk_tree_view_scroll_to_cell (GTK_TREE_VIEW (view->tree_view), path, NULL, FALSE, 0, 0);
      gtk_tree_view_set_cursor (GTK_TREE_VIEW (view->tree_view), path, NULL, FALSE);
      gtk_tree_path_free (path);
    }

  g_free (sel_items);
}



static void
xfce_item_list_view_item_up (XfceItemListView *view)
{
  xfce_item_list_view_move_item (view, -1);
}



static void
xfce_item_list_view_item_down (XfceItemListView *view)
{
  xfce_item_list_view_move_item (view, 1);
}



static void
xfce_item_list_view_toggle_item (XfceItemListView *view,
                                 const gchar *path_string)
{
  GtkTreePath *path = gtk_tree_path_new_from_string (path_string);
  GtkTreeIter iter;

  if (gtk_tree_model_get_iter (GTK_TREE_MODEL (view->model), &iter, path))
    {
      gint index = xfce_item_list_model_get_index (view->model, &iter);
      xfce_item_list_model_set_activity (view->model, index, !xfce_item_list_model_test (view->model, index, XFCE_ITEM_LIST_MODEL_COLUMN_ACTIVE));
    }
}



static void
xfce_item_list_view_edit_item (XfceItemListView *view)
{
  gint *sel_items = NULL;
  gint n_sel_items = xfce_item_list_view_get_selected_items (view, &sel_items);

  if (n_sel_items == 1)
    {
      gboolean stop_propagation = FALSE;
      g_signal_emit (view, signals[EDIT_ITEM], 0, sel_items[0], &stop_propagation);
      if (!stop_propagation)
        xfce_item_list_model_changed (view->model);
    }
}



static void
xfce_item_list_view_add_item (XfceItemListView *view)
{
  gboolean stop_propagation = FALSE;

  g_signal_emit (view, signals[ADD_ITEM], 0, &stop_propagation);
  if (!stop_propagation)
    {
      /* Move cursor to added element */
      gint n_items = xfce_item_list_model_get_n_items (view->model);
      if (n_items > 0)
        {
          GtkTreePath *path = gtk_tree_path_new_from_indices (n_items - 1, -1);
          gtk_tree_view_set_cursor (GTK_TREE_VIEW (view->tree_view), path, NULL, FALSE);
          gtk_tree_path_free (path);
        }
    }
}



static void
xfce_item_list_view_remove_item (XfceItemListView *view)
{
  gint *sel_items = NULL;
  gint n_sel_items = xfce_item_list_view_get_selected_items (view, &sel_items);

  gboolean stop_propagation = FALSE;
  g_signal_emit (view, signals[REMOVE_ITEMS], 0, sel_items, n_sel_items, &stop_propagation);
  if (!stop_propagation)
    {
      for (gint i = n_sel_items - 1; i >= 0; --i)
        xfce_item_list_model_remove (view->model, sel_items[i]);

      gtk_tree_selection_unselect_all (gtk_tree_view_get_selection (GTK_TREE_VIEW (view->tree_view)));
    }
  g_free (sel_items);
}



static void
xfce_item_list_view_reset (XfceItemListView *view)
{
  gboolean stop_propagation = FALSE;

  g_signal_emit (view, signals[RESET_ITEMS], 0, &stop_propagation);
  if (!stop_propagation)
    xfce_item_list_model_reset (view->model);
}



static gboolean
xfce_item_list_view_tree_button_pressed (XfceItemListView *view,
                                         GdkEventButton *event)
{
  if (event->button == GDK_BUTTON_SECONDARY)
    {
      gboolean stop_propagation = FALSE;
      GMenu *menu_model = xfce_item_list_view_create_context_menu_model (view);
      if (g_menu_model_get_n_items (G_MENU_MODEL (menu_model)) > 0)
        {
          GtkWidget *context_menu = gtk_menu_new_from_model (G_MENU_MODEL (menu_model));
          gtk_menu_attach_to_widget (GTK_MENU (context_menu), view->tree_view, NULL);
          gtk_widget_show_all (context_menu);
          gtk_menu_popup_at_pointer (GTK_MENU (context_menu), (GdkEvent *) event);

          /* If the click occurs on selected items, the event should be stopped, otherwise it will result in re-selecting one item */
          GtkTreePath *path = NULL;
          if (gtk_tree_view_get_path_at_pos (GTK_TREE_VIEW (view->tree_view), event->x, event->y, &path, NULL, NULL, NULL))
            {
              GtkTreeSelection *selection = gtk_tree_view_get_selection (GTK_TREE_VIEW (view->tree_view));
              stop_propagation = gtk_tree_selection_path_is_selected (selection, path);
            }
          gtk_tree_path_free (path);
        }
      g_object_unref (menu_model);
      return stop_propagation;
    }

  return FALSE;
}



static gboolean
xfce_item_list_view_tree_key_released (XfceItemListView *view,
                                       GdkEventKey *event)
{
  if (event->keyval == GDK_KEY_Delete)
    {
      if (g_action_get_enabled (G_ACTION (view->remove_action)))
        g_action_activate (G_ACTION (view->remove_action), NULL);
    }

  return FALSE;
}



static void
xfce_item_list_view_row_activate (XfceItemListView *view)
{
  if (g_action_get_enabled (G_ACTION (view->edit_action)))
    g_action_activate (G_ACTION (view->edit_action), NULL);
}



static GMenu *
xfce_item_list_view_create_context_menu_model (XfceItemListView *view)
{
  GMenu *new_menu = g_menu_new ();

  gint n_items = g_menu_model_get_n_items (G_MENU_MODEL (view->menu));
  for (gint i = 0; i < n_items; ++i)
    {
      GVariant *label = g_menu_model_get_item_attribute_value (G_MENU_MODEL (view->menu), i, G_MENU_ATTRIBUTE_LABEL, G_VARIANT_TYPE_STRING);
      GVariant *hide_in_context_menu = g_menu_model_get_item_attribute_value (G_MENU_MODEL (view->menu), i, XFCE_MENU_ATTRIBUTE_HIDE_IN_CONTEXT_MENU, G_VARIANT_TYPE_BOOLEAN);
      if (label != NULL && (hide_in_context_menu == NULL || !g_variant_get_boolean (hide_in_context_menu)))
        {
          GMenuItem *tmp_item = g_menu_item_new_from_model (G_MENU_MODEL (view->menu), i);
          g_menu_append_item (new_menu, tmp_item);
          g_object_unref (tmp_item);
        }
      g_clear_pointer (&label, g_variant_unref);
      g_clear_pointer (&hide_in_context_menu, g_variant_unref);
    }

  return new_menu;
}



/**
 * xfce_item_list_view_new:
 * @model: Model to display
 *
 * Returns: (transfer full): #XfceItemListView widget
 *
 * Since: 4.21.2
 **/
GtkWidget *
xfce_item_list_view_new (XfceItemListModel *model)
{
  return gtk_widget_new (XFCE_TYPE_ITEM_LIST_VIEW, "model", model, NULL);
}



/**
 * xfce_item_list_view_get_model:
 * @view: #XfceItemListView
 *
 * Returns: (transfer none): #XfceItemListModel
 *
 * Since: 4.21.2
 **/
XfceItemListModel *
xfce_item_list_view_get_model (XfceItemListView *view)
{
  g_return_val_if_fail (XFCE_IS_ITEM_LIST_VIEW (view), NULL);

  return view->model;
}



/**
 * xfce_item_list_view_set_model:
 * @view: #XfceItemListView
 * @model: (nullable): #XfceItemListModel
 *
 * Since: 4.21.2
 **/
void
xfce_item_list_view_set_model (XfceItemListView *view,
                               XfceItemListModel *model)
{
  g_return_if_fail (model == NULL || XFCE_IS_ITEM_LIST_MODEL (model));

  /* Replace model */
  if (view->model != NULL)
    g_signal_handlers_disconnect_by_data (view->model, view);

  gtk_tree_view_set_model (GTK_TREE_VIEW (view->tree_view), GTK_TREE_MODEL (model));
  view->model = model;

  /* Remove old standard menu items */
  for (gint i = 0; i < g_menu_model_get_n_items (G_MENU_MODEL (view->menu)); ++i)
    {
      GVariant *action = g_menu_model_get_item_attribute_value (G_MENU_MODEL (view->menu), i, G_MENU_ATTRIBUTE_ACTION, G_VARIANT_TYPE_STRING);
      if (action == NULL)
        continue;

      const char *actions[] = { "xfce.move-item-up", "xfce.move-item-down", "xfce.edit-item",
                                "xfce.add-item", "xfce.remove-item", "xfce.reset" };

      for (gint j = 0; j < (gint) G_N_ELEMENTS (actions); ++j)
        {
          if (g_strcmp0 (g_variant_get_string (action, NULL), actions[j]) == 0)
            {
              g_menu_remove (view->menu, i);
              --i;
              break;
            }
        }

      g_clear_pointer (&action, g_variant_unref);
    }

  /* Signals */
  if (model != NULL)
    {
      g_signal_connect_swapped (model, "row-changed", G_CALLBACK (xfce_item_list_view_update_actions), view);
      g_signal_connect_swapped (model, "row-deleted", G_CALLBACK (xfce_item_list_view_update_actions), view);
      g_signal_connect_swapped (model, "row-inserted", G_CALLBACK (xfce_item_list_view_update_actions), view);
      g_signal_connect_swapped (model, "rows-reordered", G_CALLBACK (xfce_item_list_view_update_actions), view);
    }

  /* Creating menus and configuring widgets based on model capabilities */
  XfceItemListModelFlags flags = model != NULL ? xfce_item_list_model_get_list_flags (model) : XFCE_ITEM_LIST_MODEL_NONE;
  gint index = 0;

  if (flags & XFCE_ITEM_LIST_MODEL_REORDERABLE)
    {
      gtk_tree_view_set_reorderable (GTK_TREE_VIEW (view->tree_view), TRUE);

      xfce_item_list_view_add_menu_item (view, index++, TRUE, TRUE, NULL, _("Move item up"), "go-up-symbolic", "xfce.move-item-up");
      xfce_item_list_view_add_menu_item (view, index++, TRUE, TRUE, NULL, _("Move item down"), "go-down-symbolic", "xfce.move-item-down");
    }
  else
    {
      gtk_tree_view_set_reorderable (GTK_TREE_VIEW (view->tree_view), FALSE);
    }

  if (flags & XFCE_ITEM_LIST_MODEL_EDITABLE)
    xfce_item_list_view_add_menu_item (view, index++, FALSE, FALSE, _("_Edit"), _("Edit"), "document-edit-symbolic", "xfce.edit-item");

  if (flags & XFCE_ITEM_LIST_MODEL_ADDABLE)
    xfce_item_list_view_add_menu_item (view, index++, FALSE, TRUE, _("_Add"), _("Add"), "list-add-symbolic", "xfce.add-item");

  if (flags & XFCE_ITEM_LIST_MODEL_REMOVABLE)
    xfce_item_list_view_add_menu_item (view, index++, FALSE, FALSE, _("_Remove"), _("Remove"), "list-remove-symbolic", "xfce.remove-item");

  if (flags & XFCE_ITEM_LIST_MODEL_RESETTABLE)
    xfce_item_list_view_add_menu_item (view, index++, FALSE, TRUE, _("Reset to de_faults"), _("Reset to defaults"), "document-revert-symbolic", "xfce.reset");
}



/**
 * xfce_item_list_view_get_menu:
 * @view: #XfceItemListView
 *
 * Returns a menu to which you can add your own items
 *
 * Returns: (transfer none): Model responsible for buttons and context menu
 *
 * Since: 4.21.2
 **/
GMenu *
xfce_item_list_view_get_menu (XfceItemListView *view)
{
  g_return_val_if_fail (XFCE_IS_ITEM_LIST_VIEW (view), NULL);

  return view->menu;
}



/**
 * xfce_item_list_view_get_tree_view:
 * @view: #XfceItemListView
 *
 * Returns a #GtkTreeView to which you can add your own columns, or customize the selection mode
 *
 * Returns: (transfer none): Internal #GtkTreeView
 *
 * Since: 4.21.2
 **/
GtkWidget *
xfce_item_list_view_get_tree_view (XfceItemListView *view)
{
  g_return_val_if_fail (XFCE_IS_ITEM_LIST_VIEW (view), NULL);

  return view->tree_view;
}



/**
 * xfce_item_list_view_get_selected_items:
 * @view: #XfceItemListView
 * @items: (out) (nullable): Indexes of selected items
 *
 * Returns a #GtkTreeView to which you can add your own columns, or customize the selection mode
 *
 * Returns: Number of selected items
 *
 * Since: 4.21.2
 **/
gint
xfce_item_list_view_get_selected_items (XfceItemListView *view,
                                        gint **items)
{
  g_return_val_if_fail (XFCE_IS_ITEM_LIST_VIEW (view), 0);

  GtkTreeSelection *selection = gtk_tree_view_get_selection (GTK_TREE_VIEW (view->tree_view));
  GList *rows = gtk_tree_selection_get_selected_rows (selection, NULL);
  gint n_items = g_list_length (rows);

  if (items != NULL)
    {
      *items = g_new (gint, n_items);

      gint i = 0;
      for (GList *l = rows; l != NULL; l = l->next, ++i)
        (*items)[i] = xfce_item_list_view_get_index_by_path (view, l->data);
    }

  g_list_free_full (rows, (GDestroyNotify) gtk_tree_path_free);

  return n_items;
}



#define __XFCE_ITEM_LIST_VIEW_C__
#include "libxfce4ui-visibility.c"
