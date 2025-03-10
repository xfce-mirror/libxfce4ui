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
#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#define EXO_PARAM_READWRITE G_PARAM_READWRITE
//~ #include <exo/exo-config.h>
//~ #include <exo/exo-gtk-extensions.h>
//~ #include <exo/exo-private.h>
//~ #include <exo/exo-tree-view.h>
//~ #include <exo/exo-utils.h>
//~ #include <exo/exo-alias.h>
#include <gtk/gtk.h>
#include <libxfce4util/libxfce4util.h>

#include "xfce-list-view.h"
#include "libxfce4ui-visibility.h"

/**
 * SECTION: xfce-list-view
 * @title: XfceListView
 * @short_description: An improved version of #GtkTreeView
 * @include: libxfce4ui/libxfce4ui.h
 *
 * The #XfceListView class derives from #GtkTreeView and extends it with
 * the ability to activate rows using single button clicks instead of
 * the default double button clicks. Columns are drag and drop
 * reorderable, and can be hidden and restored as needed.
 *
 * It also works around a few shortcomings
 * of #GtkTreeView, i.e. #XfceListView allows the user to drag around multiple
 * selected rows. The API is
 * also simplified by abstracting the cell renderers for common use
 * cases.
 **/
 
 typedef struct _XfceListViewPrivate XfceListViewPrivate;

 

 
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

struct _XfceListViewPrivate
{
  /* whether the next button-release-event should emit "row-activate" */
  guint button_release_activates : 1;
  /* whether drag and drop must be re-enabled on button-release-event (rubberbanding active) */
  guint button_release_unblocks_dnd : 1;
  /* whether rubberbanding must be re-enabled on button-release-event (drag and drop active) */
  guint button_release_enables_rubber_banding : 1;
  /* single click mode */
  guint single_click : 1;
  guint single_click_timeout;
  gint single_click_timeout_id;
  guint single_click_timeout_state;
  /* the path below the pointer or NULL */
  GtkTreePath *hover_path;
 
  /* for storing hidden columns */
  GListStore *possible_columns;
 
  /* the right-click menu */
  GtkWidget *popover;
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
 
 
/* Property identifiers */
enum
{
  PROP_0,
  PROP_SINGLE_CLICK,
  PROP_SINGLE_CLICK_TIMEOUT,
};
static void
xfce_list_view_finalize (GObject *object);
static void
xfce_list_view_get_property (GObject *object,
                             guint prop_id,
                             GValue *value,
                             GParamSpec *pspec);
static void
xfce_list_view_set_property (GObject *object,
                             guint prop_id,
                             const GValue *value,
                             GParamSpec *pspec);
static gboolean
xfce_list_view_button_press_event (GtkWidget *widget,
                                   GdkEventButton *event);
static gboolean
xfce_list_view_button_release_event (GtkWidget *widget,
                                     GdkEventButton *event);
static gboolean
xfce_list_view_motion_notify_event (GtkWidget *widget,
                                    GdkEventMotion *event);
static gboolean
xfce_list_view_leave_notify_event (GtkWidget *widget,
                                   GdkEventCrossing *event);
static void
xfce_list_view_drag_begin (GtkWidget *widget,
                           GdkDragContext *context);
static gboolean
xfce_list_view_move_cursor (GtkTreeView *view,
                            GtkMovementStep step,
                            gint count);
static gboolean
xfce_list_view_single_click_timeout (gpointer user_data);
static void
xfce_list_view_single_click_timeout_destroy (gpointer user_data);
static gboolean
select_true (GtkTreeSelection *selection,
             GtkTreeModel *model,
             GtkTreePath *path,
             gboolean path_currently_selected,
             gpointer data);
static gboolean
select_false (GtkTreeSelection *selection,
              GtkTreeModel *model,
              GtkTreePath *path,
              gboolean path_currently_selected,
              gpointer data);
gboolean
xfce_list_view_column_equal_id (gconstpointer a,
                                gconstpointer b,
                                gpointer user_data);
gint
xfce_list_view_column_equal_id_2 (gconstpointer a,
                                  gconstpointer b);
static void
xfce_list_view_column_editor_popup_refresh(XfceListView *list_view);

G_DEFINE_TYPE_WITH_PRIVATE(XfceListView, xfce_list_view, GTK_TYPE_TREE_VIEW)

static void
xfce_list_view_class_init (XfceListViewClass *klass)
{
  GtkTreeViewClass *gtklist_view_class;
  GtkWidgetClass *gtkwidget_class;
  GObjectClass *gobject_class;
  gobject_class = G_OBJECT_CLASS (klass);
  gobject_class->finalize = xfce_list_view_finalize;
  gobject_class->get_property = xfce_list_view_get_property;
  gobject_class->set_property = xfce_list_view_set_property;
  gtkwidget_class = GTK_WIDGET_CLASS (klass);
  gtkwidget_class->button_press_event = xfce_list_view_button_press_event;
  gtkwidget_class->button_release_event = xfce_list_view_button_release_event;
  gtkwidget_class->motion_notify_event = xfce_list_view_motion_notify_event;
  gtkwidget_class->leave_notify_event = xfce_list_view_leave_notify_event;
  gtkwidget_class->drag_begin = xfce_list_view_drag_begin;
  gtklist_view_class = GTK_TREE_VIEW_CLASS (klass);
  gtklist_view_class->move_cursor = xfce_list_view_move_cursor;
  /* initialize the library's i18n support */
  //~ _exo_i18n_init ();
  /**
   * XfceListView:single-click:
   *
   * %TRUE to activate items using a single click instead of a
   * double click.
   *
   * Since: 0.3.1.3
   **/
  g_object_class_install_property (gobject_class,
                                   PROP_SINGLE_CLICK,
                                   g_param_spec_boolean ("single-click",
                                                         _("Single Click"),
                                                           _("Whether the items in the view can be activated with single clicks"),
                                                         FALSE,
                                                         EXO_PARAM_READWRITE));
  /**
   * XfceListView:single-click-timeout:
   *
   * The amount of time in milliseconds after which the hover row (the row
   * which is hovered by the mouse cursor) will be selected automatically
   * in single-click mode. A value of %0 disables the automatic selection.
   *
   * Since: 0.3.1.5
   **/
  g_object_class_install_property (gobject_class,
                                   PROP_SINGLE_CLICK_TIMEOUT,
                                   g_param_spec_uint ("single-click-timeout",
                                                      _("Single Click Timeout"),
                                                        _("The amount of time after which the item under the mouse cursor will be selected automatically in single click mode"),
                                                      0, G_MAXUINT, 0,
                                                      EXO_PARAM_READWRITE));
}
static void
xfce_list_view_init (XfceListView *list_view)
{
  /* grab a pointer on the private data */
  list_view->priv = xfce_list_view_get_instance_private (list_view);
  list_view->priv->single_click_timeout_id = -1;
  //~ gtk_tree_view_set_search_position_func (GTK_TREE_VIEW (list_view),
  //~ (GtkTreeViewSearchPositionFunc) exo_gtk_position_search_box,
  //~ NULL, g_object_unref);
 
  list_view->priv->possible_columns = g_list_store_new (GTK_TYPE_TREE_VIEW_COLUMN);
  list_view->priv->popover = gtk_popover_new(GTK_WIDGET(list_view));
}
static void
xfce_list_view_finalize (GObject *object)
{
  XfceListView *list_view = XFCE_LIST_VIEW (object);
  /* be sure to cancel any single-click timeout */
  if (G_UNLIKELY (list_view->priv->single_click_timeout_id >= 0))
    g_source_remove (list_view->priv->single_click_timeout_id);
  /* be sure to release the hover path */
  if (G_UNLIKELY (list_view->priv->hover_path != NULL))
    gtk_tree_path_free (list_view->priv->hover_path);
  (*G_OBJECT_CLASS (xfce_list_view_parent_class)->finalize) (object);
}
static void
xfce_list_view_get_property (GObject *object,
                             guint prop_id,
                             GValue *value,
                             GParamSpec *pspec)
{
  XfceListView *list_view = XFCE_LIST_VIEW (object);
  switch (prop_id)
    {
    case PROP_SINGLE_CLICK:
      g_value_set_boolean (value, xfce_list_view_get_single_click (list_view));
      break;
    case PROP_SINGLE_CLICK_TIMEOUT:
      g_value_set_uint (value, xfce_list_view_get_single_click_timeout (list_view));
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
    }
}
static void
xfce_list_view_set_property (GObject *object,
                             guint prop_id,
                             const GValue *value,
                             GParamSpec *pspec)
{
  XfceListView *list_view = XFCE_LIST_VIEW (object);
  switch (prop_id)
    {
    case PROP_SINGLE_CLICK:
      xfce_list_view_set_single_click (list_view, g_value_get_boolean (value));
      break;
    case PROP_SINGLE_CLICK_TIMEOUT:
      xfce_list_view_set_single_click_timeout (list_view, g_value_get_uint (value));
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
    }
}
static void
xfce_list_view_column_editor_popup_check_button_toggled (GtkWidget *widget,
                                                         GtkWidget *list_view)
{
  gchar* column_id;
  gboolean current_state;
 
  column_id = (gchar*) g_object_get_data (G_OBJECT(widget), "column_id");
  current_state = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(widget));
  xfce_list_view_set_column_visible(XFCE_LIST_VIEW(list_view),column_id, current_state);
  xfce_list_view_column_editor_popup_refresh(XFCE_LIST_VIEW(list_view));
}
static void
xfce_list_view_column_editor_popup_down_button_clicked (GtkWidget *widget,
                                                         GtkWidget *list_view)
{
  gchar* column_id;
  gint current_pos;
 
  column_id = (gchar*) g_object_get_data (G_OBJECT(widget), "column_id");
  current_pos = xfce_list_view_get_column_position(XFCE_LIST_VIEW(list_view), column_id);
  xfce_list_view_insert_column_at_position(XFCE_LIST_VIEW(list_view), column_id, current_pos + 1);
  xfce_list_view_column_editor_popup_refresh(XFCE_LIST_VIEW(list_view));
}
static void
xfce_list_view_column_editor_popup_up_button_clicked (GtkWidget *widget,
                                                       GtkWidget *list_view)
{
  gchar* column_id;
  gint current_pos;
 
  column_id = (gchar*) g_object_get_data (G_OBJECT(widget), "column_id");
  current_pos = xfce_list_view_get_column_position(XFCE_LIST_VIEW(list_view), column_id);
  xfce_list_view_insert_column_at_position(XFCE_LIST_VIEW(list_view), column_id, current_pos - 1);
  xfce_list_view_column_editor_popup_refresh(XFCE_LIST_VIEW(list_view));
}
static void
xfce_list_view_column_editor_popup_refresh(XfceListView *list_view)
{
  GtkWidget *menu;
  GtkWidget *vbox;
  GtkWidget *checkbutton;
  GtkWidget *grid;
  GtkWidget *upbutton;
  GtkWidget *downbutton;
 
  GObject *col;
 
  gint index;
  gint max_index;
  gint appended_cols;
  gint insert_row;
  gint pos;
 
  menu = list_view->priv->popover;
  // Delete the old popover if it exists
  vbox = gtk_bin_get_child(GTK_BIN(menu));
  if (vbox != NULL)
    gtk_container_remove(GTK_CONTAINER(menu), vbox);
 
  // Now create the new popover
  vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 2);
  gtk_container_add(GTK_CONTAINER(menu), vbox);
  grid = gtk_grid_new();
  gtk_container_add(GTK_CONTAINER(vbox), grid);
  // Cycle through the columns
  index = 0;
  appended_cols = 0;
  max_index = g_list_model_get_n_items(G_LIST_MODEL(list_view->priv->possible_columns));
  col = g_list_model_get_item(G_LIST_MODEL(list_view->priv->possible_columns), index);
  while (col != NULL)
  {
    gchar* column_id = (gchar*)g_object_get_data(col, "column_id");
    gboolean is_vis = xfce_list_view_get_column_visible(list_view, column_id);
    pos = xfce_list_view_get_column_position(list_view, column_id);
    if (pos >= 0) insert_row = pos;
    else
    {
      // This column is hidden, so add it to the end of the list
      insert_row =  max_index - appended_cols;
      appended_cols++;
    }
   
    const gchar* title = gtk_tree_view_column_get_title (GTK_TREE_VIEW_COLUMN (col));
    checkbutton = gtk_check_button_new_with_label(title);
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(checkbutton), is_vis);
    upbutton = gtk_button_new_from_icon_name("go-up", GTK_ICON_SIZE_BUTTON);
    if ( (insert_row == 0) || (pos == -1) )
      gtk_widget_set_sensitive(GTK_WIDGET(upbutton), FALSE);
    downbutton = gtk_button_new_from_icon_name("go-down", GTK_ICON_SIZE_SMALL_TOOLBAR);
    if ( (pos == -1) )
      gtk_widget_set_sensitive(GTK_WIDGET(downbutton), FALSE);
    g_object_set_data(G_OBJECT(checkbutton), "column_id", column_id);
    g_object_set_data(G_OBJECT(upbutton), "column_id", column_id);
    g_object_set_data(G_OBJECT(downbutton), "column_id", column_id);
    gtk_grid_attach(GTK_GRID(grid), checkbutton, 0, insert_row, 1, 1);
    gtk_grid_attach(GTK_GRID(grid), upbutton, 1, insert_row, 1, 1);
    gtk_grid_attach(GTK_GRID(grid), downbutton, 2, insert_row, 1, 1);
    g_signal_connect(G_OBJECT(checkbutton), "toggled", G_CALLBACK(xfce_list_view_column_editor_popup_check_button_toggled), list_view);
    g_signal_connect(G_OBJECT(upbutton), "clicked", G_CALLBACK(xfce_list_view_column_editor_popup_up_button_clicked), list_view);
    g_signal_connect(G_OBJECT(downbutton), "clicked", G_CALLBACK(xfce_list_view_column_editor_popup_down_button_clicked), list_view);
    index ++;
    col = g_list_model_get_item(G_LIST_MODEL(list_view->priv->possible_columns), index);
  }
 
  // Desensitive the down button for the last visible col
  checkbutton = gtk_grid_get_child_at(GTK_GRID(grid), 2, ( (max_index - appended_cols) - 1));
  gtk_widget_set_sensitive(checkbutton, FALSE);
  gtk_widget_show_all (menu);
}
static gboolean
xfce_list_view_column_header_clicked   (XfceListView *list_view,
                                        GdkEventButton *event)
{
  GtkWidget *menu;
  GdkRectangle rect ;
 
  if (event->button == 3)
    {
      /* Need to determine ideal values for positioning */
      rect = (GdkRectangle) {.x = (int)event->x, .y = (int)event->y, .width = 1, .height = 1};
      menu = list_view->priv->popover;
      xfce_list_view_column_editor_popup_refresh(list_view);
     
      gtk_popover_set_relative_to(GTK_POPOVER(menu), GTK_WIDGET(list_view));
      gtk_popover_set_pointing_to(GTK_POPOVER(menu), &rect);
      gtk_widget_show_all (menu);
      gtk_popover_popup(GTK_POPOVER(menu));
      return TRUE;
    }
  return FALSE;
}
static gboolean
xfce_list_view_button_press_event (GtkWidget *widget,
                                   GdkEventButton *event)
{
  GtkTreeSelection *selection;
  XfceListView *list_view = XFCE_LIST_VIEW (widget);
  GtkTreePath *path = NULL;
  GtkTreePath *cursor_path_start = NULL;
  GtkTreePath *cursor_path_end = NULL;
  gboolean ctrl_shift_clicked = FALSE;
  gboolean result;
  gpointer drag_data;
  /* check if the event is for the bin window */
  if (G_UNLIKELY (event->window != gtk_tree_view_get_bin_window (GTK_TREE_VIEW(widget))))
    return xfce_list_view_column_header_clicked (list_view, event);
  /* by default we won't emit "row-activated" on button-release-events */
  list_view->priv->button_release_activates = FALSE;
  /* grab the tree selection */
  selection = gtk_tree_view_get_selection (GTK_TREE_VIEW (list_view));
  /* be sure to cancel any pending single-click timeout */
  if (G_UNLIKELY (list_view->priv->single_click_timeout_id >= 0))
    g_source_remove (list_view->priv->single_click_timeout_id);
  /* check if the button press was on the internal tree view window */
  if (G_LIKELY (event->window == gtk_tree_view_get_bin_window (GTK_TREE_VIEW (list_view))))
    {
      /* determine the path at the event coordinates */
      if (!gtk_tree_view_get_path_at_pos (GTK_TREE_VIEW (list_view), event->x, event->y, &path, NULL, NULL, NULL))
        path = NULL;
      /* we unselect all selected items if the user clicks on an empty
       * area of the tree view and no modifier key is active.
       */
      if (path == NULL && (event->state & gtk_accelerator_get_default_mod_mask ()) == 0)
        gtk_tree_selection_unselect_all (selection);
      /* completely ignore double-clicks in single-click mode */
      if (list_view->priv->single_click && event->type == GDK_2BUTTON_PRESS)
        {
          /* make sure we ignore the GDK_BUTTON_RELEASE
           * event for this GDK_2BUTTON_PRESS event.
           */
          gtk_tree_path_free (path);
          return TRUE;
        }
      /* check if the next button-release-event should activate the selected row (single click support) */
      list_view->priv->button_release_activates = (list_view->priv->single_click && event->type == GDK_BUTTON_PRESS && event->button == 1
                                                   && (event->state & gtk_accelerator_get_default_mod_mask ()) == 0);
    }
  /* Rubberbanding in GtkTreeView 2.9.0 and above is rather buggy, unfortunately, and
   * doesn't interact properly with GTKs own DnD mechanism. So we need to block all
   * dragging here when pressing the mouse button on a not yet selected row if
   * rubberbanding is active, or disable rubberbanding when starting a drag.
   */
  if (gtk_tree_selection_get_mode (selection) == GTK_SELECTION_MULTIPLE
      && gtk_tree_view_get_rubber_banding (GTK_TREE_VIEW (list_view))
      && event->button == 1 && event->type == GDK_BUTTON_PRESS)
    {
      /* check if clicked on empty area or on a not yet selected row */
      if (G_LIKELY (path == NULL || !gtk_tree_selection_path_is_selected (selection, path)))
        {
          /* need to disable drag and drop because we're rubberbanding now */
          drag_data = g_object_get_data (G_OBJECT (list_view), I_ ("gtk-site-data"));
          if (G_LIKELY (drag_data != NULL))
            {
              g_signal_handlers_block_matched (G_OBJECT (list_view),
                                               G_SIGNAL_MATCH_DATA,
                                               0, 0, NULL, NULL,
                                               drag_data);
            }
          /* remember to re-enable drag and drop later */
          list_view->priv->button_release_unblocks_dnd = TRUE;
        }
      else
        {
          /* need to disable rubberbanding because we're dragging now */
          gtk_tree_view_set_rubber_banding (GTK_TREE_VIEW (list_view), FALSE);
          /* remember to re-enable rubberbanding later */
          list_view->priv->button_release_enables_rubber_banding = TRUE;
        }
    }
  /* determine if a ctrl+shift click occurred */
  if (gtk_tree_selection_get_mode (selection) == GTK_SELECTION_MULTIPLE
      && event->button == 1 && event->type == GDK_BUTTON_PRESS
      && event->state & GDK_CONTROL_MASK && event->state & GDK_SHIFT_MASK)
    {
      ctrl_shift_clicked = TRUE;
      /* store old cursor position */
      gtk_tree_view_get_cursor (GTK_TREE_VIEW (list_view), &cursor_path_start, NULL);
    }
  /* unfortunately GtkTreeView will unselect rows except the clicked one,
   * which makes dragging from a GtkTreeView problematic.
   * So we temporary disable selection updates if the path is still selected
   */
  if (event->type == GDK_BUTTON_PRESS && (event->state & gtk_accelerator_get_default_mod_mask ()) == 0
      && path != NULL && gtk_tree_selection_path_is_selected (selection, path))
    {
      gtk_tree_selection_set_select_function (selection, select_false, NULL, NULL);
    }
  /* call the parent's button press handler */
  result = (*GTK_WIDGET_CLASS (xfce_list_view_parent_class)->button_press_event) (widget, event);
  /* Note that since we have already "chained up" by calling the parent's button press handler,
   * we must re-grab the tree selection, since the old one might be corrupted
   */
  selection = gtk_tree_view_get_selection (GTK_TREE_VIEW (list_view));
  if (GTK_IS_TREE_SELECTION (selection))
    {
      /* Re-enable selection updates */
      if (G_LIKELY (gtk_tree_selection_get_select_function (selection) == select_false))
        {
          gtk_tree_selection_set_select_function (selection, select_true, NULL, NULL);
        }
    }
  /* extend selection range on ctrl+shift click */
  if (ctrl_shift_clicked)
    {
      /* store new cursor position */
      gtk_tree_view_get_cursor (GTK_TREE_VIEW (list_view), &cursor_path_end, NULL);
      /* select all items between the old and new cursor position */
      if (cursor_path_start != NULL && cursor_path_end != NULL)
        gtk_tree_selection_select_range (selection, cursor_path_start, cursor_path_end);
      gtk_tree_path_free (cursor_path_start);
      gtk_tree_path_free (cursor_path_end);
    }
  /* release the path (if any) */
  if (G_LIKELY (path != NULL))
    gtk_tree_path_free (path);
  return result;
}
static gboolean
xfce_list_view_button_release_event (GtkWidget *widget,
                                     GdkEventButton *event)
{
  GtkTreeViewColumn *column;
  GtkTreeSelection *selection;
  GtkTreePath *path;
  XfceListView *list_view = XFCE_LIST_VIEW (widget);
  gpointer drag_data;
  /* verify that the release event is for the internal tree view window */
  if (G_LIKELY (event->window == gtk_tree_view_get_bin_window (GTK_TREE_VIEW (list_view))))
    {
      /* check if we're in single-click mode and the button-release-event should emit a "row-activate" */
      if (G_UNLIKELY (list_view->priv->single_click && list_view->priv->button_release_activates))
        {
          /* reset the "release-activates" flag */
          list_view->priv->button_release_activates = FALSE;
          /* determine the path to the row that should be activated */
          if (gtk_tree_view_get_path_at_pos (GTK_TREE_VIEW (list_view), event->x, event->y, &path, &column, NULL, NULL))
            {
              /* emit row-activated for the determined row */
              gtk_tree_view_row_activated (GTK_TREE_VIEW (list_view), path, column);
              /* cleanup */
              gtk_tree_path_free (path);
            }
        }
      else if ((event->state & gtk_accelerator_get_default_mod_mask ()) == 0 && !list_view->priv->button_release_unblocks_dnd)
        {
          /* determine the path on which the button was released and select only this row, this way
           * the user is still able to alter the selection easily if all rows are selected.
           */
          if (gtk_tree_view_get_path_at_pos (GTK_TREE_VIEW (list_view), event->x, event->y, &path, &column, NULL, NULL))
            {
              /* check if the path is selected */
              selection = gtk_tree_view_get_selection (GTK_TREE_VIEW (list_view));
              if (gtk_tree_selection_path_is_selected (selection, path))
                {
                  /* unselect all rows */
                  gtk_tree_selection_unselect_all (selection);
                  /* select the row and place the cursor on it */
                  gtk_tree_view_set_cursor (GTK_TREE_VIEW (list_view), path, column, FALSE);
                }
              /* cleanup */
              gtk_tree_path_free (path);
            }
        }
    }
  /* check if we need to re-enable drag and drop */
  if (G_LIKELY (list_view->priv->button_release_unblocks_dnd))
    {
      drag_data = g_object_get_data (G_OBJECT (list_view), I_ ("gtk-site-data"));
      if (G_LIKELY (drag_data != NULL))
        {
          g_signal_handlers_unblock_matched (G_OBJECT (list_view),
                                             G_SIGNAL_MATCH_DATA,
                                             0, 0, NULL, NULL,
                                             drag_data);
        }
      list_view->priv->button_release_unblocks_dnd = FALSE;
    }
  /* check if we need to re-enable rubberbanding */
  if (G_UNLIKELY (list_view->priv->button_release_enables_rubber_banding))
    {
      gtk_tree_view_set_rubber_banding (GTK_TREE_VIEW (list_view), TRUE);
      list_view->priv->button_release_enables_rubber_banding = FALSE;
    }
  /* call the parent's button release handler */
  return (*GTK_WIDGET_CLASS (xfce_list_view_parent_class)->button_release_event) (widget, event);
}
static gboolean
xfce_list_view_motion_notify_event (GtkWidget *widget,
                                    GdkEventMotion *event)
{
  XfceListView *list_view = XFCE_LIST_VIEW (widget);
  GtkTreePath *path;
  GdkCursor *cursor;
  /* check if the event occurred on the tree view internal window and we are in single-click mode */
  if (event->window == gtk_tree_view_get_bin_window (GTK_TREE_VIEW (list_view)) && list_view->priv->single_click)
    {
      /* check if we're doing a rubberband selection right now (which means DnD is blocked) */
      if (G_UNLIKELY (list_view->priv->button_release_unblocks_dnd))
        {
          /* we're doing a rubberband selection, so don't activate anything */
          list_view->priv->button_release_activates = FALSE;
          /* also be sure to reset the cursor */
          gdk_window_set_cursor (event->window, NULL);
        }
      else
        {
          /* determine the path at the event coordinates */
          if (!gtk_tree_view_get_path_at_pos (GTK_TREE_VIEW (list_view), event->x, event->y, &path, NULL, NULL, NULL))
            path = NULL;
          /* check if we have a new path */
          if ((path == NULL && list_view->priv->hover_path != NULL) || (path != NULL && list_view->priv->hover_path == NULL)
              || (path != NULL && list_view->priv->hover_path != NULL && gtk_tree_path_compare (path, list_view->priv->hover_path) != 0))
            {
              /* release the previous hover path */
              if (list_view->priv->hover_path != NULL)
                gtk_tree_path_free (list_view->priv->hover_path);
              /* setup the new path */
              list_view->priv->hover_path = path;
              /* check if we're over a row right now */
              if (G_LIKELY (path != NULL))
                {
                  /* setup the hand cursor to indicate that the row at the pointer can be activated with a single click */
                  cursor = gdk_cursor_new_for_display (gdk_window_get_display (event->window), GDK_HAND2);
                  gdk_window_set_cursor (event->window, cursor);
                  g_object_unref (cursor);
                }
              else
                {
                  /* reset the cursor to its default */
                  gdk_window_set_cursor (event->window, NULL);
                }
              /* check if autoselection is enabled and the pointer is over a row */
              if (G_LIKELY (list_view->priv->single_click_timeout > 0 && list_view->priv->hover_path != NULL))
                {
                  /* cancel any previous single-click timeout */
                  if (G_LIKELY (list_view->priv->single_click_timeout_id >= 0))
                    g_source_remove (list_view->priv->single_click_timeout_id);
                  /* remember the current event state */
                  list_view->priv->single_click_timeout_state = event->state;
                  /* schedule a new single-click timeout */
                  list_view->priv->single_click_timeout_id = gdk_threads_add_timeout_full (G_PRIORITY_LOW, list_view->priv->single_click_timeout,
                                                                                           xfce_list_view_single_click_timeout, list_view,
                                                                                           xfce_list_view_single_click_timeout_destroy);
                }
            }
          else
            {
              /* release the path resources */
              if (path != NULL)
                gtk_tree_path_free (path);
            }
        }
    }
  /* call the parent's motion notify handler */
  return (*GTK_WIDGET_CLASS (xfce_list_view_parent_class)->motion_notify_event) (widget, event);
}
static gboolean
xfce_list_view_leave_notify_event (GtkWidget *widget,
                                   GdkEventCrossing *event)
{
  XfceListView *list_view = XFCE_LIST_VIEW (widget);
  /* be sure to cancel any pending single-click timeout */
  if (G_UNLIKELY (list_view->priv->single_click_timeout_id >= 0))
    g_source_remove (list_view->priv->single_click_timeout_id);
  /* release and reset the hover path (if any) */
  if (list_view->priv->hover_path != NULL)
    {
      gtk_tree_path_free (list_view->priv->hover_path);
      list_view->priv->hover_path = NULL;
    }
  /* reset the cursor for the tree view internal window */
  if (gtk_widget_get_realized (GTK_WIDGET (list_view)))
    gdk_window_set_cursor (gtk_tree_view_get_bin_window (GTK_TREE_VIEW (list_view)), NULL);
  /* the next button-release-event should not activate */
  list_view->priv->button_release_activates = FALSE;
  /* call the parent's leave notify handler */
  return (*GTK_WIDGET_CLASS (xfce_list_view_parent_class)->leave_notify_event) (widget, event);
}
static void
xfce_list_view_drag_begin (GtkWidget *widget,
                           GdkDragContext *context)
{
  XfceListView *list_view = XFCE_LIST_VIEW (widget);
  /* the next button-release-event should not activate */
  list_view->priv->button_release_activates = FALSE;
  /* call the parent's drag begin handler */
  (*GTK_WIDGET_CLASS (xfce_list_view_parent_class)->drag_begin) (widget, context);
}
static gboolean
xfce_list_view_move_cursor (GtkTreeView *view,
                            GtkMovementStep step,
                            gint count)
{
  XfceListView *list_view = XFCE_LIST_VIEW (view);
  /* be sure to cancel any pending single-click timeout */
  if (G_UNLIKELY (list_view->priv->single_click_timeout_id >= 0))
    g_source_remove (list_view->priv->single_click_timeout_id);
  /* release and reset the hover path (if any) */
  if (list_view->priv->hover_path != NULL)
    {
      gtk_tree_path_free (list_view->priv->hover_path);
      list_view->priv->hover_path = NULL;
    }
  /* reset the cursor for the tree view internal window */
  if (gtk_widget_get_realized (GTK_WIDGET (list_view)))
    gdk_window_set_cursor (gtk_tree_view_get_bin_window (GTK_TREE_VIEW (list_view)), NULL);
  /* call the parent's handler */
  return (*GTK_TREE_VIEW_CLASS (xfce_list_view_parent_class)->move_cursor) (view, step, count);
}
static gboolean
xfce_list_view_single_click_timeout (gpointer user_data)
{
  GtkTreeViewColumn *cursor_column;
  GtkTreeSelection *selection;
  GtkTreeModel *model;
  GtkTreePath *cursor_path;
  GtkTreeIter iter;
  XfceListView *list_view = XFCE_LIST_VIEW (user_data);
  gboolean hover_path_selected;
  GtkWidget *toplevel = gtk_widget_get_toplevel (GTK_WIDGET (list_view));
  /* verify that we are in single-click mode on an active window and have a hover path */
  if (GTK_IS_WINDOW (toplevel) && gtk_window_is_active (GTK_WINDOW (toplevel)) && list_view->priv->single_click && list_view->priv->hover_path != NULL)
    {
      gtk_widget_grab_focus (GTK_WIDGET (list_view));
      /* transform the hover_path to a tree iterator */
      model = gtk_tree_view_get_model (GTK_TREE_VIEW (list_view));
      if (model != NULL && gtk_tree_model_get_iter (model, &iter, list_view->priv->hover_path))
        {
          /* determine the current cursor path/column */
          gtk_tree_view_get_cursor (GTK_TREE_VIEW (list_view), &cursor_path, &cursor_column);
          /* be sure the row is fully visible */
          gtk_tree_view_scroll_to_cell (GTK_TREE_VIEW (list_view), list_view->priv->hover_path, cursor_column, FALSE, 0.0f, 0.0f);
          /* determine the selection and change it appropriately */
          selection = gtk_tree_view_get_selection (GTK_TREE_VIEW (list_view));
          if (gtk_tree_selection_get_mode (selection) == GTK_SELECTION_NONE)
            {
              /* just place the cursor on the row */
              gtk_tree_view_set_cursor (GTK_TREE_VIEW (list_view), list_view->priv->hover_path, cursor_column, FALSE);
            }
          else if ((list_view->priv->single_click_timeout_state & GDK_SHIFT_MASK) != 0
                   && gtk_tree_selection_get_mode (selection) == GTK_SELECTION_MULTIPLE)
            {
              /* check if the item is not already selected (otherwise do nothing) */
              if (!gtk_tree_selection_path_is_selected (selection, list_view->priv->hover_path))
                {
                  /* skip if control is being pressed */
                  if ((list_view->priv->single_click_timeout_state & GDK_CONTROL_MASK) == 0)
                    {
                      /* unselect all previously selected items */
                      gtk_tree_selection_unselect_all (selection);
                    }
                  /* since we cannot access the anchor of a GtkTreeView, we
                   * use the cursor instead which is usually the same row.
                   */
                  if (G_UNLIKELY (cursor_path == NULL))
                    {
                      /* place the cursor on the new row */
                      gtk_tree_view_set_cursor (GTK_TREE_VIEW (list_view), list_view->priv->hover_path, cursor_column, FALSE);
                    }
                  else
                    {
                      /* select all between the cursor and the current row */
                      gtk_tree_selection_select_range (selection, list_view->priv->hover_path, cursor_path);
                    }
                }
            }
          else
            {
              /* check if the hover path is selected (as it will be selected after the set_cursor() call) */
              hover_path_selected = gtk_tree_selection_path_is_selected (selection, list_view->priv->hover_path);
              /* disable selection updates if the path is still selected */
              gtk_tree_selection_set_select_function (selection, select_false, NULL, NULL);
              /* place the cursor on the hover row */
              gtk_tree_view_set_cursor (GTK_TREE_VIEW (list_view), list_view->priv->hover_path, cursor_column, FALSE);
              /* re-enable selection updates */
              gtk_tree_selection_set_select_function (selection, select_true, NULL, NULL);
              /* check what to do */
              if ((gtk_tree_selection_get_mode (selection) == GTK_SELECTION_MULTIPLE || (gtk_tree_selection_get_mode (selection) == GTK_SELECTION_SINGLE && hover_path_selected))
                  && (list_view->priv->single_click_timeout_state & GDK_CONTROL_MASK) != 0)
                {
                  /* toggle the selection state of the row */
                  if (G_UNLIKELY (hover_path_selected))
                    gtk_tree_selection_unselect_path (selection, list_view->priv->hover_path);
                  else
                    gtk_tree_selection_select_path (selection, list_view->priv->hover_path);
                }
              else if (G_UNLIKELY (!hover_path_selected))
                {
                  /* unselect all other rows */
                  gtk_tree_selection_unselect_all (selection);
                  /* select only the hover row */
                  gtk_tree_selection_select_path (selection, list_view->priv->hover_path);
                }
            }
          /* cleanup */
          if (G_LIKELY (cursor_path != NULL))
            gtk_tree_path_free (cursor_path);
        }
    }
  return FALSE;
}
static void
xfce_list_view_single_click_timeout_destroy (gpointer user_data)
{
  XFCE_LIST_VIEW (user_data)->priv->single_click_timeout_id = -1;
}
gboolean
xfce_list_view_column_equal_id (gconstpointer a,
                                gconstpointer b,
                                gpointer user_data)
{
  if (!g_strcmp0 (g_object_get_data (G_OBJECT (a), "column_id"), user_data))
    return TRUE;
  else
    return FALSE;
}
gint
xfce_list_view_column_equal_id_2 (gconstpointer a,
                                  gconstpointer b)
{
  if (!g_strcmp0 (g_object_get_data (G_OBJECT (a), "column_id"), b))
    return 0;
  else
    return 1;
}
static gboolean
select_true (GtkTreeSelection *selection,
             GtkTreeModel *model,
             GtkTreePath *path,
             gboolean path_currently_selected,
             gpointer data)
{
  return TRUE;
}
static gboolean
select_false (GtkTreeSelection *selection,
              GtkTreeModel *model,
              GtkTreePath *path,
              gboolean path_currently_selected,
              gpointer data)
{
  return FALSE;
}
/**
 * xfce_list_view_new:
 *
 * Allocates a new #XfceListView instance.
 *
 * Returns: the newly allocated #XfceListView.
 *
 * Since: 4.21
 **/
GtkWidget *
xfce_list_view_new (void)
{
  return g_object_new (XFCE_TYPE_LIST_VIEW, NULL);
}
/**
 * xfce_list_view_get_single_click:
 * @list_view : an #XfceListView.
 *
 * Returns %TRUE if @list_view is in single-click mode, else %FALSE.
 *
 * Returns: whether @list_view is in single-click mode.
 *
 * Since: 4.21
 **/
gboolean
xfce_list_view_get_single_click (const XfceListView *list_view)
{
  //~ g_return_val_if_fail (XFCE_IS_LIST_VIEW (list_view), FALSE);
  return list_view->priv->single_click;
}
/**
 * xfce_list_view_set_single_click:
 * @list_view    : an #XfceListView.
 * @single_click : %TRUE to use single-click for @list_view, %FALSE otherwise.
 *
 * If @single_click is %TRUE, @list_view will use single-click mode, else
 * the default double-click mode will be used.
 *
 * Since: 4.21
 **/
void
xfce_list_view_set_single_click (XfceListView *list_view,
                                 gboolean single_click)
{
  g_return_if_fail (XFCE_IS_LIST_VIEW (list_view));
  if (list_view->priv->single_click != !!single_click)
    {
      list_view->priv->single_click = !!single_click;
      g_object_notify (G_OBJECT (list_view), "single-click");
    }
}
/**
 * xfce_list_view_get_single_click_timeout:
 * @list_view : a #XfceListView.
 *
 * Returns the amount of time in milliseconds after which the
 * item under the mouse cursor will be selected automatically
 * in single click mode. A value of %0 means that the behavior
 * is disabled and the user must alter the selection manually.
 *
 * Returns: the single click autoselect timeout or %0 if
 *               the behavior is disabled.
 *
 * Since: 4.21
 **/
guint
xfce_list_view_get_single_click_timeout (const XfceListView *list_view)
{
  //~ g_return_val_if_fail (XFCE_IS_LIST_VIEW (list_view), 0u);
  return list_view->priv->single_click_timeout;
}
/**
 * xfce_list_view_set_single_click_timeout:
 * @list_view            : a #XfceListView.
 * @single_click_timeout : the new timeout or %0 to disable.
 *
 * If @single_click_timeout is a value greater than zero, it specifies
 * the amount of time in milliseconds after which the item under the
 * mouse cursor will be selected automatically in single click mode.
 * A value of %0 for @single_click_timeout disables the autoselection
 * for @list_view.
 *
 * This setting does not have any effect unless the @list_view is in
 * single-click mode, see xfce_list_view_set_single_click().
 *
 * Since: 4.21
 **/
void
xfce_list_view_set_single_click_timeout (XfceListView *list_view,
                                         guint single_click_timeout)
{
  g_return_if_fail (XFCE_IS_LIST_VIEW (list_view));
  /* check if we have a new setting */
  if (list_view->priv->single_click_timeout != single_click_timeout)
    {
      /* apply the new setting */
      list_view->priv->single_click_timeout = single_click_timeout;
      /* be sure to cancel any pending single click timeout */
      if (G_UNLIKELY (list_view->priv->single_click_timeout_id >= 0))
        g_source_remove (list_view->priv->single_click_timeout_id);
      /* notify listeners */
      g_object_notify (G_OBJECT (list_view), "single-click-timeout");
    }
}
/**
 * xfce_list_view_add_possible_column:
 * @list_view     : a #XfceListView
 * @column_id     : a unique string used to identify the column. Do not transalate!
 * @column_title  : the header diaplayed above the column
 *
 * This registers a column with @list_view. It does not display the
 * column. This function must be called before attempting to interact
 * with the column. You generally do not need to use the returned
 * widget unless you intend to modify the default rendering. @list_view
 * will be able to retrieve the column by @column_id.
 *
 * Returns :(transfer none): The newly created #GtkTreeViewColumn
 *
 * Since: 4.21
 *
 **/
GtkTreeViewColumn*
xfce_list_view_add_possible_column (const XfceListView *list_view,
                                    const gchar *column_id,
                                    const gchar *column_title)
{
  GListStore *possible_columns = list_view->priv->possible_columns;
  GtkTreeViewColumn *column = gtk_tree_view_column_new ();
  gtk_tree_view_column_set_title (column, column_title);
  gtk_tree_view_column_set_reorderable (column, TRUE);
  g_object_set_data (G_OBJECT (column), "column_id", (gpointer) column_id);
  g_list_store_append (possible_columns, column);
  return column;
}
/**
 * xfce_list_view_get_column:
 * @list_view  : a #XfceListView
 * @column_id  : the id used to register the column
 *
 * This function can be used to retrieve a previously registered column.
 * This function will return columns regardless of whether they are
 * in active use.
 *
 * Returns:(transfer none): A #GtkTreeViewColumn or %NULL if no column was found
 *
 **/
GtkTreeViewColumn *
xfce_list_view_get_column (XfceListView *list_view,
                           gchar *column_id)
{
  GListStore *possible_columns = list_view->priv->possible_columns;
  guint pos = INT_MAX;
  // We have to pass a dummy tree column because GIO doesn't support a NULL until v2.76
  g_list_store_find_with_equal_func_full (possible_columns, gtk_tree_view_column_new (), xfce_list_view_column_equal_id, column_id, &pos);
  if (pos != INT_MAX)
    {
      GObject *obj = g_list_model_get_item (G_LIST_MODEL (possible_columns), pos);
      return GTK_TREE_VIEW_COLUMN (obj);
    }
  else
    {
      return NULL;
    }
}
/**
 * xfce_list_view_set_column_visible:
 * @list_view    : a #XfceListView
 * @column_id    : the column to adjust
 * @visible      : the new visibility setting
 *
 * This function can be used to toggle the visibility of a column.
 *
 * Passing the current value will have no effect.
 *
 * Passing %FALSE will not remove the column from the @list_view widget,
 * but will not delete the column, and it can be restored later.
 *
 * Passing %TRUE will append the column to the end of @list_view. Use
 * xfce_list_view_insert_column_at_position if you want to control where
 * the column will appear.
 *
 * Warning: this function is not idempotent.
 * Setting the visibility to %FALSE and then %TRUE will likely adjust
 * the ordering of columns. Use other functions such as
 * xfce_list_view_insert_column_at_position or
 * xfce_list_view_get_column_position if preserving the order is
 * important.
 *
 * Since: 4.21
 **/
void
xfce_list_view_set_column_visible (XfceListView *list_view,
                                   const gchar *column_id,
                                   const gboolean visible)
{
  gboolean curr = xfce_list_view_get_column_visible (list_view, column_id);
  if (curr == visible)
    return;
  if (visible)
    {
      GtkTreeViewColumn *col = xfce_list_view_get_column (XFCE_LIST_VIEW (list_view), (gchar *) column_id);
      gtk_tree_view_insert_column (GTK_TREE_VIEW (list_view), col, -1);
    }
  else
    {
      GtkTreeViewColumn *col = xfce_list_view_get_column (XFCE_LIST_VIEW (list_view), (gchar *) column_id);
      gtk_tree_view_remove_column (GTK_TREE_VIEW (list_view), col);
    }
}
/**
 * xfce_list_view_get_column_visible
 * @list_view  : a #XfceListView
 * @column_id  : the id of the column
 *
 * Query the current visibility of a column. This will only check if
 * the column is currently visible, and not if it is a valid column. It
 * is assumed that you either registered @column_id yourself or else
 * verified it first with xfce_list_view_get_column. Otherwise,
 * xfce_list_view_get_column_position can be used.
 *
 * Returns: %TRUE if the column is visible, %FALSE if it is hidden or
 *          nonexistent
 *
 * Since: 4.21
 **/
gboolean
xfce_list_view_get_column_visible (const XfceListView *list_view,
                                   const gchar *column_id)
{
  GList *cols = gtk_tree_view_get_columns (GTK_TREE_VIEW (list_view));
  gpointer found_col = g_list_find_custom (cols, column_id, xfce_list_view_column_equal_id_2);
  if (found_col != NULL)
    return TRUE;
  else
    return FALSE;
}
/**
 * xfce_list_view_get_column_position:
 * @list_view  : a #XfceListView
 * @column_id  : the id of the column
 *
 * Query the current position of the column
 *
 * Returns : The current position of the column, or -1 if it is hidden
 *
 * Since : 4.21
 **/
guint
xfce_list_view_get_column_position (const XfceListView *list_view,
                                    const gchar *column_id)
{
  GList *cols = gtk_tree_view_get_columns (GTK_TREE_VIEW (list_view));
  gpointer found_col = g_list_find_custom (cols, column_id, xfce_list_view_column_equal_id_2);
  gint pos = g_list_position (cols, (GList *) found_col);
  return pos;
}
/**
 * xfce_list_view_insert_column_at_position:
 * @list_view : a #XfceListView
 * @column_id : the id of the column
 * @position  : the position at which to insert the column
 *
 * Inserts the given column at the sepcified position. This can be used
 * for both visible and hidden columns. In the former case, the column
 * will be repositioned, otherwise it will be added. Passing -1 as the
 * position will append the column to the end of the visible columns
 *
 * Since : 4.21
 **/
void
xfce_list_view_insert_column_at_position (XfceListView *list_view,
                                          gchar *column_id,
                                          const guint position)
{
  GtkTreeViewColumn *col = xfce_list_view_get_column (list_view, (gchar *) column_id);
  g_return_if_fail (col != NULL);
  if (gtk_tree_view_column_get_tree_view (GTK_TREE_VIEW_COLUMN (col)) != NULL)
    gtk_tree_view_remove_column (GTK_TREE_VIEW (list_view), col);
  gtk_tree_view_insert_column (GTK_TREE_VIEW (list_view), col, position);
}
/**
 * xfce_list_view_bind_model:
 * @list_view : a #XfceListView
 * @model     : a #GtkTreeModel
 *
 * Binds @model to the list_view
 *
 * Since : 4.21
 **/
void
xfce_list_view_bind_model (XfceListView *list_view,
                           GtkTreeModel *model)
{
  gtk_tree_view_set_model (GTK_TREE_VIEW (list_view), model);
}
/**
 * xfce_list_view_get_model:
 * @list_view : a #XfceListView
 *
 * Returns :(transfer none): The #GtkTreeModel bound to @list_view
 * Since: 4.21
 **/
GtkTreeModel *
xfce_list_view_get_model (XfceListView *list_view)
{
  return gtk_tree_view_get_model (GTK_TREE_VIEW (list_view));
}
/**
 * xfce_list_view_render_text:
 * @list_view : a #XfceListView
 * @column_id : the id of the column
 * @column    : the model column
 *
 * Binds the text in @column of @list_view 's model to @column_id.
 * This function handles creating and binding the cell renderers for
 * cases in which the text is displayed directl from the model.
 *
 * Since: 4.21
 **/
void
xfce_list_view_render_text (XfceListView *list_view,
                            const gchar *column_id,
                            gint column)
{
  GtkCellRenderer *renderer = gtk_cell_renderer_text_new ();
  GtkTreeViewColumn *col = xfce_list_view_get_column (list_view, (gchar *) column_id);
  g_return_if_fail (col != NULL);
  gtk_tree_view_column_pack_start (col, renderer, TRUE);
  gtk_tree_view_column_set_sort_column_id (col, column);
  gtk_tree_view_column_set_attributes (GTK_TREE_VIEW_COLUMN (col), GTK_CELL_RENDERER (renderer), "text", column, NULL);
}
/**
 * xfce_list_view_render_text_with_func:
 * @list_view : a #XfceListView
 * @column_id : the id of the column
 * @func      :(scope forever): the custom render function
 *
 * Creates and binds a cell renderer to @column_id. It uses @func to
 * bind the data to the cell renderer. Use this if text needs to be
 * combined from multiple columns or formatted before display. Otherwise,
 * xfce_list_view_render_text is a better choice.
 *
 * Since: 4.21
 **/
void
xfce_list_view_render_text_with_func (XfceListView *list_view,
                                      const gchar *column_id,
                                      GtkTreeCellDataFunc func)
{
  GtkCellRenderer *renderer = gtk_cell_renderer_text_new ();
  GtkTreeViewColumn *col = xfce_list_view_get_column (list_view, (gchar *) column_id);
  g_return_if_fail (col != NULL);
  gtk_tree_view_column_pack_start (col, renderer, TRUE);
  gtk_tree_view_column_set_cell_data_func (GTK_TREE_VIEW_COLUMN (col), GTK_CELL_RENDERER (renderer), func, NULL, NULL);
}
/**
 * xfce_list_view_render_pixbuf_text:
 * @list_view : a #XfceListView
 * @column_id : the id of the column
 * @pixbuf_column : the model column containing the pixbuf
 * @text_column   : the model column containing the text
 *
 * Creates and binds two cell renderers to @column_id. The first
 * displays a pixbuf from @pixbuf_column of the model. The second
 * displays the text in @text_column of the model.
 *
 * Since: 4.21
 **/
void
xfce_list_view_render_pixbuf_text (XfceListView *list_view,
                                   const gchar *column_id,
                                   gint pixbuf_column,
                                   gint text_column)
{
  GtkTreeViewColumn *col = xfce_list_view_get_column (list_view, (gchar *) column_id);
  g_return_if_fail (col != NULL);
  GtkCellRenderer *pix_renderer = gtk_cell_renderer_pixbuf_new ();
  gtk_tree_view_column_pack_start (col, pix_renderer, FALSE);
  gtk_tree_view_column_set_sort_column_id (col, text_column);
  gtk_tree_view_column_set_attributes (GTK_TREE_VIEW_COLUMN (col), GTK_CELL_RENDERER (pix_renderer), "pixbuf", pixbuf_column, NULL);
  GtkCellRenderer *text_renderer = gtk_cell_renderer_text_new ();
  gtk_tree_view_column_pack_start (col, text_renderer, TRUE);
  gtk_tree_view_column_set_attributes (GTK_TREE_VIEW_COLUMN (col), GTK_CELL_RENDERER (text_renderer), "text", text_column, NULL);
}
/**
 * xfce_list_view_serialize_state:
 * @list_view : a #XfceListView
 *
 * Generate a sring representation of the current column ordering and visibility
 * of @list_view.
 *
 *  This representation can be stored and used to restore the state at
 * a later time.
 *
 * Returns : The strig representation of the current state
 *
 * Since: 4.21
 **/
gchar *
xfce_list_view_serialize_state (XfceListView *list_view)
{
  gchar *state = "";
  GList *cols = gtk_tree_view_get_columns (GTK_TREE_VIEW (list_view));
  while (cols != NULL)
    {
      gchar *id = g_object_get_data (G_OBJECT (cols->data), "column_id");
      if (strcmp (state, "") == 0)
        state = id;
      else
        state = g_strconcat (state, ";", id, NULL);
      cols = cols->next;
    }
  return state;
}
/**
 * xfce_list_view_set_state_from_string:
 * @list_view : a #XfceListView
 * @state     : a string representing the column ordering
 *
 * Sets the column ordering of @list_view to match the state represented
 * by @state.
 *
 * This string is generally obtained from xfce_list_view_serialize_state,
 * but can be generated manually. This function does not create any
 * columns that do not exist, and will ignore column ids it does not
 * recognize. Thus, it is important to initialize @list_view using
 * xfce_list_view_add_possible_column with the relevant columns
 * before calling this function.
 *
 * Since: 4.21
 **/
void
xfce_list_view_set_state_from_string (XfceListView *list_view,
                                      gchar *state)
{
  GList *cols = gtk_tree_view_get_columns (GTK_TREE_VIEW (list_view));
  while (cols != NULL)
    {
      gtk_tree_view_remove_column (GTK_TREE_VIEW (list_view), GTK_TREE_VIEW_COLUMN (cols->data));
      cols = cols->next;
    }
  gchar **col_array = g_strsplit (state, ";", 0);
  int i = 0;
  while (col_array[i] != NULL)
    {
      xfce_list_view_insert_column_at_position (XFCE_LIST_VIEW (list_view), col_array[i], -1);
      i++;
    }
}
#define __XFCE_LIST_VIEW_C__
#include "libxfce4ui-visibility.c"
