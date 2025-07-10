/*-
 * Copyright (c) 2008       Jannis Pohlmann <jannis@xfce.org>
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

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#ifdef HAVE_MATH_H
#include <math.h>
#endif
#ifdef HAVE_MEMORY_H
#include <memory.h>
#endif
#ifdef HAVE_STDLIB_H
#include <stdlib.h>
#endif
#ifdef HAVE_STRING_H
#include <string.h>
#endif

#include <gdk/gdkkeysyms.h>
#include <libxfce4util/libxfce4util.h>

#include "libxfce4ui-enum-types.h"
#include "libxfce4ui-marshal.h"
#include "libxfce4ui-private.h"
#include "xfce-cell-renderer-icon.h"
#include "xfce-gtk-extensions.h"
#include "xfce-icon-view.h"
#include "libxfce4ui-visibility.h"

/**
 * SECTION: xfce-icon-view
 * @title: XfceIconView
 * @short_description: A widget which displays a list of icons in a grid
 * @include: libxfce4ui/libxfce4ui.h
 *
 * #XfceIconView provides an alternative view on a list model.
 * It displays the model as a grid of icons with labels. Like
 * #GtkTreeView, it allows to select one or multiple items
 * (depending on the selection mode, see xfce_icon_view_set_selection_mode()).
 * In addition to selection with the arrow keys, #XfceIconView supports
 * rubberband selection, which is controlled by dragging the pointer.
 **/

/* resurrect dead gdk apis for Gtk3
 * This is easier than using #ifs everywhere
 */

#define GdkRectangle cairo_rectangle_int_t
#define GdkRegion cairo_region_t
#define gdk_region_rectangle(rect) cairo_region_create_rectangle (rect)
#define gdk_region_destroy(region) cairo_region_destroy (region)
#define gdk_region_subtract(dst, other) cairo_region_subtract (dst, other)
#define gdk_region_union_with_rect(dst, rect) cairo_region_union_rectangle (dst, rect)

#ifdef gdk_cursor_unref
#undef gdk_cursor_unref
#endif
#define gdk_cursor_unref(cursor) g_object_unref (cursor)

/* the search dialog timeout (in ms) */
#define XFCE_ICON_VIEW_SEARCH_DIALOG_TIMEOUT (5000)

#define SCROLL_EDGE_SIZE 15



/* Property identifiers */
enum
{
  PROP_0,
  PROP_PIXBUF_COLUMN,
  PROP_ICON_COLUMN,
  PROP_TEXT_COLUMN,
  PROP_MARKUP_COLUMN,
  PROP_SELECTION_MODE,
  PROP_LAYOUT_MODE,
  PROP_ORIENTATION,
  PROP_MODEL,
  PROP_COLUMNS,
  PROP_ITEM_WIDTH,
  PROP_SPACING,
  PROP_ROW_SPACING,
  PROP_COLUMN_SPACING,
  PROP_MARGIN,
  PROP_REORDERABLE,
  PROP_SINGLE_CLICK,
  PROP_SINGLE_CLICK_TIMEOUT,
  PROP_ENABLE_SEARCH,
  PROP_SEARCH_COLUMN,
  PROP_HADJUSTMENT,
  PROP_VADJUSTMENT,
  PROP_VSCROLL_POLICY,
  PROP_HSCROLL_POLICY
};

/* Signal identifiers */
enum
{
  ITEM_ACTIVATED,
  SELECTION_CHANGED,
  SELECT_ALL,
  UNSELECT_ALL,
  SELECT_CURSOR_ITEM,
  TOGGLE_CURSOR_ITEM,
  MOVE_CURSOR,
  ACTIVATE_CURSOR_ITEM,
  START_INTERACTIVE_SEARCH,
  LAST_SIGNAL
};

/* Icon view flags */
typedef enum
{
  XFCE_ICON_VIEW_DRAW_KEYFOCUS = (1l << 0), /* whether to draw keyboard focus */
  XFCE_ICON_VIEW_ITERS_PERSIST = (1l << 1), /* whether current model provides persistent iterators */
} XfceIconViewFlags;

#define XFCE_ICON_VIEW_SET_FLAG(icon_view, flag) \
  G_STMT_START { (get_instance_private (icon_view)->flags |= flag); } \
  G_STMT_END
#define XFCE_ICON_VIEW_UNSET_FLAG(icon_view, flag) \
  G_STMT_START { (get_instance_private (icon_view)->flags &= ~(flag)); } \
  G_STMT_END
#define XFCE_ICON_VIEW_FLAG_SET(icon_view, flag) \
  ((get_instance_private (icon_view)->flags & (flag)) == (flag))



typedef struct _XfceIconViewCellInfo XfceIconViewCellInfo;
typedef struct _XfceIconViewChild XfceIconViewChild;
typedef struct _XfceIconViewItem XfceIconViewItem;



#define XFCE_ICON_VIEW_CELL_INFO(obj) ((XfceIconViewCellInfo *) (obj))
#define XFCE_ICON_VIEW_CHILD(obj) ((XfceIconViewChild *) (obj))
#define XFCE_ICON_VIEW_ITEM(obj) ((XfceIconViewItem *) (obj))
#define get_instance_private(instance) \
  ((XfceIconViewPrivate *) xfce_icon_view_get_instance_private (XFCE_ICON_VIEW (instance)))



static void
xfce_icon_view_cell_layout_init (GtkCellLayoutIface *iface);
static void
xfce_icon_view_dispose (GObject *object);
static void
xfce_icon_view_finalize (GObject *object);
static void
xfce_icon_view_get_property (GObject *object,
                             guint prop_id,
                             GValue *value,
                             GParamSpec *pspec);
static void
xfce_icon_view_set_property (GObject *object,
                             guint prop_id,
                             const GValue *value,
                             GParamSpec *pspec);
static void
xfce_icon_view_realize (GtkWidget *widget);
static void
xfce_icon_view_unrealize (GtkWidget *widget);
static void
xfce_icon_view_get_preferred_width (GtkWidget *widget,
                                    gint *minimal_width,
                                    gint *natural_width);
static void
xfce_icon_view_get_preferred_height (GtkWidget *widget,
                                     gint *minimal_height,
                                     gint *natural_height);
static void
xfce_icon_view_size_allocate (GtkWidget *widget,
                              GtkAllocation *allocation);
static gboolean
xfce_icon_view_draw (GtkWidget *widget,
                     cairo_t *cr);
static gboolean
xfce_icon_view_motion_notify_event (GtkWidget *widget,
                                    GdkEventMotion *event);
static gboolean
xfce_icon_view_button_press_event (GtkWidget *widget,
                                   GdkEventButton *event);
static gboolean
xfce_icon_view_button_release_event (GtkWidget *widget,
                                     GdkEventButton *event);
static gboolean
xfce_icon_view_scroll_event (GtkWidget *widget,
                             GdkEventScroll *event);
static gboolean
xfce_icon_view_key_press_event (GtkWidget *widget,
                                GdkEventKey *event);
static gboolean
xfce_icon_view_focus_out_event (GtkWidget *widget,
                                GdkEventFocus *event);
static gboolean
xfce_icon_view_leave_notify_event (GtkWidget *widget,
                                   GdkEventCrossing *event);
static void
xfce_icon_view_remove (GtkContainer *container,
                       GtkWidget *widget);
static void
xfce_icon_view_forall (GtkContainer *container,
                       gboolean include_internals,
                       GtkCallback callback,
                       gpointer callback_data);
static void
xfce_icon_view_set_adjustments (XfceIconView *icon_view,
                                GtkAdjustment *hadj,
                                GtkAdjustment *vadj);
static void
xfce_icon_view_real_select_all (XfceIconView *icon_view);
static void
xfce_icon_view_real_unselect_all (XfceIconView *icon_view);
static void
xfce_icon_view_real_select_cursor_item (XfceIconView *icon_view);
static void
xfce_icon_view_real_toggle_cursor_item (XfceIconView *icon_view);
static gboolean
xfce_icon_view_real_activate_cursor_item (XfceIconView *icon_view);
static gboolean
xfce_icon_view_real_start_interactive_search (XfceIconView *icon_view);
static void
xfce_icon_view_adjustment_changed (GtkAdjustment *adjustment,
                                   XfceIconView *icon_view);
static gint
xfce_icon_view_layout_cols (XfceIconView *icon_view,
                            gint item_height,
                            gint *x,
                            gint *maximum_height,
                            gint max_rows);
static gint
xfce_icon_view_layout_rows (XfceIconView *icon_view,
                            gint item_width,
                            gint *y,
                            gint *maximum_width,
                            gint max_cols);
static void
xfce_icon_view_layout (XfceIconView *icon_view);
static void
xfce_icon_view_paint_item (XfceIconView *icon_view,
                           XfceIconViewItem *item,
                           cairo_t *cr,
                           gint x,
                           gint y,
                           gboolean draw_focus);
static void
xfce_icon_view_queue_draw_item (XfceIconView *icon_view,
                                XfceIconViewItem *item);
static void
xfce_icon_view_queue_layout (XfceIconView *icon_view);
static void
xfce_icon_view_set_cursor_item (XfceIconView *icon_view,
                                XfceIconViewItem *item,
                                gint cursor_cell);
static void
xfce_icon_view_start_rubberbanding (XfceIconView *icon_view,
                                    gint x,
                                    gint y);
static void
xfce_icon_view_stop_rubberbanding (XfceIconView *icon_view);
static void
xfce_icon_view_update_rubberband_selection (XfceIconView *icon_view);
static gboolean
xfce_icon_view_item_hit_test (XfceIconView *icon_view,
                              XfceIconViewItem *item,
                              gint x,
                              gint y,
                              gint width,
                              gint height);
static gboolean
xfce_icon_view_unselect_all_internal (XfceIconView *icon_view);
static void
xfce_icon_view_calculate_item_size (XfceIconView *icon_view,
                                    XfceIconViewItem *item);
static void
xfce_icon_view_calculate_item_size2 (XfceIconView *icon_view,
                                     XfceIconViewItem *item,
                                     gint *max_width,
                                     gint *max_height);
static void
xfce_icon_view_update_rubberband (gpointer data);
static void
xfce_icon_view_invalidate_sizes (XfceIconView *icon_view);
static void
xfce_icon_view_add_move_binding (GtkBindingSet *binding_set,
                                 guint keyval,
                                 guint modmask,
                                 GtkMovementStep step,
                                 gint count);
static gboolean
xfce_icon_view_real_move_cursor (XfceIconView *icon_view,
                                 GtkMovementStep step,
                                 gint count);
static void
xfce_icon_view_move_cursor_up_down (XfceIconView *icon_view,
                                    gint count);
static void
xfce_icon_view_move_cursor_page_up_down (XfceIconView *icon_view,
                                         gint count);
static void
xfce_icon_view_move_cursor_left_right (XfceIconView *icon_view,
                                       gint count);
static void
xfce_icon_view_move_cursor_start_end (XfceIconView *icon_view,
                                      gint count);
static void
xfce_icon_view_scroll_to_item (XfceIconView *icon_view,
                               XfceIconViewItem *item);
static void
xfce_icon_view_select_item (XfceIconView *icon_view,
                            XfceIconViewItem *item);
static void
xfce_icon_view_unselect_item (XfceIconView *icon_view,
                              XfceIconViewItem *item);
static gboolean
xfce_icon_view_select_all_between (XfceIconView *icon_view,
                                   XfceIconViewItem *anchor,
                                   XfceIconViewItem *cursor);
static XfceIconViewItem *
xfce_icon_view_get_item_at_coords (XfceIconView *icon_view,
                                   gint x,
                                   gint y,
                                   gboolean only_in_cell,
                                   XfceIconViewCellInfo **cell_at_pos);
static void
xfce_icon_view_get_cell_area (XfceIconView *icon_view,
                              XfceIconViewItem *item,
                              XfceIconViewCellInfo *cell_info,
                              GdkRectangle *cell_area);
static XfceIconViewCellInfo *
xfce_icon_view_get_cell_info (XfceIconView *icon_view,
                              GtkCellRenderer *renderer);
static void
xfce_icon_view_set_cell_data (XfceIconView *icon_view,
                              XfceIconViewItem *item);
static void
xfce_icon_view_cell_layout_pack_start (GtkCellLayout *layout,
                                       GtkCellRenderer *renderer,
                                       gboolean expand);
static void
xfce_icon_view_cell_layout_pack_end (GtkCellLayout *layout,
                                     GtkCellRenderer *renderer,
                                     gboolean expand);
static void
xfce_icon_view_cell_layout_add_attribute (GtkCellLayout *layout,
                                          GtkCellRenderer *renderer,
                                          const gchar *attribute,
                                          gint column);
static void
xfce_icon_view_cell_layout_clear (GtkCellLayout *layout);
static void
xfce_icon_view_cell_layout_clear_attributes (GtkCellLayout *layout,
                                             GtkCellRenderer *renderer);
static void
xfce_icon_view_cell_layout_set_cell_data_func (GtkCellLayout *layout,
                                               GtkCellRenderer *cell,
                                               GtkCellLayoutDataFunc func,
                                               gpointer func_data,
                                               GDestroyNotify destroy);
static void
xfce_icon_view_cell_layout_reorder (GtkCellLayout *layout,
                                    GtkCellRenderer *cell,
                                    gint position);
static void
xfce_icon_view_item_activate_cell (XfceIconView *icon_view,
                                   XfceIconViewItem *item,
                                   XfceIconViewCellInfo *cell_info,
                                   GdkEvent *event);
static void
xfce_icon_view_put (XfceIconView *icon_view,
                    GtkWidget *widget,
                    XfceIconViewItem *item,
                    gint cell);
static void
xfce_icon_view_remove_widget (GtkCellEditable *editable,
                              XfceIconView *icon_view);
static void
xfce_icon_view_start_editing (XfceIconView *icon_view,
                              XfceIconViewItem *item,
                              XfceIconViewCellInfo *cell_info,
                              GdkEvent *event);
static void
xfce_icon_view_stop_editing (XfceIconView *icon_view,
                             gboolean cancel_editing);
static void
xfce_icon_view_set_pixbuf_column (XfceIconView *icon_view,
                                  gint column);
static void
xfce_icon_view_set_icon_column (XfceIconView *icon_view,
                                gint column);

/* Source side drag signals */
static void
xfce_icon_view_drag_begin (GtkWidget *widget,
                           GdkDragContext *context);
static void
xfce_icon_view_drag_end (GtkWidget *widget,
                         GdkDragContext *context);
static void
xfce_icon_view_drag_data_get (GtkWidget *widget,
                              GdkDragContext *context,
                              GtkSelectionData *selection_data,
                              guint info,
                              guint drag_time);
static void
xfce_icon_view_drag_data_delete (GtkWidget *widget,
                                 GdkDragContext *context);

/* Target side drag signals */
static void
xfce_icon_view_drag_leave (GtkWidget *widget,
                           GdkDragContext *context,
                           guint drag_time);
static gboolean
xfce_icon_view_drag_motion (GtkWidget *widget,
                            GdkDragContext *context,
                            gint x,
                            gint y,
                            guint drag_time);
static gboolean
xfce_icon_view_drag_drop (GtkWidget *widget,
                          GdkDragContext *context,
                          gint x,
                          gint y,
                          guint drag_time);
static void
xfce_icon_view_drag_data_received (GtkWidget *widget,
                                   GdkDragContext *context,
                                   gint x,
                                   gint y,
                                   GtkSelectionData *selection_data,
                                   guint info,
                                   guint drag_time);
static gboolean
xfce_icon_view_maybe_begin_drag (XfceIconView *icon_view,
                                 GdkEventMotion *event);

static void
remove_scroll_timeout (XfceIconView *icon_view);

/* single-click autoselection support */
static gboolean
xfce_icon_view_single_click_timeout (gpointer user_data);
static void
xfce_icon_view_single_click_timeout_destroy (gpointer user_data);

/* Interactive search support */
static void
xfce_icon_view_search_activate (GtkEntry *entry,
                                XfceIconView *icon_view);
static void
xfce_icon_view_search_dialog_hide (GtkWidget *search_dialog,
                                   XfceIconView *icon_view);
static void
xfce_icon_view_search_ensure_directory (XfceIconView *icon_view);
static void
xfce_icon_view_search_init (GtkWidget *search_entry,
                            XfceIconView *icon_view);
static gboolean
xfce_icon_view_search_iter (XfceIconView *icon_view,
                            GtkTreeModel *model,
                            GtkTreeIter *iter,
                            const gchar *text,
                            gint *count,
                            gint n);
static void
xfce_icon_view_search_move (GtkWidget *widget,
                            XfceIconView *icon_view,
                            gboolean move_up);
static gboolean
xfce_icon_view_search_start (XfceIconView *icon_view,
                             gboolean keybinding);
static gboolean
xfce_icon_view_search_equal_func (GtkTreeModel *model,
                                  gint column,
                                  const gchar *key,
                                  GtkTreeIter *iter,
                                  gpointer user_data);
static gboolean
xfce_icon_view_search_button_press_event (GtkWidget *widget,
                                          GdkEventButton *event,
                                          XfceIconView *icon_view);
static gboolean
xfce_icon_view_search_delete_event (GtkWidget *widget,
                                    GdkEventAny *event,
                                    XfceIconView *icon_view);
static gboolean
xfce_icon_view_search_key_press_event (GtkWidget *widget,
                                       GdkEventKey *event,
                                       XfceIconView *icon_view);
static gboolean
xfce_icon_view_search_scroll_event (GtkWidget *widget,
                                    GdkEventScroll *event,
                                    XfceIconView *icon_view);
static gboolean
xfce_icon_view_search_timeout (gpointer user_data);
static void
xfce_icon_view_search_timeout_destroy (gpointer user_data);
static void
xfce_icon_view_release_items (XfceIconView *icon_view);


struct _XfceIconViewCellInfo
{
  GtkCellRenderer *cell;
  guint expand : 1;
  guint pack : 1;
  guint editing : 1;
  gint position;
  GSList *attributes;
  GtkCellLayoutDataFunc func;
  gpointer func_data;
  GDestroyNotify destroy;
  gboolean is_text;
};

struct _XfceIconViewChild
{
  XfceIconViewItem *item;
  GtkWidget *widget;
  gint cell;
};

struct _XfceIconViewItem
{
  GtkTreeIter iter;

  /* pointer to itself in the items-list of the icon-view */
  /* used in order to speed up index-lookup */
  GSequenceIter *item_iter;

  /* Bounding box (a value of -1 for width indicates
   * that the item needs to be layouted first)
   */
  GdkRectangle area;

  /* Individual cells.
   * box[i] is the actual area occupied by cell i,
   * before, after are used to calculate the cell
   * area relative to the box.
   * See xfce_icon_view_get_cell_area().
   */
  gint n_cells;
  GdkRectangle *box;
  gint *before;
  gint *after;

  guint row : ((sizeof (guint) / 2) * 8) - 1;
  guint col : ((sizeof (guint) / 2) * 8) - 1;
  guint selected : 1;
  guint selected_before_rubberbanding : 1;
};

typedef struct _XfceIconViewPrivate
{
  gint width, height;
  gint rows, cols;

  GtkSelectionMode selection_mode;

  XfceIconViewLayoutMode layout_mode;

  GdkWindow *bin_window;

  GList *children;

  GtkTreeModel *model;

  /* all items which are managed by this view */
  GSequence *items;

  GtkAdjustment *hadjustment;
  GtkAdjustment *vadjustment;

  GtkScrollablePolicy hscroll_policy;
  GtkScrollablePolicy vscroll_policy;

  guint layout_idle_id;

  gboolean doing_rubberband;
  gint rubberband_x_1, rubberband_y_1;
  gint rubberband_x2, rubberband_y2;

  guint scroll_timeout_id;
  gint scroll_value_diff;
  gint event_last_x, event_last_y;

  XfceIconViewItem *anchor_item;
  XfceIconViewItem *cursor_item;
  XfceIconViewItem *edited_item;
  GtkCellEditable *editable;
  XfceIconViewItem *prelit_item;

  XfceIconViewItem *last_single_clicked;

  GList *cell_list;
  gint n_cells;

  gint cursor_cell;

  GtkOrientation orientation;

  gint columns;
  gint item_width;
  gint spacing;
  gint row_spacing;
  gint column_spacing;
  gint margin;

  gint text_column;
  gint markup_column;
  gint pixbuf_column;
  gint icon_column;

  gint pixbuf_cell;
  gint text_cell;

  /* Drag-and-drop. */
  GdkModifierType start_button_mask;
  gint pressed_button;
  gint press_start_x;
  gint press_start_y;

  GtkTargetList *source_targets;
  GdkDragAction source_actions;

  GtkTargetList *dest_targets;
  GdkDragAction dest_actions;

  GtkTreeRowReference *dest_item;
  XfceIconViewDropPosition dest_pos;

  /* delayed scrolling */
  GtkTreeRowReference *scroll_to_path;
  gfloat scroll_to_row_align;
  gfloat scroll_to_col_align;
  guint scroll_to_use_align : 1;

  /* misc flags */
  guint source_set : 1;
  guint dest_set : 1;
  guint reorderable : 1;
  guint empty_view_drop : 1;

  guint ctrl_pressed : 1;
  guint shift_pressed : 1;

  /* Single-click support
   * The single_click_timeout is the timeout after which the
   * prelited item will be automatically selected in single
   * click mode (0 to disable).
   */
  guint single_click : 1;
  guint single_click_timeout;
  guint single_click_timeout_id;
  guint single_click_timeout_state;

  /* Interactive search support */
  guint enable_search : 1;
  gint search_column;
  gint search_selected_iter;
  guint search_timeout_id;
  gboolean search_disable_popdown;
  XfceIconViewSearchEqualFunc search_equal_func;
  gpointer search_equal_data;
  GDestroyNotify search_equal_destroy;
  XfceIconViewSearchPositionFunc search_position_func;
  gpointer search_position_data;
  GDestroyNotify search_position_destroy;
  gint search_entry_changed_id;
  GtkWidget *search_entry;
  GtkWidget *search_window;

  /* XfceIconViewFlags */
  guint flags;
} XfceIconViewPrivate;



static guint icon_view_signals[LAST_SIGNAL];



G_DEFINE_TYPE_WITH_CODE (XfceIconView, xfce_icon_view, GTK_TYPE_CONTAINER,
                         G_IMPLEMENT_INTERFACE (GTK_TYPE_CELL_LAYOUT, xfce_icon_view_cell_layout_init)
                         G_IMPLEMENT_INTERFACE (GTK_TYPE_SCROLLABLE, NULL)
                         G_ADD_PRIVATE (XfceIconView))



#include "xfce-icon-view-accessible.c"



static AtkObject *
xfce_icon_view_get_accessible (GtkWidget *widget)
{
  static gboolean initited = FALSE;
  GType derived_type;
  AtkObjectFactory *factory;
  AtkRegistry *registry;
  GType derived_atk_type;

  if (!initited)
    {
      derived_type = g_type_parent (XFCE_TYPE_ICON_VIEW);

      registry = atk_get_default_registry ();
      factory = atk_registry_get_factory (registry, derived_type);
      derived_atk_type = atk_object_factory_get_accessible_type (factory);

      if (g_type_is_a (derived_atk_type, GTK_TYPE_ACCESSIBLE))
        {
          atk_registry_set_factory_type (registry, XFCE_TYPE_ICON_VIEW,
                                         xfce_icon_view_accessible_factory_get_type ());
        }

      initited = TRUE;
    }

  return GTK_WIDGET_CLASS (xfce_icon_view_parent_class)->get_accessible (widget);
}



static void
xfce_icon_view_class_init (XfceIconViewClass *klass)
{
  GtkContainerClass *gtkcontainer_class;
  GtkWidgetClass *gtkwidget_class;
  GtkBindingSet *gtkbinding_set;
  GObjectClass *gobject_class;

  gobject_class = G_OBJECT_CLASS (klass);
  gobject_class->dispose = xfce_icon_view_dispose;
  gobject_class->finalize = xfce_icon_view_finalize;
  gobject_class->set_property = xfce_icon_view_set_property;
  gobject_class->get_property = xfce_icon_view_get_property;

  gtkwidget_class = GTK_WIDGET_CLASS (klass);
  gtkwidget_class->realize = xfce_icon_view_realize;
  gtkwidget_class->unrealize = xfce_icon_view_unrealize;
  gtkwidget_class->get_preferred_width = xfce_icon_view_get_preferred_width;
  gtkwidget_class->get_preferred_height = xfce_icon_view_get_preferred_height;
  gtkwidget_class->size_allocate = xfce_icon_view_size_allocate;
  gtkwidget_class->get_accessible = xfce_icon_view_get_accessible;
  gtkwidget_class->draw = xfce_icon_view_draw;
  gtkwidget_class->motion_notify_event = xfce_icon_view_motion_notify_event;
  gtkwidget_class->button_press_event = xfce_icon_view_button_press_event;
  gtkwidget_class->button_release_event = xfce_icon_view_button_release_event;
  gtkwidget_class->scroll_event = xfce_icon_view_scroll_event;
  gtkwidget_class->key_press_event = xfce_icon_view_key_press_event;
  gtkwidget_class->focus_out_event = xfce_icon_view_focus_out_event;
  gtkwidget_class->leave_notify_event = xfce_icon_view_leave_notify_event;
  gtkwidget_class->drag_begin = xfce_icon_view_drag_begin;
  gtkwidget_class->drag_end = xfce_icon_view_drag_end;
  gtkwidget_class->drag_data_get = xfce_icon_view_drag_data_get;
  gtkwidget_class->drag_data_delete = xfce_icon_view_drag_data_delete;
  gtkwidget_class->drag_leave = xfce_icon_view_drag_leave;
  gtkwidget_class->drag_motion = xfce_icon_view_drag_motion;
  gtkwidget_class->drag_drop = xfce_icon_view_drag_drop;
  gtkwidget_class->drag_data_received = xfce_icon_view_drag_data_received;

  gtkcontainer_class = GTK_CONTAINER_CLASS (klass);
  gtkcontainer_class->remove = xfce_icon_view_remove;
  gtkcontainer_class->forall = xfce_icon_view_forall;

  klass->set_scroll_adjustments = xfce_icon_view_set_adjustments;
  klass->select_all = xfce_icon_view_real_select_all;
  klass->unselect_all = xfce_icon_view_real_unselect_all;
  klass->select_cursor_item = xfce_icon_view_real_select_cursor_item;
  klass->toggle_cursor_item = xfce_icon_view_real_toggle_cursor_item;
  klass->move_cursor = xfce_icon_view_real_move_cursor;
  klass->activate_cursor_item = xfce_icon_view_real_activate_cursor_item;
  klass->start_interactive_search = xfce_icon_view_real_start_interactive_search;

  /**
   * XfceIconView:column-spacing:
   *
   * The column-spacing property specifies the space which is inserted between
   * the columns of the icon view.
   *
   * Since: 4.21.0
   **/
  g_object_class_install_property (gobject_class,
                                   PROP_COLUMN_SPACING,
                                   g_param_spec_int ("column-spacing",
                                                     "Column Spacing",
                                                     "Space which is inserted between grid column",
                                                     0, G_MAXINT, 6,
                                                     G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));

  /**
   * XfceIconView:columns:
   *
   * The columns property contains the number of the columns in which the
   * items should be displayed. If it is -1, the number of columns will
   * be chosen automatically to fill the available area.
   *
   * Since: 4.21.0
   **/
  g_object_class_install_property (gobject_class,
                                   PROP_COLUMNS,
                                   g_param_spec_int ("columns",
                                                     "Number of columns",
                                                     "Number of columns to display",
                                                     -1, G_MAXINT, -1,
                                                     G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));

  /**
   * XfceIconView:enable-search:
   *
   * View allows user to search through columns interactively.
   *
   * Since: 4.21.0
   **/
  g_object_class_install_property (gobject_class,
                                   PROP_ENABLE_SEARCH,
                                   g_param_spec_boolean ("enable-search",
                                                         "Enable Search",
                                                         "View allows user to search through columns interactively",
                                                         TRUE,
                                                         G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));


  /**
   * XfceIconView:item-width:
   *
   * The item-width property specifies the width to use for each item.
   * If it is set to -1, the icon view will automatically determine a
   * suitable item size.
   *
   * Since: 4.21.0
   **/
  g_object_class_install_property (gobject_class,
                                   PROP_ITEM_WIDTH,
                                   g_param_spec_int ("item-width",
                                                     "Width for each item",
                                                     "The width used for each item",
                                                     -1, G_MAXINT, -1,
                                                     G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));

  /**
   * XfceIconView:layout-mode:
   *
   * The layout-mode property specifies the way items are layed out in
   * the #XfceIconView. This can be either %XFCE_ICON_VIEW_LAYOUT_ROWS,
   * which is the default, where items are layed out horizontally in
   * rows from top to bottom, or %XFCE_ICON_VIEW_LAYOUT_COLS, where items
   * are layed out vertically in columns from left to right.
   *
   * Since: 4.21.0
   **/
  g_object_class_install_property (gobject_class,
                                   PROP_LAYOUT_MODE,
                                   g_param_spec_enum ("layout-mode",
                                                      "Layout mode",
                                                      "The layout mode",
                                                      XFCE_TYPE_ICON_VIEW_LAYOUT_MODE,
                                                      XFCE_ICON_VIEW_LAYOUT_ROWS,
                                                      G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));

  /**
   * XfceIconView:margin:
   *
   * The margin property specifies the space which is inserted
   * at the edges of the icon view.
   *
   * Since: 4.21.0
   **/
  g_object_class_install_property (gobject_class,
                                   PROP_MARGIN,
                                   g_param_spec_int ("margin",
                                                     "Margin",
                                                     "Space which is inserted at the edges of the icon view",
                                                     0, G_MAXINT, 6,
                                                     G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));

  /**
   * XfceIconView:markup-column:
   *
   * The markup-column property contains the number of the model column
   * containing markup information to be displayed. The markup column must be
   * of type #G_TYPE_STRING. If this property and the text-column property
   * are both set to column numbers, it overrides the text column.
   * If both are set to -1, no texts are displayed.
   *
   * Since: 4.21.0
   **/
  g_object_class_install_property (gobject_class,
                                   PROP_MARKUP_COLUMN,
                                   g_param_spec_int ("markup-column",
                                                     "Markup column",
                                                     "Model column used to retrieve the text if using Pango markup",
                                                     -1, G_MAXINT, -1,
                                                     G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));

  /**
   * XfceIconView:model:
   *
   * The model property contains the #GtkTreeModel, which should be
   * display by this icon view. Setting this property to %NULL turns
   * off the display of anything.
   *
   * Since: 4.21.0
   **/
  g_object_class_install_property (gobject_class,
                                   PROP_MODEL,
                                   g_param_spec_object ("model",
                                                        "Icon View Model",
                                                        "The model for the icon view",
                                                        GTK_TYPE_TREE_MODEL,
                                                        G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));

  /**
   * XfceIconView:orientation:
   *
   * The orientation property specifies how the cells (i.e. the icon and
   * the text) of the item are positioned relative to each other.
   *
   * Since: 4.21.0
   **/
  g_object_class_install_property (gobject_class,
                                   PROP_ORIENTATION,
                                   g_param_spec_enum ("orientation",
                                                      "Orientation",
                                                      "How the text and icon of each item are positioned relative to each other",
                                                      GTK_TYPE_ORIENTATION,
                                                      GTK_ORIENTATION_VERTICAL,
                                                      G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));

  /**
   * XfceIconView:pixbuf-column:
   *
   * The ::pixbuf-column property contains the number of the model column
   * containing the pixbufs which are displayed. The pixbuf column must be
   * of type #GDK_TYPE_PIXBUF. Setting this property to -1 turns off the
   * display of pixbufs.
   *
   * Since: 4.21.0
   **/
  g_object_class_install_property (gobject_class,
                                   PROP_PIXBUF_COLUMN,
                                   g_param_spec_int ("pixbuf-column",
                                                     "Pixbuf column",
                                                     "Model column used to retrieve the icon pixbuf from",
                                                     -1, G_MAXINT, -1,
                                                     G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));

  /**
   * XfceIconView:icon-column:
   *
   * The ::icon-column property contains the number of the model column
   * containing an absolute path to an image file to render. The icon column
   * must be of type #G_TYPE_STRING. Setting this property to -1 turns off
   * the display of icons.
   *
   * Since: 4.21.0
   **/
  g_object_class_install_property (gobject_class,
                                   PROP_ICON_COLUMN,
                                   g_param_spec_int ("icon-column",
                                                     "Icon column",
                                                     "Model column used to retrieve the absolute path of an image file to render",
                                                     -1, G_MAXINT, -1,
                                                     G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));

  /**
   * XfceIconView:reorderable:
   *
   * The reorderable property specifies if the items can be reordered
   * by Drag and Drop.
   *
   * Since: 4.21.0
   **/
  g_object_class_install_property (gobject_class,
                                   PROP_REORDERABLE,
                                   g_param_spec_boolean ("reorderable",
                                                         "Reorderable",
                                                         "View is reorderable",
                                                         FALSE,
                                                         G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));

  /**
   * XfceIconView:row-spacing:
   *
   * The row-spacing property specifies the space which is inserted between
   * the rows of the icon view.
   *
   * Since: 4.21.0
   **/
  g_object_class_install_property (gobject_class,
                                   PROP_ROW_SPACING,
                                   g_param_spec_int ("row-spacing",
                                                     "Row Spacing",
                                                     "Space which is inserted between grid rows",
                                                     0, G_MAXINT, 6,
                                                     G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));

  /**
   * XfceIconView:search-column:
   *
   * Model column to search through when searching through code.
   *
   * Since: 4.21.0
   **/
  g_object_class_install_property (gobject_class,
                                   PROP_SEARCH_COLUMN,
                                   g_param_spec_int ("search-column",
                                                     "Search Column",
                                                     "Model column to search through when searching through item",
                                                     -1, G_MAXINT, -1,
                                                     G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));

  /**
   * XfceIconView:selection-mode:
   *
   * The selection-mode property specifies the selection mode of
   * icon view. If the mode is #GTK_SELECTION_MULTIPLE, rubberband selection
   * is enabled, for the other modes, only keyboard selection is possible.
   *
   * Since: 4.21.0
   **/
  g_object_class_install_property (gobject_class,
                                   PROP_SELECTION_MODE,
                                   g_param_spec_enum ("selection-mode",
                                                      "Selection mode",
                                                      "The selection mode",
                                                      GTK_TYPE_SELECTION_MODE,
                                                      GTK_SELECTION_SINGLE,
                                                      G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));

  /**
   * XfceIconView:single-click:
   *
   * Determines whether items can be activated by single or double clicks.
   *
   * Since: 4.21.0
   **/
  g_object_class_install_property (gobject_class,
                                   PROP_SINGLE_CLICK,
                                   g_param_spec_boolean ("single-click",
                                                         "Single Click",
                                                         "Whether the items in the view can be activated with single clicks",
                                                         FALSE,
                                                         G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));

  /**
   * XfceIconView:single-click-timeout:
   *
   * The amount of time in milliseconds after which a prelited item (an item
   * which is hovered by the mouse cursor) will be selected automatically in
   * single click mode. A value of %0 disables the automatic selection.
   *
   * Since: 4.21.0
   **/
  g_object_class_install_property (gobject_class,
                                   PROP_SINGLE_CLICK_TIMEOUT,
                                   g_param_spec_uint ("single-click-timeout",
                                                      "Single Click Timeout",
                                                      "The amount of time after which the item under the mouse cursor will be selected automatically in single click mode",
                                                      0, G_MAXUINT, 0,
                                                      G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));

  /**
   * XfceIconView:spacing:
   *
   * The spacing property specifies the space which is inserted between
   * the cells (i.e. the icon and the text) of an item.
   *
   * Since: 4.21.0
   **/
  g_object_class_install_property (gobject_class,
                                   PROP_SPACING,
                                   g_param_spec_int ("spacing",
                                                     "Spacing",
                                                     "Space which is inserted between cells of an item",
                                                     0, G_MAXINT, 0,
                                                     G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));

  /**
   * XfceIconView:text-column:
   *
   * The text-column property contains the number of the model column
   * containing the texts which are displayed. The text column must be
   * of type #G_TYPE_STRING. If this property and the markup-column
   * property are both set to -1, no texts are displayed.
   *
   * Since: 4.21.0
   **/
  g_object_class_install_property (gobject_class,
                                   PROP_TEXT_COLUMN,
                                   g_param_spec_int ("text-column",
                                                     "Text column",
                                                     "Model column used to retrieve the text from",
                                                     -1, G_MAXINT, -1,
                                                     G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));

  g_object_class_override_property (gobject_class, PROP_HADJUSTMENT, "hadjustment");
  g_object_class_override_property (gobject_class, PROP_VADJUSTMENT, "vadjustment");
  g_object_class_override_property (gobject_class, PROP_HSCROLL_POLICY, "hscroll-policy");
  g_object_class_override_property (gobject_class, PROP_VSCROLL_POLICY, "vscroll-policy");

  /**
   * XfceIconView::item-activated:
   * @icon_view : a #XfceIconView.
   * @path      : the #GtkTreePath of the activated item.
   *
   * The ::item-activated signal is emitted when the method
   * xfce_icon_view_item_activated() is called, when the user double clicks
   * an item with the "activate-on-single-click" property set to %FALSE, or
   * when the user single clicks an item when the "activate-on-single-click"
   * property set to %TRUE. It is also emitted when a non-editable item is
   * selected and one of the keys: Space, Return or Enter is pressed.
   *
   * Since: 4.21.0
   **/
  icon_view_signals[ITEM_ACTIVATED] =
    g_signal_new (I_ ("item-activated"),
                  G_TYPE_FROM_CLASS (gobject_class),
                  G_SIGNAL_RUN_LAST,
                  G_STRUCT_OFFSET (XfceIconViewClass, item_activated),
                  NULL, NULL,
                  g_cclosure_marshal_VOID__BOXED,
                  G_TYPE_NONE, 1,
                  GTK_TYPE_TREE_PATH);

  /**
   * XfceIconView::selection-changed:
   * @icon_view : a #XfceIconView.
   *
   * The ::selection-changed signal is emitted when the selection
   * (i.e. the set of selected items) changes.
   *
   * Since: 4.21.0
   **/
  icon_view_signals[SELECTION_CHANGED] =
    g_signal_new (I_ ("selection-changed"),
                  G_TYPE_FROM_CLASS (gobject_class),
                  G_SIGNAL_RUN_FIRST,
                  G_STRUCT_OFFSET (XfceIconViewClass, selection_changed),
                  NULL, NULL,
                  g_cclosure_marshal_VOID__VOID,
                  G_TYPE_NONE, 0);

  /**
   * XfceIconView::select-all:
   * @icon_view : a #XfceIconView.
   *
   * A #GtkBindingSignal which gets emitted when the user selects all items.
   *
   * Applications should not connect to it, but may emit it with
   * g_signal_emit_by_name() if they need to control selection
   * programmatically.
   *
   * The default binding for this signal is Ctrl-a.
   *
   * Since: 4.21.0
   **/
  icon_view_signals[SELECT_ALL] =
    g_signal_new (I_ ("select-all"),
                  G_TYPE_FROM_CLASS (gobject_class),
                  G_SIGNAL_RUN_LAST | G_SIGNAL_ACTION,
                  G_STRUCT_OFFSET (XfceIconViewClass, select_all),
                  NULL, NULL,
                  g_cclosure_marshal_VOID__VOID,
                  G_TYPE_NONE, 0);

  /**
   * XfceIconView::unselect-all:
   * @icon_view : a #XfceIconView.
   *
   * A #GtkBindingSignal which gets emitted when the user unselects all items.
   *
   * Applications should not connect to it, but may emit it with
   * g_signal_emit_by_name() if they need to control selection
   * programmatically.
   *
   * The default binding for this signal is Ctrl-Shift-a.
   *
   * Since: 4.21.0
   **/
  icon_view_signals[UNSELECT_ALL] =
    g_signal_new (I_ ("unselect-all"),
                  G_TYPE_FROM_CLASS (gobject_class),
                  G_SIGNAL_RUN_LAST | G_SIGNAL_ACTION,
                  G_STRUCT_OFFSET (XfceIconViewClass, unselect_all),
                  NULL, NULL,
                  g_cclosure_marshal_VOID__VOID,
                  G_TYPE_NONE, 0);

  /**
   * XfceIconView::select-cursor-item:
   * @icon_view : a #XfceIconView.
   *
   * A #GtkBindingSignal which gets emitted when the user selects the item
   * that is currently focused.
   *
   * Applications should not connect to it, but may emit it with
   * g_signal_emit_by_name() if they need to control selection
   * programmatically.
   *
   * There is no default binding for this signal.
   *
   * Since: 4.21.0
   **/
  icon_view_signals[SELECT_CURSOR_ITEM] =
    g_signal_new (I_ ("select-cursor-item"),
                  G_TYPE_FROM_CLASS (gobject_class),
                  G_SIGNAL_RUN_LAST | G_SIGNAL_ACTION,
                  G_STRUCT_OFFSET (XfceIconViewClass, select_cursor_item),
                  NULL, NULL,
                  g_cclosure_marshal_VOID__VOID,
                  G_TYPE_NONE, 0);

  /**
   * XfceIconView::toggle-cursor-item:
   * @icon_view : a #XfceIconView.
   *
   * A #GtkBindingSignal which gets emitted when the user toggles whether
   * the currently focused item is selected or not. The exact effect of
   * this depend on the selection mode.
   *
   * Applications should not connect to it, but may emit it with
   * g_signal_emit_by_name() if they need to control selection
   * programmatically.
   *
   * There is no default binding for this signal is Ctrl-Space.
   *
   * Since: 4.21.0
   **/
  icon_view_signals[TOGGLE_CURSOR_ITEM] =
    g_signal_new (I_ ("toggle-cursor-item"),
                  G_TYPE_FROM_CLASS (gobject_class),
                  G_SIGNAL_RUN_LAST | G_SIGNAL_ACTION,
                  G_STRUCT_OFFSET (XfceIconViewClass, toggle_cursor_item),
                  NULL, NULL,
                  g_cclosure_marshal_VOID__VOID,
                  G_TYPE_NONE, 0);

  /**
   * XfceIconView::activate-cursor-item:
   * @icon_view : a #XfceIconView.
   *
   * A #GtkBindingSignal which gets emitted when the user activates the
   * currently focused item.
   *
   * Applications should not connect to it, but may emit it with
   * g_signal_emit_by_name() if they need to control activation
   * programmatically.
   *
   * The default bindings for this signal are Space, Return and Enter.
   *
   * Since: 4.21.0
   **/
  icon_view_signals[ACTIVATE_CURSOR_ITEM] =
    g_signal_new (I_ ("activate-cursor-item"),
                  G_TYPE_FROM_CLASS (gobject_class),
                  G_SIGNAL_RUN_LAST | G_SIGNAL_ACTION,
                  G_STRUCT_OFFSET (XfceIconViewClass, activate_cursor_item),
                  NULL, NULL,
                  _libxfce4ui_marshal_BOOLEAN__VOID,
                  G_TYPE_BOOLEAN, 0);

  /**
   * XfceIconView::start-interactive-search:
   * @icon_view : a #XfceIconView.
   *
   * The ::start-interative-search signal is emitted when the user starts
   * typing to jump to an item in the icon view.
   *
   * Since: 4.21.0
   **/
  icon_view_signals[START_INTERACTIVE_SEARCH] =
    g_signal_new (I_ ("start-interactive-search"),
                  G_TYPE_FROM_CLASS (gobject_class),
                  G_SIGNAL_RUN_LAST | G_SIGNAL_ACTION,
                  G_STRUCT_OFFSET (XfceIconViewClass, start_interactive_search),
                  NULL, NULL,
                  _libxfce4ui_marshal_BOOLEAN__VOID,
                  G_TYPE_BOOLEAN, 0);

  /**
   * XfceIconView::move-cursor:
   * @icon_view : a #XfceIconView.
   * @step      : the granularity of the move, as a #GtkMovementStep
   * @count     : the number of @step units to move
   *
   * The ::move-cursor signal is a keybinding signal which gets emitted when
   * the user initiates a cursor movement.
   *
   * Applications should not connect to it, but may emit it with
   * g_signal_emit_by_name() if they need to control the cursor
   * programmatically.
   *
   * The default bindings for this signal include
   * * Arrow keys which move by individual steps
   * * Home/End keys which move to the first/last item
   * * PageUp/PageDown which move by "pages" All of these will extend the
   * selection when combined with the Shift modifier.
   *
   * Since: 4.21.0
   **/
  icon_view_signals[MOVE_CURSOR] =
    g_signal_new (I_ ("move-cursor"),
                  G_TYPE_FROM_CLASS (gobject_class),
                  G_SIGNAL_RUN_LAST | G_SIGNAL_ACTION,
                  G_STRUCT_OFFSET (XfceIconViewClass, move_cursor),
                  NULL, NULL,
                  _libxfce4ui_marshal_BOOLEAN__ENUM_INT,
                  G_TYPE_BOOLEAN, 2,
                  GTK_TYPE_MOVEMENT_STEP,
                  G_TYPE_INT);

  /* Key bindings */
  gtkbinding_set = gtk_binding_set_by_class (klass);
  gtk_binding_entry_add_signal (gtkbinding_set, GDK_KEY_a, GDK_CONTROL_MASK, "select-all", 0);
  gtk_binding_entry_add_signal (gtkbinding_set, GDK_KEY_a, GDK_CONTROL_MASK | GDK_SHIFT_MASK, "unselect-all", 0);
  gtk_binding_entry_add_signal (gtkbinding_set, GDK_KEY_space, GDK_CONTROL_MASK, "toggle-cursor-item", 0);
  gtk_binding_entry_add_signal (gtkbinding_set, GDK_KEY_space, 0, "activate-cursor-item", 0);
  gtk_binding_entry_add_signal (gtkbinding_set, GDK_KEY_Return, 0, "activate-cursor-item", 0);
  gtk_binding_entry_add_signal (gtkbinding_set, GDK_KEY_ISO_Enter, 0, "activate-cursor-item", 0);
  gtk_binding_entry_add_signal (gtkbinding_set, GDK_KEY_KP_Enter, 0, "activate-cursor-item", 0);
  gtk_binding_entry_add_signal (gtkbinding_set, GDK_KEY_f, GDK_CONTROL_MASK, "start-interactive-search", 0);
  gtk_binding_entry_add_signal (gtkbinding_set, GDK_KEY_F, GDK_CONTROL_MASK, "start-interactive-search", 0);

  xfce_icon_view_add_move_binding (gtkbinding_set, GDK_KEY_Up, 0, GTK_MOVEMENT_DISPLAY_LINES, -1);
  xfce_icon_view_add_move_binding (gtkbinding_set, GDK_KEY_KP_Up, 0, GTK_MOVEMENT_DISPLAY_LINES, -1);
  xfce_icon_view_add_move_binding (gtkbinding_set, GDK_KEY_Down, 0, GTK_MOVEMENT_DISPLAY_LINES, 1);
  xfce_icon_view_add_move_binding (gtkbinding_set, GDK_KEY_KP_Down, 0, GTK_MOVEMENT_DISPLAY_LINES, 1);
  xfce_icon_view_add_move_binding (gtkbinding_set, GDK_KEY_p, GDK_CONTROL_MASK, GTK_MOVEMENT_DISPLAY_LINES, -1);
  xfce_icon_view_add_move_binding (gtkbinding_set, GDK_KEY_n, GDK_CONTROL_MASK, GTK_MOVEMENT_DISPLAY_LINES, 1);
  xfce_icon_view_add_move_binding (gtkbinding_set, GDK_KEY_Home, 0, GTK_MOVEMENT_BUFFER_ENDS, -1);
  xfce_icon_view_add_move_binding (gtkbinding_set, GDK_KEY_KP_Home, 0, GTK_MOVEMENT_BUFFER_ENDS, -1);
  xfce_icon_view_add_move_binding (gtkbinding_set, GDK_KEY_End, 0, GTK_MOVEMENT_BUFFER_ENDS, 1);
  xfce_icon_view_add_move_binding (gtkbinding_set, GDK_KEY_KP_End, 0, GTK_MOVEMENT_BUFFER_ENDS, 1);
  xfce_icon_view_add_move_binding (gtkbinding_set, GDK_KEY_Page_Up, 0, GTK_MOVEMENT_PAGES, -1);
  xfce_icon_view_add_move_binding (gtkbinding_set, GDK_KEY_KP_Page_Up, 0, GTK_MOVEMENT_PAGES, -1);
  xfce_icon_view_add_move_binding (gtkbinding_set, GDK_KEY_Page_Down, 0, GTK_MOVEMENT_PAGES, 1);
  xfce_icon_view_add_move_binding (gtkbinding_set, GDK_KEY_KP_Page_Down, 0, GTK_MOVEMENT_PAGES, 1);
  xfce_icon_view_add_move_binding (gtkbinding_set, GDK_KEY_Right, 0, GTK_MOVEMENT_VISUAL_POSITIONS, 1);
  xfce_icon_view_add_move_binding (gtkbinding_set, GDK_KEY_Left, 0, GTK_MOVEMENT_VISUAL_POSITIONS, -1);
  xfce_icon_view_add_move_binding (gtkbinding_set, GDK_KEY_KP_Right, 0, GTK_MOVEMENT_VISUAL_POSITIONS, 1);
  xfce_icon_view_add_move_binding (gtkbinding_set, GDK_KEY_KP_Left, 0, GTK_MOVEMENT_VISUAL_POSITIONS, -1);
}



static void
xfce_icon_view_cell_layout_init (GtkCellLayoutIface *iface)
{
  iface->pack_start = xfce_icon_view_cell_layout_pack_start;
  iface->pack_end = xfce_icon_view_cell_layout_pack_end;
  iface->clear = xfce_icon_view_cell_layout_clear;
  iface->add_attribute = xfce_icon_view_cell_layout_add_attribute;
  iface->set_cell_data_func = xfce_icon_view_cell_layout_set_cell_data_func;
  iface->clear_attributes = xfce_icon_view_cell_layout_clear_attributes;
  iface->reorder = xfce_icon_view_cell_layout_reorder;
}



static void
xfce_icon_view_init (XfceIconView *icon_view)
{
  XfceIconViewPrivate *priv = get_instance_private (icon_view);

  gtk_style_context_add_class (gtk_widget_get_style_context (GTK_WIDGET (icon_view)),
                               GTK_STYLE_CLASS_VIEW);

  priv->items = g_sequence_new (NULL);
  priv->selection_mode = GTK_SELECTION_SINGLE;
  priv->pressed_button = -1;
  priv->press_start_x = -1;
  priv->press_start_y = -1;
  priv->text_column = -1;
  priv->markup_column = -1;
  priv->pixbuf_column = -1;
  priv->icon_column = -1;
  priv->text_cell = -1;
  priv->pixbuf_cell = -1;

  gtk_widget_set_can_focus (GTK_WIDGET (icon_view), TRUE);

  xfce_icon_view_set_adjustments (icon_view, NULL, NULL);

  priv->cursor_cell = -1;

  priv->orientation = GTK_ORIENTATION_VERTICAL;

  priv->columns = -1;
  priv->item_width = -1;
  priv->row_spacing = 6;
  priv->column_spacing = 6;
  priv->margin = 6;

  priv->enable_search = TRUE;
  priv->search_column = -1;
  priv->search_equal_func = xfce_icon_view_search_equal_func;
  priv->search_position_func = (XfceIconViewSearchPositionFunc) xfce_gtk_position_search_box;

  priv->flags = XFCE_ICON_VIEW_DRAW_KEYFOCUS;
}



static void
xfce_icon_view_dispose (GObject *object)
{
  XfceIconView *icon_view = XFCE_ICON_VIEW (object);
  XfceIconViewPrivate *priv = get_instance_private (icon_view);

  /* cancel any pending search timeout */
  if (G_UNLIKELY (priv->search_timeout_id != 0))
    g_source_remove (priv->search_timeout_id);

  /* destroy the interactive search dialog */
  if (G_UNLIKELY (priv->search_window != NULL))
    {
      gtk_widget_destroy (priv->search_window);
      priv->search_entry = NULL;
      priv->search_window = NULL;
    }

  /* drop search equal and position functions (if any) */
  xfce_icon_view_set_search_equal_func (icon_view, NULL, NULL, NULL);
  xfce_icon_view_set_search_position_func (icon_view, NULL, NULL, NULL);

  /* reset the drag dest item */
  xfce_icon_view_set_drag_dest_item (icon_view, NULL, XFCE_ICON_VIEW_NO_DROP);

  /* drop the scroll to path (if any) */
  if (G_UNLIKELY (priv->scroll_to_path != NULL))
    {
      gtk_tree_row_reference_free (priv->scroll_to_path);
      priv->scroll_to_path = NULL;
    }

  /* reset the model (also stops any active editing) */
  xfce_icon_view_set_model (icon_view, NULL);

  /* drop the scroll timer */
  remove_scroll_timeout (icon_view);

  G_OBJECT_CLASS (xfce_icon_view_parent_class)->dispose (object);
}



static void
xfce_icon_view_finalize (GObject *object)
{
  XfceIconView *icon_view = XFCE_ICON_VIEW (object);
  XfceIconViewPrivate *priv = get_instance_private (icon_view);

  /* drop the scroll adjustments */
  g_object_unref (G_OBJECT (priv->hadjustment));
  g_object_unref (G_OBJECT (priv->vadjustment));

  /* drop the cell renderers */
  xfce_icon_view_cell_layout_clear (GTK_CELL_LAYOUT (icon_view));

  /* be sure to cancel the single click timeout */
  if (G_UNLIKELY (priv->single_click_timeout_id != 0))
    g_source_remove (priv->single_click_timeout_id);

  /* drop all items belonging to the model and release the sequence */
  xfce_icon_view_release_items (icon_view);
  g_sequence_free (priv->items);

  /* kill the layout idle source (it's important to have this last!) */
  if (G_UNLIKELY (priv->layout_idle_id != 0))
    g_source_remove (priv->layout_idle_id);

  G_OBJECT_CLASS (xfce_icon_view_parent_class)->finalize (object);
}


static void
xfce_icon_view_get_property (GObject *object,
                             guint prop_id,
                             GValue *value,
                             GParamSpec *pspec)
{
  XfceIconViewPrivate *priv = get_instance_private (object);

  switch (prop_id)
    {
    case PROP_COLUMN_SPACING:
      g_value_set_int (value, priv->column_spacing);
      break;

    case PROP_COLUMNS:
      g_value_set_int (value, priv->columns);
      break;

    case PROP_ENABLE_SEARCH:
      g_value_set_boolean (value, priv->enable_search);
      break;

    case PROP_ITEM_WIDTH:
      g_value_set_int (value, priv->item_width);
      break;

    case PROP_MARGIN:
      g_value_set_int (value, priv->margin);
      break;

    case PROP_MARKUP_COLUMN:
      g_value_set_int (value, priv->markup_column);
      break;

    case PROP_MODEL:
      g_value_set_object (value, priv->model);
      break;

    case PROP_ORIENTATION:
      g_value_set_enum (value, priv->orientation);
      break;

    case PROP_PIXBUF_COLUMN:
      g_value_set_int (value, priv->pixbuf_column);
      break;

    case PROP_ICON_COLUMN:
      g_value_set_int (value, priv->icon_column);
      break;

    case PROP_REORDERABLE:
      g_value_set_boolean (value, priv->reorderable);
      break;

    case PROP_ROW_SPACING:
      g_value_set_int (value, priv->row_spacing);
      break;

    case PROP_SEARCH_COLUMN:
      g_value_set_int (value, priv->search_column);
      break;

    case PROP_SELECTION_MODE:
      g_value_set_enum (value, priv->selection_mode);
      break;

    case PROP_SINGLE_CLICK:
      g_value_set_boolean (value, priv->single_click);
      break;

    case PROP_SINGLE_CLICK_TIMEOUT:
      g_value_set_uint (value, priv->single_click_timeout);
      break;

    case PROP_SPACING:
      g_value_set_int (value, priv->spacing);
      break;

    case PROP_TEXT_COLUMN:
      g_value_set_int (value, priv->text_column);
      break;

    case PROP_LAYOUT_MODE:
      g_value_set_enum (value, priv->layout_mode);
      break;

    case PROP_HADJUSTMENT:
      g_value_set_object (value, priv->hadjustment);
      break;

    case PROP_VADJUSTMENT:
      g_value_set_object (value, priv->vadjustment);
      break;

    case PROP_HSCROLL_POLICY:
      g_value_set_enum (value, priv->hscroll_policy);
      break;

    case PROP_VSCROLL_POLICY:
      g_value_set_enum (value, priv->vscroll_policy);
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
    }
}



static void
xfce_icon_view_set_property (GObject *object,
                             guint prop_id,
                             const GValue *value,
                             GParamSpec *pspec)
{
  XfceIconView *icon_view = XFCE_ICON_VIEW (object);
  XfceIconViewPrivate *priv = get_instance_private (icon_view);

  switch (prop_id)
    {
    case PROP_COLUMN_SPACING:
      xfce_icon_view_set_column_spacing (icon_view, g_value_get_int (value));
      break;

    case PROP_COLUMNS:
      xfce_icon_view_set_columns (icon_view, g_value_get_int (value));
      break;

    case PROP_ENABLE_SEARCH:
      xfce_icon_view_set_enable_search (icon_view, g_value_get_boolean (value));
      break;

    case PROP_ITEM_WIDTH:
      xfce_icon_view_set_item_width (icon_view, g_value_get_int (value));
      break;

    case PROP_MARGIN:
      xfce_icon_view_set_margin (icon_view, g_value_get_int (value));
      break;

    case PROP_MODEL:
      xfce_icon_view_set_model (icon_view, g_value_get_object (value));
      break;

    case PROP_ORIENTATION:
      xfce_icon_view_set_orientation (icon_view, g_value_get_enum (value));
      break;

    case PROP_PIXBUF_COLUMN:
      xfce_icon_view_set_pixbuf_column (icon_view, g_value_get_int (value));
      break;

    case PROP_ICON_COLUMN:
      xfce_icon_view_set_icon_column (icon_view, g_value_get_int (value));
      break;

    case PROP_REORDERABLE:
      xfce_icon_view_set_reorderable (icon_view, g_value_get_boolean (value));
      break;

    case PROP_ROW_SPACING:
      xfce_icon_view_set_row_spacing (icon_view, g_value_get_int (value));
      break;

    case PROP_SEARCH_COLUMN:
      xfce_icon_view_set_search_column (icon_view, g_value_get_int (value));
      break;

    case PROP_SELECTION_MODE:
      xfce_icon_view_set_selection_mode (icon_view, g_value_get_enum (value));
      break;

    case PROP_SINGLE_CLICK:
      xfce_icon_view_set_single_click (icon_view, g_value_get_boolean (value));
      break;

    case PROP_SINGLE_CLICK_TIMEOUT:
      xfce_icon_view_set_single_click_timeout (icon_view, g_value_get_uint (value));
      break;

    case PROP_SPACING:
      xfce_icon_view_set_spacing (icon_view, g_value_get_int (value));
      break;

    case PROP_LAYOUT_MODE:
      xfce_icon_view_set_layout_mode (icon_view, g_value_get_enum (value));
      break;

    case PROP_HADJUSTMENT:
      xfce_icon_view_set_adjustments (icon_view, g_value_get_object (value), priv->vadjustment);
      break;

    case PROP_VADJUSTMENT:
      xfce_icon_view_set_adjustments (icon_view, priv->hadjustment, g_value_get_object (value));
      break;

    case PROP_HSCROLL_POLICY:
      if (priv->hscroll_policy != (GtkScrollablePolicy) g_value_get_enum (value))
        {
          priv->hscroll_policy = g_value_get_enum (value);
          gtk_widget_queue_resize (GTK_WIDGET (icon_view));
          g_object_notify_by_pspec (object, pspec);
        }
      break;

    case PROP_VSCROLL_POLICY:
      if (priv->vscroll_policy != (GtkScrollablePolicy) g_value_get_enum (value))
        {
          priv->vscroll_policy = g_value_get_enum (value);
          gtk_widget_queue_resize (GTK_WIDGET (icon_view));
          g_object_notify_by_pspec (object, pspec);
        }
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
    }
}



static void
xfce_icon_view_realize (GtkWidget *widget)
{
  XfceIconViewPrivate *priv = get_instance_private (widget);
  GdkWindowAttr attributes;
  gint attributes_mask;
  GtkAllocation allocation;

  gtk_widget_set_realized (widget, TRUE);
  gtk_widget_get_allocation (widget, &allocation);

  /* Allocate the clipping window */
  attributes.window_type = GDK_WINDOW_CHILD;
  attributes.x = allocation.x;
  attributes.y = allocation.y;
  attributes.width = allocation.width;
  attributes.height = allocation.height;
  attributes.wclass = GDK_INPUT_OUTPUT;
  attributes.visual = gtk_widget_get_visual (widget);
#define GDK_WA_COLORMAP 0
  attributes.event_mask = GDK_VISIBILITY_NOTIFY_MASK;
  attributes_mask = GDK_WA_X | GDK_WA_Y | GDK_WA_VISUAL | GDK_WA_COLORMAP;
  gtk_widget_set_window (widget, gdk_window_new (gtk_widget_get_parent_window (widget), &attributes, attributes_mask));
  gdk_window_set_user_data (gtk_widget_get_window (widget), widget);

  /* Allocate the icons window */
  attributes.x = 0;
  attributes.y = 0;
  attributes.width = MAX (priv->width, allocation.width);
  attributes.height = MAX (priv->height, allocation.height);
  attributes.event_mask = GDK_EXPOSURE_MASK
                          | GDK_SCROLL_MASK
                          | GDK_SMOOTH_SCROLL_MASK
                          | GDK_POINTER_MOTION_MASK
                          | GDK_LEAVE_NOTIFY_MASK
                          | GDK_BUTTON_PRESS_MASK
                          | GDK_BUTTON_RELEASE_MASK
                          | GDK_KEY_PRESS_MASK
                          | GDK_KEY_RELEASE_MASK
                          | gtk_widget_get_events (widget);
  priv->bin_window = gdk_window_new (gtk_widget_get_window (widget), &attributes, attributes_mask);
  gdk_window_set_user_data (priv->bin_window, widget);

  /* map the icons window */
  gdk_window_show (priv->bin_window);
}

static void
xfce_icon_view_unrealize (GtkWidget *widget)
{
  XfceIconViewPrivate *priv = get_instance_private (widget);

  /* drop the icons window */
  gdk_window_set_user_data (priv->bin_window, NULL);
  gdk_window_destroy (priv->bin_window);
  priv->bin_window = NULL;

  /* let GtkWidget destroy children and widget->window */
  if (GTK_WIDGET_CLASS (xfce_icon_view_parent_class)->unrealize)
    (*GTK_WIDGET_CLASS (xfce_icon_view_parent_class)->unrealize) (widget);
}

static void
xfce_icon_view_get_preferred_width (GtkWidget *widget,
                                    gint *minimal_width,
                                    gint *natural_width)
{
  XfceIconViewPrivate *priv = get_instance_private (widget);
  XfceIconViewChild *child;
  gint child_minimal, child_natural;
  GList *lp;

  /* well, this is easy */
  if (priv->item_width < 0)
    *minimal_width = priv->width;
  *natural_width = priv->width;

  /* handle the child widgets */
  for (lp = priv->children; lp != NULL; lp = lp->next)
    {
      child = lp->data;
      if (gtk_widget_get_visible (child->widget))
        gtk_widget_get_preferred_width (child->widget, &child_minimal, &child_natural);
    }
}

static void
xfce_icon_view_get_preferred_height (GtkWidget *widget,
                                     gint *minimal_height,
                                     gint *natural_height)
{
  XfceIconViewPrivate *priv = get_instance_private (widget);
  XfceIconViewChild *child;
  gint child_minimal, child_natural;
  GList *lp;

  /* well, this is easy */
  *minimal_height = priv->height;
  *natural_height = priv->height;

  /* handle the child widgets */
  for (lp = priv->children; lp != NULL; lp = lp->next)
    {
      child = lp->data;
      if (gtk_widget_get_visible (child->widget))
        gtk_widget_get_preferred_height (child->widget, &child_minimal, &child_natural);
    }
}

static void
xfce_icon_view_allocate_children (XfceIconView *icon_view)
{
  XfceIconViewPrivate *priv = get_instance_private (icon_view);
  const XfceIconViewChild *child;
  GtkAllocation allocation;
  const GList *lp;
  gint focus_line_width;
  gint focus_padding;

  for (lp = priv->children; lp != NULL; lp = lp->next)
    {
      child = XFCE_ICON_VIEW_CHILD (lp->data);

      /* totally ignore our child's requisition */
      if (child->cell < 0)
        allocation = child->item->area;
      else
        allocation = child->item->box[child->cell];

      /* increase the item area by focus width/padding */
      gtk_widget_style_get (GTK_WIDGET (icon_view), "focus-line-width", &focus_line_width, "focus-padding", &focus_padding, NULL);
      allocation.x = MAX (0, allocation.x - (focus_line_width + focus_padding));
      allocation.y = MAX (0, allocation.y - (focus_line_width + focus_padding));
      allocation.width = MIN (priv->width - allocation.x, allocation.width + 2 * (focus_line_width + focus_padding));
      allocation.height = MIN (priv->height - allocation.y, allocation.height + 2 * (focus_line_width + focus_padding));

      /* allocate the area to the child */
      gtk_widget_size_allocate (child->widget, &allocation);
    }
}



static void
xfce_icon_view_size_allocate (GtkWidget *widget,
                              GtkAllocation *allocation)
{
  GtkAdjustment *hadjustment;
  GtkAdjustment *vadjustment;
  XfceIconView *icon_view = XFCE_ICON_VIEW (widget);
  XfceIconViewPrivate *priv = get_instance_private (icon_view);

  /* apply the new size allocation */
  gtk_widget_set_allocation (widget, allocation);

  /* move/resize the clipping window, the icons window
   * will be handled by xfce_icon_view_layout().
   */
  if (gtk_widget_get_realized (widget))
    gdk_window_move_resize (gtk_widget_get_window (widget),
                            allocation->x, allocation->y,
                            allocation->width, allocation->height);

  /* layout the items */
  xfce_icon_view_layout (icon_view);

  /* allocate space to the widgets (editing) */
  xfce_icon_view_allocate_children (icon_view);

  /* update the horizontal scroll adjustment accordingly */
  hadjustment = priv->hadjustment;
  gtk_adjustment_set_page_size (hadjustment, allocation->width);
  gtk_adjustment_set_page_increment (hadjustment, allocation->width * 0.9);
  gtk_adjustment_set_step_increment (hadjustment, allocation->width * 0.1);
  gtk_adjustment_set_lower (hadjustment, 0);
  gtk_adjustment_set_upper (hadjustment, MAX (allocation->width, priv->width));
  if (gtk_adjustment_get_value (hadjustment) > gtk_adjustment_get_upper (hadjustment) - gtk_adjustment_get_lower (hadjustment))
    gtk_adjustment_set_value (hadjustment, MAX (0, gtk_adjustment_get_upper (hadjustment)
                                                     - gtk_adjustment_get_page_size (hadjustment)));

  /* update the vertical scroll adjustment accordingly */
  vadjustment = priv->vadjustment;
  gtk_adjustment_set_page_size (vadjustment, allocation->height);
  gtk_adjustment_set_page_increment (vadjustment, allocation->height * 0.9);
  gtk_adjustment_set_step_increment (vadjustment, allocation->height * 0.1);
  gtk_adjustment_set_lower (vadjustment, 0);
  gtk_adjustment_set_upper (vadjustment, MAX (allocation->height, priv->height));
  if (gtk_adjustment_get_value (vadjustment) > gtk_adjustment_get_upper (vadjustment) - gtk_adjustment_get_page_size (vadjustment))
    gtk_adjustment_set_value (vadjustment, MAX (0, gtk_adjustment_get_upper (vadjustment)
                                                     - gtk_adjustment_get_page_size (vadjustment)));
}



static gboolean
xfce_icon_view_draw (GtkWidget *widget,
                     cairo_t *cr)
{
  XfceIconView *icon_view = XFCE_ICON_VIEW (widget);
  XfceIconViewPrivate *priv = get_instance_private (icon_view);
  XfceIconViewDropPosition dest_pos;
  XfceIconViewItem *dest_item = NULL;
  XfceIconViewItem *item;
  GtkTreePath *path;
  GdkRectangle rect;
  GdkRectangle clip;
  GdkRectangle paint_area;
  GSequenceIter *iter;
  gint dest_index = -1;
  GtkStyleContext *context;

  /* verify that the expose happened on the icon window */
  if (!gtk_cairo_should_draw_window (cr, priv->bin_window))
    return FALSE;

  /* don't handle expose if the layout isn't done yet; the layout
   * method will schedule a redraw when done.
   */
  if (G_UNLIKELY (priv->layout_idle_id != 0))
    return FALSE;

  /* "returns [...] FALSE if all of cr is clipped and all drawing can be skipped" [sic] */
  if (!gdk_cairo_get_clip_rectangle (cr, &clip))
    return FALSE;

  context = gtk_widget_get_style_context (widget);

  /* draw a background according to the css theme */
  gtk_render_background (context, cr,
                         0, 0,
                         gtk_widget_get_allocated_width (widget),
                         gtk_widget_get_allocated_height (widget));

  /* transform coordinates so our old calculations work */
  gtk_cairo_transform_to_window (cr, widget, priv->bin_window);

  /* retrieve the clipping rectangle again, with the transformed coordinates */
  gdk_cairo_get_clip_rectangle (cr, &clip);

  /* scroll to the previously remembered path (if any) */
  if (G_UNLIKELY (priv->scroll_to_path != NULL))
    {
      /* grab the path from the reference and invalidate the reference */
      path = gtk_tree_row_reference_get_path (priv->scroll_to_path);
      gtk_tree_row_reference_free (priv->scroll_to_path);
      priv->scroll_to_path = NULL;

      /* check if the reference was still valid */
      if (G_LIKELY (path != NULL))
        {
          /* try to scroll again */
          xfce_icon_view_scroll_to_path (icon_view, path,
                                         priv->scroll_to_use_align,
                                         priv->scroll_to_row_align,
                                         priv->scroll_to_col_align);

          /* release the path */
          gtk_tree_path_free (path);
        }
    }

  /* check if we need to draw a drag indicator */
  xfce_icon_view_get_drag_dest_item (icon_view, &path, &dest_pos);
  if (G_UNLIKELY (path != NULL))
    {
      dest_index = gtk_tree_path_get_indices (path)[0];
      gtk_tree_path_free (path);
    }

  /* paint all items that are affected by the expose event */
  for (iter = g_sequence_get_begin_iter (priv->items);
       !g_sequence_iter_is_end (iter);
       iter = g_sequence_iter_next (iter))
    {
      item = XFCE_ICON_VIEW_ITEM (g_sequence_get (iter));

      /* FIXME: padding? */
      paint_area.x = item->area.x;
      paint_area.y = item->area.y;
      paint_area.width = item->area.width;
      paint_area.height = item->area.height;

      /* check whether we are clipped fully */
      if (!gdk_rectangle_intersect (&paint_area, &clip, NULL))
        continue;

      /* paint the item */
      xfce_icon_view_paint_item (icon_view, item, cr, item->area.x, item->area.y, TRUE);
      if (G_UNLIKELY (dest_index >= 0 && dest_item == NULL))
        {
          if (dest_index == g_sequence_iter_get_position (item->item_iter))
            dest_item = item;
        }
    }

  /* draw the drag indicator */
  if (G_UNLIKELY (dest_item != NULL))
    {
      switch (dest_pos)
        {
        case XFCE_ICON_VIEW_DROP_INTO:
          gtk_render_focus (context,
                            cr,
                            dest_item->area.x, dest_item->area.y,
                            dest_item->area.width, dest_item->area.height);
          break;

        case XFCE_ICON_VIEW_DROP_ABOVE:
          gtk_render_focus (context,
                            cr,
                            dest_item->area.x, dest_item->area.y - 1,
                            dest_item->area.width, 2);
          break;

        case XFCE_ICON_VIEW_DROP_LEFT:
          gtk_render_focus (context,
                            cr,
                            dest_item->area.x - 1, dest_item->area.y,
                            2, dest_item->area.height);
          break;

        case XFCE_ICON_VIEW_DROP_BELOW:
          gtk_render_focus (context,
                            cr,
                            dest_item->area.x, dest_item->area.y + dest_item->area.height - 1,
                            dest_item->area.width, 2);
          break;

        case XFCE_ICON_VIEW_DROP_RIGHT:
          gtk_render_focus (context,
                            cr,
                            dest_item->area.x + dest_item->area.width - 1, dest_item->area.y,
                            2, dest_item->area.height);

        case XFCE_ICON_VIEW_NO_DROP:
          break;

        default:
          g_assert_not_reached ();
        }
    }

  /* draw the rubberband border */
  if (G_UNLIKELY (priv->doing_rubberband))
    {
      cairo_save (cr);

      rect.x = MIN (priv->rubberband_x_1, priv->rubberband_x2);
      rect.y = MIN (priv->rubberband_y_1, priv->rubberband_y2);
      rect.width = ABS (priv->rubberband_x_1 - priv->rubberband_x2) + 1;
      rect.height = ABS (priv->rubberband_y_1 - priv->rubberband_y2) + 1;

      gtk_style_context_save (context);
      gtk_style_context_add_class (context, GTK_STYLE_CLASS_RUBBERBAND);

      gdk_cairo_rectangle (cr, &rect);
      cairo_clip (cr);

      gtk_render_background (context, cr,
                             rect.x, rect.y,
                             rect.width, rect.height);
      gtk_render_frame (context, cr,
                        rect.x, rect.y,
                        rect.width, rect.height);

      gtk_style_context_restore (context);
      cairo_restore (cr);
    }

  /* let the GtkContainer forward the draw event to all children */
  GTK_WIDGET_CLASS (xfce_icon_view_parent_class)->draw (widget, cr);

  return FALSE;
}



static gboolean
rubberband_scroll_timeout (gpointer user_data)
{
  GtkAdjustment *adjustment;
  XfceIconView *icon_view = XFCE_ICON_VIEW (user_data);
  XfceIconViewPrivate *priv = get_instance_private (icon_view);
  gdouble value;

  /* determine the adjustment for the scroll direction */
  adjustment = (priv->layout_mode == XFCE_ICON_VIEW_LAYOUT_ROWS)
                 ? priv->vadjustment
                 : priv->hadjustment;

  /* determine the new scroll value */
  value = MIN (gtk_adjustment_get_value (adjustment) + priv->scroll_value_diff,
               gtk_adjustment_get_upper (adjustment) - gtk_adjustment_get_page_size (adjustment));

  /* apply the new value */
  gtk_adjustment_set_value (adjustment, value);

  /* update the rubberband */
  xfce_icon_view_update_rubberband (icon_view);

  return TRUE;
}


static gboolean
xfce_icon_view_motion_notify_event (GtkWidget *widget,
                                    GdkEventMotion *event)
{
  XfceIconViewItem *item;
  XfceIconView *icon_view = XFCE_ICON_VIEW (widget);
  XfceIconViewPrivate *priv = get_instance_private (icon_view);
  GdkCursor *cursor;
  gint size;
  gint abso;
  GtkAllocation allocation;

  xfce_icon_view_maybe_begin_drag (icon_view, event);
  gtk_widget_get_allocation (widget, &allocation);

  if (priv->doing_rubberband)
    {
      if ((event->state & GDK_CONTROL_MASK) == GDK_CONTROL_MASK)
        priv->ctrl_pressed = TRUE;
      if ((event->state & GDK_SHIFT_MASK) == GDK_SHIFT_MASK)
        priv->shift_pressed = TRUE;

      xfce_icon_view_update_rubberband (widget);

      if (priv->layout_mode == XFCE_ICON_VIEW_LAYOUT_ROWS)
        {
          abso = event->y
                 - priv->height
                     * gtk_adjustment_get_value (priv->vadjustment)
                     / (gtk_adjustment_get_upper (priv->vadjustment)
                        - gtk_adjustment_get_lower (priv->vadjustment));

          size = allocation.height;
        }
      else
        {
          abso = event->x
                 - priv->width
                     * gtk_adjustment_get_value (priv->hadjustment)
                     / (gtk_adjustment_get_upper (priv->hadjustment)
                        - gtk_adjustment_get_lower (priv->hadjustment));

          size = allocation.width;
        }

      if (abso < 0 || abso > size)
        {
          if (abso < 0)
            priv->scroll_value_diff = abso;
          else
            priv->scroll_value_diff = abso - size;
          priv->event_last_x = event->x;
          priv->event_last_y = event->y;

          if (priv->scroll_timeout_id == 0)
            priv->scroll_timeout_id = gdk_threads_add_timeout (30, rubberband_scroll_timeout, icon_view);
        }
      else
        {
          remove_scroll_timeout (icon_view);
        }
    }
  else
    {
      item = xfce_icon_view_get_item_at_coords (icon_view, event->x, event->y, TRUE, NULL);
      if (item != priv->prelit_item)
        {
          if (G_LIKELY (priv->prelit_item != NULL))
            xfce_icon_view_queue_draw_item (icon_view, priv->prelit_item);
          priv->prelit_item = item;
          if (G_LIKELY (item != NULL))
            xfce_icon_view_queue_draw_item (icon_view, item);

          /* check if we are in single click mode right now */
          if (G_UNLIKELY (priv->single_click))
            {
              /* display a hand cursor when pointer is above an item */
              if (G_LIKELY (item != NULL))
                {
                  /* hand2 seems to be what we should use */
                  cursor = gdk_cursor_new_for_display (gdk_window_get_display (event->window), GDK_HAND2);
                  gdk_window_set_cursor (event->window, cursor);
                  gdk_cursor_unref (cursor);
                }
              else
                {
                  /* reset the cursor */
                  gdk_window_set_cursor (event->window, NULL);
                }

              /* check if autoselection is enabled */
              if (G_LIKELY (priv->single_click_timeout > 0))
                {
                  /* drop any running timeout */
                  if (G_LIKELY (priv->single_click_timeout_id != 0))
                    g_source_remove (priv->single_click_timeout_id);

                  /* remember the current event state */
                  priv->single_click_timeout_state = event->state;

                  /* schedule a new timeout */
                  priv->single_click_timeout_id = gdk_threads_add_timeout_full (
                    G_PRIORITY_LOW,
                    priv->single_click_timeout,
                    xfce_icon_view_single_click_timeout,
                    icon_view,
                    xfce_icon_view_single_click_timeout_destroy);
                }
            }
        }
    }

  return TRUE;
}



static void
xfce_icon_view_remove (GtkContainer *container,
                       GtkWidget *widget)
{
  XfceIconViewChild *child;
  XfceIconView *icon_view = XFCE_ICON_VIEW (container);
  XfceIconViewPrivate *priv = get_instance_private (icon_view);

  for (GList *lp = priv->children; lp != NULL; lp = lp->next)
    {
      child = lp->data;
      if (G_LIKELY (child->widget == widget))
        {
          priv->children = g_list_delete_link (priv->children, lp);
          gtk_widget_unparent (widget);
          g_slice_free (XfceIconViewChild, child);
          return;
        }
    }
}



static void
xfce_icon_view_forall (GtkContainer *container,
                       gboolean include_internals,
                       GtkCallback callback,
                       gpointer callback_data)
{
  XfceIconView *icon_view = XFCE_ICON_VIEW (container);
  XfceIconViewPrivate *priv = get_instance_private (icon_view);

  for (GList *lp = priv->children; lp != NULL; lp = lp->next)
    (*callback) (((XfceIconViewChild *) lp->data)->widget, callback_data);
}



static void
xfce_icon_view_item_activate_cell (XfceIconView *icon_view,
                                   XfceIconViewItem *item,
                                   XfceIconViewCellInfo *info,
                                   GdkEvent *event)
{
  GtkCellRendererMode mode;
  GdkRectangle cell_area;
  GtkTreePath *path;
  gboolean visible;
  gchar *path_string;

  xfce_icon_view_set_cell_data (icon_view, item);

  g_object_get (G_OBJECT (info->cell), "visible", &visible, "mode", &mode, NULL);

  if (G_UNLIKELY (visible && mode == GTK_CELL_RENDERER_MODE_ACTIVATABLE))
    {
      xfce_icon_view_get_cell_area (icon_view, item, info, &cell_area);

      path = gtk_tree_path_new_from_indices (g_sequence_iter_get_position (item->item_iter), -1);
      path_string = gtk_tree_path_to_string (path);
      gtk_tree_path_free (path);

      gtk_cell_renderer_activate (info->cell, event, GTK_WIDGET (icon_view), path_string, &cell_area, &cell_area, 0);

      g_free (path_string);
    }
}



static void
xfce_icon_view_put (XfceIconView *icon_view,
                    GtkWidget *widget,
                    XfceIconViewItem *item,
                    gint cell)
{
  XfceIconViewPrivate *priv = get_instance_private (icon_view);
  XfceIconViewChild *child;

  /* allocate the new child */
  child = g_slice_new (XfceIconViewChild);
  child->widget = widget;
  child->item = item;
  child->cell = cell;

  /* hook up the child */
  priv->children = g_list_append (priv->children, child);

  /* setup the parent for the child */
  if (gtk_widget_get_realized (GTK_WIDGET (icon_view)))
    gtk_widget_set_parent_window (child->widget, priv->bin_window);
  gtk_widget_set_parent (widget, GTK_WIDGET (icon_view));
}



static void
xfce_icon_view_remove_widget (GtkCellEditable *editable,
                              XfceIconView *icon_view)
{
  XfceIconViewPrivate *priv = get_instance_private (icon_view);
  XfceIconViewItem *item;

  if (G_LIKELY (priv->edited_item != NULL))
    {
      item = priv->edited_item;
      priv->edited_item = NULL;
      priv->editable = NULL;

      for (GList *lp = priv->cell_list; lp != NULL; lp = lp->next)
        ((XfceIconViewCellInfo *) lp->data)->editing = FALSE;

      if (gtk_widget_has_focus (GTK_WIDGET (editable)))
        gtk_widget_grab_focus (GTK_WIDGET (icon_view));

      g_signal_handlers_disconnect_by_func (editable, xfce_icon_view_remove_widget, icon_view);
      gtk_container_remove (GTK_CONTAINER (icon_view), GTK_WIDGET (editable));

      xfce_icon_view_queue_draw_item (icon_view, item);
    }
}



static void
xfce_icon_view_start_editing (XfceIconView *icon_view,
                              XfceIconViewItem *item,
                              XfceIconViewCellInfo *info,
                              GdkEvent *event)
{
  XfceIconViewPrivate *priv = get_instance_private (icon_view);
  GtkCellRendererMode mode;
  GtkCellEditable *editable;
  GdkRectangle cell_area;
  GtkTreePath *path;
  gboolean visible;
  gchar *path_string;

  /* setup cell data for the given item */
  xfce_icon_view_set_cell_data (icon_view, item);

  /* check if the cell is visible and editable (given the updated cell data) */
  g_object_get (info->cell, "visible", &visible, "mode", &mode, NULL);
  if (G_LIKELY (visible && mode == GTK_CELL_RENDERER_MODE_EDITABLE))
    {
      /* draw keyboard focus while editing */
      XFCE_ICON_VIEW_SET_FLAG (icon_view, XFCE_ICON_VIEW_DRAW_KEYFOCUS);

      /* determine the cell area */
      xfce_icon_view_get_cell_area (icon_view, item, info, &cell_area);

      /* determine the tree path */
      path = gtk_tree_path_new_from_indices (g_sequence_iter_get_position (item->item_iter), -1);
      path_string = gtk_tree_path_to_string (path);
      gtk_tree_path_free (path);

      /* allocate the editable from the cell renderer */
      editable = gtk_cell_renderer_start_editing (info->cell, event, GTK_WIDGET (icon_view),
                                                  path_string, &cell_area, &cell_area, 0);

      /* ugly hack, but works */
      if (g_object_class_find_property (G_OBJECT_GET_CLASS (editable), "has-frame") != NULL)
        g_object_set (editable, "has-frame", TRUE, NULL);

      /* setup the editing widget */
      priv->edited_item = item;
      priv->editable = editable;
      info->editing = TRUE;

      xfce_icon_view_put (icon_view, GTK_WIDGET (editable), item, info->position);
      gtk_cell_editable_start_editing (GTK_CELL_EDITABLE (editable), (GdkEvent *) event);
      gtk_widget_grab_focus (GTK_WIDGET (editable));
      g_signal_connect (G_OBJECT (editable), "remove-widget", G_CALLBACK (xfce_icon_view_remove_widget), icon_view);

      /* cleanup */
      g_free (path_string);
    }
}



static void
xfce_icon_view_stop_editing (XfceIconView *icon_view,
                             gboolean cancel_editing)
{
  XfceIconViewPrivate *priv = get_instance_private (icon_view);
  XfceIconViewItem *item;
  GtkCellRenderer *cell = NULL;

  if (priv->edited_item == NULL)
    return;

  /*
   * This is very evil. We need to do this, because
   * gtk_cell_editable_editing_done may trigger xfce_icon_view_row_changed
   * later on. If xfce_icon_view_row_changed notices
   * priv->edited_item != NULL, it'll call
   * xfce_icon_view_stop_editing again. Bad things will happen then.
   *
   * Please read that again if you intend to modify anything here.
   */

  item = priv->edited_item;
  priv->edited_item = NULL;

  for (GList *lp = priv->cell_list; lp != NULL; lp = lp->next)
    {
      XfceIconViewCellInfo *info = lp->data;
      if (info->editing)
        {
          cell = info->cell;
          break;
        }
    }

  if (G_UNLIKELY (cell == NULL))
    return;

  gtk_cell_renderer_stop_editing (cell, cancel_editing);
  if (G_LIKELY (!cancel_editing))
    gtk_cell_editable_editing_done (priv->editable);

  priv->edited_item = item;

  gtk_cell_editable_remove_widget (priv->editable);
}



static gboolean
xfce_icon_view_button_press_event (GtkWidget *widget,
                                   GdkEventButton *event)
{
  XfceIconViewPrivate *priv = get_instance_private (widget);
  XfceIconViewCellInfo *info = NULL;
  GtkCellRendererMode mode;
  XfceIconViewItem *item;
  XfceIconView *icon_view;
  GtkTreePath *path;
  gboolean dirty = FALSE;
  gint cursor_cell;

  icon_view = XFCE_ICON_VIEW (widget);

  if (event->window != priv->bin_window)
    return FALSE;

  /* stop any pending "single-click-timeout" */
  if (G_UNLIKELY (priv->single_click_timeout_id != 0))
    g_source_remove (priv->single_click_timeout_id);

  if (G_UNLIKELY (!gtk_widget_has_focus (widget)))
    gtk_widget_grab_focus (widget);

  if (event->button == 1 && event->type == GDK_BUTTON_PRESS)
    {
      item = xfce_icon_view_get_item_at_coords (icon_view,
                                                event->x, event->y,
                                                TRUE,
                                                &info);
      if (item != NULL)
        {
          g_object_get (info->cell, "mode", &mode, NULL);

          if (mode == GTK_CELL_RENDERER_MODE_ACTIVATABLE || mode == GTK_CELL_RENDERER_MODE_EDITABLE)
            cursor_cell = g_list_index (priv->cell_list, info);
          else
            cursor_cell = -1;

          xfce_icon_view_scroll_to_item (icon_view, item);

          if (priv->selection_mode == GTK_SELECTION_NONE)
            {
              xfce_icon_view_set_cursor_item (icon_view, item, cursor_cell);
            }
          else if (priv->selection_mode == GTK_SELECTION_MULTIPLE && (event->state & GDK_SHIFT_MASK))
            {
              if (!(event->state & GDK_CONTROL_MASK))
                xfce_icon_view_unselect_all_internal (icon_view);

              xfce_icon_view_set_cursor_item (icon_view, item, cursor_cell);
              if (!priv->anchor_item)
                priv->anchor_item = item;
              else
                xfce_icon_view_select_all_between (icon_view,
                                                   priv->anchor_item,
                                                   item);
              dirty = TRUE;
            }
          else
            {
              if ((priv->selection_mode == GTK_SELECTION_MULTIPLE
                   || ((priv->selection_mode == GTK_SELECTION_SINGLE)
                       && item->selected))
                  && (event->state & GDK_CONTROL_MASK))
                {
                  item->selected = !item->selected;
                  xfce_icon_view_queue_draw_item (icon_view, item);
                  dirty = TRUE;
                }
              else
                {
                  if (!item->selected)
                    {
                      xfce_icon_view_unselect_all_internal (icon_view);

                      item->selected = TRUE;
                      xfce_icon_view_queue_draw_item (icon_view, item);
                      dirty = TRUE;
                    }
                }
              xfce_icon_view_set_cursor_item (icon_view, item, cursor_cell);
              priv->anchor_item = item;
            }

          /* Save press to possibly begin a drag */
          if (priv->pressed_button < 0)
            {
              priv->pressed_button = event->button;
              priv->press_start_x = event->x;
              priv->press_start_y = event->y;
            }

          priv->last_single_clicked = item;

          /* cancel the current editing, if it exists */
          xfce_icon_view_stop_editing (icon_view, TRUE);

          if (mode == GTK_CELL_RENDERER_MODE_ACTIVATABLE)
            xfce_icon_view_item_activate_cell (icon_view, item, info,
                                               (GdkEvent *) event);
          else if (mode == GTK_CELL_RENDERER_MODE_EDITABLE)
            xfce_icon_view_start_editing (icon_view, item, info,
                                          (GdkEvent *) event);
        }
      else
        {
          /* cancel the current editing, if it exists */
          xfce_icon_view_stop_editing (icon_view, TRUE);

          if (priv->selection_mode != GTK_SELECTION_BROWSE
              && !(event->state & (GDK_CONTROL_MASK | GDK_SHIFT_MASK)))
            {
              dirty = xfce_icon_view_unselect_all_internal (icon_view);
            }

          if (priv->selection_mode == GTK_SELECTION_MULTIPLE)
            xfce_icon_view_start_rubberbanding (icon_view, event->x, event->y);
        }
    }
  else if (event->button == 1 && event->type == GDK_2BUTTON_PRESS)
    {
      /* ignore double-click events in single-click mode */
      if (G_LIKELY (!priv->single_click))
        {
          item = xfce_icon_view_get_item_at_coords (icon_view,
                                                    event->x, event->y,
                                                    TRUE,
                                                    NULL);
          if (G_LIKELY (item != NULL))
            {
              path = gtk_tree_path_new_from_indices (g_sequence_iter_get_position (item->item_iter), -1);
              xfce_icon_view_item_activated (icon_view, path);
              gtk_tree_path_free (path);
            }
        }

      priv->last_single_clicked = NULL;
      priv->pressed_button = -1;
    }

  /* grab focus and stop drawing the keyboard focus indicator on single clicks */
  if (G_LIKELY (event->type != GDK_2BUTTON_PRESS && event->type != GDK_3BUTTON_PRESS))
    {
      if (!gtk_widget_has_focus (GTK_WIDGET (icon_view)))
        gtk_widget_grab_focus (GTK_WIDGET (icon_view));
      XFCE_ICON_VIEW_UNSET_FLAG (icon_view, XFCE_ICON_VIEW_DRAW_KEYFOCUS);
    }

  if (dirty)
    g_signal_emit (icon_view, icon_view_signals[SELECTION_CHANGED], 0);

  return event->button == 1;
}



static gboolean
xfce_icon_view_button_release_event (GtkWidget *widget,
                                     GdkEventButton *event)
{
  XfceIconViewItem *item;
  XfceIconView *icon_view = XFCE_ICON_VIEW (widget);
  XfceIconViewPrivate *priv = get_instance_private (icon_view);
  GtkTreePath *path;

  if (priv->pressed_button == (gint) event->button)
    {
      if (G_LIKELY (event->state & (GDK_CONTROL_MASK | GDK_SHIFT_MASK)) == 0)
        {
          /* determine the item at the mouse coords and check if this is the last single clicked one */
          item = xfce_icon_view_get_item_at_coords (icon_view, event->x, event->y, TRUE, NULL);
          if (G_LIKELY (item != NULL && item == priv->last_single_clicked))
            {
              if (priv->single_click)
                {
                  /* emit an "item-activated" signal for this item */
                  path = gtk_tree_path_new_from_indices (g_sequence_iter_get_position (item->item_iter), -1);
                  xfce_icon_view_item_activated (icon_view, path);
                  gtk_tree_path_free (path);
                }
              else
                {
                  /* reduce the selection to just the clicked item  */
                  xfce_icon_view_unselect_all_internal (icon_view);
                  xfce_icon_view_select_item (icon_view, item);
                }
            }

          /* reset the last single clicked item */
          priv->last_single_clicked = NULL;
        }

      /* reset the pressed_button state */
      priv->pressed_button = -1;
    }

  xfce_icon_view_stop_rubberbanding (icon_view);

  remove_scroll_timeout (icon_view);

  return TRUE;
}



static gboolean
xfce_icon_view_scroll_event (GtkWidget *widget,
                             GdkEventScroll *event)
{
  XfceIconView *icon_view = XFCE_ICON_VIEW (widget);
  XfceIconViewPrivate *priv = get_instance_private (icon_view);

  /* we don't care for scroll events in "rows" layout mode, as
   * that's completely handled by GtkScrolledWindow.
   */
  if (priv->layout_mode != XFCE_ICON_VIEW_LAYOUT_COLS)
    return FALSE;

  /* convert vertical scroll events to horizontal ones in "columns" layout mode */
  if (event->direction == GDK_SCROLL_UP)
    event->direction = GDK_SCROLL_LEFT;
  else if (event->direction == GDK_SCROLL_DOWN)
    event->direction = GDK_SCROLL_RIGHT;
  else if (event->direction == GDK_SCROLL_SMOOTH)
    {
      if (event->delta_x == 0.0)
        {
          event->delta_x = event->delta_y;
          event->delta_y = 0.0;
        }
    }

  /* scrolling will be handled by GtkScrolledWindow */
  return FALSE;
}



static gboolean
xfce_icon_view_key_press_event (GtkWidget *widget,
                                GdkEventKey *event)
{
  XfceIconView *icon_view = XFCE_ICON_VIEW (widget);
  XfceIconViewPrivate *priv = get_instance_private (icon_view);
  gboolean retval;

  /* let the parent class handle the key bindings and stuff */
  if ((*GTK_WIDGET_CLASS (xfce_icon_view_parent_class)->key_press_event) (widget, event))
    return TRUE;

  /* check if typeahead search is enabled */
  if (G_UNLIKELY (!priv->enable_search))
    return FALSE;

  xfce_icon_view_search_ensure_directory (icon_view);

  /* check if keypress results in a text change in search_entry; prevents showing the search
   * window when only modifier keys (shift, control, ...) are pressed */
  retval = gtk_entry_im_context_filter_keypress (GTK_ENTRY (priv->search_entry), event);

  if (retval)
    {
      if (xfce_icon_view_search_start (icon_view, FALSE))
        {
          gtk_entry_grab_focus_without_selecting (GTK_ENTRY (priv->search_entry));
          return TRUE;
        }
      else
        {
          gtk_entry_set_text (GTK_ENTRY (priv->search_entry), "");
          return FALSE;
        }
    }

  return FALSE;
}



static gboolean
xfce_icon_view_focus_out_event (GtkWidget *widget,
                                GdkEventFocus *event)
{
  XfceIconView *icon_view = XFCE_ICON_VIEW (widget);
  XfceIconViewPrivate *priv = get_instance_private (icon_view);

  /* be sure to cancel any single-click timeout */
  if (G_UNLIKELY (priv->single_click_timeout_id != 0))
    g_source_remove (priv->single_click_timeout_id);

  /* reset the cursor if we're still realized */
  if (G_LIKELY (priv->bin_window != NULL))
    gdk_window_set_cursor (priv->bin_window, NULL);

  /* destroy the interactive search dialog */
  if (G_UNLIKELY (priv->search_window != NULL))
    xfce_icon_view_search_dialog_hide (priv->search_window, icon_view);

  /* schedule a redraw with the new focus state */
  gtk_widget_queue_draw (widget);

  return FALSE;
}



static gboolean
xfce_icon_view_leave_notify_event (GtkWidget *widget,
                                   GdkEventCrossing *event)
{
  XfceIconView *icon_view = XFCE_ICON_VIEW (widget);
  XfceIconViewPrivate *priv = get_instance_private (icon_view);

  /* reset cursor to default */
  if (gtk_widget_get_realized (widget))
    gdk_window_set_cursor (gtk_widget_get_window (widget), NULL);

  /* reset the prelit item (if any) */
  if (G_LIKELY (priv->prelit_item != NULL))
    {
      xfce_icon_view_queue_draw_item (icon_view, priv->prelit_item);
      priv->prelit_item = NULL;
    }

  /* call the parent's leave_notify_event (if any) */
  if (GTK_WIDGET_CLASS (xfce_icon_view_parent_class)->leave_notify_event != NULL)
    return (*GTK_WIDGET_CLASS (xfce_icon_view_parent_class)->leave_notify_event) (widget, event);

  /* other signal handlers may be invoked */
  return FALSE;
}



static void
xfce_icon_view_update_rubberband (gpointer data)
{
  XfceIconView *icon_view = XFCE_ICON_VIEW (data);
  XfceIconViewPrivate *priv = get_instance_private (icon_view);
  gint x, y;
  GdkRectangle old_area;
  GdkRectangle new_area;
  GdkRectangle common;
  GdkRegion *invalid_region;
  GdkSeat *seat;
  GdkDevice *pointer_dev;

  seat = gdk_display_get_default_seat (gdk_window_get_display (priv->bin_window));
  pointer_dev = gdk_seat_get_pointer (seat);
  gdk_window_get_device_position (priv->bin_window, pointer_dev, &x, &y, NULL);

  x = MAX (x, 0);
  y = MAX (y, 0);

  old_area.x = MIN (priv->rubberband_x_1,
                    priv->rubberband_x2);
  old_area.y = MIN (priv->rubberband_y_1,
                    priv->rubberband_y2);
  old_area.width = ABS (priv->rubberband_x2 - priv->rubberband_x_1) + 1;
  old_area.height = ABS (priv->rubberband_y2 - priv->rubberband_y_1) + 1;

  new_area.x = MIN (priv->rubberband_x_1, x);
  new_area.y = MIN (priv->rubberband_y_1, y);
  new_area.width = ABS (x - priv->rubberband_x_1) + 1;
  new_area.height = ABS (y - priv->rubberband_y_1) + 1;

  invalid_region = gdk_region_rectangle (&old_area);
  gdk_region_union_with_rect (invalid_region, &new_area);

  gdk_rectangle_intersect (&old_area, &new_area, &common);
  if (common.width > 2 && common.height > 2)
    {
      GdkRegion *common_region;

      /* make sure the border is invalidated */
      common.x += 1;
      common.y += 1;
      common.width -= 2;
      common.height -= 2;

      common_region = gdk_region_rectangle (&common);

      gdk_region_subtract (invalid_region, common_region);
      gdk_region_destroy (common_region);
    }

  gdk_window_invalidate_region (priv->bin_window, invalid_region, TRUE);

  gdk_region_destroy (invalid_region);

  priv->rubberband_x2 = x;
  priv->rubberband_y2 = y;

  xfce_icon_view_update_rubberband_selection (icon_view);
}



static void
xfce_icon_view_start_rubberbanding (XfceIconView *icon_view,
                                    gint x,
                                    gint y)
{
  XfceIconViewPrivate *priv = get_instance_private (icon_view);
  gpointer drag_data;
  GSequenceIter *iter;

  /* be sure to disable any previously active rubberband */
  xfce_icon_view_stop_rubberbanding (icon_view);

  for (iter = g_sequence_get_begin_iter (priv->items);
       !g_sequence_iter_is_end (iter);
       iter = g_sequence_iter_next (iter))
    {
      XfceIconViewItem *item = g_sequence_get (iter);
      item->selected_before_rubberbanding = item->selected;
    }

  priv->rubberband_x_1 = x;
  priv->rubberband_y_1 = y;
  priv->rubberband_x2 = x;
  priv->rubberband_y2 = y;

  priv->doing_rubberband = TRUE;

  gtk_grab_add (GTK_WIDGET (icon_view));

  /* be sure to disable Gtk+ DnD callbacks, because else rubberbanding will be interrupted */
  drag_data = g_object_get_data (G_OBJECT (icon_view), I_ ("gtk-site-data"));
  if (G_LIKELY (drag_data != NULL))
    {
      g_signal_handlers_block_matched (G_OBJECT (icon_view),
                                       G_SIGNAL_MATCH_DATA,
                                       0, 0, NULL, NULL,
                                       drag_data);
    }
}



static void
xfce_icon_view_stop_rubberbanding (XfceIconView *icon_view)
{
  XfceIconViewPrivate *priv = get_instance_private (icon_view);
  gpointer drag_data;

  if (G_LIKELY (priv->doing_rubberband))
    {
      priv->doing_rubberband = FALSE;
      priv->ctrl_pressed = FALSE;
      priv->shift_pressed = FALSE;
      gtk_grab_remove (GTK_WIDGET (icon_view));
      gtk_widget_queue_draw (GTK_WIDGET (icon_view));

      /* re-enable Gtk+ DnD callbacks again */
      drag_data = g_object_get_data (G_OBJECT (icon_view), I_ ("gtk-site-data"));
      if (G_LIKELY (drag_data != NULL))
        {
          g_signal_handlers_unblock_matched (G_OBJECT (icon_view),
                                             G_SIGNAL_MATCH_DATA,
                                             0, 0, NULL, NULL,
                                             drag_data);
        }
    }
}



static void
xfce_icon_view_update_rubberband_selection (XfceIconView *icon_view)
{
  XfceIconViewPrivate *priv = get_instance_private (icon_view);
  XfceIconViewItem *item;
  gboolean selected;
  gboolean changed = FALSE;
  gboolean is_in;
  gint x, y;
  gint width;
  gint height;
  GSequenceIter *iter;

  /* determine the new rubberband area */
  x = MIN (priv->rubberband_x_1, priv->rubberband_x2);
  y = MIN (priv->rubberband_y_1, priv->rubberband_y2);
  width = ABS (priv->rubberband_x_1 - priv->rubberband_x2);
  height = ABS (priv->rubberband_y_1 - priv->rubberband_y2);

  /* check all items */
  for (iter = g_sequence_get_begin_iter (priv->items);
       !g_sequence_iter_is_end (iter);
       iter = g_sequence_iter_next (iter))
    {
      item = XFCE_ICON_VIEW_ITEM (g_sequence_get (iter));

      is_in = xfce_icon_view_item_hit_test (icon_view, item, x, y, width, height);

      selected = is_in ^ item->selected_before_rubberbanding;

      if (G_UNLIKELY (item->selected != selected))
        {
          /* extend */
          if (priv->shift_pressed && !priv->ctrl_pressed)
            {
              if (!item->selected)
                {
                  changed = TRUE;
                  item->selected = TRUE;
                }
            }
          /* add/remove */
          else
            {
              changed = TRUE;
              item->selected = selected;
            }

          if (changed)
            xfce_icon_view_queue_draw_item (icon_view, item);
        }

      if (item->selected)
        priv->cursor_item = item;
    }

  if (G_LIKELY (changed))
    g_signal_emit (G_OBJECT (icon_view), icon_view_signals[SELECTION_CHANGED], 0);
}



static gboolean
xfce_icon_view_item_hit_test (XfceIconView *icon_view,
                              XfceIconViewItem *item,
                              gint x,
                              gint y,
                              gint width,
                              gint height)
{
  XfceIconViewPrivate *priv = get_instance_private (icon_view);
  GList *l;
  GdkRectangle box;
  XfceIconViewCellInfo *info;

  for (l = priv->cell_list; l; l = l->next)
    {
      info = l->data;

      if (!gtk_cell_renderer_get_visible (info->cell)
          || item->box == NULL)
        continue;

      box = item->box[info->position];

      if (MIN (x + width, box.x + box.width) - MAX (x, box.x) > 0
          && MIN (y + height, box.y + box.height) - MAX (y, box.y) > 0)
        return TRUE;
    }

  return FALSE;
}



static gboolean
xfce_icon_view_unselect_all_internal (XfceIconView *icon_view)
{
  XfceIconViewPrivate *priv = get_instance_private (icon_view);
  XfceIconViewItem *item;
  GSequenceIter *iter;
  gboolean dirty = FALSE;

  if (G_LIKELY (priv->selection_mode != GTK_SELECTION_NONE))
    {
      for (iter = g_sequence_get_begin_iter (priv->items);
           !g_sequence_iter_is_end (iter);
           iter = g_sequence_iter_next (iter))
        {
          item = XFCE_ICON_VIEW_ITEM (g_sequence_get (iter));
          if (item->selected)
            {
              dirty = TRUE;
              item->selected = FALSE;
              xfce_icon_view_queue_draw_item (icon_view, item);
            }
        }
    }

  return dirty;
}



static void
xfce_icon_view_set_adjustments (XfceIconView *icon_view,
                                GtkAdjustment *hadj,
                                GtkAdjustment *vadj)
{
  XfceIconViewPrivate *priv = get_instance_private (icon_view);
  gboolean need_adjust = FALSE;

  if (hadj)
    g_return_if_fail (GTK_IS_ADJUSTMENT (hadj));
  else
    hadj = GTK_ADJUSTMENT (gtk_adjustment_new (0.0, 0.0, 0.0, 0.0, 0.0, 0.0));
  if (vadj)
    g_return_if_fail (GTK_IS_ADJUSTMENT (vadj));
  else
    vadj = GTK_ADJUSTMENT (gtk_adjustment_new (0.0, 0.0, 0.0, 0.0, 0.0, 0.0));

  if (priv->hadjustment && (priv->hadjustment != hadj))
    {
      g_signal_handlers_disconnect_matched (priv->hadjustment, G_SIGNAL_MATCH_DATA,
                                            0, 0, NULL, NULL, icon_view);
      g_object_unref (priv->hadjustment);
    }

  if (priv->vadjustment && (priv->vadjustment != vadj))
    {
      g_signal_handlers_disconnect_matched (priv->vadjustment, G_SIGNAL_MATCH_DATA,
                                            0, 0, NULL, NULL, icon_view);
      g_object_unref (priv->vadjustment);
    }

  if (priv->hadjustment != hadj)
    {
      priv->hadjustment = hadj;
      g_object_ref_sink (priv->hadjustment);

      g_signal_connect (priv->hadjustment, "value-changed",
                        G_CALLBACK (xfce_icon_view_adjustment_changed),
                        icon_view);
      need_adjust = TRUE;
    }

  if (priv->vadjustment != vadj)
    {
      priv->vadjustment = vadj;
      g_object_ref_sink (priv->vadjustment);

      g_signal_connect (priv->vadjustment, "value-changed",
                        G_CALLBACK (xfce_icon_view_adjustment_changed),
                        icon_view);
      need_adjust = TRUE;
    }

  if (need_adjust)
    xfce_icon_view_adjustment_changed (NULL, icon_view);
}



static void
xfce_icon_view_real_select_all (XfceIconView *icon_view)
{
  xfce_icon_view_select_all (icon_view);
}



static void
xfce_icon_view_real_unselect_all (XfceIconView *icon_view)
{
  xfce_icon_view_unselect_all (icon_view);
}



static void
xfce_icon_view_real_select_cursor_item (XfceIconView *icon_view)
{
  XfceIconViewPrivate *priv = get_instance_private (icon_view);
  xfce_icon_view_unselect_all (icon_view);

  if (priv->cursor_item != NULL)
    xfce_icon_view_select_item (icon_view, priv->cursor_item);
}



static gboolean
xfce_icon_view_real_activate_cursor_item (XfceIconView *icon_view)
{
  XfceIconViewPrivate *priv = get_instance_private (icon_view);
  GtkTreePath *path;
  GtkCellRendererMode mode;
  XfceIconViewCellInfo *info = NULL;

  if (!priv->cursor_item)
    return FALSE;

  info = g_list_nth_data (priv->cell_list,
                          priv->cursor_cell);

  if (info)
    {
      g_object_get (info->cell, "mode", &mode, NULL);

      if (mode == GTK_CELL_RENDERER_MODE_ACTIVATABLE)
        {
          xfce_icon_view_item_activate_cell (icon_view,
                                             priv->cursor_item,
                                             info, NULL);
          return TRUE;
        }
      else if (mode == GTK_CELL_RENDERER_MODE_EDITABLE)
        {
          xfce_icon_view_start_editing (icon_view,
                                        priv->cursor_item,
                                        info, NULL);
          return TRUE;
        }
    }

  path = gtk_tree_path_new_from_indices (g_sequence_iter_get_position (priv->cursor_item->item_iter), -1);
  xfce_icon_view_item_activated (icon_view, path);
  gtk_tree_path_free (path);

  return TRUE;
}



static gboolean
xfce_icon_view_real_start_interactive_search (XfceIconView *icon_view)
{
  return xfce_icon_view_search_start (icon_view, TRUE);
}



static void
xfce_icon_view_real_toggle_cursor_item (XfceIconView *icon_view)
{
  XfceIconViewPrivate *priv = get_instance_private (icon_view);

  if (G_LIKELY (priv->cursor_item != NULL))
    {
      switch (priv->selection_mode)
        {
        case GTK_SELECTION_NONE:
          break;

        case GTK_SELECTION_BROWSE:
          xfce_icon_view_select_item (icon_view, priv->cursor_item);
          break;

        case GTK_SELECTION_SINGLE:
          if (priv->cursor_item->selected)
            xfce_icon_view_unselect_item (icon_view, priv->cursor_item);
          else
            xfce_icon_view_select_item (icon_view, priv->cursor_item);
          break;

        case GTK_SELECTION_MULTIPLE:
          priv->cursor_item->selected = !priv->cursor_item->selected;
          g_signal_emit (icon_view, icon_view_signals[SELECTION_CHANGED], 0);
          xfce_icon_view_queue_draw_item (icon_view, priv->cursor_item);
          break;

        default:
          g_assert_not_reached ();
        }
    }
}



static void
xfce_icon_view_adjustment_changed (GtkAdjustment *adjustment,
                                   XfceIconView *icon_view)
{
  XfceIconViewPrivate *priv = get_instance_private (icon_view);

  if (gtk_widget_get_realized (GTK_WIDGET (icon_view)))
    {
      gdk_window_move (priv->bin_window,
                       -gtk_adjustment_get_value (priv->hadjustment),
                       -gtk_adjustment_get_value (priv->vadjustment));

      if (G_UNLIKELY (priv->doing_rubberband))
        xfce_icon_view_update_rubberband (GTK_WIDGET (icon_view));
    }
}



static GSequenceIter *
xfce_icon_view_layout_single_row (XfceIconView *icon_view,
                                  GSequenceIter *first_item,
                                  gint item_width,
                                  gint row,
                                  gint *y,
                                  gint *maximum_width,
                                  gint max_cols)
{
  XfceIconViewPrivate *priv = get_instance_private (icon_view);
  XfceIconViewItem *item;
  gboolean rtl;
  GSequenceIter *last_item;
  GSequenceIter *iter = first_item;
  gint *max_width;
  gint *max_height;
  gint focus_width;
  gint current_width;
  gint colspan;
  gint col = 0;
  gint x;
  gint i;
  GtkAllocation allocation;

  rtl = (gtk_widget_get_direction (GTK_WIDGET (icon_view)) == GTK_TEXT_DIR_RTL);
  gtk_widget_get_allocation (GTK_WIDGET (icon_view), &allocation);

  max_width = g_newa0 (gint, priv->n_cells);
  max_height = g_newa0 (gint, priv->n_cells);

  gtk_widget_style_get (GTK_WIDGET (icon_view),
                        "focus-line-width", &focus_width,
                        NULL);

  x = priv->margin + focus_width;
  current_width = 2 * (priv->margin + focus_width);

  for (iter = first_item; !g_sequence_iter_is_end (iter); iter = g_sequence_iter_next (iter))
    {
      item = XFCE_ICON_VIEW_ITEM (g_sequence_get (iter));

      xfce_icon_view_calculate_item_size (icon_view, item);
      colspan = 1 + (item->area.width - 1) / (item_width + priv->column_spacing);

      item->area.width = colspan * item_width + (colspan - 1) * priv->column_spacing;

      current_width += item->area.width + priv->column_spacing + 2 * focus_width;

      if (G_LIKELY (iter != first_item))
        {
          if ((priv->columns <= 0 && current_width > allocation.width)
              || (priv->columns > 0 && col >= priv->columns)
              || (max_cols > 0 && col >= max_cols))
            break;
        }

      item->area.y = *y + focus_width;
      item->area.x = rtl ? allocation.width - item->area.width - x : x;

      x = current_width - (priv->margin + focus_width);

      for (i = 0; i < priv->n_cells; i++)
        {
          max_width[i] = MAX (max_width[i], item->box[i].width);
          max_height[i] = MAX (max_height[i], item->box[i].height);
        }

      if (current_width > *maximum_width)
        *maximum_width = current_width;

      item->row = row;
      item->col = col;

      col += colspan;
    }

  last_item = iter;

  /* Now go through the row again and align the icons */
  for (iter = first_item; iter != last_item; iter = g_sequence_iter_next (iter))
    {
      item = XFCE_ICON_VIEW_ITEM (g_sequence_get (iter));

      xfce_icon_view_calculate_item_size2 (icon_view, item, max_width, max_height);

      /* We may want to readjust the new y coordinate. */
      if (item->area.y + item->area.height + focus_width + priv->row_spacing > *y)
        *y = item->area.y + item->area.height + focus_width + priv->row_spacing;

      if (G_UNLIKELY (rtl))
        item->col = col - 1 - item->col;
    }

  return last_item;
}



static GSequenceIter *
xfce_icon_view_layout_single_col (XfceIconView *icon_view,
                                  GSequenceIter *first_item,
                                  gint item_height,
                                  gint col,
                                  gint *x,
                                  gint *maximum_height,
                                  gint max_rows)
{
  XfceIconViewPrivate *priv = get_instance_private (icon_view);
  XfceIconViewItem *item;
  GSequenceIter *iter = first_item;
  GSequenceIter *last_item;
  gint *max_width;
  gint *max_height;
  gint focus_width;
  gint current_height;
  gint rowspan;
  gint row = 0;
  gint y;
  gint i;
  GtkAllocation allocation;

  max_width = g_newa0 (gint, priv->n_cells);
  max_height = g_newa0 (gint, priv->n_cells);

  gtk_widget_style_get (GTK_WIDGET (icon_view),
                        "focus-line-width", &focus_width,
                        NULL);
  gtk_widget_get_allocation (GTK_WIDGET (icon_view), &allocation);

  y = priv->margin + focus_width;
  current_height = 2 * (priv->margin + focus_width);

  for (iter = first_item; !g_sequence_iter_is_end (iter); iter = g_sequence_iter_next (iter))
    {
      item = XFCE_ICON_VIEW_ITEM (g_sequence_get (iter));

      xfce_icon_view_calculate_item_size (icon_view, item);

      rowspan = 1 + (item->area.height - 1) / (item_height + priv->row_spacing);

      item->area.height = rowspan * item_height + (rowspan - 1) * priv->row_spacing;

      current_height += item->area.height + priv->row_spacing + 2 * focus_width;

      if (G_LIKELY (iter != first_item))
        {
          if (current_height >= allocation.height || (max_rows > 0 && row >= max_rows))
            break;
        }

      item->area.y = y + focus_width;
      item->area.x = *x;

      y = current_height - (priv->margin + focus_width);

      for (i = 0; i < priv->n_cells; i++)
        {
          max_width[i] = MAX (max_width[i], item->box[i].width);
          max_height[i] = MAX (max_height[i], item->box[i].height);
        }

      if (current_height > *maximum_height)
        *maximum_height = current_height;

      item->row = row;
      item->col = col;

      row += rowspan;
    }

  last_item = iter;

  /* Now go through the column again and align the icons */
  for (iter = first_item; iter != last_item; iter = g_sequence_iter_next (iter))
    {
      item = XFCE_ICON_VIEW_ITEM (g_sequence_get (iter));

      xfce_icon_view_calculate_item_size2 (icon_view, item, max_width, max_height);

      /* We may want to readjust the new x coordinate. */
      if (item->area.x + item->area.width + focus_width + priv->column_spacing > *x)
        *x = item->area.x + item->area.width + focus_width + priv->column_spacing;
    }

  return last_item;
}



static void
xfce_icon_view_set_adjustment_upper (GtkAdjustment *adj,
                                     gdouble upper)
{
  if (upper != gtk_adjustment_get_upper (adj))
    {
      gdouble min = MAX (0.0, upper - gtk_adjustment_get_page_size (adj));

      gtk_adjustment_set_upper (adj, upper);

      if (gtk_adjustment_get_value (adj) > min)
        {
          gtk_adjustment_set_value (adj, min);
        }
    }
}



static gint
xfce_icon_view_layout_cols (XfceIconView *icon_view,
                            gint item_height,
                            gint *x,
                            gint *maximum_height,
                            gint max_rows)
{
  XfceIconViewPrivate *priv = get_instance_private (icon_view);
  GSequenceIter *icons = g_sequence_get_begin_iter (priv->items);
  GSequenceIter *iter;
  gint col = 0;
  gint rows = 0;

  *x = priv->margin;

  do
    {
      icons = xfce_icon_view_layout_single_col (icon_view, icons,
                                                item_height, col,
                                                x, maximum_height, max_rows);

      /* count the number of rows in the first column */
      if (G_UNLIKELY (col == 0))
        {
          for (iter = g_sequence_get_begin_iter (priv->items), rows = 0;
               iter != icons;
               iter = g_sequence_iter_next (iter), ++rows)
            ;
        }

      col++;
    }
  while (!g_sequence_iter_is_end (icons));

  *x += priv->margin;
  priv->cols = col;

  return rows;
}



static gint
xfce_icon_view_layout_rows (XfceIconView *icon_view,
                            gint item_width,
                            gint *y,
                            gint *maximum_width,
                            gint max_cols)
{
  XfceIconViewPrivate *priv = get_instance_private (icon_view);
  GSequenceIter *icons = g_sequence_get_begin_iter (priv->items);
  GSequenceIter *iter;
  gint row = 0;
  gint cols = 0;

  *y = priv->margin;

  do
    {
      icons = xfce_icon_view_layout_single_row (icon_view, icons,
                                                item_width, row,
                                                y, maximum_width, max_cols);

      /* count the number of columns in the first row */
      if (G_UNLIKELY (row == 0))
        {
          for (iter = g_sequence_get_begin_iter (priv->items), cols = 0;
               iter != icons;
               iter = g_sequence_iter_next (iter), ++cols)
            ;
        }

      row++;
    }
  while (!g_sequence_iter_is_end (icons));

  *y += priv->margin;
  priv->rows = row;

  return cols;
}



static void
xfce_icon_view_layout (XfceIconView *icon_view)
{
  XfceIconViewPrivate *priv = get_instance_private (icon_view);
  XfceIconViewItem *item;
  GSequenceIter *iter;
  gint maximum_height = 0;
  gint maximum_width = 0;
  gint item_height;
  gint item_width;
  gint rows, cols;
  gint x, y;
  GtkAllocation allocation;
  GtkRequisition requisition;

  /* verify that we still have a valid model */
  if (G_UNLIKELY (priv->model == NULL))
    return;

  gtk_widget_get_allocation (GTK_WIDGET (icon_view), &allocation);

  gtk_widget_get_preferred_width (GTK_WIDGET (icon_view), NULL, &requisition.width);
  gtk_widget_get_preferred_height (GTK_WIDGET (icon_view), NULL, &requisition.height);

  /* determine the layout mode */
  if (G_LIKELY (priv->layout_mode == XFCE_ICON_VIEW_LAYOUT_ROWS))
    {
      /* calculate item sizes on-demand */
      item_width = priv->item_width;
      if (item_width < 0)
        {
          for (iter = g_sequence_get_begin_iter (priv->items);
               !g_sequence_iter_is_end (iter);
               iter = g_sequence_iter_next (iter))
            {
              item = g_sequence_get (iter);
              xfce_icon_view_calculate_item_size (icon_view, item);
              item_width = MAX (item_width, item->area.width);
            }
        }

      cols = xfce_icon_view_layout_rows (icon_view, item_width, &y, &maximum_width, 0);

      /* If, by adding another column, we increase the height of the icon view, thus forcing a
       * vertical scrollbar to appear that would prevent the last column from being able to fit,
       * we need to relayout the icons with one less column.
       */
      if (cols == priv->cols + 1
          && y > allocation.height
          && priv->height <= allocation.height)
        {
          cols = xfce_icon_view_layout_rows (icon_view, item_width, &y, &maximum_width, priv->cols);
        }

      priv->width = maximum_width;
      priv->height = y;
      priv->cols = cols;
    }
  else
    {
      /* calculate item sizes on-demand */
      item_height = 0;
      for (iter = g_sequence_get_begin_iter (priv->items);
           !g_sequence_iter_is_end (iter);
           iter = g_sequence_iter_next (iter))
        {
          item = g_sequence_get (iter);
          xfce_icon_view_calculate_item_size (icon_view, item);
          item_height = MAX (item_height, item->area.height);
        }

      rows = xfce_icon_view_layout_cols (icon_view, item_height, &x, &maximum_height, 0);

      /* If, by adding another row, we increase the width of the icon view, thus forcing a
       * horizontal scrollbar to appear that would prevent the last row from being able to fit,
       * we need to relayout the icons with one less row.
       */
      if (rows == priv->rows + 1
          && x > allocation.width
          && priv->width <= allocation.width)
        {
          rows = xfce_icon_view_layout_cols (icon_view, item_height, &x, &maximum_height, priv->rows);
        }

      priv->height = maximum_height;
      priv->width = x;
      priv->rows = rows;
    }

  xfce_icon_view_set_adjustment_upper (priv->hadjustment, priv->width);
  xfce_icon_view_set_adjustment_upper (priv->vadjustment, priv->height);

  if (priv->width != requisition.width
      || priv->height != requisition.height)
    gtk_widget_queue_resize_no_redraw (GTK_WIDGET (icon_view));

  if (gtk_widget_get_realized (GTK_WIDGET (icon_view)))
    {
      gdk_window_resize (priv->bin_window,
                         MAX (priv->width, allocation.width),
                         MAX (priv->height, allocation.height));
    }

  /* drop any pending layout idle source */
  if (priv->layout_idle_id != 0)
    g_source_remove (priv->layout_idle_id);

  gtk_widget_queue_draw (GTK_WIDGET (icon_view));
}



static void
xfce_icon_view_get_cell_area (XfceIconView *icon_view,
                              XfceIconViewItem *item,
                              XfceIconViewCellInfo *info,
                              GdkRectangle *cell_area)
{
  XfceIconViewPrivate *priv = get_instance_private (icon_view);

  if (priv->orientation == GTK_ORIENTATION_HORIZONTAL)
    {
      cell_area->x = item->box[info->position].x - item->before[info->position];
      cell_area->y = item->area.y;
      cell_area->width = item->box[info->position].width + item->before[info->position] + item->after[info->position];
      cell_area->height = item->area.height;
    }
  else
    {
      cell_area->x = item->area.x;
      cell_area->y = item->box[info->position].y - item->before[info->position];
      cell_area->width = item->area.width;
      cell_area->height = item->box[info->position].height + item->before[info->position] + item->after[info->position];
    }
}



static void
xfce_icon_view_calculate_item_size (XfceIconView *icon_view,
                                    XfceIconViewItem *item)
{
  XfceIconViewPrivate *priv = get_instance_private (icon_view);
  XfceIconViewCellInfo *info;
  GList *lp;
  gchar *buffer;

  if (G_LIKELY (item->area.width != -1))
    return;

  if (G_UNLIKELY (item->n_cells != priv->n_cells))
    {
      /* apply the new cell size */
      item->n_cells = priv->n_cells;

      /* release the memory chunk (if any) */
      g_free (item->box);

      /* allocate a single memory chunk for box, after and before */
      buffer = g_malloc0 (item->n_cells * (sizeof (GdkRectangle) + 2 * sizeof (gint)));

      item->box = (GdkRectangle *) (gpointer) buffer;
      item->after = (gint *) (gpointer) (buffer + item->n_cells * sizeof (GdkRectangle));
      item->before = item->after + item->n_cells;
    }

  xfce_icon_view_set_cell_data (icon_view, item);

  item->area.width = 0;
  item->area.height = 0;
  for (lp = priv->cell_list; lp != NULL; lp = lp->next)
    {
      info = XFCE_ICON_VIEW_CELL_INFO (lp->data);
      if (G_UNLIKELY (!gtk_cell_renderer_get_visible (info->cell)))
        continue;

      {
        GtkRequisition req;

        gtk_cell_renderer_get_preferred_size (info->cell, GTK_WIDGET (icon_view),
                                              &req, NULL);

        if (info->is_text && priv->orientation == GTK_ORIENTATION_VERTICAL)
          {
            GdkRectangle cell_area, aligned_area;

            cell_area.x = cell_area.y = 0;
            cell_area.width = req.width;
            cell_area.height = req.height;

            gtk_cell_renderer_get_aligned_area (info->cell, GTK_WIDGET (icon_view),
                                                0, &cell_area, &aligned_area);

            item->box[info->position].width = aligned_area.width;
            item->box[info->position].height = aligned_area.height;
          }
        else
          {
            item->box[info->position].width = req.width;
            item->box[info->position].height = req.height;
          }
      }

      if (priv->orientation == GTK_ORIENTATION_HORIZONTAL)
        {
          item->area.width += item->box[info->position].width + (info->position > 0 ? priv->spacing : 0);
          item->area.height = MAX (item->area.height, item->box[info->position].height);
        }
      else
        {
          item->area.width = MAX (item->area.width, item->box[info->position].width);
          item->area.height += item->box[info->position].height + (info->position > 0 ? priv->spacing : 0);
        }
    }
}



static void
xfce_icon_view_calculate_item_size2 (XfceIconView *icon_view,
                                     XfceIconViewItem *item,
                                     gint *max_width,
                                     gint *max_height)
{
  XfceIconViewPrivate *priv = get_instance_private (icon_view);
  XfceIconViewCellInfo *info;
  GdkRectangle *box;
  GdkRectangle cell_area;
  gboolean rtl;
  GList *lp;
  gint spacing;
  gint i, k;
  gfloat cell_xalign, cell_yalign;
  gint cell_xpad, cell_ypad;

  rtl = (gtk_widget_get_direction (GTK_WIDGET (icon_view)) == GTK_TEXT_DIR_RTL);

  spacing = priv->spacing;

  if (G_LIKELY (priv->layout_mode == XFCE_ICON_VIEW_LAYOUT_ROWS))
    {
      item->area.height = 0;
      for (i = 0; i < priv->n_cells; ++i)
        {
          if (priv->orientation == GTK_ORIENTATION_HORIZONTAL)
            item->area.height = MAX (item->area.height, max_height[i]);
          else
            item->area.height += max_height[i] + (i > 0 ? spacing : 0);
        }
    }
  else
    {
      item->area.width = 0;
      for (i = 0; i < priv->n_cells; ++i)
        {
          if (priv->orientation == GTK_ORIENTATION_HORIZONTAL)
            item->area.width += max_width[i] + (i > 0 ? spacing : 0);
          else
            item->area.width = MAX (item->area.width, max_width[i]);
        }
    }

  cell_area.x = item->area.x;
  cell_area.y = item->area.y;

  for (k = 0; k < 2; ++k)
    {
      for (lp = priv->cell_list, i = 0; lp != NULL; lp = lp->next, ++i)
        {
          info = XFCE_ICON_VIEW_CELL_INFO (lp->data);
          if (G_UNLIKELY (!gtk_cell_renderer_get_visible (info->cell) || info->pack == (k ? GTK_PACK_START : GTK_PACK_END)))
            continue;

          gtk_cell_renderer_get_alignment (info->cell, &cell_xalign, &cell_yalign);
          gtk_cell_renderer_get_padding (info->cell, &cell_xpad, &cell_ypad);

          if (priv->orientation == GTK_ORIENTATION_HORIZONTAL)
            {
              cell_area.width = item->box[info->position].width;
              cell_area.height = item->area.height;
            }
          else
            {
              cell_area.width = item->area.width;
              cell_area.height = max_height[i];
            }

          box = item->box + info->position;
          box->x = cell_area.x + cell_xalign * (cell_area.width - box->width - (2 * cell_xpad));
          box->x = MAX (box->x, 0);
          box->y = cell_area.y + cell_yalign * (cell_area.height - box->height - (2 * cell_ypad));
          box->y = MAX (box->y, 0);

          if (priv->orientation == GTK_ORIENTATION_HORIZONTAL)
            {
              item->before[info->position] = item->box[info->position].x - cell_area.x;
              item->after[info->position] = cell_area.width - item->box[info->position].width - item->before[info->position];
              cell_area.x += cell_area.width + spacing;
            }
          else
            {
              if (item->box[info->position].width > item->area.width)
                {
                  item->area.width = item->box[info->position].width;
                  cell_area.width = item->area.width;
                }
              item->before[info->position] = item->box[info->position].y - cell_area.y;
              item->after[info->position] = cell_area.height - item->box[info->position].height - item->before[info->position];
              cell_area.y += cell_area.height + spacing;
            }
        }
    }

  if (G_UNLIKELY (rtl && priv->orientation == GTK_ORIENTATION_HORIZONTAL))
    {
      for (i = 0; i < priv->n_cells; i++)
        item->box[i].x = item->area.x + item->area.width - (item->box[i].x + item->box[i].width - item->area.x);
    }
}



static void
xfce_icon_view_invalidate_sizes (XfceIconView *icon_view)
{
  XfceIconViewPrivate *priv = get_instance_private (icon_view);
  GSequenceIter *iter;

  for (iter = g_sequence_get_begin_iter (priv->items);
       !g_sequence_iter_is_end (iter);
       iter = g_sequence_iter_next (iter))
    XFCE_ICON_VIEW_ITEM (g_sequence_get (iter))->area.width = -1;

  xfce_icon_view_queue_layout (icon_view);
}



static void
xfce_icon_view_paint_item (XfceIconView *icon_view,
                           XfceIconViewItem *item,
                           cairo_t *cr,
                           gint x,
                           gint y,
                           gboolean draw_focus)
{
  XfceIconViewPrivate *priv = get_instance_private (icon_view);
  GtkCellRendererState flags = 0;
  XfceIconViewCellInfo *info;
  GtkStateFlags state;
  GdkRectangle cell_area;
  GdkRectangle aligned_area;
  GtkStyleContext *style_context;
  GList *lp;

  if (G_UNLIKELY (priv->model == NULL))
    return;

  xfce_icon_view_set_cell_data (icon_view, item);

  style_context = gtk_widget_get_style_context (GTK_WIDGET (icon_view));
  state = gtk_widget_get_state_flags (GTK_WIDGET (icon_view));

  gtk_style_context_save (style_context);
  gtk_style_context_add_class (style_context, GTK_STYLE_CLASS_CELL);

  state &= ~(GTK_STATE_FLAG_SELECTED | GTK_STATE_FLAG_PRELIGHT);

  if (G_UNLIKELY (XFCE_ICON_VIEW_FLAG_SET (icon_view, XFCE_ICON_VIEW_DRAW_KEYFOCUS)
                  && (state & GTK_STATE_FLAG_FOCUSED) && item == priv->cursor_item))
    {
      flags |= GTK_CELL_RENDERER_FOCUSED;
    }

  if (G_UNLIKELY (item->selected))
    {
      state |= GTK_STATE_FLAG_SELECTED;
      flags |= GTK_CELL_RENDERER_SELECTED;
    }

  if (G_UNLIKELY (priv->prelit_item == item))
    {
      state |= GTK_STATE_FLAG_PRELIGHT;
      flags |= GTK_CELL_RENDERER_PRELIT;
    }

  gtk_style_context_set_state (style_context, state);

  for (lp = priv->cell_list; lp != NULL; lp = lp->next)
    {
      info = XFCE_ICON_VIEW_CELL_INFO (lp->data);

      if (G_UNLIKELY (!gtk_cell_renderer_get_visible (info->cell)))
        continue;

      cairo_save (cr);

      xfce_icon_view_get_cell_area (icon_view, item, info, &cell_area);

      cell_area.x = x - item->area.x + cell_area.x;
      cell_area.y = y - item->area.y + cell_area.y;

      /* FIXME: this is bad CSS usage */
      if (info->is_text)
        {
          gtk_cell_renderer_get_aligned_area (info->cell,
                                              GTK_WIDGET (icon_view),
                                              flags,
                                              &cell_area,
                                              &aligned_area);

          gtk_render_background (style_context, cr,
                                 aligned_area.x, aligned_area.y,
                                 aligned_area.width, aligned_area.height);

          gtk_render_frame (style_context, cr,
                            aligned_area.x, aligned_area.y,
                            aligned_area.width, aligned_area.height);

          /* draw outline if focused */
          if (flags & GTK_CELL_RENDERER_FOCUSED)
            {
              gtk_render_focus (style_context, cr,
                                aligned_area.x, aligned_area.y,
                                aligned_area.width, aligned_area.height);
            }
        }

      gtk_cell_renderer_render (info->cell,
                                cr,
                                GTK_WIDGET (icon_view),
                                &cell_area, &cell_area, flags);

      cairo_restore (cr);
    }

  gtk_style_context_restore (style_context);
}



static void
xfce_icon_view_queue_draw_item (XfceIconView *icon_view,
                                XfceIconViewItem *item)
{
  XfceIconViewPrivate *priv = get_instance_private (icon_view);
  GdkRectangle rect;
  gint focus_width;

  gtk_widget_style_get (GTK_WIDGET (icon_view),
                        "focus-line-width", &focus_width,
                        NULL);

  rect.x = item->area.x - focus_width;
  rect.y = item->area.y - focus_width;
  rect.width = item->area.width + 2 * focus_width;
  rect.height = item->area.height + 2 * focus_width;

  if (priv->bin_window)
    gdk_window_invalidate_rect (priv->bin_window, &rect, TRUE);
}



static gboolean
layout_callback (gpointer user_data)
{
  XfceIconView *icon_view = XFCE_ICON_VIEW (user_data);

  xfce_icon_view_layout (icon_view);

  return FALSE;
}



static void
layout_destroy (gpointer user_data)
{
  get_instance_private (user_data)->layout_idle_id = 0;
}



static void
xfce_icon_view_queue_layout (XfceIconView *icon_view)
{
  XfceIconViewPrivate *priv = get_instance_private (icon_view);

  if (G_UNLIKELY (priv->layout_idle_id == 0))
    priv->layout_idle_id = gdk_threads_add_idle_full (G_PRIORITY_DEFAULT_IDLE,
                                                      layout_callback,
                                                      icon_view,
                                                      layout_destroy);
}



static void
xfce_icon_view_set_cursor_item (XfceIconView *icon_view,
                                XfceIconViewItem *item,
                                gint cursor_cell)
{
  XfceIconViewPrivate *priv = get_instance_private (icon_view);

  if (priv->cursor_item == item
      && (cursor_cell < 0 || cursor_cell == priv->cursor_cell))
    return;

  if (priv->cursor_item != NULL)
    xfce_icon_view_queue_draw_item (icon_view, priv->cursor_item);

  priv->cursor_item = item;
  if (cursor_cell >= 0)
    priv->cursor_cell = cursor_cell;

  xfce_icon_view_queue_draw_item (icon_view, item);
}



static XfceIconViewItem *
xfce_icon_view_get_item_at_coords (XfceIconView *icon_view,
                                   gint x,
                                   gint y,
                                   gboolean only_in_cell,
                                   XfceIconViewCellInfo **cell_at_pos)
{
  XfceIconViewPrivate *priv = get_instance_private (icon_view);
  XfceIconViewCellInfo *info;
  XfceIconViewItem *item;
  GdkRectangle box;
  GSequenceIter *iter;
  const GList *lp;

  for (iter = g_sequence_get_begin_iter (priv->items);
       !g_sequence_iter_is_end (iter);
       iter = g_sequence_iter_next (iter))
    {
      item = g_sequence_get (iter);
      if (x >= item->area.x - priv->row_spacing / 2
          && x <= item->area.x + item->area.width + priv->row_spacing / 2
          && y >= item->area.y - priv->column_spacing / 2
          && y <= item->area.y + item->area.height + priv->column_spacing / 2)
        {
          if (only_in_cell || cell_at_pos)
            {
              xfce_icon_view_set_cell_data (icon_view, item);
              for (lp = priv->cell_list; lp != NULL; lp = lp->next)
                {
                  /* check if the cell is visible */
                  info = (XfceIconViewCellInfo *) lp->data;
                  if (!gtk_cell_renderer_get_visible (info->cell))
                    continue;

                  box = item->box[info->position];
                  if (x >= box.x && x <= box.x + box.width
                      && y >= box.y && y <= box.y + box.height)
                    {
                      if (cell_at_pos != NULL)
                        *cell_at_pos = info;

                      return item;
                    }
                }

              if (only_in_cell)
                return NULL;

              if (cell_at_pos != NULL)
                *cell_at_pos = NULL;
            }

          return item;
        }
    }

  return NULL;
}



static void
xfce_icon_view_select_item (XfceIconView *icon_view,
                            XfceIconViewItem *item)
{
  XfceIconViewPrivate *priv = get_instance_private (icon_view);

  if (item->selected || priv->selection_mode == GTK_SELECTION_NONE)
    return;
  else if (priv->selection_mode != GTK_SELECTION_MULTIPLE)
    xfce_icon_view_unselect_all_internal (icon_view);

  item->selected = TRUE;

  xfce_icon_view_queue_draw_item (icon_view, item);

  g_signal_emit (icon_view, icon_view_signals[SELECTION_CHANGED], 0);
}



static void
xfce_icon_view_unselect_item (XfceIconView *icon_view,
                              XfceIconViewItem *item)
{
  XfceIconViewPrivate *priv = get_instance_private (icon_view);

  if (!item->selected)
    return;

  if (priv->selection_mode == GTK_SELECTION_NONE
      || priv->selection_mode == GTK_SELECTION_BROWSE)
    return;

  item->selected = FALSE;

  g_signal_emit (G_OBJECT (icon_view), icon_view_signals[SELECTION_CHANGED], 0);

  xfce_icon_view_queue_draw_item (icon_view, item);
}



static void
xfce_icon_view_row_changed (GtkTreeModel *model,
                            GtkTreePath *path,
                            GtkTreeIter *iter,
                            XfceIconView *icon_view)
{
  XfceIconViewPrivate *priv = get_instance_private (icon_view);
  XfceIconViewItem *item;
  GSequenceIter *item_iter;

  /* ignore changes in branches */
  if (gtk_tree_path_get_depth (path) > 1)
    return;

  item_iter = g_sequence_get_iter_at_pos (priv->items, gtk_tree_path_get_indices (path)[0]);

  if (g_sequence_iter_is_end (item_iter))
    return;

  item = g_sequence_get (item_iter);

  /* stop editing this item */
  if (G_UNLIKELY (item == priv->edited_item))
    xfce_icon_view_stop_editing (icon_view, TRUE);

  /* emit "selection-changed" if the item is selected */
  if (G_UNLIKELY (item->selected))
    g_signal_emit (icon_view, icon_view_signals[SELECTION_CHANGED], 0);

  /* recalculate layout (a value of -1 for width
   * indicates that the item needs to be layouted).
   */
  item->area.width = -1;
  xfce_icon_view_queue_layout (icon_view);
}



static void
xfce_icon_view_row_inserted (GtkTreeModel *model,
                             GtkTreePath *path,
                             GtkTreeIter *iter,
                             XfceIconView *icon_view)
{
  XfceIconViewPrivate *priv = get_instance_private (icon_view);
  XfceIconViewItem *item;
  GSequenceIter *item_iter;
  gint idx;

  /* ignore changes in branches */
  if (gtk_tree_path_get_depth (path) > 1)
    return;

  idx = gtk_tree_path_get_indices (path)[0];
  item_iter = g_sequence_get_iter_at_pos (priv->items, idx);

  /* allocate the new item */
  item = g_slice_new0 (XfceIconViewItem);
  item->iter = *iter;
  item->area.width = -1;

  if (g_sequence_iter_is_end (item_iter))
    item_iter = g_sequence_append (priv->items, item);
  else
    item_iter = g_sequence_insert_before (item_iter, item);

  /* keep a link to it's own iter to speedup index lookup */
  item->item_iter = item_iter;

  /* recalculate the layout */
  xfce_icon_view_queue_layout (icon_view);
}



static void
xfce_icon_view_row_deleted (GtkTreeModel *model,
                            GtkTreePath *path,
                            XfceIconView *icon_view)
{
  XfceIconViewPrivate *priv = get_instance_private (icon_view);
  XfceIconViewItem *item;
  gboolean changed = FALSE;
  GSequenceIter *iter, *next, *prev;

  /* ignore changes in branches */
  if (gtk_tree_path_get_depth (path) > 1)
    return;

  /* determine the position and the item for the path */
  iter = g_sequence_get_iter_at_pos (priv->items, gtk_tree_path_get_indices (path)[0]);
  if (g_sequence_iter_is_end (iter))
    return;

  item = g_sequence_get (iter);

  if (G_UNLIKELY (item == priv->edited_item))
    xfce_icon_view_stop_editing (icon_view, TRUE);

  next = g_sequence_iter_next (iter);
  prev = g_sequence_iter_prev (iter);

  /* use the next item (if any) as anchor, else use prev, otherwise reset anchor */
  if (G_UNLIKELY (item == priv->anchor_item))
    priv->anchor_item = (!g_sequence_iter_is_end (next)) ? g_sequence_get (next)
                                                         : ((prev != iter) ? g_sequence_get (prev)
                                                                           : NULL);

  /* use the next item (if any) as cursor, else use prev, otherwise reset cursor */
  if (G_UNLIKELY (item == priv->cursor_item))
    priv->cursor_item = (!g_sequence_iter_is_end (next)) ? g_sequence_get (next)
                                                         : ((prev != iter) ? g_sequence_get (prev)
                                                                           : NULL);

  if (G_UNLIKELY (item == priv->prelit_item))
    {
      /* reset the prelit item */
      priv->prelit_item = NULL;

      /* cancel any pending single click timer */
      if (G_UNLIKELY (priv->single_click_timeout_id != 0))
        g_source_remove (priv->single_click_timeout_id);

      /* in single click mode, we also reset the cursor when realized */
      if (G_UNLIKELY (priv->single_click && gtk_widget_get_realized (GTK_WIDGET (icon_view))))
        gdk_window_set_cursor (priv->bin_window, NULL);
    }

  /* check if the selection changed */
  if (G_UNLIKELY (item->selected))
    changed = TRUE;

  /* release the item resources */
  g_free (item->box);

  /* drop the item from the list */
  g_sequence_remove (iter);

  /* release the item */
  g_slice_free (XfceIconViewItem, item);

  /* recalculate the layout */
  xfce_icon_view_queue_layout (icon_view);

  /* if we removed a previous selected item, we need
   * to tell others that we have a new selection.
   */
  if (G_UNLIKELY (changed))
    g_signal_emit (G_OBJECT (icon_view), icon_view_signals[SELECTION_CHANGED], 0);
}



static void
xfce_icon_view_rows_reordered (GtkTreeModel *model,
                               GtkTreePath *parent,
                               GtkTreeIter *iter,
                               gint *new_order, /*  new_order [newpos] = oldpos */
                               XfceIconView *icon_view)
{
  XfceIconViewPrivate *priv = get_instance_private (icon_view);
  gint i;
  GSequence *new_sequence;
  GSequenceIter *old_iter;
  GSequenceIter *new_iter;
  gint old_pos;
  XfceIconViewItem *item;

  /* ignore changes in branches */
  if (iter != NULL)
    return;

  /* cancel any editing attempt */
  xfce_icon_view_stop_editing (icon_view, TRUE);

  /* create a new sequence and move the data over */
  new_sequence = g_sequence_new (NULL);
  for (i = 0; i < g_sequence_get_length (priv->items); i++)
    {
      old_pos = new_order[i];
      old_iter = g_sequence_get_iter_at_pos (priv->items, old_pos);
      item = g_sequence_get (old_iter);
      new_iter = g_sequence_append (new_sequence, item);
      item->item_iter = new_iter;
    }

  g_sequence_free (priv->items);
  priv->items = new_sequence;
  xfce_icon_view_queue_layout (icon_view);
}



static void
xfce_icon_view_add_move_binding (GtkBindingSet *binding_set,
                                 guint keyval,
                                 guint modmask,
                                 GtkMovementStep step,
                                 gint count)
{
  gtk_binding_entry_add_signal (binding_set, keyval, modmask, "move-cursor", 2, G_TYPE_ENUM, step, G_TYPE_INT, count);

  /* skip shift+n and shift+p because this blocks type-ahead search.
   * see https://bugzilla.xfce.org/show_bug.cgi?id=4633
   */
  if (G_LIKELY (keyval != GDK_KEY_p && keyval != GDK_KEY_n))
    gtk_binding_entry_add_signal (binding_set, keyval, GDK_SHIFT_MASK, "move-cursor", 2, G_TYPE_ENUM, step, G_TYPE_INT, count);

  if ((modmask & GDK_CONTROL_MASK) != GDK_CONTROL_MASK)
    {
      gtk_binding_entry_add_signal (binding_set, keyval, GDK_CONTROL_MASK | GDK_SHIFT_MASK, "move-cursor", 2, G_TYPE_ENUM, step, G_TYPE_INT, count);
      gtk_binding_entry_add_signal (binding_set, keyval, GDK_CONTROL_MASK, "move-cursor", 2, G_TYPE_ENUM, step, G_TYPE_INT, count);
    }
}



static gboolean
xfce_icon_view_real_move_cursor (XfceIconView *icon_view,
                                 GtkMovementStep step,
                                 gint count)
{
  XfceIconViewPrivate *priv = get_instance_private (icon_view);
  GdkModifierType state;

  g_return_val_if_fail (XFCE_ICON_VIEW (icon_view), FALSE);
  g_return_val_if_fail (step == GTK_MOVEMENT_LOGICAL_POSITIONS
                          || step == GTK_MOVEMENT_VISUAL_POSITIONS
                          || step == GTK_MOVEMENT_DISPLAY_LINES
                          || step == GTK_MOVEMENT_PAGES
                          || step == GTK_MOVEMENT_BUFFER_ENDS,
                        FALSE);

  if (!gtk_widget_has_focus (GTK_WIDGET (icon_view)))
    return FALSE;

  xfce_icon_view_stop_editing (icon_view, FALSE);
  XFCE_ICON_VIEW_SET_FLAG (icon_view, XFCE_ICON_VIEW_DRAW_KEYFOCUS);
  gtk_widget_grab_focus (GTK_WIDGET (icon_view));

  if (gtk_get_current_event_state (&state))
    {
      if ((state & GDK_CONTROL_MASK) == GDK_CONTROL_MASK)
        priv->ctrl_pressed = TRUE;
      if ((state & GDK_SHIFT_MASK) == GDK_SHIFT_MASK)
        priv->shift_pressed = TRUE;
    }
  /* else we assume not pressed */

  switch (step)
    {
    case GTK_MOVEMENT_LOGICAL_POSITIONS:
    case GTK_MOVEMENT_VISUAL_POSITIONS:
      xfce_icon_view_move_cursor_left_right (icon_view, count);
      break;
    case GTK_MOVEMENT_DISPLAY_LINES:
      xfce_icon_view_move_cursor_up_down (icon_view, count);
      break;
    case GTK_MOVEMENT_PAGES:
      xfce_icon_view_move_cursor_page_up_down (icon_view, count);
      break;
    case GTK_MOVEMENT_BUFFER_ENDS:
      xfce_icon_view_move_cursor_start_end (icon_view, count);
      break;
    default:
      g_assert_not_reached ();
    }

  priv->ctrl_pressed = FALSE;
  priv->shift_pressed = FALSE;

  return TRUE;
}



static gint
find_cell (XfceIconView *icon_view,
           XfceIconViewItem *item,
           gint cell,
           GtkOrientation orientation,
           gint step,
           gint *count)
{
  XfceIconViewPrivate *priv = get_instance_private (icon_view);
  gint n_focusable;
  gint *focusable;
  gint first_text;
  gint current;
  gint i, k;
  GList *l;
  GtkCellRendererMode mode;

  if (priv->orientation != orientation)
    return cell;

  xfce_icon_view_set_cell_data (icon_view, item);

  focusable = g_new0 (gint, priv->n_cells);
  n_focusable = 0;

  first_text = 0;
  current = 0;
  for (k = 0; k < 2; k++)
    for (l = priv->cell_list, i = 0; l; l = l->next, i++)
      {
        XfceIconViewCellInfo *info = (XfceIconViewCellInfo *) l->data;

        if (info->pack == (k ? GTK_PACK_START : GTK_PACK_END))
          continue;

        if (!gtk_cell_renderer_get_visible (info->cell))
          continue;

        if (GTK_IS_CELL_RENDERER_TEXT (info->cell))
          first_text = i;

        g_object_get (info->cell, "mode", &mode, NULL);
        if (mode != GTK_CELL_RENDERER_MODE_INERT)
          {
            if (cell == i)
              current = n_focusable;

            focusable[n_focusable] = i;

            n_focusable++;
          }
      }

  if (n_focusable == 0)
    focusable[n_focusable++] = first_text;

  if (cell < 0)
    {
      current = step > 0 ? 0 : n_focusable - 1;
    }

  if (current + *count < 0)
    {
      cell = -1;
      *count = current + *count;
    }
  else if (current + *count > n_focusable - 1)
    {
      cell = -1;
      *count = current + *count - (n_focusable - 1);
    }
  else
    {
      cell = focusable[current + *count];
      *count = 0;
    }

  g_free (focusable);

  return cell;
}



static XfceIconViewItem *
find_item_page_up_down (XfceIconView *icon_view,
                        XfceIconViewItem *current,
                        gint count)
{
  XfceIconViewPrivate *priv = get_instance_private (icon_view);
  GSequenceIter *iter;
  GSequenceIter *column_match = NULL;
  gint col = current->col;
  gint y = current->area.y + count * gtk_adjustment_get_page_size (priv->vadjustment);

  if (count > 0)
    {
      GSequenceIter *next;

      for (iter = g_sequence_get_begin_iter (priv->items);
           !g_sequence_iter_is_end (iter);
           iter = g_sequence_iter_next (iter))
        {
          next = g_sequence_iter_next (iter);

          /* if we found an item which matches the correct column */
          if (!g_sequence_iter_is_end (next) && XFCE_ICON_VIEW_ITEM (g_sequence_get (next))->col == col)
            {
              /* found an item which is far enough down .. lets use our previous column match */
              if (XFCE_ICON_VIEW_ITEM (g_sequence_get (next))->area.y > y)
                break;
              column_match = next;
            }
        }
    }
  else
    {
      GSequenceIter *prev;

      for (iter = g_sequence_iter_prev (current->item_iter);; iter = g_sequence_iter_prev (iter))
        {
          prev = g_sequence_iter_prev (iter);

          /* we reached the first element of the sequence, let's just use our latest column_match */
          if (prev == iter)
            break;

          /* if we found an item which matches the correct column */
          if (prev != iter && XFCE_ICON_VIEW_ITEM (g_sequence_get (prev))->col == col)
            {
              /* found an item which is far enough up .. lets use our previous column match */
              if (XFCE_ICON_VIEW_ITEM (g_sequence_get (prev))->area.y < y)
                break;
              column_match = prev;
            }
        }
    }

  if (column_match != NULL && !g_sequence_iter_is_end (column_match))
    return g_sequence_get (column_match);
  return NULL;
}



static gboolean
xfce_icon_view_select_all_between (XfceIconView *icon_view,
                                   XfceIconViewItem *anchor,
                                   XfceIconViewItem *cursor)
{
  XfceIconViewPrivate *priv = get_instance_private (icon_view);
  GSequenceIter *iter;
  XfceIconViewItem *item, *last = NULL;
  gboolean dirty = FALSE;

  for (iter = g_sequence_get_begin_iter (priv->items);
       !g_sequence_iter_is_end (iter);
       iter = g_sequence_iter_next (iter))
    {
      item = g_sequence_get (iter);

      if (item == anchor)
        {
          last = cursor;
          break;
        }
      else if (item == cursor)
        {
          last = anchor;
          break;
        }
    }

  for (; !g_sequence_iter_is_end (iter); iter = g_sequence_iter_next (iter))
    {
      item = g_sequence_get (iter);

      if (!item->selected)
        dirty = TRUE;

      item->selected = TRUE;

      xfce_icon_view_queue_draw_item (icon_view, item);

      if (item == last)
        break;
    }

  return dirty;
}



static void
xfce_icon_view_move_cursor_up_down (XfceIconView *icon_view,
                                    gint count)
{
  XfceIconViewPrivate *priv = get_instance_private (icon_view);
  XfceIconViewItem *item = NULL;
  gboolean dirty = FALSE;
  gint cell = -1;
  gint step;
  GtkDirectionType direction;
  GtkWidget *toplevel;
  GSequenceIter *iter;
  gboolean found = FALSE;

  if (!gtk_widget_has_focus (GTK_WIDGET (icon_view)))
    return;

  direction = count < 0 ? GTK_DIR_UP : GTK_DIR_DOWN;

  if (!priv->cursor_item)
    {
      if (count > 0)
        iter = g_sequence_get_begin_iter (priv->items);
      else
        iter = g_sequence_iter_prev (g_sequence_get_end_iter (priv->items));

      if (!g_sequence_iter_is_end (iter))

        item = g_sequence_get (iter);
    }
  else
    {
      item = priv->cursor_item;
      cell = priv->cursor_cell;
      step = count > 0 ? 1 : -1;
      while (item)
        {
          cell = find_cell (icon_view, item, cell,
                            GTK_ORIENTATION_VERTICAL,
                            step, &count);
          if (count == 0)
            break;

          /* determine the list position for the item */
          iter = item->item_iter;
          if (g_sequence_iter_is_end (iter))
            break;

          if (G_LIKELY (priv->layout_mode == XFCE_ICON_VIEW_LAYOUT_ROWS))
            {
              /* determine the item in the next/prev row */
              if (step > 0)
                {
                  for (iter = g_sequence_iter_next (iter);
                       !g_sequence_iter_is_end (iter);
                       iter = g_sequence_iter_next (iter))
                    if (XFCE_ICON_VIEW_ITEM (g_sequence_get (iter))->row == item->row + step
                        && XFCE_ICON_VIEW_ITEM (g_sequence_get (iter))->col == item->col)
                      break;
                }
              else
                {
                  while (!g_sequence_iter_is_begin (iter))
                    {
                      if (XFCE_ICON_VIEW_ITEM (g_sequence_get (iter))->row == 0
                          && XFCE_ICON_VIEW_ITEM (g_sequence_get (iter))->col == item->col)
                        {
                          found = TRUE;
                          break;
                        }

                      if (XFCE_ICON_VIEW_ITEM (g_sequence_get (iter))->row == item->row + step
                          && XFCE_ICON_VIEW_ITEM (g_sequence_get (iter))->col == item->col)
                        break;

                      iter = g_sequence_iter_prev (iter);
                    }
                }
            }
          else
            {
              iter = (step > 0) ? g_sequence_iter_next (iter) : g_sequence_iter_prev (iter);
            }

          /* determine the item for the list position (if any) */
          item = (!g_sequence_iter_is_end (iter)) ? g_sequence_get (iter) : NULL;

          /* stop if we reached the beginning or found the correct item */
          if (found || g_sequence_iter_is_begin (iter))
            break;

          count = count - step;
        }
    }

  if (!item)
    {
      if (!gtk_widget_keynav_failed (GTK_WIDGET (icon_view), direction))
        {
          toplevel = gtk_widget_get_toplevel (GTK_WIDGET (icon_view));
          if (toplevel != NULL)
            {
              gtk_widget_child_focus (toplevel,
                                      direction == GTK_DIR_UP ? GTK_DIR_TAB_BACKWARD : GTK_DIR_TAB_FORWARD);
            }

          return;
        }

      /* check if we should select the cursor item */
      item = priv->cursor_item;
      if (!item || item->selected)
        return;
    }

  if (priv->ctrl_pressed
      || !priv->shift_pressed
      || !priv->anchor_item
      || priv->selection_mode != GTK_SELECTION_MULTIPLE)
    priv->anchor_item = item;

  xfce_icon_view_set_cursor_item (icon_view, item, cell);

  if (!priv->ctrl_pressed && priv->selection_mode != GTK_SELECTION_NONE)
    {
      dirty = xfce_icon_view_unselect_all_internal (icon_view);
      dirty = xfce_icon_view_select_all_between (icon_view,
                                                 priv->anchor_item,
                                                 item)
              || dirty;
    }

  xfce_icon_view_scroll_to_item (icon_view, item);

  if (dirty)
    g_signal_emit (icon_view, icon_view_signals[SELECTION_CHANGED], 0);
}



static void
xfce_icon_view_move_cursor_page_up_down (XfceIconView *icon_view,
                                         gint count)
{
  XfceIconViewPrivate *priv = get_instance_private (icon_view);
  XfceIconViewItem *item = NULL;
  gboolean dirty = FALSE;

  if (!gtk_widget_has_focus (GTK_WIDGET (icon_view)))
    return;

  if (!priv->cursor_item)
    {
      GSequenceIter *iter;

      if (count > 0)
        iter = g_sequence_get_begin_iter (priv->items);
      else
        iter = g_sequence_iter_prev (g_sequence_get_end_iter (priv->items));

      if (!g_sequence_iter_is_end (iter))
        item = g_sequence_get (iter);
    }
  else
    item = find_item_page_up_down (icon_view,
                                   priv->cursor_item,
                                   count);

  if (item == NULL)
    return;

  if (priv->ctrl_pressed
      || !priv->shift_pressed
      || !priv->anchor_item
      || priv->selection_mode != GTK_SELECTION_MULTIPLE)
    priv->anchor_item = item;

  xfce_icon_view_set_cursor_item (icon_view, item, -1);

  if (!priv->ctrl_pressed && priv->selection_mode != GTK_SELECTION_NONE)
    {
      dirty = xfce_icon_view_unselect_all_internal (icon_view);
      dirty = xfce_icon_view_select_all_between (icon_view,
                                                 priv->anchor_item,
                                                 item)
              || dirty;
    }

  xfce_icon_view_scroll_to_item (icon_view, item);

  if (dirty)
    g_signal_emit (icon_view, icon_view_signals[SELECTION_CHANGED], 0);
}



static void
xfce_icon_view_move_cursor_left_right (XfceIconView *icon_view,
                                       gint count)
{
  XfceIconViewPrivate *priv = get_instance_private (icon_view);
  XfceIconViewItem *item = NULL;
  gboolean dirty = FALSE;
  gint cell = -1;
  gint step;
  GtkDirectionType direction;
  GtkWidget *toplevel;
  GSequenceIter *iter;
  gboolean found = FALSE;

  if (!gtk_widget_has_focus (GTK_WIDGET (icon_view)))
    return;

  if (gtk_widget_get_direction (GTK_WIDGET (icon_view)) == GTK_TEXT_DIR_RTL)
    count *= -1;

  direction = count < 0 ? GTK_DIR_LEFT : GTK_DIR_RIGHT;

  if (!priv->cursor_item)
    {
      if (count > 0)
        iter = g_sequence_get_begin_iter (priv->items);
      else
        iter = g_sequence_iter_prev (g_sequence_get_end_iter (priv->items));

      if (!g_sequence_iter_is_end (iter))
        item = g_sequence_get (iter);
    }
  else
    {
      item = priv->cursor_item;
      cell = priv->cursor_cell;
      step = count > 0 ? 1 : -1;
      while (item)
        {
          cell = find_cell (icon_view, item, cell,
                            GTK_ORIENTATION_HORIZONTAL,
                            step, &count);
          if (count == 0)
            break;

          /* lookup the item in the list */
          iter = item->item_iter;

          if (G_LIKELY (priv->layout_mode == XFCE_ICON_VIEW_LAYOUT_ROWS))
            {
              /* determine the next/prev list item depending on step,
               * support wrapping around on the edges, as requested
               * in https://bugzilla.xfce.org/show_bug.cgi?id=1623.
               */
              iter = (step > 0) ? g_sequence_iter_next (iter) : g_sequence_iter_prev (iter);
            }
          else
            {
              /* determine the item in the next/prev row */
              if (step > 0)
                {
                  for (iter = g_sequence_iter_next (iter);
                       !g_sequence_iter_is_end (iter);
                       iter = g_sequence_iter_next (iter))
                    if (XFCE_ICON_VIEW_ITEM (g_sequence_get (iter))->col == item->col + step
                        && XFCE_ICON_VIEW_ITEM (g_sequence_get (iter))->row == item->row)
                      break;
                }
              else
                {
                  while (!g_sequence_iter_is_begin (iter))
                    {
                      if (XFCE_ICON_VIEW_ITEM (g_sequence_get (iter))->col == 0
                          && XFCE_ICON_VIEW_ITEM (g_sequence_get (iter))->row == item->row)
                        {
                          found = TRUE;
                          break;
                        }

                      if (XFCE_ICON_VIEW_ITEM (g_sequence_get (iter))->col == item->col + step
                          && XFCE_ICON_VIEW_ITEM (g_sequence_get (iter))->row == item->row)
                        break;

                      iter = g_sequence_iter_prev (iter);
                    }
                }
            }

          /* determine the item for the list position (if any) */
          item = (!g_sequence_iter_is_end (iter)) ? g_sequence_get (iter) : NULL;

          /* stop if we reached the beginning or found the correct item */
          if (found || g_sequence_iter_is_begin (iter))
            break;

          count = count - step;
        }
    }

  if (!item)
    {
      if (!gtk_widget_keynav_failed (GTK_WIDGET (icon_view), direction))
        {
          toplevel = gtk_widget_get_toplevel (GTK_WIDGET (icon_view));
          if (toplevel != NULL)
            {
              gtk_widget_child_focus (toplevel,
                                      direction == GTK_DIR_LEFT ? GTK_DIR_TAB_BACKWARD : GTK_DIR_TAB_FORWARD);
            }

          return;
        }

      /* check if we should select the cursor item */
      item = priv->cursor_item;
      if (!item || item->selected)
        return;
    }

  if (priv->ctrl_pressed
      || !priv->shift_pressed
      || !priv->anchor_item
      || priv->selection_mode != GTK_SELECTION_MULTIPLE)
    priv->anchor_item = item;

  xfce_icon_view_set_cursor_item (icon_view, item, cell);

  if (!priv->ctrl_pressed && priv->selection_mode != GTK_SELECTION_NONE)
    {
      dirty = xfce_icon_view_unselect_all_internal (icon_view);
      dirty = xfce_icon_view_select_all_between (icon_view,
                                                 priv->anchor_item,
                                                 item)
              || dirty;
    }

  xfce_icon_view_scroll_to_item (icon_view, item);

  if (dirty)
    g_signal_emit (icon_view, icon_view_signals[SELECTION_CHANGED], 0);
}



static void
xfce_icon_view_move_cursor_start_end (XfceIconView *icon_view,
                                      gint count)
{
  XfceIconViewPrivate *priv = get_instance_private (icon_view);
  XfceIconViewItem *item;
  gboolean dirty = FALSE;
  GSequenceIter *iter;

  if (!gtk_widget_has_focus (GTK_WIDGET (icon_view)))
    return;

  iter = (count < 0) ? g_sequence_get_begin_iter (priv->items)
                     : g_sequence_iter_prev (g_sequence_get_end_iter (priv->items));
  if (G_UNLIKELY (iter == NULL || g_sequence_iter_is_end (iter)))
    return;

  item = XFCE_ICON_VIEW_ITEM (g_sequence_get (iter));
  if (priv->ctrl_pressed
      || !priv->shift_pressed
      || !priv->anchor_item
      || priv->selection_mode != GTK_SELECTION_MULTIPLE)
    priv->anchor_item = item;

  xfce_icon_view_set_cursor_item (icon_view, item, -1);

  if (!priv->ctrl_pressed && priv->selection_mode != GTK_SELECTION_NONE)
    {
      dirty = xfce_icon_view_unselect_all_internal (icon_view);
      dirty = xfce_icon_view_select_all_between (icon_view,
                                                 priv->anchor_item,
                                                 item)
              || dirty;
    }

  xfce_icon_view_scroll_to_item (icon_view, item);

  if (G_UNLIKELY (dirty))
    g_signal_emit (icon_view, icon_view_signals[SELECTION_CHANGED], 0);
}



/* Get the actual size needed by an item (as opposed to the size
 * allocated based on the largest item in the same row/column).
 */
static void
xfce_icon_view_get_item_needed_size (XfceIconView *icon_view,
                                     XfceIconViewItem *item,
                                     gint *width,
                                     gint *height)
{
  XfceIconViewPrivate *priv = get_instance_private (icon_view);
  GList *lp;
  XfceIconViewCellInfo *info;

  *width = 0;
  *height = 0;

  for (lp = priv->cell_list; lp != NULL; lp = lp->next)
    {
      info = XFCE_ICON_VIEW_CELL_INFO (lp->data);
      if (G_UNLIKELY (!gtk_cell_renderer_get_visible (info->cell)))
        continue;

      if (priv->orientation == GTK_ORIENTATION_HORIZONTAL)
        {
          *width += item->box[info->position].width
                    + (info->position > 0 ? priv->spacing : 0);
          *height = MAX (*height, item->box[info->position].height);
        }
      else
        {
          *width = MAX (*width, item->box[info->position].width);
          *height += item->box[info->position].height
                     + (info->position > 0 ? priv->spacing : 0);
        }
    }
}



static void
xfce_icon_view_scroll_to_item (XfceIconView *icon_view,
                               XfceIconViewItem *item)
{
  XfceIconViewPrivate *priv = get_instance_private (icon_view);
  gint x, y;
  gint focus_width;
  gint item_width, item_height;
  GtkAllocation allocation;
  GtkTreePath *path;

  /* Delay scrolling if either not realized or pending layout() */
  if (!gtk_widget_get_realized (GTK_WIDGET (icon_view)) || priv->layout_idle_id != 0)
    {
      /* release the previous scroll_to_path reference */
      if (G_UNLIKELY (priv->scroll_to_path != NULL))
        gtk_tree_row_reference_free (priv->scroll_to_path);

      /* remember a reference for the new path and settings */

      path = gtk_tree_path_new_from_indices (g_sequence_iter_get_position (item->item_iter), -1);
      priv->scroll_to_path = gtk_tree_row_reference_new_proxy (G_OBJECT (icon_view), priv->model, path);
      gtk_tree_path_free (path);

      priv->scroll_to_use_align = FALSE;

      return;
    }

  gtk_widget_style_get (GTK_WIDGET (icon_view),
                        "focus-line-width", &focus_width,
                        NULL);

  gdk_window_get_position (priv->bin_window, &x, &y);
  xfce_icon_view_get_item_needed_size (icon_view, item, &item_width, &item_height);
  gtk_widget_get_allocation (GTK_WIDGET (icon_view), &allocation);

  /*
   * If an item reaches beyond the edges of the view, we scroll just enough
   * to make as much of it visible as possible.  This avoids interfering
   * with double-click (since the second click will not scroll differently),
   * prevents narrow items in wide columns from being scrolled out of view
   * when selected, and ensures that items will be brought into view when
   * selected even if it was done by a keystroke instead of a mouse click.
   * See bugs 1683 and 6014 for some problems seen in the past.
   */

  if (y + item->area.y - focus_width < 0)
    {
      gtk_adjustment_set_value (priv->vadjustment,
                                gtk_adjustment_get_value (priv->vadjustment) + y + item->area.y - focus_width);
    }
  else if (y + item->area.y + item_height + focus_width > allocation.height
           && y + item->area.y - focus_width > 0)
    {
      gtk_adjustment_set_value (priv->vadjustment,
                                gtk_adjustment_get_value (priv->vadjustment)
                                  + MIN (y + item->area.y - focus_width,
                                         y + item->area.y + item_height + focus_width - allocation.height));
    }

  if (x + item->area.x - focus_width < 0)
    {
      gtk_adjustment_set_value (priv->hadjustment,
                                gtk_adjustment_get_value (priv->hadjustment) + x + item->area.x - focus_width);
    }
  else if (x + item->area.x + item_width + focus_width > allocation.width
           && x + item->area.x - focus_width > 0)
    {
      gtk_adjustment_set_value (priv->hadjustment,
                                gtk_adjustment_get_value (priv->hadjustment)
                                  + MIN (x + item->area.x - focus_width,
                                         x + item->area.x + item_width + focus_width - allocation.width));
    }
}



static XfceIconViewCellInfo *
xfce_icon_view_get_cell_info (XfceIconView *icon_view,
                              GtkCellRenderer *renderer)
{
  XfceIconViewPrivate *priv = get_instance_private (icon_view);
  GList *lp;

  for (lp = priv->cell_list; lp != NULL; lp = lp->next)
    if (XFCE_ICON_VIEW_CELL_INFO (lp->data)->cell == renderer)
      return lp->data;

  return NULL;
}



static void
xfce_icon_view_set_cell_data (XfceIconView *icon_view,
                              XfceIconViewItem *item)
{
  XfceIconViewPrivate *priv = get_instance_private (icon_view);
  XfceIconViewCellInfo *info;
  GtkTreePath *path;
  GtkTreeIter iter;
  GValue value = G_VALUE_INIT;
  GSList *slp;
  GList *lp;

  if (G_UNLIKELY (!XFCE_ICON_VIEW_FLAG_SET (icon_view, XFCE_ICON_VIEW_ITERS_PERSIST)))
    {
      path = gtk_tree_path_new_from_indices (g_sequence_iter_get_position (item->item_iter), -1);
      gtk_tree_model_get_iter (priv->model, &iter, path);
      gtk_tree_path_free (path);
    }
  else
    {
      iter = item->iter;
    }

  for (lp = priv->cell_list; lp != NULL; lp = lp->next)
    {
      info = XFCE_ICON_VIEW_CELL_INFO (lp->data);

      for (slp = info->attributes; slp != NULL && slp->next != NULL; slp = slp->next->next)
        {
          gtk_tree_model_get_value (priv->model, &iter, GPOINTER_TO_INT (slp->next->data), &value);
          g_object_set_property (G_OBJECT (info->cell), slp->data, &value);
          g_value_unset (&value);
        }

      if (G_UNLIKELY (info->func != NULL))
        (*info->func) (GTK_CELL_LAYOUT (icon_view), info->cell, priv->model, &iter, info->func_data);
    }
}



static void
free_cell_attributes (XfceIconViewCellInfo *info)
{
  GSList *lp;

  for (lp = info->attributes; lp != NULL && lp->next != NULL; lp = lp->next->next)
    g_free (lp->data);
  g_slist_free (info->attributes);
  info->attributes = NULL;
}



static void
free_cell_info (XfceIconViewCellInfo *info)
{
  if (G_UNLIKELY (info->destroy != NULL))
    (*info->destroy) (info->func_data);

  free_cell_attributes (info);
  g_object_unref (G_OBJECT (info->cell));
  g_slice_free (XfceIconViewCellInfo, info);
}



static void
xfce_icon_view_cell_layout_pack_start (GtkCellLayout *layout,
                                       GtkCellRenderer *renderer,
                                       gboolean expand)
{
  XfceIconView *icon_view = XFCE_ICON_VIEW (layout);
  XfceIconViewPrivate *priv = get_instance_private (icon_view);
  XfceIconViewCellInfo *info;

  g_return_if_fail (GTK_IS_CELL_RENDERER (renderer));
  g_return_if_fail (xfce_icon_view_get_cell_info (icon_view, renderer) == NULL);

  g_object_ref_sink (renderer);

  info = g_slice_new0 (XfceIconViewCellInfo);
  info->cell = renderer;
  info->expand = !!expand;
  info->pack = GTK_PACK_START;
  info->position = priv->n_cells;
  info->is_text = GTK_IS_CELL_RENDERER_TEXT (renderer);

  priv->cell_list = g_list_append (priv->cell_list, info);
  priv->n_cells++;

  xfce_icon_view_invalidate_sizes (icon_view);
}



static void
xfce_icon_view_cell_layout_pack_end (GtkCellLayout *layout,
                                     GtkCellRenderer *renderer,
                                     gboolean expand)
{
  XfceIconView *icon_view = XFCE_ICON_VIEW (layout);
  XfceIconViewPrivate *priv = get_instance_private (icon_view);
  XfceIconViewCellInfo *info;

  g_return_if_fail (GTK_IS_CELL_RENDERER (renderer));
  g_return_if_fail (xfce_icon_view_get_cell_info (icon_view, renderer) == NULL);

  g_object_ref_sink (renderer);

  info = g_slice_new0 (XfceIconViewCellInfo);
  info->cell = renderer;
  info->expand = !!expand;
  info->pack = GTK_PACK_END;
  info->position = priv->n_cells;
  info->is_text = GTK_IS_CELL_RENDERER_TEXT (renderer);

  priv->cell_list = g_list_append (priv->cell_list, info);
  priv->n_cells++;

  xfce_icon_view_invalidate_sizes (icon_view);
}



static void
xfce_icon_view_cell_layout_add_attribute (GtkCellLayout *layout,
                                          GtkCellRenderer *renderer,
                                          const gchar *attribute,
                                          gint column)
{
  XfceIconViewCellInfo *info;

  info = xfce_icon_view_get_cell_info (XFCE_ICON_VIEW (layout), renderer);
  if (G_LIKELY (info != NULL))
    {
      info->attributes = g_slist_prepend (info->attributes, GINT_TO_POINTER (column));
      info->attributes = g_slist_prepend (info->attributes, g_strdup (attribute));

      xfce_icon_view_invalidate_sizes (XFCE_ICON_VIEW (layout));
    }
}



static void
xfce_icon_view_cell_layout_clear (GtkCellLayout *layout)
{
  XfceIconView *icon_view = XFCE_ICON_VIEW (layout);
  XfceIconViewPrivate *priv = get_instance_private (icon_view);

  g_list_free_full (priv->cell_list, (GDestroyNotify) free_cell_info);
  priv->cell_list = NULL;
  priv->n_cells = 0;

  xfce_icon_view_invalidate_sizes (icon_view);
}



static void
xfce_icon_view_cell_layout_set_cell_data_func (GtkCellLayout *layout,
                                               GtkCellRenderer *cell,
                                               GtkCellLayoutDataFunc func,
                                               gpointer func_data,
                                               GDestroyNotify destroy)
{
  XfceIconViewCellInfo *info;
  GDestroyNotify notify;

  info = xfce_icon_view_get_cell_info (XFCE_ICON_VIEW (layout), cell);
  if (G_LIKELY (info != NULL))
    {
      if (G_UNLIKELY (info->destroy != NULL))
        {
          notify = info->destroy;
          info->destroy = NULL;
          (*notify) (info->func_data);
        }

      info->func = func;
      info->func_data = func_data;
      info->destroy = destroy;

      xfce_icon_view_invalidate_sizes (XFCE_ICON_VIEW (layout));
    }
}



static void
xfce_icon_view_cell_layout_clear_attributes (GtkCellLayout *layout,
                                             GtkCellRenderer *renderer)
{
  XfceIconViewCellInfo *info;

  info = xfce_icon_view_get_cell_info (XFCE_ICON_VIEW (layout), renderer);
  if (G_LIKELY (info != NULL))
    {
      free_cell_attributes (info);

      xfce_icon_view_invalidate_sizes (XFCE_ICON_VIEW (layout));
    }
}



static void
xfce_icon_view_cell_layout_reorder (GtkCellLayout *layout,
                                    GtkCellRenderer *cell,
                                    gint position)
{
  XfceIconViewCellInfo *info;
  XfceIconView *icon_view = XFCE_ICON_VIEW (layout);
  XfceIconViewPrivate *priv = get_instance_private (icon_view);
  GList *lp;
  gint n;

  info = xfce_icon_view_get_cell_info (icon_view, cell);
  if (G_LIKELY (info != NULL))
    {
      lp = g_list_find (priv->cell_list, info);

      priv->cell_list = g_list_remove_link (priv->cell_list, lp);
      priv->cell_list = g_list_insert (priv->cell_list, info, position);

      for (lp = priv->cell_list, n = 0; lp != NULL; lp = lp->next, ++n)
        XFCE_ICON_VIEW_CELL_INFO (lp->data)->position = n;

      xfce_icon_view_invalidate_sizes (icon_view);
    }
}



/**
 * xfce_icon_view_new:
 *
 * Creates a new #XfceIconView widget
 *
 * Returns: (transfer full): A newly created #XfceIconView widget
 *
 * Since: 4.21.0
 **/
GtkWidget *
xfce_icon_view_new (void)
{
  return g_object_new (XFCE_TYPE_ICON_VIEW, NULL);
}



/**
 * xfce_icon_view_new_with_model:
 * @model: The model.
 *
 * Creates a new #XfceIconView widget with the model @model.
 *
 * Returns: (transfer full): A newly created #XfceIconView widget.
 *
 * Since: 4.21.0
 **/
GtkWidget *
xfce_icon_view_new_with_model (GtkTreeModel *model)
{
  g_return_val_if_fail (model == NULL || GTK_IS_TREE_MODEL (model), NULL);

  return g_object_new (XFCE_TYPE_ICON_VIEW,
                       "model", model,
                       NULL);
}



/**
 * xfce_icon_view_widget_to_icon_coords:
 * @icon_view: a #XfceIconView.
 * @wx: widget x coordinate.
 * @wy: widget y coordinate.
 * @ix: return location for icon x coordinate or %NULL.
 * @iy: return location for icon y coordinate or %NULL.
 *
 * Converts widget coordinates to coordinates for the icon window
 * (the full scrollable area of the icon view).
 *
 * Since: 4.21.0
 **/
void
xfce_icon_view_widget_to_icon_coords (XfceIconView *icon_view,
                                      gint wx,
                                      gint wy,
                                      gint *ix,
                                      gint *iy)
{
  XfceIconViewPrivate *priv = get_instance_private (icon_view);

  g_return_if_fail (XFCE_IS_ICON_VIEW (icon_view));

  if (G_LIKELY (ix != NULL))
    *ix = wx + gtk_adjustment_get_value (priv->hadjustment);
  if (G_LIKELY (iy != NULL))
    *iy = wy + gtk_adjustment_get_value (priv->vadjustment);
}



/**
 * xfce_icon_view_icon_to_widget_coords:
 * @icon_view : a #XfceIconView.
 * @ix        : icon x coordinate.
 * @iy        : icon y coordinate.
 * @wx        : return location for widget x coordinate or %NULL.
 * @wy        : return location for widget y coordinate or %NULL.
 *
 * Converts icon view coordinates (coordinates in full scrollable
 * area of the icon view) to widget coordinates.
 *
 * Since: 4.21.0
 **/
void
xfce_icon_view_icon_to_widget_coords (XfceIconView *icon_view,
                                      gint ix,
                                      gint iy,
                                      gint *wx,
                                      gint *wy)
{
  XfceIconViewPrivate *priv = get_instance_private (icon_view);

  g_return_if_fail (XFCE_IS_ICON_VIEW (icon_view));

  if (G_LIKELY (wx != NULL))
    *wx = ix - gtk_adjustment_get_value (priv->hadjustment);
  if (G_LIKELY (wy != NULL))
    *wy = iy - gtk_adjustment_get_value (priv->vadjustment);
}



/**
 * xfce_icon_view_get_path_at_pos:
 * @icon_view : A #XfceIconView.
 * @x         : The x position to be identified
 * @y         : The y position to be identified
 *
 * Finds the path at the point (@x, @y), relative to widget coordinates.
 * See xfce_icon_view_get_item_at_pos(), if you are also interested in
 * the cell at the specified position.
 *
 * Returns: (transfer full) (nullable): The #GtkTreePath corresponding to the icon or %NULL
 *          if no icon exists at that position.
 *
 * Since: 4.21.0
 **/
GtkTreePath *
xfce_icon_view_get_path_at_pos (XfceIconView *icon_view,
                                gint x,
                                gint y)
{
  XfceIconViewPrivate *priv = get_instance_private (icon_view);
  XfceIconViewItem *item;

  g_return_val_if_fail (XFCE_IS_ICON_VIEW (icon_view), NULL);

  /* translate the widget coordinates to icon window coordinates */
  x += gtk_adjustment_get_value (priv->hadjustment);
  y += gtk_adjustment_get_value (priv->vadjustment);

  item = xfce_icon_view_get_item_at_coords (icon_view, x, y, TRUE, NULL);

  return (item != NULL) ? gtk_tree_path_new_from_indices (g_sequence_iter_get_position (item->item_iter), -1) : NULL;
}



/**
 * xfce_icon_view_get_item_at_pos:
 * @icon_view: A #XfceIconView.
 * @x: The x position to be identified
 * @y: The y position to be identified
 * @path: Return location for the path, or %NULL
 * @cell: Return location for the renderer responsible for the cell
 *   at (@x, @y), or %NULL
 *
 * Finds the path at the point (@x, @y), relative to widget coordinates.
 * In contrast to xfce_icon_view_get_path_at_pos(), this function also
 * obtains the cell at the specified position. The returned path should
 * be freed with gtk_tree_path_free().
 *
 * Returns: %TRUE if an item exists at the specified position
 *
 * Since: 4.21.0
 **/
gboolean
xfce_icon_view_get_item_at_pos (XfceIconView *icon_view,
                                gint x,
                                gint y,
                                GtkTreePath **path,
                                GtkCellRenderer **cell)
{
  XfceIconViewCellInfo *info = NULL;
  XfceIconViewItem *item;

  g_return_val_if_fail (XFCE_IS_ICON_VIEW (icon_view), FALSE);

  item = xfce_icon_view_get_item_at_coords (icon_view, x, y, TRUE, &info);

  if (G_LIKELY (path != NULL))
    *path = (item != NULL) ? gtk_tree_path_new_from_indices (g_sequence_iter_get_position (item->item_iter), -1) : NULL;

  if (G_LIKELY (cell != NULL))
    *cell = (info != NULL) ? info->cell : NULL;

  return (item != NULL);
}



/**
 * xfce_icon_view_get_visible_range:
 * @icon_view  : A #XfceIconView
 * @start_path : Return location for start of region, or %NULL
 * @end_path   : Return location for end of region, or %NULL
 *
 * Sets @start_path and @end_path to be the first and last visible path.
 * Note that there may be invisible paths in between.
 *
 * Both paths should be freed with gtk_tree_path_free() after use.
 *
 * Returns: %TRUE, if valid paths were placed in @start_path and @end_path
 *
 * Since: 4.21.0
 **/
gboolean
xfce_icon_view_get_visible_range (XfceIconView *icon_view,
                                  GtkTreePath **start_path,
                                  GtkTreePath **end_path)
{
  XfceIconViewPrivate *priv = get_instance_private (icon_view);
  const XfceIconViewItem *item;
  gint start_index = -1;
  gint end_index = -1;
  gint i = 0;
  GSequenceIter *iter;

  g_return_val_if_fail (XFCE_IS_ICON_VIEW (icon_view), FALSE);

  if (priv->hadjustment == NULL || priv->vadjustment == NULL)
    return FALSE;

  if (start_path == NULL && end_path == NULL)
    return FALSE;

  for (iter = g_sequence_get_begin_iter (priv->items);
       !g_sequence_iter_is_end (iter);
       ++i, iter = g_sequence_iter_next (iter))
    {
      item = g_sequence_get (iter);
      if (item->area.x + item->area.width >= (gint) gtk_adjustment_get_value (priv->hadjustment)
          && item->area.y + item->area.height >= (gint) gtk_adjustment_get_value (priv->vadjustment)
          && item->area.x <= (gint) (gtk_adjustment_get_value (priv->hadjustment) + gtk_adjustment_get_page_size (priv->hadjustment))
          && item->area.y <= (gint) (gtk_adjustment_get_value (priv->vadjustment) + gtk_adjustment_get_page_size (priv->vadjustment)))
        {
          if (start_index == -1)
            start_index = i;
          end_index = i;
        }
    }

  if (start_path != NULL && start_index != -1)
    *start_path = gtk_tree_path_new_from_indices (start_index, -1);
  if (end_path != NULL && end_index != -1)
    *end_path = gtk_tree_path_new_from_indices (end_index, -1);

  return (start_index != -1);
}



/**
 * xfce_icon_view_selected_foreach:
 * @icon_view : A #XfceIconView.
 * @func: (scope call): The funcion to call for each selected icon.
 * @data      : User data to pass to the function.
 *
 * Calls a function for each selected icon. Note that the model or
 * selection cannot be modified from within this function.
 *
 * Since: 4.21.0
 **/
void
xfce_icon_view_selected_foreach (XfceIconView *icon_view,
                                 XfceIconViewForeachFunc func,
                                 gpointer data)
{
  XfceIconViewPrivate *priv = get_instance_private (icon_view);
  GtkTreePath *path;
  GSequenceIter *iter;

  path = gtk_tree_path_new_first ();
  for (iter = g_sequence_get_begin_iter (priv->items);
       !g_sequence_iter_is_end (iter);
       iter = g_sequence_iter_next (iter))
    {
      if (XFCE_ICON_VIEW_ITEM (g_sequence_get (iter))->selected)
        (*func) (icon_view, path, data);
      gtk_tree_path_next (path);
    }
  gtk_tree_path_free (path);
}



/**
 * xfce_icon_view_get_selection_mode:
 * @icon_view : A #XfceIconView.
 *
 * Gets the selection mode of the @icon_view.
 *
 * Returns: the current selection mode
 *
 * Since: 4.21.0
 **/
GtkSelectionMode
xfce_icon_view_get_selection_mode (XfceIconView *icon_view)
{
  XfceIconViewPrivate *priv = get_instance_private (icon_view);
  g_return_val_if_fail (XFCE_IS_ICON_VIEW (icon_view), GTK_SELECTION_SINGLE);
  return priv->selection_mode;
}



/**
 * xfce_icon_view_set_selection_mode:
 * @icon_view : A #XfceIconView.
 * @mode      : The selection mode
 *
 * Sets the selection mode of the @icon_view.
 *
 * Since: 4.21.0
 **/
void
xfce_icon_view_set_selection_mode (XfceIconView *icon_view,
                                   GtkSelectionMode mode)
{
  XfceIconViewPrivate *priv = get_instance_private (icon_view);

  g_return_if_fail (XFCE_IS_ICON_VIEW (icon_view));

  if (G_LIKELY (mode != priv->selection_mode))
    {
      if (mode == GTK_SELECTION_NONE || priv->selection_mode == GTK_SELECTION_MULTIPLE)
        xfce_icon_view_unselect_all (icon_view);

      priv->selection_mode = mode;

      g_object_notify (G_OBJECT (icon_view), "selection-mode");
    }
}



/**
 * xfce_icon_view_get_layout_mode:
 * @icon_view : A #XfceIconView.
 *
 * Returns the #XfceIconViewLayoutMode used to layout the
 * items in the @icon_view.
 *
 * Returns: the layout mode of @icon_view.
 *
 * Since: 4.21.0
 **/
XfceIconViewLayoutMode
xfce_icon_view_get_layout_mode (XfceIconView *icon_view)
{
  XfceIconViewPrivate *priv = get_instance_private (icon_view);
  g_return_val_if_fail (XFCE_IS_ICON_VIEW (icon_view), XFCE_ICON_VIEW_LAYOUT_ROWS);
  return priv->layout_mode;
}



/**
 * xfce_icon_view_set_layout_mode:
 * @icon_view   : a #XfceIconView.
 * @layout_mode : the new #XfceIconViewLayoutMode for @icon_view.
 *
 * Sets the layout mode of @icon_view to @layout_mode.
 *
 * Since: 4.21.0
 **/
void
xfce_icon_view_set_layout_mode (XfceIconView *icon_view,
                                XfceIconViewLayoutMode layout_mode)
{
  XfceIconViewPrivate *priv = get_instance_private (icon_view);

  g_return_if_fail (XFCE_IS_ICON_VIEW (icon_view));

  /* check if we have a new setting */
  if (G_LIKELY (priv->layout_mode != layout_mode))
    {
      /* apply the new setting */
      priv->layout_mode = layout_mode;

      /* cancel any active cell editor */
      xfce_icon_view_stop_editing (icon_view, TRUE);

      /* invalidate the current item sizes */
      xfce_icon_view_invalidate_sizes (icon_view);
      xfce_icon_view_queue_layout (icon_view);

      /* notify listeners */
      g_object_notify (G_OBJECT (icon_view), "layout-mode");
    }
}



/**
 * xfce_icon_view_get_model:
 * @icon_view : a #XfceIconView
 *
 * Returns the model the #XfceIconView is based on. Returns %NULL if the
 * model is unset.
 *
 * Returns: (nullable) (transfer none): A #GtkTreeModel, or %NULL if none is currently being used.
 *
 * Since: 4.21.0
 **/
GtkTreeModel *
xfce_icon_view_get_model (XfceIconView *icon_view)
{
  XfceIconViewPrivate *priv = get_instance_private (icon_view);
  g_return_val_if_fail (XFCE_IS_ICON_VIEW (icon_view), NULL);
  return priv->model;
}



/**
 * xfce_icon_view_set_model:
 * @icon_view : A #XfceIconView.
 * @model     : The model.
 *
 * Sets the model for a #XfceIconView.
 * If the @icon_view already has a model set, it will remove
 * it before setting the new model.  If @model is %NULL, then
 * it will unset the old model.
 *
 * Since: 4.21.0
 **/
void
xfce_icon_view_set_model (XfceIconView *icon_view,
                          GtkTreeModel *model)
{
  XfceIconViewPrivate *priv = get_instance_private (icon_view);
  XfceIconViewItem *item;
  GtkTreeIter iter;
  gint n;
  GSequenceIter *item_iter;

  g_return_if_fail (XFCE_IS_ICON_VIEW (icon_view));
  g_return_if_fail (model == NULL || GTK_IS_TREE_MODEL (model));

  /* verify that we don't already use that model */
  if (G_UNLIKELY (priv->model == model))
    return;

  /* verify the new model */
  if (G_LIKELY (model != NULL))
    {

      if (G_UNLIKELY (priv->pixbuf_column != -1))
        g_return_if_fail (gtk_tree_model_get_column_type (model, priv->pixbuf_column) == GDK_TYPE_PIXBUF);

      if (G_UNLIKELY (priv->icon_column != -1))
        g_return_if_fail (gtk_tree_model_get_column_type (model, priv->icon_column) == G_TYPE_STRING);

      if (G_UNLIKELY (priv->text_column != -1))
        g_return_if_fail (gtk_tree_model_get_column_type (model, priv->text_column) == G_TYPE_STRING);

      if (G_UNLIKELY (priv->markup_column != -1))
        g_return_if_fail (gtk_tree_model_get_column_type (model, priv->markup_column) == G_TYPE_STRING);
    }

  /* be sure to cancel any pending editor */
  xfce_icon_view_stop_editing (icon_view, TRUE);

  /* disconnect from the previous model */
  if (G_LIKELY (priv->model != NULL))
    {
      /* disconnect signals handlers from the previous model */
      g_signal_handlers_disconnect_by_func (G_OBJECT (priv->model), xfce_icon_view_row_changed, icon_view);
      g_signal_handlers_disconnect_by_func (G_OBJECT (priv->model), xfce_icon_view_row_inserted, icon_view);
      g_signal_handlers_disconnect_by_func (G_OBJECT (priv->model), xfce_icon_view_row_deleted, icon_view);
      g_signal_handlers_disconnect_by_func (G_OBJECT (priv->model), xfce_icon_view_rows_reordered, icon_view);

      /* release our reference on the model */
      g_object_unref (G_OBJECT (priv->model));

      if (!g_sequence_is_empty (priv->items))
        {
          /* drop all items belonging to the previous model */
          xfce_icon_view_release_items (icon_view);
          g_sequence_free (priv->items);
          priv->items = g_sequence_new (NULL);
        }

      /* reset statistics */
      priv->search_column = -1;
      priv->anchor_item = NULL;
      priv->cursor_item = NULL;
      priv->prelit_item = NULL;
      priv->last_single_clicked = NULL;
      priv->width = 0;
      priv->height = 0;

      /* cancel any pending single click timer */
      if (G_UNLIKELY (priv->single_click_timeout_id != 0))
        g_source_remove (priv->single_click_timeout_id);

      /* reset cursor when in single click mode and realized */
      if (G_UNLIKELY (priv->single_click && gtk_widget_get_realized (GTK_WIDGET (icon_view))))
        gdk_window_set_cursor (priv->bin_window, NULL);
    }

  /* be sure to drop any previous scroll_to_path reference,
   * as it points to the old (no longer valid) model.
   */
  if (G_UNLIKELY (priv->scroll_to_path != NULL))
    {
      gtk_tree_row_reference_free (priv->scroll_to_path);
      priv->scroll_to_path = NULL;
    }

  /* activate the new model */
  priv->model = model;

  /* connect to the new model */
  if (G_LIKELY (model != NULL))
    {
      /* take a reference on the model */
      g_object_ref (G_OBJECT (model));

      /* connect signals */
      g_signal_connect (G_OBJECT (model), "row-changed", G_CALLBACK (xfce_icon_view_row_changed), icon_view);
      g_signal_connect (G_OBJECT (model), "row-inserted", G_CALLBACK (xfce_icon_view_row_inserted), icon_view);
      g_signal_connect (G_OBJECT (model), "row-deleted", G_CALLBACK (xfce_icon_view_row_deleted), icon_view);
      g_signal_connect (G_OBJECT (model), "rows-reordered", G_CALLBACK (xfce_icon_view_rows_reordered), icon_view);

      /* check if the new model supports persistent iterators */
      if (gtk_tree_model_get_flags (model) & GTK_TREE_MODEL_ITERS_PERSIST)
        XFCE_ICON_VIEW_SET_FLAG (icon_view, XFCE_ICON_VIEW_ITERS_PERSIST);
      else
        XFCE_ICON_VIEW_UNSET_FLAG (icon_view, XFCE_ICON_VIEW_ITERS_PERSIST);

      /* determine an appropriate search column */
      if (priv->search_column <= 0)
        {
          /* we simply use the first string column */
          for (n = 0; n < gtk_tree_model_get_n_columns (model); ++n)
            if (g_value_type_transformable (gtk_tree_model_get_column_type (model, n), G_TYPE_STRING))
              {
                priv->search_column = n;
                break;
              }
        }

      /* build up the initial items list */
      if (gtk_tree_model_get_iter_first (model, &iter))
        {
          do
            {
              item = g_slice_new0 (XfceIconViewItem);
              item->iter = iter;
              item->area.width = -1;
              item_iter = g_sequence_append (priv->items, item);
              item->item_iter = item_iter;
            }
          while (gtk_tree_model_iter_next (model, &iter));
        }

      /* layout the new items */
      xfce_icon_view_queue_layout (icon_view);
    }

  /* hide the interactive search dialog (if any) */
  if (G_LIKELY (priv->search_window != NULL))
    xfce_icon_view_search_dialog_hide (priv->search_window, icon_view);

  /* notify listeners */
  g_object_notify (G_OBJECT (icon_view), "model");

  if (gtk_widget_get_realized (GTK_WIDGET (icon_view)))
    gtk_widget_queue_resize (GTK_WIDGET (icon_view));
}



static void
update_text_cell (XfceIconView *icon_view)
{
  XfceIconViewPrivate *priv = get_instance_private (icon_view);
  XfceIconViewCellInfo *info;
  GList *l;
  gint i;

  if (priv->text_column == -1 && priv->markup_column == -1)
    {
      if (priv->text_cell != -1)
        {
          info = g_list_nth_data (priv->cell_list,
                                  priv->text_cell);

          priv->cell_list = g_list_remove (priv->cell_list, info);

          free_cell_info (info);

          priv->n_cells--;
          priv->text_cell = -1;
        }
    }
  else
    {
      if (priv->text_cell == -1)
        {
          GtkCellRenderer *cell = gtk_cell_renderer_text_new ();
          gtk_cell_layout_pack_end (GTK_CELL_LAYOUT (icon_view), cell, FALSE);
          for (l = priv->cell_list, i = 0; l; l = l->next, i++)
            {
              info = l->data;
              if (info->cell == cell)
                {
                  priv->text_cell = i;
                  break;
                }
            }
        }

      info = g_list_nth_data (priv->cell_list,
                              priv->text_cell);

      if (priv->markup_column != -1)
        gtk_cell_layout_set_attributes (GTK_CELL_LAYOUT (icon_view),
                                        info->cell,
                                        "markup", priv->markup_column,
                                        NULL);
      else
        gtk_cell_layout_set_attributes (GTK_CELL_LAYOUT (icon_view),
                                        info->cell,
                                        "text", priv->text_column,
                                        NULL);
    }
}

static void
update_pixbuf_cell (XfceIconView *icon_view)
{
  XfceIconViewPrivate *priv = get_instance_private (icon_view);
  XfceIconViewCellInfo *info;
  GList *l;
  gint i;

  if (priv->pixbuf_column == -1 && priv->icon_column == -1)
    {
      if (priv->pixbuf_cell != -1)
        {
          info = g_list_nth_data (priv->cell_list,
                                  priv->pixbuf_cell);

          priv->cell_list = g_list_remove (priv->cell_list, info);

          free_cell_info (info);

          priv->n_cells--;
          priv->pixbuf_cell = -1;
        }
    }
  else
    {
      if (priv->pixbuf_cell == -1)
        {
          GtkCellRenderer *cell;

          if (priv->pixbuf_column != -1)
            {
              cell = gtk_cell_renderer_pixbuf_new ();
            }
          else
            {
              cell = xfce_cell_renderer_icon_new ();
            }

          gtk_cell_layout_pack_start (GTK_CELL_LAYOUT (icon_view), cell, FALSE);
          for (l = priv->cell_list, i = 0; l; l = l->next, i++)
            {
              info = l->data;
              if (info->cell == cell)
                {
                  priv->pixbuf_cell = i;
                  break;
                }
            }
        }

      info = g_list_nth_data (priv->cell_list,
                              priv->pixbuf_cell);

      if (priv->pixbuf_column != -1)
        {
          gtk_cell_layout_set_attributes (GTK_CELL_LAYOUT (icon_view),
                                          info->cell,
                                          "pixbuf", priv->pixbuf_column,
                                          NULL);
        }
      else
        {
          gtk_cell_layout_set_attributes (GTK_CELL_LAYOUT (icon_view),
                                          info->cell,
                                          "icon", priv->icon_column,
                                          NULL);
        }
    }
}



/**
 * xfce_icon_view_select_path:
 * @icon_view : A #XfceIconView.
 * @path      : The #GtkTreePath to be selected.
 *
 * Selects the row at @path.
 *
 * Since: 4.21.0
 **/
void
xfce_icon_view_select_path (XfceIconView *icon_view,
                            GtkTreePath *path)
{
  XfceIconViewPrivate *priv = get_instance_private (icon_view);
  XfceIconViewItem *item;
  GSequenceIter *iter;

  g_return_if_fail (XFCE_IS_ICON_VIEW (icon_view));
  g_return_if_fail (priv->model != NULL);
  g_return_if_fail (gtk_tree_path_get_depth (path) > 0);

  iter = g_sequence_get_iter_at_pos (priv->items, gtk_tree_path_get_indices (path)[0]);
  if (g_sequence_iter_is_end (iter))
    return;

  item = g_sequence_get (iter);
  if (G_LIKELY (item != NULL))
    xfce_icon_view_select_item (icon_view, item);
}



/**
 * xfce_icon_view_unselect_path:
 * @icon_view : A #XfceIconView.
 * @path      : The #GtkTreePath to be unselected.
 *
 * Unselects the row at @path.
 *
 * Since: 4.21.0
 **/
void
xfce_icon_view_unselect_path (XfceIconView *icon_view,
                              GtkTreePath *path)
{
  XfceIconViewPrivate *priv = get_instance_private (icon_view);
  XfceIconViewItem *item;
  GSequenceIter *iter;

  g_return_if_fail (XFCE_IS_ICON_VIEW (icon_view));
  g_return_if_fail (priv->model != NULL);
  g_return_if_fail (gtk_tree_path_get_depth (path) > 0);

  iter = g_sequence_get_iter_at_pos (priv->items, gtk_tree_path_get_indices (path)[0]);
  if (g_sequence_iter_is_end (iter))
    return;

  item = g_sequence_get (iter);

  if (G_LIKELY (item != NULL))
    xfce_icon_view_unselect_item (icon_view, item);
}



/**
 * xfce_icon_view_get_selected_items:
 * @icon_view: A #XfceIconView.
 *
 * Creates a list of paths of all selected items. Additionally, if you are
 * planning on modifying the model after calling this function, you may
 * want to convert the returned list into a list of #GtkTreeRowReference<!-- -->s.
 * To do this, you can use gtk_tree_row_reference_new().
 *
 * To free the return value, use:
 * <informalexample><programlisting>
 * g_list_free_full (list, (GDestroyNotify) gtk_tree_path_free);
 * </programlisting></informalexample>
 *
 * Returns: (transfer full) (element-type GtkTreePath): A #GList containing a #GtkTreePath for each selected row.
 *
 * Since: 4.21.0
 **/
GList *
xfce_icon_view_get_selected_items (XfceIconView *icon_view)
{
  XfceIconViewPrivate *priv = get_instance_private (icon_view);
  GList *selected = NULL;
  GSequenceIter *iter;
  gint i = 0;

  g_return_val_if_fail (XFCE_IS_ICON_VIEW (icon_view), NULL);

  for (iter = g_sequence_get_begin_iter (priv->items);
       !g_sequence_iter_is_end (iter);
       ++i, iter = g_sequence_iter_next (iter))
    {
      if (XFCE_ICON_VIEW_ITEM (g_sequence_get (iter))->selected)
        selected = g_list_prepend (selected, gtk_tree_path_new_from_indices (i, -1));
    }

  return g_list_reverse (selected);
}



/**
 * xfce_icon_view_select_all:
 * @icon_view : A #XfceIconView.
 *
 * Selects all the icons. @icon_view must has its selection mode set
 * to #GTK_SELECTION_MULTIPLE.
 *
 * Since: 4.21.0
 **/
void
xfce_icon_view_select_all (XfceIconView *icon_view)
{
  XfceIconViewPrivate *priv = get_instance_private (icon_view);
  GSequenceIter *iter;
  gboolean dirty = FALSE;

  g_return_if_fail (XFCE_IS_ICON_VIEW (icon_view));

  if (priv->selection_mode != GTK_SELECTION_MULTIPLE)
    return;

  for (iter = g_sequence_get_begin_iter (priv->items);
       !g_sequence_iter_is_end (iter);
       iter = g_sequence_iter_next (iter))
    {
      XfceIconViewItem *item = g_sequence_get (iter);

      if (!item->selected)
        {
          dirty = TRUE;
          item->selected = TRUE;
          xfce_icon_view_queue_draw_item (icon_view, item);
        }
    }

  if (dirty)
    g_signal_emit (icon_view, icon_view_signals[SELECTION_CHANGED], 0);
}



/**
 * xfce_icon_view_selection_invert:
 * @icon_view : A #XfceIconView.
 *
 * Selects all the icons that are currently not selected. @icon_view must
 * has its selection mode set to #GTK_SELECTION_MULTIPLE.
 *
 * Since: 4.21.0
 **/
void
xfce_icon_view_selection_invert (XfceIconView *icon_view)
{
  XfceIconViewPrivate *priv = get_instance_private (icon_view);
  GSequenceIter *iter;
  gboolean dirty = FALSE;
  XfceIconViewItem *item;

  g_return_if_fail (XFCE_IS_ICON_VIEW (icon_view));

  if (priv->selection_mode != GTK_SELECTION_MULTIPLE)
    return;

  for (iter = g_sequence_get_begin_iter (priv->items);
       !g_sequence_iter_is_end (iter);
       iter = g_sequence_iter_next (iter))
    {
      item = g_sequence_get (iter);

      item->selected = !item->selected;
      xfce_icon_view_queue_draw_item (icon_view, item);

      dirty = TRUE;
    }

  if (dirty)
    g_signal_emit (icon_view, icon_view_signals[SELECTION_CHANGED], 0);
}



/**
 * xfce_icon_view_unselect_all:
 * @icon_view : A #XfceIconView.
 *
 * Unselects all the icons.
 *
 * Since: 4.21.0
 **/
void
xfce_icon_view_unselect_all (XfceIconView *icon_view)
{
  XfceIconViewPrivate *priv = get_instance_private (icon_view);

  g_return_if_fail (XFCE_IS_ICON_VIEW (icon_view));

  if (G_UNLIKELY (priv->selection_mode == GTK_SELECTION_BROWSE))
    return;

  if (xfce_icon_view_unselect_all_internal (icon_view))
    g_signal_emit (icon_view, icon_view_signals[SELECTION_CHANGED], 0);
}



/**
 * xfce_icon_view_path_is_selected:
 * @icon_view: A #XfceIconView.
 * @path: A #GtkTreePath to check selection on.
 *
 * Returns %TRUE if the icon pointed to by @path is currently
 * selected. If @icon does not point to a valid location, %FALSE is returned.
 *
 * Returns: %TRUE if @path is selected.
 *
 * Since: 4.21.0
 **/
gboolean
xfce_icon_view_path_is_selected (XfceIconView *icon_view,
                                 GtkTreePath *path)
{
  XfceIconViewPrivate *priv = get_instance_private (icon_view);
  XfceIconViewItem *item;
  GSequenceIter *iter;

  g_return_val_if_fail (XFCE_IS_ICON_VIEW (icon_view), FALSE);
  g_return_val_if_fail (priv->model != NULL, FALSE);
  g_return_val_if_fail (gtk_tree_path_get_depth (path) > 0, FALSE);

  iter = g_sequence_get_iter_at_pos (priv->items, gtk_tree_path_get_indices (path)[0]);
  if (g_sequence_iter_is_end (iter))
    return FALSE;

  item = g_sequence_get (iter);
  return (item != NULL && item->selected);
}



/**
 * xfce_icon_view_item_activated:
 * @icon_view : a #XfceIconView
 * @path      : the #GtkTreePath to be activated
 *
 * Activates the item determined by @path.
 *
 * Since: 4.21.0
 **/
void
xfce_icon_view_item_activated (XfceIconView *icon_view,
                               GtkTreePath *path)
{
  g_return_if_fail (XFCE_IS_ICON_VIEW (icon_view));
  g_return_if_fail (gtk_tree_path_get_depth (path) > 0);

  g_signal_emit (icon_view, icon_view_signals[ITEM_ACTIVATED], 0, path);
}



/**
 * xfce_icon_view_get_item_column:
 * @icon_view : A #XfceIconView.
 * @path      : The #GtkTreePath of the item.
 *
 * Gets the column in which the item @path is currently
 * displayed. Column numbers start at 0.
 *
 * Returns: The column in which the item is displayed
 *
 * Since: 4.21.0
 **/
gint
xfce_icon_view_get_item_column (XfceIconView *icon_view,
                                GtkTreePath *path)
{
  XfceIconViewPrivate *priv = get_instance_private (icon_view);
  XfceIconViewItem *item;
  GSequenceIter *iter;

  g_return_val_if_fail (XFCE_IS_ICON_VIEW (icon_view), -1);
  g_return_val_if_fail (priv->model != NULL, -1);
  g_return_val_if_fail (gtk_tree_path_get_depth (path) > 0, -1);

  iter = g_sequence_get_iter_at_pos (priv->items, gtk_tree_path_get_indices (path)[0]);
  if (g_sequence_iter_is_end (iter))
    return -1;

  item = g_sequence_get (iter);

  if (G_UNLIKELY (item == NULL))
    return -1;

  return item->col;
}



/**
 * xfce_icon_view_get_item_row:
 * @icon_view : A #XfceIconView.
 * @path      : The #GtkTreePath of the item.
 *
 * Gets the row in which the item @path is currently
 * displayed. Row numbers start at 0.
 *
 * Returns: The row in which the item is displayed
 *
 * Since: 4.21.0
 */
gint
xfce_icon_view_get_item_row (XfceIconView *icon_view,
                             GtkTreePath *path)
{
  XfceIconViewPrivate *priv = get_instance_private (icon_view);
  XfceIconViewItem *item;
  GSequenceIter *iter;

  g_return_val_if_fail (XFCE_IS_ICON_VIEW (icon_view), -1);
  g_return_val_if_fail (priv->model != NULL, -1);
  g_return_val_if_fail (gtk_tree_path_get_depth (path) > 0, -1);

  iter = g_sequence_get_iter_at_pos (priv->items, gtk_tree_path_get_indices (path)[0]);
  if (g_sequence_iter_is_end (iter))
    return -1;

  item = g_sequence_get (iter);

  if (G_UNLIKELY (item == NULL))
    return -1;

  return item->row;
}



/**
 * xfce_icon_view_get_cursor:
 * @icon_view : A #XfceIconView
 * @path      : Return location for the current cursor path, or %NULL
 * @cell      : Return location the current focus cell, or %NULL
 *
 * Fills in @path and @cell with the current cursor path and cell.
 * If the cursor isn't currently set, then *@path will be %NULL.
 * If no cell currently has focus, then *@cell will be %NULL.
 *
 * The returned #GtkTreePath must be freed with gtk_tree_path_free().
 *
 * Returns: %TRUE if the cursor is set.
 *
 * Since: 4.21.0
 **/
gboolean
xfce_icon_view_get_cursor (XfceIconView *icon_view,
                           GtkTreePath **path,
                           GtkCellRenderer **cell)
{
  XfceIconViewPrivate *priv = get_instance_private (icon_view);
  XfceIconViewCellInfo *info;
  XfceIconViewItem *item;

  g_return_val_if_fail (XFCE_IS_ICON_VIEW (icon_view), FALSE);

  item = priv->cursor_item;
  info = (priv->cursor_cell < 0)
           ? NULL
           : g_list_nth_data (priv->cell_list, priv->cursor_cell);

  if (G_LIKELY (path != NULL))
    *path = (item != NULL)
              ? gtk_tree_path_new_from_indices (g_sequence_iter_get_position (item->item_iter), -1)
              : NULL;

  if (G_LIKELY (cell != NULL))
    *cell = (info != NULL) ? info->cell : NULL;

  return (item != NULL);
}



/**
 * xfce_icon_view_set_cursor:
 * @icon_view     : a #XfceIconView
 * @path          : a #GtkTreePath
 * @cell          : a #GtkCellRenderer or %NULL
 * @start_editing : %TRUE if the specified cell should start being edited.
 *
 * Sets the current keyboard focus to be at @path, and selects it.  This is
 * useful when you want to focus the user's attention on a particular item.
 * If @cell is not %NULL, then focus is given to the cell specified by
 * it. Additionally, if @start_editing is %TRUE, then editing should be
 * started in the specified cell.
 *
 * This function is often followed by <literal>gtk_widget_grab_focus
 * (icon_view)</literal> in order to give keyboard focus to the widget.
 * Please note that editing can only happen when the widget is realized.
 *
 * Since: 4.21.0
 **/
void
xfce_icon_view_set_cursor (XfceIconView *icon_view,
                           GtkTreePath *path,
                           GtkCellRenderer *cell,
                           gboolean start_editing)
{
  XfceIconViewPrivate *priv = get_instance_private (icon_view);
  XfceIconViewItem *item;
  XfceIconViewCellInfo *info = NULL;
  GList *l;
  gint i, cell_pos;
  GSequenceIter *iter;

  g_return_if_fail (XFCE_IS_ICON_VIEW (icon_view));
  g_return_if_fail (path != NULL);
  g_return_if_fail (cell == NULL || GTK_IS_CELL_RENDERER (cell));

  xfce_icon_view_stop_editing (icon_view, TRUE);

  iter = g_sequence_get_iter_at_pos (priv->items, gtk_tree_path_get_indices (path)[0]);
  if (g_sequence_iter_is_end (iter))
    return;

  item = g_sequence_get (iter);
  if (G_UNLIKELY (item == NULL))
    return;

  cell_pos = -1;
  for (l = priv->cell_list, i = 0; l; l = l->next, i++)
    {
      info = l->data;

      if (info->cell == cell)
        {
          cell_pos = i;
          break;
        }

      info = NULL;
    }

  /* place the cursor on the item */
  xfce_icon_view_set_cursor_item (icon_view, item, cell_pos);
  priv->anchor_item = item;

  /* scroll to the item (maybe delayed) */
  xfce_icon_view_scroll_to_path (icon_view, path, FALSE, 0.0f, 0.0f);

  if (!info)
    return;

  if (start_editing)
    xfce_icon_view_start_editing (icon_view, item, info, NULL);
}



/**
 * xfce_icon_view_scroll_to_path:
 * @icon_view: A #XfceIconView.
 * @path: The path of the item to move to.
 * @use_align: whether to use alignment arguments, or %FALSE.
 * @row_align: The vertical alignment of the item specified by @path.
 * @col_align: The horizontal alignment of the item specified by @column.
 *
 * Moves the alignments of @icon_view to the position specified by @path.
 * @row_align determines where the row is placed, and @col_align determines where
 * @column is placed.  Both are expected to be between 0.0 and 1.0.
 * 0.0 means left/top alignment, 1.0 means right/bottom alignment, 0.5 means center.
 *
 * If @use_align is %FALSE, then the alignment arguments are ignored, and the
 * tree does the minimum amount of work to scroll the item onto the screen.
 * This means that the item will be scrolled to the edge closest to its current
 * position.  If the item is currently visible on the screen, nothing is done.
 *
 * This function only works if the model is set, and @path is a valid row on the
 * model.  If the model changes before the @tree_view is realized, the centered
 * path will be modified to reflect this change.
 *
 * Since: 4.21.0
 **/
void
xfce_icon_view_scroll_to_path (XfceIconView *icon_view,
                               GtkTreePath *path,
                               gboolean use_align,
                               gfloat row_align,
                               gfloat col_align)
{
  XfceIconViewPrivate *priv = get_instance_private (icon_view);
  XfceIconViewItem *item;
  GtkAllocation allocation;
  GSequenceIter *iter;

  g_return_if_fail (XFCE_IS_ICON_VIEW (icon_view));
  g_return_if_fail (gtk_tree_path_get_depth (path) > 0);
  g_return_if_fail (row_align >= 0.0 && row_align <= 1.0);
  g_return_if_fail (col_align >= 0.0 && col_align <= 1.0);

  gtk_widget_get_allocation (GTK_WIDGET (icon_view), &allocation);

  /* Delay scrolling if either not realized or pending layout() */
  if (!gtk_widget_get_realized (GTK_WIDGET (icon_view)) || priv->layout_idle_id != 0)
    {
      /* release the previous scroll_to_path reference */
      if (G_UNLIKELY (priv->scroll_to_path != NULL))
        gtk_tree_row_reference_free (priv->scroll_to_path);

      /* remember a reference for the new path and settings */
      priv->scroll_to_path = gtk_tree_row_reference_new_proxy (G_OBJECT (icon_view), priv->model, path);
      priv->scroll_to_use_align = use_align;
      priv->scroll_to_row_align = row_align;
      priv->scroll_to_col_align = col_align;
    }
  else
    {
      iter = g_sequence_get_iter_at_pos (priv->items, gtk_tree_path_get_indices (path)[0]);
      if (g_sequence_iter_is_end (iter))
        return;

      item = g_sequence_get (iter);
      if (G_UNLIKELY (item == NULL))
        return;

      if (use_align)
        {
          gint x, y;
          gint focus_width;
          gfloat offset, value;

          gtk_widget_style_get (GTK_WIDGET (icon_view),
                                "focus-line-width", &focus_width,
                                NULL);

          gdk_window_get_position (priv->bin_window, &x, &y);

          offset = y + item->area.y - focus_width - row_align * (allocation.height - item->area.height);
          value = CLAMP (gtk_adjustment_get_value (priv->vadjustment) + offset,
                         gtk_adjustment_get_lower (priv->vadjustment),
                         gtk_adjustment_get_upper (priv->vadjustment)
                           - gtk_adjustment_get_page_size (priv->vadjustment));
          gtk_adjustment_set_value (priv->vadjustment, value);

          offset = x + item->area.x - focus_width - col_align * (allocation.width - item->area.width);
          value = CLAMP (gtk_adjustment_get_value (priv->hadjustment) + offset,
                         gtk_adjustment_get_lower (priv->hadjustment),
                         gtk_adjustment_get_upper (priv->hadjustment)
                           - gtk_adjustment_get_page_size (priv->hadjustment));
          gtk_adjustment_set_value (priv->hadjustment, value);
        }
      else
        {
          xfce_icon_view_scroll_to_item (icon_view, item);
        }
    }
}



/**
 * xfce_icon_view_get_orientation:
 * @icon_view : a #XfceIconView
 *
 * Returns the value of the ::orientation property which determines
 * whether the labels are drawn beside the icons instead of below.
 *
 * Returns: the relative position of texts and icons
 *
 * Since: 4.21.0
 **/
GtkOrientation
xfce_icon_view_get_orientation (XfceIconView *icon_view)
{
  XfceIconViewPrivate *priv = get_instance_private (icon_view);
  g_return_val_if_fail (XFCE_IS_ICON_VIEW (icon_view), GTK_ORIENTATION_VERTICAL);
  return priv->orientation;
}



/**
 * xfce_icon_view_set_orientation:
 * @icon_view   : a #XfceIconView
 * @orientation : the relative position of texts and icons
 *
 * Sets the ::orientation property which determines whether the labels
 * are drawn beside the icons instead of below.
 *
 * Since: 4.21.0
 **/
void
xfce_icon_view_set_orientation (XfceIconView *icon_view,
                                GtkOrientation orientation)
{
  XfceIconViewPrivate *priv = get_instance_private (icon_view);

  g_return_if_fail (XFCE_IS_ICON_VIEW (icon_view));

  if (G_LIKELY (priv->orientation != orientation))
    {
      priv->orientation = orientation;

      xfce_icon_view_stop_editing (icon_view, TRUE);
      xfce_icon_view_invalidate_sizes (icon_view);

      update_text_cell (icon_view);
      update_pixbuf_cell (icon_view);

      g_object_notify (G_OBJECT (icon_view), "orientation");
    }
}



/**
 * xfce_icon_view_get_columns:
 * @icon_view: a #XfceIconView
 *
 * Returns the value of the ::columns property.
 *
 * Returns: the number of columns, or -1
 *
 * Since: 4.21.0
 */
gint
xfce_icon_view_get_columns (XfceIconView *icon_view)
{
  XfceIconViewPrivate *priv = get_instance_private (icon_view);
  g_return_val_if_fail (XFCE_IS_ICON_VIEW (icon_view), -1);
  return priv->columns;
}



/**
 * xfce_icon_view_set_columns:
 * @icon_view : a #XfceIconView
 * @columns   : the number of columns
 *
 * Sets the ::columns property which determines in how
 * many columns the icons are arranged. If @columns is
 * -1, the number of columns will be chosen automatically
 * to fill the available area.
 *
 * Since: 4.21.0
 */
void
xfce_icon_view_set_columns (XfceIconView *icon_view,
                            gint columns)
{
  XfceIconViewPrivate *priv = get_instance_private (icon_view);

  g_return_if_fail (XFCE_IS_ICON_VIEW (icon_view));

  if (G_LIKELY (priv->columns != columns))
    {
      priv->columns = columns;

      xfce_icon_view_stop_editing (icon_view, TRUE);
      xfce_icon_view_queue_layout (icon_view);

      g_object_notify (G_OBJECT (icon_view), "columns");
    }
}



/**
 * xfce_icon_view_get_item_width:
 * @icon_view: a #XfceIconView
 *
 * Returns the value of the ::item-width property.
 *
 * Returns: the width of a single item, or -1
 *
 * Since: 4.21.0
 */
gint
xfce_icon_view_get_item_width (XfceIconView *icon_view)
{
  XfceIconViewPrivate *priv = get_instance_private (icon_view);
  g_return_val_if_fail (XFCE_IS_ICON_VIEW (icon_view), -1);
  return priv->item_width;
}



/**
 * xfce_icon_view_set_item_width:
 * @icon_view  : a #XfceIconView
 * @item_width : the width for each item
 *
 * Sets the ::item-width property which specifies the width
 * to use for each item. If it is set to -1, the icon view will
 * automatically determine a suitable item size.
 *
 * Since: 4.21.0
 */
void
xfce_icon_view_set_item_width (XfceIconView *icon_view,
                               gint item_width)
{
  XfceIconViewPrivate *priv = get_instance_private (icon_view);

  g_return_if_fail (XFCE_IS_ICON_VIEW (icon_view));

  if (priv->item_width != item_width)
    {
      priv->item_width = item_width;

      xfce_icon_view_stop_editing (icon_view, TRUE);
      xfce_icon_view_invalidate_sizes (icon_view);

      update_text_cell (icon_view);

      g_object_notify (G_OBJECT (icon_view), "item-width");
    }
}



/**
 * xfce_icon_view_get_spacing:
 * @icon_view: a #XfceIconView
 *
 * Returns the value of the ::spacing property.
 *
 * Returns: the space between cells
 *
 * Since: 4.21.0
 */
gint
xfce_icon_view_get_spacing (XfceIconView *icon_view)
{
  XfceIconViewPrivate *priv = get_instance_private (icon_view);
  g_return_val_if_fail (XFCE_IS_ICON_VIEW (icon_view), -1);
  return priv->spacing;
}



/**
 * xfce_icon_view_set_spacing:
 * @icon_view : a #XfceIconView
 * @spacing   : the spacing
 *
 * Sets the ::spacing property which specifies the space
 * which is inserted between the cells (i.e. the icon and
 * the text) of an item.
 *
 * Since: 4.21.0
 */
void
xfce_icon_view_set_spacing (XfceIconView *icon_view,
                            gint spacing)
{
  XfceIconViewPrivate *priv = get_instance_private (icon_view);

  g_return_if_fail (XFCE_IS_ICON_VIEW (icon_view));

  if (G_LIKELY (priv->spacing != spacing))
    {
      priv->spacing = spacing;

      xfce_icon_view_stop_editing (icon_view, TRUE);
      xfce_icon_view_invalidate_sizes (icon_view);

      g_object_notify (G_OBJECT (icon_view), "spacing");
    }
}



/**
 * xfce_icon_view_get_row_spacing:
 * @icon_view: a #XfceIconView
 *
 * Returns the value of the ::row-spacing property.
 *
 * Returns: the space between rows
 *
 * Since: 4.21.0
 */
gint
xfce_icon_view_get_row_spacing (XfceIconView *icon_view)
{
  XfceIconViewPrivate *priv = get_instance_private (icon_view);
  g_return_val_if_fail (XFCE_IS_ICON_VIEW (icon_view), -1);
  return priv->row_spacing;
}



/**
 * xfce_icon_view_set_row_spacing:
 * @icon_view   : a #XfceIconView
 * @row_spacing : the row spacing
 *
 * Sets the ::row-spacing property which specifies the space
 * which is inserted between the rows of the icon view.
 *
 * Since: 4.21.0
 */
void
xfce_icon_view_set_row_spacing (XfceIconView *icon_view,
                                gint row_spacing)
{
  XfceIconViewPrivate *priv = get_instance_private (icon_view);

  g_return_if_fail (XFCE_IS_ICON_VIEW (icon_view));

  if (G_LIKELY (priv->row_spacing != row_spacing))
    {
      priv->row_spacing = row_spacing;

      xfce_icon_view_stop_editing (icon_view, TRUE);
      xfce_icon_view_invalidate_sizes (icon_view);

      g_object_notify (G_OBJECT (icon_view), "row-spacing");
    }
}



/**
 * xfce_icon_view_get_column_spacing:
 * @icon_view: a #XfceIconView
 *
 * Returns the value of the ::column-spacing property.
 *
 * Returns: the space between columns
 *
 * Since: 4.21.0
 **/
gint
xfce_icon_view_get_column_spacing (XfceIconView *icon_view)
{
  XfceIconViewPrivate *priv = get_instance_private (icon_view);
  g_return_val_if_fail (XFCE_IS_ICON_VIEW (icon_view), -1);
  return priv->column_spacing;
}



/**
 * xfce_icon_view_set_column_spacing:
 * @icon_view      : a #XfceIconView
 * @column_spacing : the column spacing
 *
 * Sets the ::column-spacing property which specifies the space
 * which is inserted between the columns of the icon view.
 *
 * Since: 4.21.0
 **/
void
xfce_icon_view_set_column_spacing (XfceIconView *icon_view,
                                   gint column_spacing)
{
  XfceIconViewPrivate *priv = get_instance_private (icon_view);

  g_return_if_fail (XFCE_IS_ICON_VIEW (icon_view));

  if (G_LIKELY (priv->column_spacing != column_spacing))
    {
      priv->column_spacing = column_spacing;

      xfce_icon_view_stop_editing (icon_view, TRUE);
      xfce_icon_view_invalidate_sizes (icon_view);

      g_object_notify (G_OBJECT (icon_view), "column-spacing");
    }
}



/**
 * xfce_icon_view_get_margin:
 * @icon_view : a #XfceIconView
 *
 * Returns the value of the ::margin property.
 *
 * Returns: the space at the borders
 *
 * Since: 4.21.0
 **/
gint
xfce_icon_view_get_margin (XfceIconView *icon_view)
{
  XfceIconViewPrivate *priv = get_instance_private (icon_view);
  g_return_val_if_fail (XFCE_IS_ICON_VIEW (icon_view), -1);
  return priv->margin;
}



/**
 * xfce_icon_view_set_margin:
 * @icon_view : a #XfceIconView
 * @margin    : the margin
 *
 * Sets the ::margin property which specifies the space
 * which is inserted at the top, bottom, left and right
 * of the icon view.
 *
 * Since: 4.21.0
 **/
void
xfce_icon_view_set_margin (XfceIconView *icon_view,
                           gint margin)
{
  XfceIconViewPrivate *priv = get_instance_private (icon_view);

  g_return_if_fail (XFCE_IS_ICON_VIEW (icon_view));

  if (G_LIKELY (priv->margin != margin))
    {
      priv->margin = margin;

      xfce_icon_view_stop_editing (icon_view, TRUE);
      xfce_icon_view_invalidate_sizes (icon_view);

      g_object_notify (G_OBJECT (icon_view), "margin");
    }
}



/* Get/set whether drag_motion requested the drag data and
 * drag_data_received should thus not actually insert the data,
 * since the data doesn't result from a drop.
 */
static void
set_status_pending (GdkDragContext *context,
                    GdkDragAction suggested_action)
{
  g_object_set_data (G_OBJECT (context),
                     I_ ("xfce-icon-view-status-pending"),
                     GINT_TO_POINTER (suggested_action));
}

static GdkDragAction
get_status_pending (GdkDragContext *context)
{
  return GPOINTER_TO_INT (g_object_get_data (G_OBJECT (context), I_ ("xfce-icon-view-status-pending")));
}

static void
unset_reorderable (XfceIconView *icon_view)
{
  XfceIconViewPrivate *priv = get_instance_private (icon_view);

  if (priv->reorderable)
    {
      priv->reorderable = FALSE;
      g_object_notify (G_OBJECT (icon_view), "reorderable");
    }
}

static void
clear_source_info (XfceIconView *icon_view)
{
  XfceIconViewPrivate *priv = get_instance_private (icon_view);

  if (priv->source_targets)
    gtk_target_list_unref (priv->source_targets);
  priv->source_targets = NULL;

  priv->source_set = FALSE;
}

static void
clear_dest_info (XfceIconView *icon_view)
{
  XfceIconViewPrivate *priv = get_instance_private (icon_view);

  if (priv->dest_targets)
    gtk_target_list_unref (priv->dest_targets);
  priv->dest_targets = NULL;

  priv->dest_set = FALSE;
}

static void
set_source_row (GdkDragContext *context,
                GtkTreeModel *model,
                GtkTreePath *source_row)
{
  if (source_row)
    g_object_set_data_full (G_OBJECT (context),
                            I_ ("xfce-icon-view-source-row"),
                            gtk_tree_row_reference_new (model, source_row),
                            (GDestroyNotify) gtk_tree_row_reference_free);
  else
    g_object_set_data_full (G_OBJECT (context),
                            I_ ("xfce-icon-view-source-row"),
                            NULL, NULL);
}

static GtkTreePath *
get_source_row (GdkDragContext *context)
{
  GtkTreeRowReference *ref;

  ref = g_object_get_data (G_OBJECT (context), I_ ("xfce-icon-view-source-row"));

  if (ref)
    return gtk_tree_row_reference_get_path (ref);
  else
    return NULL;
}

typedef struct
{
  GtkTreeRowReference *dest_row;
  gboolean empty_view_drop;
  gboolean drop_append_mode;
} DestRow;

static void
dest_row_free (gpointer data)
{
  DestRow *dr = (DestRow *) data;

  gtk_tree_row_reference_free (dr->dest_row);
  g_slice_free (DestRow, dr);
}

static void
set_dest_row (GdkDragContext *context,
              GtkTreeModel *model,
              GtkTreePath *dest_row,
              gboolean empty_view_drop,
              gboolean drop_append_mode)
{
  DestRow *dr;

  if (!dest_row)
    {
      g_object_set_data_full (G_OBJECT (context),
                              I_ ("xfce-icon-view-dest-row"),
                              NULL, NULL);
      return;
    }

  dr = g_slice_new0 (DestRow);

  dr->dest_row = gtk_tree_row_reference_new (model, dest_row);
  dr->empty_view_drop = empty_view_drop;
  dr->drop_append_mode = drop_append_mode;
  g_object_set_data_full (G_OBJECT (context),
                          I_ ("xfce-icon-view-dest-row"),
                          dr, (GDestroyNotify) dest_row_free);
}



static GtkTreePath *
get_dest_row (GdkDragContext *context)
{
  DestRow *dr;

  dr = g_object_get_data (G_OBJECT (context), I_ ("xfce-icon-view-dest-row"));

  if (dr)
    {
      GtkTreePath *path = NULL;

      if (dr->dest_row)
        path = gtk_tree_row_reference_get_path (dr->dest_row);
      else if (dr->empty_view_drop)
        path = gtk_tree_path_new_from_indices (0, -1);
      else
        path = NULL;

      if (path && dr->drop_append_mode)
        gtk_tree_path_next (path);

      return path;
    }
  else
    return NULL;
}



static gboolean
check_model_dnd (GtkTreeModel *model,
                 GType required_iface,
                 const gchar *_signal)
{
  if (model == NULL || !G_TYPE_CHECK_INSTANCE_TYPE ((model), required_iface))
    {
      g_warning ("You must override the default '%s' handler "
                 "on XfceIconView when using models that don't support "
                 "the %s interface and enabling drag-and-drop. The simplest way to do this "
                 "is to connect to '%s' and call "
                 "g_signal_stop_emission_by_name() in your signal handler to prevent "
                 "the default handler from running. Look at the source code "
                 "for the default handler in gtkiconview.c to get an idea what "
                 "your handler should do. (gtkiconview.c is in the GTK+ source "
                 "code.) If you're using GTK+ from a language other than C, "
                 "there may be a more natural way to override default handlers, e.g. via derivation.",
                 _signal, g_type_name (required_iface), _signal);
      return FALSE;
    }
  else
    return TRUE;
}



static void
remove_scroll_timeout (XfceIconView *icon_view)
{
  XfceIconViewPrivate *priv = get_instance_private (icon_view);

  if (priv->scroll_timeout_id != 0)
    {
      g_source_remove (priv->scroll_timeout_id);

      priv->scroll_timeout_id = 0;
    }
}



static void
xfce_icon_view_autoscroll (XfceIconView *icon_view)
{
  XfceIconViewPrivate *priv = get_instance_private (icon_view);
  gint px, py, x, y, width, height;
  gint hoffset, voffset;
  gfloat value;

  GdkSeat *seat;
  GdkDevice *pointer_dev;

  seat = gdk_display_get_default_seat (gdk_window_get_display (gtk_widget_get_window (GTK_WIDGET (icon_view))));
  pointer_dev = gdk_seat_get_pointer (seat);

  gdk_window_get_device_position (gtk_widget_get_window (GTK_WIDGET (icon_view)), pointer_dev, &px, &py, NULL);
  gdk_window_get_geometry (gtk_widget_get_window (GTK_WIDGET (icon_view)), &x, &y, &width, &height);

  /* see if we are near the edge. */
  voffset = py - (y + 2 * SCROLL_EDGE_SIZE);
  if (voffset > 0)
    voffset = MAX (py - (y + height - 2 * SCROLL_EDGE_SIZE), 0);

  hoffset = px - (x + 2 * SCROLL_EDGE_SIZE);
  if (hoffset > 0)
    hoffset = MAX (px - (x + width - 2 * SCROLL_EDGE_SIZE), 0);

  if (voffset != 0)
    {
      value = CLAMP (gtk_adjustment_get_value (priv->vadjustment) + voffset,
                     gtk_adjustment_get_lower (priv->vadjustment),
                     gtk_adjustment_get_upper (priv->vadjustment)
                       - gtk_adjustment_get_page_size (priv->vadjustment));
      gtk_adjustment_set_value (priv->vadjustment, value);
    }
  if (hoffset != 0)
    {
      value = CLAMP (gtk_adjustment_get_value (priv->hadjustment) + hoffset,
                     gtk_adjustment_get_lower (priv->hadjustment),
                     gtk_adjustment_get_upper (priv->hadjustment)
                       - gtk_adjustment_get_page_size (priv->hadjustment));
      gtk_adjustment_set_value (priv->hadjustment, value);
    }
}


static gboolean
drag_scroll_timeout (gpointer data)
{
  XfceIconView *icon_view = XFCE_ICON_VIEW (data);

  xfce_icon_view_autoscroll (icon_view);

  return TRUE;
}


static gboolean
set_destination (XfceIconView *icon_view,
                 GdkDragContext *context,
                 gint x,
                 gint y,
                 GdkDragAction *suggested_action,
                 GdkAtom *target)
{
  XfceIconViewPrivate *priv = get_instance_private (icon_view);
  GtkWidget *widget;
  GtkTreePath *path = NULL;
  XfceIconViewDropPosition pos;
  XfceIconViewDropPosition old_pos;
  GtkTreePath *old_dest_path = NULL;
  gboolean can_drop = FALSE;

  widget = GTK_WIDGET (icon_view);

  *suggested_action = 0;
  *target = GDK_NONE;

  if (!priv->dest_set)
    {
      /* someone unset us as a drag dest, note that if
       * we return FALSE drag_leave isn't called
       */

      xfce_icon_view_set_drag_dest_item (icon_view,
                                         NULL,
                                         XFCE_ICON_VIEW_DROP_LEFT);

      remove_scroll_timeout (XFCE_ICON_VIEW (widget));

      return FALSE; /* no longer a drop site */
    }

  *target = gtk_drag_dest_find_target (widget, context, priv->dest_targets);
  if (*target == GDK_NONE)
    return FALSE;

  if (!xfce_icon_view_get_dest_item_at_pos (icon_view, x, y, &path, &pos))
    {
      gint n_children;
      GtkTreeModel *model;

      /* the row got dropped on empty space, let's setup a special case
       */

      if (path)
        gtk_tree_path_free (path);

      model = xfce_icon_view_get_model (icon_view);

      n_children = gtk_tree_model_iter_n_children (model, NULL);
      if (n_children)
        {
          pos = XFCE_ICON_VIEW_DROP_BELOW;
          path = gtk_tree_path_new_from_indices (n_children - 1, -1);
        }
      else
        {
          pos = XFCE_ICON_VIEW_DROP_ABOVE;
          path = gtk_tree_path_new_from_indices (0, -1);
        }

      can_drop = TRUE;

      goto out;
    }

  g_assert (path);

  xfce_icon_view_get_drag_dest_item (icon_view,
                                     &old_dest_path,
                                     &old_pos);

  if (old_dest_path)
    gtk_tree_path_free (old_dest_path);

  if (TRUE /* FIXME if the location droppable predicate */)
    {
      can_drop = TRUE;
    }

out:
  if (can_drop)
    {
      GtkWidget *source_widget;

      *suggested_action = gdk_drag_context_get_suggested_action (context);
      source_widget = gtk_drag_get_source_widget (context);

      if (source_widget == widget)
        {
          /* Default to MOVE, unless the user has
           * pressed ctrl or shift to affect available actions
           */
          if ((gdk_drag_context_get_actions (context) & GDK_ACTION_MOVE) != 0)
            *suggested_action = GDK_ACTION_MOVE;
        }

      xfce_icon_view_set_drag_dest_item (XFCE_ICON_VIEW (widget),
                                         path, pos);
    }
  else
    {
      /* can't drop here */
      xfce_icon_view_set_drag_dest_item (XFCE_ICON_VIEW (widget),
                                         NULL,
                                         XFCE_ICON_VIEW_DROP_LEFT);
    }

  if (path)
    gtk_tree_path_free (path);

  return TRUE;
}

static GtkTreePath *
get_logical_destination (XfceIconView *icon_view,
                         gboolean *drop_append_mode)
{
  /* adjust path to point to the row the drop goes in front of */
  XfceIconViewPrivate *priv = get_instance_private (icon_view);
  GtkTreePath *path = NULL;
  XfceIconViewDropPosition pos;

  *drop_append_mode = FALSE;

  xfce_icon_view_get_drag_dest_item (icon_view, &path, &pos);

  if (path == NULL)
    return NULL;

  if (pos == XFCE_ICON_VIEW_DROP_RIGHT || pos == XFCE_ICON_VIEW_DROP_BELOW)
    {
      GtkTreeIter iter;
      GtkTreeModel *model = priv->model;

      if (!gtk_tree_model_get_iter (model, &iter, path) || !gtk_tree_model_iter_next (model, &iter))
        *drop_append_mode = TRUE;
      else
        {
          *drop_append_mode = FALSE;
          gtk_tree_path_next (path);
        }
    }

  return path;
}

static gboolean
xfce_icon_view_maybe_begin_drag (XfceIconView *icon_view,
                                 GdkEventMotion *event)
{
  XfceIconViewPrivate *priv = get_instance_private (icon_view);
  GdkDragContext *context;
  GtkTreePath *path = NULL;
  gint button;
  GtkTreeModel *model;
  gboolean retval = FALSE;

  if (!priv->source_set)
    goto out;

  if (priv->pressed_button < 0)
    goto out;

  if (!gtk_drag_check_threshold (GTK_WIDGET (icon_view),
                                 priv->press_start_x,
                                 priv->press_start_y,
                                 event->x, event->y))
    goto out;

  model = xfce_icon_view_get_model (icon_view);

  if (model == NULL)
    goto out;

  button = priv->pressed_button;
  priv->pressed_button = -1;

  path = xfce_icon_view_get_path_at_pos (icon_view,
                                         priv->press_start_x,
                                         priv->press_start_y);

  if (path == NULL)
    goto out;

  if (!GTK_IS_TREE_DRAG_SOURCE (model)
      || !gtk_tree_drag_source_row_draggable (GTK_TREE_DRAG_SOURCE (model), path))
    goto out;

  /* FIXME Check whether we're a start button, if not return FALSE and
   * free path
   */

  /* Now we can begin the drag */

  retval = TRUE;

  context = gtk_drag_begin_with_coordinates (GTK_WIDGET (icon_view),
                                             priv->source_targets,
                                             priv->source_actions,
                                             button,
                                             (GdkEvent *) event,
                                             event->x, event->y);

  set_source_row (context, model, path);

out:
  if (path)
    gtk_tree_path_free (path);

  return retval;
}

/* Source side drag signals */
static void
xfce_icon_view_drag_begin (GtkWidget *widget,
                           GdkDragContext *context)
{
  XfceIconView *icon_view = XFCE_ICON_VIEW (widget);
  XfceIconViewPrivate *priv = get_instance_private (icon_view);
  XfceIconViewItem *item;
  cairo_surface_t *icon;
  GtkTreePath *path;

  /* if the user uses a custom DnD impl, we don't set the icon here */
  if (!priv->dest_set && !priv->source_set)
    return;

  item = xfce_icon_view_get_item_at_coords (icon_view,
                                            priv->press_start_x,
                                            priv->press_start_y,
                                            TRUE,
                                            NULL);

  g_return_if_fail (item != NULL);

  path = gtk_tree_path_new_from_indices (g_sequence_iter_get_position (item->item_iter), -1);
  icon = xfce_icon_view_create_drag_icon (icon_view, path);
  gtk_tree_path_free (path);

  gtk_drag_set_icon_surface (context, icon);

  g_object_unref (icon);
}

static void
xfce_icon_view_drag_end (GtkWidget *widget,
                         GdkDragContext *context)
{
  XfceIconView *icon_view = XFCE_ICON_VIEW (widget);
  XfceIconViewPrivate *priv = get_instance_private (icon_view);

  /* check if we're in single click mode */
  if (G_UNLIKELY (priv->single_click))
    {
      /* reset the cursor if we're still realized */
      if (G_LIKELY (priv->bin_window != NULL))
        gdk_window_set_cursor (priv->bin_window, NULL);

      /* reset the last single clicked item */
      priv->last_single_clicked = NULL;
    }

  /* reset the pressed_button state */
  priv->pressed_button = -1;
}

static void
xfce_icon_view_drag_data_get (GtkWidget *widget,
                              GdkDragContext *context,
                              GtkSelectionData *selection_data,
                              guint info,
                              guint drag_time)
{
  XfceIconView *icon_view = XFCE_ICON_VIEW (widget);
  XfceIconViewPrivate *priv = get_instance_private (icon_view);
  GtkTreeModel *model;
  GtkTreePath *source_row;

  model = xfce_icon_view_get_model (icon_view);

  if (model == NULL)
    return;

  if (!priv->dest_set)
    return;

  source_row = get_source_row (context);

  if (source_row == NULL)
    return;

  /* We can implement the GTK_TREE_MODEL_ROW target generically for
   * any model; for DragSource models there are some other targets
   * we also support.
   */

  if (GTK_IS_TREE_DRAG_SOURCE (model)
      && gtk_tree_drag_source_drag_data_get (GTK_TREE_DRAG_SOURCE (model), source_row, selection_data))
    goto done;

  /* If drag_data_get does nothing, try providing row data. */
  if (gtk_selection_data_get_target (selection_data) == gdk_atom_intern ("GTK_TREE_MODEL_ROW", FALSE))
    gtk_tree_set_row_drag_data (selection_data,
                                model,
                                source_row);

done:
  gtk_tree_path_free (source_row);
}

static void
xfce_icon_view_drag_data_delete (GtkWidget *widget,
                                 GdkDragContext *context)
{
  XfceIconView *icon_view = XFCE_ICON_VIEW (widget);
  XfceIconViewPrivate *priv = get_instance_private (icon_view);
  GtkTreeModel *model = xfce_icon_view_get_model (icon_view);
  GtkTreePath *source_row;

  if (!check_model_dnd (model, GTK_TYPE_TREE_DRAG_SOURCE, "drag_data_delete"))
    return;

  if (!priv->dest_set)
    return;

  source_row = get_source_row (context);

  if (source_row == NULL)
    return;

  gtk_tree_drag_source_drag_data_delete (GTK_TREE_DRAG_SOURCE (model),
                                         source_row);

  gtk_tree_path_free (source_row);

  set_source_row (context, NULL, NULL);
}

/* Target side drag signals */
static void
xfce_icon_view_drag_leave (GtkWidget *widget,
                           GdkDragContext *context,
                           guint drag_time)
{
  XfceIconView *icon_view;

  icon_view = XFCE_ICON_VIEW (widget);

  /* unset any highlight row */
  xfce_icon_view_set_drag_dest_item (icon_view,
                                     NULL,
                                     XFCE_ICON_VIEW_DROP_LEFT);

  remove_scroll_timeout (icon_view);
}

static gboolean
xfce_icon_view_drag_motion (GtkWidget *widget,
                            GdkDragContext *context,
                            gint x,
                            gint y,
                            guint drag_time)
{
  XfceIconViewDropPosition pos;
  GdkDragAction suggested_action = 0;
  GtkTreePath *path = NULL;
  XfceIconView *icon_view = XFCE_ICON_VIEW (widget);
  XfceIconViewPrivate *priv = get_instance_private (icon_view);
  gboolean empty;
  GdkAtom target;

  if (!set_destination (icon_view, context, x, y, &suggested_action, &target))
    return FALSE;

  xfce_icon_view_get_drag_dest_item (icon_view, &path, &pos);

  /* we only know this *after* set_desination_row */
  empty = priv->empty_view_drop;

  if (path == NULL && !empty)
    {
      /* Can't drop here. */
      gdk_drag_status (context, 0, drag_time);
    }
  else
    {
      if (priv->scroll_timeout_id == 0)
        priv->scroll_timeout_id = gdk_threads_add_timeout (50, drag_scroll_timeout, icon_view);

      if (target == gdk_atom_intern ("GTK_TREE_MODEL_ROW", FALSE))
        {
          /* Request data so we can use the source row when
           * determining whether to accept the drop
           */
          set_status_pending (context, suggested_action);
          gtk_drag_get_data (widget, context, target, drag_time);
        }
      else
        {
          set_status_pending (context, 0);
          gdk_drag_status (context, suggested_action, drag_time);
        }
    }

  if (path != NULL)
    gtk_tree_path_free (path);

  return TRUE;
}

static gboolean
xfce_icon_view_drag_drop (GtkWidget *widget,
                          GdkDragContext *context,
                          gint x,
                          gint y,
                          guint drag_time)
{
  XfceIconView *icon_view = XFCE_ICON_VIEW (widget);
  XfceIconViewPrivate *priv = get_instance_private (icon_view);
  GtkTreePath *path;
  GdkDragAction suggested_action = 0;
  GdkAtom target = GDK_NONE;
  GtkTreeModel *model;
  gboolean drop_append_mode;

  model = xfce_icon_view_get_model (icon_view);

  remove_scroll_timeout (XFCE_ICON_VIEW (widget));

  if (!priv->dest_set)
    return FALSE;

  if (!check_model_dnd (model, GTK_TYPE_TREE_DRAG_DEST, "drag_drop"))
    return FALSE;

  if (!set_destination (icon_view, context, x, y, &suggested_action, &target))
    return FALSE;

  path = get_logical_destination (icon_view, &drop_append_mode);

  if (target != GDK_NONE && path != NULL)
    {
      /* in case a motion had requested drag data, change things so we
       * treat drag data receives as a drop.
       */
      set_status_pending (context, 0);
      set_dest_row (context, model, path,
                    priv->empty_view_drop, drop_append_mode);
    }

  if (path)
    gtk_tree_path_free (path);

  /* Unset this thing */
  xfce_icon_view_set_drag_dest_item (icon_view, NULL, XFCE_ICON_VIEW_DROP_LEFT);

  if (target != GDK_NONE)
    {
      gtk_drag_get_data (widget, context, target, drag_time);
      return TRUE;
    }
  else
    return FALSE;
}

static void
xfce_icon_view_drag_data_received (GtkWidget *widget,
                                   GdkDragContext *context,
                                   gint x,
                                   gint y,
                                   GtkSelectionData *selection_data,
                                   guint info,
                                   guint drag_time)
{
  GtkTreePath *path;
  gboolean accepted = FALSE;
  GtkTreeModel *model;
  XfceIconView *icon_view = XFCE_ICON_VIEW (widget);
  XfceIconViewPrivate *priv = get_instance_private (icon_view);
  GtkTreePath *dest_row;
  GdkDragAction suggested_action;
  gboolean drop_append_mode;

  model = xfce_icon_view_get_model (icon_view);

  if (!check_model_dnd (model, GTK_TYPE_TREE_DRAG_DEST, "drag_data_received"))
    return;

  if (!priv->dest_set)
    return;

  suggested_action = get_status_pending (context);

  if (suggested_action)
    {
      /* We are getting this data due to a request in drag_motion,
       * rather than due to a request in drag_drop, so we are just
       * supposed to call drag_status, not actually paste in the
       * data.
       */
      path = get_logical_destination (icon_view, &drop_append_mode);

      if (path == NULL)
        suggested_action = 0;

      if (suggested_action)
        {
          if (!gtk_tree_drag_dest_row_drop_possible (GTK_TREE_DRAG_DEST (model),
                                                     path,
                                                     selection_data))
            suggested_action = 0;
        }

      gdk_drag_status (context, suggested_action, drag_time);

      if (path)
        gtk_tree_path_free (path);

      /* If you can't drop, remove user drop indicator until the next motion */
      if (suggested_action == 0)
        xfce_icon_view_set_drag_dest_item (icon_view,
                                           NULL,
                                           XFCE_ICON_VIEW_DROP_LEFT);
      return;
    }


  dest_row = get_dest_row (context);

  if (dest_row == NULL)
    return;

  if (gtk_selection_data_get_length (selection_data) >= 0)
    {
      if (gtk_tree_drag_dest_drag_data_received (GTK_TREE_DRAG_DEST (model),
                                                 dest_row,
                                                 selection_data))
        accepted = TRUE;
    }

  gtk_drag_finish (context,
                   accepted,
                   (gdk_drag_context_get_selected_action (context) == GDK_ACTION_MOVE),
                   drag_time);

  gtk_tree_path_free (dest_row);

  /* drop dest_row */
  set_dest_row (context, NULL, NULL, FALSE, FALSE);
}



/**
 * xfce_icon_view_enable_model_drag_source:
 * @icon_view         : a #GtkIconTreeView
 * @start_button_mask : Mask of allowed buttons to start drag
 * @targets           : the table of targets that the drag will support
 * @n_targets         : the number of items in @targets
 * @actions           : the bitmask of possible actions for a drag from this widget
 *
 * Turns @icon_view into a drag source for automatic DND.
 *
 * Since: 4.21.0
 **/
void
xfce_icon_view_enable_model_drag_source (XfceIconView *icon_view,
                                         GdkModifierType start_button_mask,
                                         const GtkTargetEntry *targets,
                                         gint n_targets,
                                         GdkDragAction actions)
{
  XfceIconViewPrivate *priv = get_instance_private (icon_view);

  g_return_if_fail (XFCE_IS_ICON_VIEW (icon_view));

  gtk_drag_source_set (GTK_WIDGET (icon_view), 0, NULL, 0, actions);

  clear_source_info (icon_view);
  priv->start_button_mask = start_button_mask;
  priv->source_targets = gtk_target_list_new (targets, n_targets);
  priv->source_actions = actions;

  priv->source_set = TRUE;

  unset_reorderable (icon_view);
}



/**
 * xfce_icon_view_enable_model_drag_dest:
 * @icon_view : a #XfceIconView
 * @targets   : the table of targets that the drag will support
 * @n_targets : the number of items in @targets
 * @actions   : the bitmask of possible actions for a drag from this widget
 *
 * Turns @icon_view into a drop destination for automatic DND.
 *
 * Since: 4.21.0
 **/
void
xfce_icon_view_enable_model_drag_dest (XfceIconView *icon_view,
                                       const GtkTargetEntry *targets,
                                       gint n_targets,
                                       GdkDragAction actions)
{
  XfceIconViewPrivate *priv = get_instance_private (icon_view);

  g_return_if_fail (XFCE_IS_ICON_VIEW (icon_view));

  gtk_drag_dest_set (GTK_WIDGET (icon_view), 0, NULL, 0, actions);

  clear_dest_info (icon_view);

  priv->dest_targets = gtk_target_list_new (targets, n_targets);
  priv->dest_actions = actions;

  priv->dest_set = TRUE;

  unset_reorderable (icon_view);
}



/**
 * xfce_icon_view_unset_model_drag_source:
 * @icon_view : a #XfceIconView
 *
 * Undoes the effect of #xfce_icon_view_enable_model_drag_source().
 *
 * Since: 4.21.0
 **/
void
xfce_icon_view_unset_model_drag_source (XfceIconView *icon_view)
{
  XfceIconViewPrivate *priv = get_instance_private (icon_view);

  g_return_if_fail (XFCE_IS_ICON_VIEW (icon_view));

  if (priv->source_set)
    {
      gtk_drag_source_unset (GTK_WIDGET (icon_view));
      clear_source_info (icon_view);
    }

  unset_reorderable (icon_view);
}



/**
 * xfce_icon_view_unset_model_drag_dest:
 * @icon_view : a #XfceIconView
 *
 * Undoes the effect of #xfce_icon_view_enable_model_drag_dest().
 *
 * Since: 4.21.0
 **/
void
xfce_icon_view_unset_model_drag_dest (XfceIconView *icon_view)
{
  XfceIconViewPrivate *priv = get_instance_private (icon_view);

  g_return_if_fail (XFCE_IS_ICON_VIEW (icon_view));

  if (priv->dest_set)
    {
      gtk_drag_dest_unset (GTK_WIDGET (icon_view));
      clear_dest_info (icon_view);
    }

  unset_reorderable (icon_view);
}



/**
 * xfce_icon_view_set_drag_dest_item:
 * @icon_view : a #XfceIconView
 * @path      : The path of the item to highlight, or %NULL.
 * @pos       : Specifies whether to drop, relative to the item
 *
 * Sets the item that is highlighted for feedback.
 *
 * Since: 4.21.0
 */
void
xfce_icon_view_set_drag_dest_item (XfceIconView *icon_view,
                                   GtkTreePath *path,
                                   XfceIconViewDropPosition pos)
{
  XfceIconViewPrivate *priv = get_instance_private (icon_view);
  XfceIconViewItem *item;
  GtkTreePath *previous_path;
  GSequenceIter *iter;

  g_return_if_fail (XFCE_IS_ICON_VIEW (icon_view));

  /* Note; this function is exported to allow a custom DND
   * implementation, so it can't touch TreeViewDragInfo
   */

  if (priv->dest_item != NULL)
    {
      /* determine and reset the previous path */
      previous_path = gtk_tree_row_reference_get_path (priv->dest_item);
      gtk_tree_row_reference_free (priv->dest_item);
      priv->dest_item = NULL;

      /* check if the path is still valid */
      if (G_LIKELY (previous_path != NULL))
        {
          /* schedule a redraw for the previous path */
          iter = g_sequence_get_iter_at_pos (priv->items, gtk_tree_path_get_indices (previous_path)[0]);
          if (!g_sequence_iter_is_end (iter))
            {
              item = g_sequence_get (iter);
              if (G_LIKELY (item != NULL))
                xfce_icon_view_queue_draw_item (icon_view, item);
            }
          gtk_tree_path_free (previous_path);
        }
    }

  /* special case a drop on an empty model */
  priv->empty_view_drop = FALSE;
  if (pos == XFCE_ICON_VIEW_NO_DROP
      && path != NULL
      && gtk_tree_path_get_depth (path) == 1
      && gtk_tree_path_get_indices (path)[0] == 0)
    {
      gint n_children;

      n_children = gtk_tree_model_iter_n_children (priv->model,
                                                   NULL);

      if (n_children == 0)
        priv->empty_view_drop = TRUE;
    }

  priv->dest_pos = pos;

  if (G_LIKELY (path != NULL))
    {
      /* take a row reference for the new item path */
      priv->dest_item =
        gtk_tree_row_reference_new_proxy (G_OBJECT (icon_view),
                                          priv->model,
                                          path);

      /* schedule a redraw on the new path */
      iter = g_sequence_get_iter_at_pos (priv->items, gtk_tree_path_get_indices (path)[0]);
      if (g_sequence_iter_is_end (iter))
        return;

      item = g_sequence_get (iter);
      if (G_LIKELY (item != NULL))
        xfce_icon_view_queue_draw_item (icon_view, item);
    }
}



/**
 * xfce_icon_view_get_drag_dest_item:
 * @icon_view : a #XfceIconView
 * @path      : Return location for the path of the highlighted item, or %NULL.
 * @pos       : Return location for the drop position, or %NULL
 *
 * Gets information about the item that is highlighted for feedback.
 *
 * Since: 4.21.0
 **/
void
xfce_icon_view_get_drag_dest_item (XfceIconView *icon_view,
                                   GtkTreePath **path,
                                   XfceIconViewDropPosition *pos)
{
  XfceIconViewPrivate *priv = get_instance_private (icon_view);

  g_return_if_fail (XFCE_IS_ICON_VIEW (icon_view));

  if (path)
    {
      if (priv->dest_item)
        *path = gtk_tree_row_reference_get_path (priv->dest_item);
      else
        *path = NULL;
    }

  if (pos)
    *pos = priv->dest_pos;
}



/**
 * xfce_icon_view_get_dest_item_at_pos:
 * @icon_view : a #XfceIconView
 * @drag_x    : the position to determine the destination item for
 * @drag_y    : the position to determine the destination item for
 * @path      : Return location for the path of the highlighted item, or %NULL.
 * @pos       : Return location for the drop position, or %NULL
 *
 * Determines the destination item for a given position.
 *
 * Both @drag_x and @drag_y are given in icon window coordinates. Use
 * #xfce_icon_view_widget_to_icon_coords() if you need to translate
 * widget coordinates first.
 *
 * Returns: whether there is an item at the given position.
 *
 * Since: 4.21.0
 **/
gboolean
xfce_icon_view_get_dest_item_at_pos (XfceIconView *icon_view,
                                     gint drag_x,
                                     gint drag_y,
                                     GtkTreePath **path,
                                     XfceIconViewDropPosition *pos)
{
  XfceIconViewPrivate *priv = get_instance_private (icon_view);
  XfceIconViewItem *item;

  /* Note; this function is exported to allow a custom DND
   * implementation, so it can't touch TreeViewDragInfo
   */

  g_return_val_if_fail (XFCE_IS_ICON_VIEW (icon_view), FALSE);
  g_return_val_if_fail (drag_x >= 0, FALSE);
  g_return_val_if_fail (drag_y >= 0, FALSE);
  g_return_val_if_fail (priv->bin_window != NULL, FALSE);

  if (G_LIKELY (path != NULL))
    *path = NULL;

  item = xfce_icon_view_get_item_at_coords (icon_view, drag_x, drag_y, FALSE, NULL);

  if (G_UNLIKELY (item == NULL))
    return FALSE;

  if (G_LIKELY (path != NULL))
    *path = gtk_tree_path_new_from_indices (g_sequence_iter_get_position (item->item_iter), -1);

  if (G_LIKELY (pos != NULL))
    {
      if (drag_x < item->area.x + item->area.width / 4)
        *pos = XFCE_ICON_VIEW_DROP_LEFT;
      else if (drag_x > item->area.x + item->area.width * 3 / 4)
        *pos = XFCE_ICON_VIEW_DROP_RIGHT;
      else if (drag_y < item->area.y + item->area.height / 4)
        *pos = XFCE_ICON_VIEW_DROP_ABOVE;
      else if (drag_y > item->area.y + item->area.height * 3 / 4)
        *pos = XFCE_ICON_VIEW_DROP_BELOW;
      else
        *pos = XFCE_ICON_VIEW_DROP_INTO;
    }

  return TRUE;
}



/**
 * xfce_icon_view_create_drag_icon:
 * @icon_view : a #XfceIconView
 * @path      : a #GtkTreePath in @icon_view
 *
 * Creates a #cairo_surface_t representation of the item at @path.
 * This image is used for a drag icon.
 *
 * Returns: (transfer full): a newly-allocated pixmap of the drag icon.
 *
 * Since: 4.21.0
 **/
cairo_surface_t *
xfce_icon_view_create_drag_icon (XfceIconView *icon_view,
                                 GtkTreePath *path)
{
  XfceIconViewPrivate *priv = get_instance_private (icon_view);
  cairo_surface_t *surface;
  cairo_t *cr;
  gint idx;
  XfceIconViewItem *item;
  GSequenceIter *iter;
  g_return_val_if_fail (XFCE_IS_ICON_VIEW (icon_view), NULL);
  g_return_val_if_fail (gtk_tree_path_get_depth (path) > 0, NULL);

  /* verify that the widget is realized */
  if (G_UNLIKELY (!gtk_widget_get_realized (GTK_WIDGET (icon_view))))
    return NULL;

  idx = gtk_tree_path_get_indices (path)[0];

  for (iter = g_sequence_get_begin_iter (priv->items);
       !g_sequence_iter_is_end (iter);
       iter = g_sequence_iter_next (iter))
    {
      item = g_sequence_get (iter);
      if (G_UNLIKELY (idx == g_sequence_iter_get_position (item->item_iter)))
        {
          surface = cairo_image_surface_create (CAIRO_FORMAT_ARGB32,
                                                item->area.width + 2,
                                                item->area.height + 2);

          cr = cairo_create (surface);

          /* TODO: background / rectangles */

          xfce_icon_view_paint_item (icon_view, item, cr, 1, 1, FALSE);

          cairo_destroy (cr);

          return surface;
        }
    }

  return NULL;
}



/**
 * xfce_icon_view_set_pixbuf_column:
 * @icon_view : a #XfceIconView
 * @column    : The column that contains the pixbuf to render.
 *
 * Sets the column that contains the pixbuf to render.
 *
 * Since: 4.21.0
 **/
void
xfce_icon_view_set_pixbuf_column (XfceIconView *icon_view, gint column)
{
  XfceIconViewPrivate *priv = get_instance_private (icon_view);

  g_return_if_fail (XFCE_IS_ICON_VIEW (icon_view));

  priv->pixbuf_column = column;

  update_pixbuf_cell (icon_view);
}



/**
 * xfce_icon_view_set_icon_column:
 * @icon_view : a #XfceIconView
 * @column    : The column that contains file to render.
 *
 * Sets the column that contains the file to render.
 *
 * Since: 4.21.0
 **/
void
xfce_icon_view_set_icon_column (XfceIconView *icon_view, gint column)
{
  XfceIconViewPrivate *priv = get_instance_private (icon_view);

  g_return_if_fail (XFCE_IS_ICON_VIEW (icon_view));

  priv->icon_column = column;

  update_pixbuf_cell (icon_view);
}



/**
 * xfce_icon_view_get_reorderable:
 * @icon_view : a #XfceIconView
 *
 * Retrieves whether the user can reorder the list via drag-and-drop.
 * See xfce_icon_view_set_reorderable().
 *
 * Returns: %TRUE if the list can be reordered.
 *
 * Since: 4.21.0
 **/
gboolean
xfce_icon_view_get_reorderable (XfceIconView *icon_view)
{
  XfceIconViewPrivate *priv = get_instance_private (icon_view);

  g_return_val_if_fail (XFCE_IS_ICON_VIEW (icon_view), FALSE);

  return priv->reorderable;
}



/**
 * xfce_icon_view_set_reorderable:
 * @icon_view   : A #XfceIconView.
 * @reorderable : %TRUE, if the list of items can be reordered.
 *
 * This function is a convenience function to allow you to reorder models that
 * support the #GtkTreeDragSourceIface and the #GtkTreeDragDestIface.  Both
 * #GtkTreeStore and #GtkListStore support these.  If @reorderable is %TRUE, then
 * the user can reorder the model by dragging and dropping rows.  The
 * developer can listen to these changes by connecting to the model's
 * ::row-inserted and ::row-deleted signals.
 *
 * This function does not give you any degree of control over the order -- any
 * reordering is allowed.  If more control is needed, you should probably
 * handle drag and drop manually.
 *
 * Since: 4.21.0
 **/
void
xfce_icon_view_set_reorderable (XfceIconView *icon_view,
                                gboolean reorderable)
{
  XfceIconViewPrivate *priv = get_instance_private (icon_view);
  static const GtkTargetEntry item_targets[] = {
    {
      "GTK_TREE_MODEL_ROW",
      GTK_TARGET_SAME_WIDGET,
      0,
    },
  };

  g_return_if_fail (XFCE_IS_ICON_VIEW (icon_view));

  reorderable = (reorderable != FALSE);

  if (G_UNLIKELY (priv->reorderable == reorderable))
    return;

  if (G_LIKELY (reorderable))
    {
      xfce_icon_view_enable_model_drag_source (icon_view, GDK_BUTTON1_MASK, item_targets,
                                               G_N_ELEMENTS (item_targets), GDK_ACTION_MOVE);
      xfce_icon_view_enable_model_drag_dest (icon_view, item_targets,
                                             G_N_ELEMENTS (item_targets), GDK_ACTION_MOVE);
    }
  else
    {
      xfce_icon_view_unset_model_drag_source (icon_view);
      xfce_icon_view_unset_model_drag_dest (icon_view);
    }

  priv->reorderable = reorderable;

  g_object_notify (G_OBJECT (icon_view), "reorderable");
}



/*----------------------*
 * Single-click support *
 *----------------------*/

/**
 * xfce_icon_view_get_single_click:
 * @icon_view : a #XfceIconView.
 *
 * Returns %TRUE if @icon_view is currently in single click mode,
 * else %FALSE will be returned.
 *
 * Returns: whether @icon_view is currently in single click mode.
 *
 * Since: 4.21.0
 **/
gboolean
xfce_icon_view_get_single_click (XfceIconView *icon_view)
{
  XfceIconViewPrivate *priv = get_instance_private (icon_view);
  g_return_val_if_fail (XFCE_IS_ICON_VIEW (icon_view), FALSE);
  return priv->single_click;
}



/**
 * xfce_icon_view_set_single_click:
 * @icon_view    : a #XfceIconView.
 * @single_click : %TRUE for single click, %FALSE for double click mode.
 *
 * If @single_click is %TRUE, @icon_view will be in single click mode
 * afterwards, else @icon_view will be in double click mode.
 *
 * Since: 4.21.0
 **/
void
xfce_icon_view_set_single_click (XfceIconView *icon_view,
                                 gboolean single_click)
{
  XfceIconViewPrivate *priv = get_instance_private (icon_view);

  g_return_if_fail (XFCE_IS_ICON_VIEW (icon_view));

  /* normalize the value */
  single_click = !!single_click;

  /* check if we have a new setting here */
  if (priv->single_click != single_click)
    {
      priv->single_click = single_click;
      g_object_notify (G_OBJECT (icon_view), "single-click");
    }
}



/**
 * xfce_icon_view_get_single_click_timeout:
 * @icon_view : a #XfceIconView.
 *
 * Returns the amount of time in milliseconds after which the
 * item under the mouse cursor will be selected automatically
 * in single click mode. A value of %0 means that the behavior
 * is disabled and the user must alter the selection manually.
 *
 * Returns: the single click autoselect timeout or %0 if
 *          the behavior is disabled.
 *
 * Since: 4.21.0
 **/
guint
xfce_icon_view_get_single_click_timeout (XfceIconView *icon_view)
{
  XfceIconViewPrivate *priv = get_instance_private (icon_view);
  g_return_val_if_fail (XFCE_IS_ICON_VIEW (icon_view), 0u);
  return priv->single_click_timeout;
}



/**
 * xfce_icon_view_set_single_click_timeout:
 * @icon_view            : a #XfceIconView.
 * @single_click_timeout : the new timeout or %0 to disable.
 *
 * If @single_click_timeout is a value greater than zero, it specifies
 * the amount of time in milliseconds after which the item under the
 * mouse cursor will be selected automatically in single click mode.
 * A value of %0 for @single_click_timeout disables the autoselection
 * for @icon_view.
 *
 * This setting does not have any effect unless the @icon_view is in
 * single-click mode, see xfce_icon_view_set_single_click().
 *
 * Since: 4.21.0
 **/
void
xfce_icon_view_set_single_click_timeout (XfceIconView *icon_view,
                                         guint single_click_timeout)
{
  XfceIconViewPrivate *priv = get_instance_private (icon_view);

  g_return_if_fail (XFCE_IS_ICON_VIEW (icon_view));

  /* check if we have a new setting */
  if (priv->single_click_timeout != single_click_timeout)
    {
      /* apply the new setting */
      priv->single_click_timeout = single_click_timeout;

      /* be sure to cancel any pending single click timeout */
      if (G_UNLIKELY (priv->single_click_timeout_id != 0))
        g_source_remove (priv->single_click_timeout_id);

      /* notify listeners */
      g_object_notify (G_OBJECT (icon_view), "single-click-timeout");
    }
}



static gboolean
xfce_icon_view_single_click_timeout (gpointer user_data)
{
  XfceIconViewItem *item;
  gboolean dirty = FALSE;
  XfceIconView *icon_view = XFCE_ICON_VIEW (user_data);
  XfceIconViewPrivate *priv = get_instance_private (icon_view);
  GtkWidget *toplevel = gtk_widget_get_toplevel (GTK_WIDGET (icon_view));

  /* verify that we are in single-click mode on an active window and have a prelit item */
  if (GTK_IS_WINDOW (toplevel)
      && gtk_window_is_active (GTK_WINDOW (toplevel))
      && priv->single_click
      && priv->prelit_item != NULL)
    {
      gtk_widget_grab_focus (GTK_WIDGET (icon_view));

      /* work on the prelit item */
      item = priv->prelit_item;

      /* be sure the item is fully visible */
      xfce_icon_view_scroll_to_item (icon_view, item);

      /* change the selection appropriately */
      if (G_UNLIKELY (priv->selection_mode == GTK_SELECTION_NONE))
        {
          xfce_icon_view_set_cursor_item (icon_view, item, -1);
        }
      else if ((priv->single_click_timeout_state & GDK_SHIFT_MASK) != 0
               && priv->selection_mode == GTK_SELECTION_MULTIPLE)
        {
          if (!(priv->single_click_timeout_state & GDK_CONTROL_MASK))
            /* unselect all previously selected items */
            xfce_icon_view_unselect_all_internal (icon_view);

          /* select all items between the anchor and the prelit item */
          xfce_icon_view_set_cursor_item (icon_view, item, -1);
          if (priv->anchor_item == NULL)
            priv->anchor_item = item;
          else
            xfce_icon_view_select_all_between (icon_view, priv->anchor_item, item);

          /* selection was changed */
          dirty = TRUE;
        }
      else
        {
          if ((priv->selection_mode == GTK_SELECTION_MULTIPLE
               || (priv->selection_mode == GTK_SELECTION_SINGLE
                   && item->selected))
              && (priv->single_click_timeout_state & GDK_CONTROL_MASK) != 0)
            {
              item->selected = !item->selected;
              xfce_icon_view_queue_draw_item (icon_view, item);
              dirty = TRUE;
            }
          else if (!item->selected)
            {
              xfce_icon_view_unselect_all_internal (icon_view);
              xfce_icon_view_queue_draw_item (icon_view, item);
              item->selected = TRUE;
              dirty = TRUE;
            }
          xfce_icon_view_set_cursor_item (icon_view, item, -1);
          priv->anchor_item = item;
        }
    }

  /* emit "selection-changed" and stop drawing keyboard
   * focus indicator if the selection was altered
   */
  if (G_LIKELY (dirty))
    {
      /* reset "draw keyfocus" flag */
      XFCE_ICON_VIEW_UNSET_FLAG (icon_view, XFCE_ICON_VIEW_DRAW_KEYFOCUS);

      /* emit "selection-changed" */
      g_signal_emit (G_OBJECT (icon_view), icon_view_signals[SELECTION_CHANGED], 0);
    }

  return FALSE;
}



static void
xfce_icon_view_single_click_timeout_destroy (gpointer user_data)
{
  get_instance_private (user_data)->single_click_timeout_id = 0;
}



/*----------------------------*
 * Interactive search support *
 *----------------------------*/

/**
 * xfce_icon_view_get_enable_search:
 * @icon_view : an #XfceIconView.
 *
 * Returns whether or not the @icon_view allows to start
 * interactive searching by typing in text.
 *
 * Returns: whether or not to let the user search interactively.
 *
 * Since: 4.21.0
 **/
gboolean
xfce_icon_view_get_enable_search (XfceIconView *icon_view)
{
  XfceIconViewPrivate *priv = get_instance_private (icon_view);
  g_return_val_if_fail (XFCE_IS_ICON_VIEW (icon_view), FALSE);
  return priv->enable_search;
}



/**
 * xfce_icon_view_set_enable_search:
 * @icon_view     : an #XfceIconView.
 * @enable_search : %TRUE if the user can search interactively.
 *
 * If @enable_search is set, then the user can type in text to search through
 * the @icon_view interactively (this is sometimes called "typeahead find").
 *
 * Note that even if this is %FALSE, the user can still initiate a search
 * using the "start-interactive-search" key binding.
 *
 * Since: 4.21.0
 **/
void
xfce_icon_view_set_enable_search (XfceIconView *icon_view,
                                  gboolean enable_search)
{
  XfceIconViewPrivate *priv = get_instance_private (icon_view);

  g_return_if_fail (XFCE_IS_ICON_VIEW (icon_view));

  enable_search = !!enable_search;

  if (G_LIKELY (priv->enable_search != enable_search))
    {
      priv->enable_search = enable_search;
      g_object_notify (G_OBJECT (icon_view), "enable-search");
    }
}



/**
 * xfce_icon_view_get_search_column:
 * @icon_view : an #XfceIconView.
 *
 * Returns the column searched on by the interactive search code.
 *
 * Returns: the column the interactive search code searches in.
 *
 * Since: 4.21.0
 **/
gint
xfce_icon_view_get_search_column (XfceIconView *icon_view)
{
  XfceIconViewPrivate *priv = get_instance_private (icon_view);
  g_return_val_if_fail (XFCE_IS_ICON_VIEW (icon_view), -1);
  return priv->search_column;
}



/**
 * xfce_icon_view_set_search_column:
 * @icon_view     : an #XfceIconView.
 * @search_column : the column of the model to search in, or -1 to disable searching.
 *
 * Sets @search_column as the column where the interactive search code should search in.
 *
 * If the search column is set, user can use the "start-interactive-search" key
 * binding to bring up search popup. The "enable-search" property controls
 * whether simply typing text will also start an interactive search.
 *
 * Note that @search_column refers to a column of the model.
 *
 * Since: 4.21.0
 **/
void
xfce_icon_view_set_search_column (XfceIconView *icon_view,
                                  gint search_column)
{
  XfceIconViewPrivate *priv = get_instance_private (icon_view);

  g_return_if_fail (XFCE_IS_ICON_VIEW (icon_view));
  g_return_if_fail (search_column >= -1);

  if (G_LIKELY (priv->search_column != search_column))
    {
      priv->search_column = search_column;
      g_object_notify (G_OBJECT (icon_view), "search-column");
    }
}



/**
 * xfce_icon_view_get_search_equal_func:
 * @icon_view : an #XfceIconView.
 *
 * Returns the compare function currently in use.
 *
 * Returns: (transfer none) (skip): the currently used compare function for the search code.
 *
 * Since: 4.21.0
 **/
XfceIconViewSearchEqualFunc
xfce_icon_view_get_search_equal_func (XfceIconView *icon_view)
{
  XfceIconViewPrivate *priv = get_instance_private (icon_view);
  g_return_val_if_fail (XFCE_IS_ICON_VIEW (icon_view), NULL);
  return priv->search_equal_func;
}



/**
 * xfce_icon_view_set_search_equal_func:
 * @icon_view            : an #XfceIconView.
 * @search_equal_func    : the compare function to use during the search, or %NULL.
 * @search_equal_data    : user data to pass to @search_equal_func, or %NULL.
 * @search_equal_destroy : destroy notifier for @search_equal_data, or %NULL.
 *
 * Sets the compare function for the interactive search capabilities;
 * note that some like strcmp() returning 0 for equality
 * #XfceIconViewSearchEqualFunc returns %FALSE on matches.
 *
 * Specifying %NULL for @search_equal_func will reset @icon_view to use the default
 * search equal function.
 *
 * Since: 4.21.0
 **/
void
xfce_icon_view_set_search_equal_func (XfceIconView *icon_view,
                                      XfceIconViewSearchEqualFunc search_equal_func,
                                      gpointer search_equal_data,
                                      GDestroyNotify search_equal_destroy)
{
  XfceIconViewPrivate *priv = get_instance_private (icon_view);

  g_return_if_fail (XFCE_IS_ICON_VIEW (icon_view));
  g_return_if_fail (search_equal_func != NULL || (search_equal_data == NULL && search_equal_destroy == NULL));

  /* destroy the previous data (if any) */
  if (G_UNLIKELY (priv->search_equal_destroy != NULL))
    (*priv->search_equal_destroy) (priv->search_equal_data);

  priv->search_equal_func = (search_equal_func != NULL) ? search_equal_func
                                                        : xfce_icon_view_search_equal_func;
  priv->search_equal_data = search_equal_data;
  priv->search_equal_destroy = search_equal_destroy;
}



/**
 * xfce_icon_view_get_search_position_func:
 * @icon_view : an #XfceIconView.
 *
 * Returns the search dialog positioning function currently in use.
 *
 * Returns: (transfer none) (skip): the currently used function for positioning the search dialog.
 *
 * Since: 4.21.0
 **/
XfceIconViewSearchPositionFunc
xfce_icon_view_get_search_position_func (XfceIconView *icon_view)
{
  XfceIconViewPrivate *priv = get_instance_private (icon_view);
  g_return_val_if_fail (XFCE_IS_ICON_VIEW (icon_view), NULL);
  return priv->search_position_func;
}



/**
 * xfce_icon_view_set_search_position_func:
 * @icon_view               : an #XfceIconView.
 * @search_position_func    : the function to use to position the search dialog, or %NULL.
 * @search_position_data    : user data to pass to @search_position_func, or %NULL.
 * @search_position_destroy : destroy notifier for @search_position_data, or %NULL.
 *
 * Sets the function to use when positioning the seach dialog.
 *
 * Specifying %NULL for @search_position_func will reset @icon_view to use the default
 * search position function.
 *
 * Since: 4.21.0
 **/
void
xfce_icon_view_set_search_position_func (XfceIconView *icon_view,
                                         XfceIconViewSearchPositionFunc search_position_func,
                                         gpointer search_position_data,
                                         GDestroyNotify search_position_destroy)
{
  XfceIconViewPrivate *priv = get_instance_private (icon_view);

  g_return_if_fail (XFCE_IS_ICON_VIEW (icon_view));
  g_return_if_fail (search_position_func != NULL
                    || (search_position_data == NULL && search_position_destroy == NULL));

  /* destroy the previous data (if any) */
  if (priv->search_position_destroy != NULL)
    (*priv->search_position_destroy) (priv->search_position_data);

  priv->search_position_func = (search_position_func != NULL)
                                 ? search_position_func
                                 : (XfceIconViewSearchPositionFunc) xfce_gtk_position_search_box;
  priv->search_position_data = search_position_data;
  priv->search_position_destroy = search_position_destroy;
}



static void
xfce_icon_view_search_activate (GtkEntry *entry,
                                XfceIconView *icon_view)
{
  XfceIconViewPrivate *priv = get_instance_private (icon_view);
  GtkTreePath *path;

  /* hide the interactive search dialog */
  xfce_icon_view_search_dialog_hide (priv->search_window, icon_view);

  /* check if we have a cursor item, and if so, activate it */
  if (xfce_icon_view_get_cursor (icon_view, &path, NULL))
    {
      /* only activate the cursor item if it's selected */
      if (xfce_icon_view_path_is_selected (icon_view, path))
        xfce_icon_view_item_activated (icon_view, path);
      gtk_tree_path_free (path);
    }
}



static void
xfce_gtk_widget_send_focus_change (GtkWidget *widget,
                                   gboolean in)
{
  GdkEvent *fevent;

  g_object_ref (G_OBJECT (widget));

  fevent = gdk_event_new (GDK_FOCUS_CHANGE);
  fevent->focus_change.type = GDK_FOCUS_CHANGE;
  fevent->focus_change.window = g_object_ref (gtk_widget_get_window (widget));
  fevent->focus_change.in = in;

  gtk_widget_send_focus_change (widget, fevent);

  g_object_unref (G_OBJECT (widget));
  gdk_event_free (fevent);
}



static void
xfce_icon_view_search_dialog_hide (GtkWidget *search_dialog,
                                   XfceIconView *icon_view)
{
  XfceIconViewPrivate *priv = get_instance_private (icon_view);

  g_return_if_fail (GTK_IS_WIDGET (search_dialog));
  g_return_if_fail (XFCE_IS_ICON_VIEW (icon_view));

  if (priv->search_disable_popdown)
    return;

  /* disconnect the "changed" signal handler */
  if (priv->search_entry_changed_id != 0)
    {
      g_signal_handler_disconnect (G_OBJECT (priv->search_entry),
                                   priv->search_entry_changed_id);
      priv->search_entry_changed_id = 0;
    }

  /* disable the flush timeout */
  if (priv->search_timeout_id != 0)
    g_source_remove (priv->search_timeout_id);

  /* send focus-out event */
  xfce_gtk_widget_send_focus_change (priv->search_entry, FALSE);
  gtk_widget_hide (search_dialog);
  gtk_entry_set_text (GTK_ENTRY (priv->search_entry), "");
}



static void
xfce_icon_view_search_ensure_directory (XfceIconView *icon_view)
{
  XfceIconViewPrivate *priv = get_instance_private (icon_view);
  GtkWidget *toplevel;
  GtkWidget *frame;
  GtkWidget *vbox;

  /* determine the toplevel window */
  toplevel = gtk_widget_get_toplevel (GTK_WIDGET (icon_view));

  /* check if we already have a search window */
  if (G_LIKELY (priv->search_window != NULL))
    {
      if (gtk_window_get_group (GTK_WINDOW (toplevel)) != NULL)
        gtk_window_group_add_window (gtk_window_get_group (GTK_WINDOW (toplevel)),
                                     GTK_WINDOW (priv->search_window));
      else if (gtk_window_get_group (GTK_WINDOW (priv->search_window)) != NULL)
        gtk_window_group_remove_window (gtk_window_get_group (GTK_WINDOW (priv->search_window)),
                                        GTK_WINDOW (priv->search_window));
      return;
    }

  /* allocate a new search window */
  priv->search_window = gtk_window_new (GTK_WINDOW_POPUP);
  if (gtk_window_get_group (GTK_WINDOW (toplevel)) != NULL)
    gtk_window_group_add_window (gtk_window_get_group (GTK_WINDOW (toplevel)),
                                 GTK_WINDOW (priv->search_window));
  gtk_window_set_modal (GTK_WINDOW (priv->search_window), TRUE);
  gtk_window_set_screen (GTK_WINDOW (priv->search_window),
                         gtk_widget_get_screen (GTK_WIDGET (icon_view)));

  /* attach the popup window to the toplevel parent (only needed on wayland).
   * see https://bugzilla.xfce.org/show_bug.cgi?id=16768
   */
  gtk_window_set_transient_for (GTK_WINDOW (priv->search_window), GTK_WINDOW (toplevel));

  /* connect signal handlers */
  g_signal_connect (G_OBJECT (priv->search_window), "delete-event",
                    G_CALLBACK (xfce_icon_view_search_delete_event), icon_view);
  g_signal_connect (G_OBJECT (priv->search_window), "scroll-event",
                    G_CALLBACK (xfce_icon_view_search_scroll_event), icon_view);
  g_signal_connect (G_OBJECT (priv->search_window), "key-press-event",
                    G_CALLBACK (xfce_icon_view_search_key_press_event), icon_view);
  g_signal_connect (G_OBJECT (priv->search_window), "button-press-event",
                    G_CALLBACK (xfce_icon_view_search_button_press_event), icon_view);

  /* allocate the frame widget */
  frame = g_object_new (GTK_TYPE_FRAME, "shadow-type", GTK_SHADOW_ETCHED_IN, NULL);
  gtk_container_add (GTK_CONTAINER (priv->search_window), frame);
  gtk_widget_show (frame);

  /* allocate the vertical box */
  vbox = g_object_new (GTK_TYPE_BOX, "orientation", GTK_ORIENTATION_VERTICAL, "border-width", 3, NULL);
  gtk_container_add (GTK_CONTAINER (frame), vbox);
  gtk_widget_show (vbox);

  /* allocate the search entry widget */
  priv->search_entry = gtk_entry_new ();
  gtk_entry_set_input_hints (GTK_ENTRY (priv->search_entry), GTK_INPUT_HINT_NO_EMOJI);
  g_signal_connect (G_OBJECT (priv->search_entry), "activate",
                    G_CALLBACK (xfce_icon_view_search_activate), icon_view);
  gtk_box_pack_start (GTK_BOX (vbox), priv->search_entry, TRUE, TRUE, 0);
  gtk_widget_realize (priv->search_entry);
  gtk_widget_show (priv->search_entry);
}



static void
xfce_icon_view_search_init (GtkWidget *search_entry,
                            XfceIconView *icon_view)
{
  XfceIconViewPrivate *priv = get_instance_private (icon_view);
  GtkTreeModel *model;
  GtkTreeIter iter;
  const gchar *text;
  gint length;
  gint count = 0;

  g_return_if_fail (GTK_IS_ENTRY (search_entry));
  g_return_if_fail (XFCE_IS_ICON_VIEW (icon_view));

  /* determine the current text for the search entry */
  text = gtk_entry_get_text (GTK_ENTRY (search_entry));
  if (G_UNLIKELY (text == NULL))
    return;

  /* unselect all items */
  xfce_icon_view_unselect_all (icon_view);

  /* renew the flush timeout */
  if ((priv->search_timeout_id != 0))
    {
      /* drop the previous timeout */
      g_source_remove (priv->search_timeout_id);

      /* schedule a new timeout */
      priv->search_timeout_id = gdk_threads_add_timeout_full (
        G_PRIORITY_LOW,
        XFCE_ICON_VIEW_SEARCH_DIALOG_TIMEOUT,
        xfce_icon_view_search_timeout,
        icon_view,
        xfce_icon_view_search_timeout_destroy);
    }

  /* verify that we have a search text */
  length = strlen (text);
  if (length < 1)
    return;

  /* verify that we have a valid model */
  model = xfce_icon_view_get_model (icon_view);
  if (G_UNLIKELY (model == NULL))
    return;

  /* start the interactive search */
  if (gtk_tree_model_get_iter_first (model, &iter))
    {
      /* let's see if we have a match */
      if (xfce_icon_view_search_iter (icon_view, model, &iter, text, &count, 1))
        priv->search_selected_iter = 1;
    }
}



static gboolean
xfce_icon_view_search_iter (XfceIconView *icon_view,
                            GtkTreeModel *model,
                            GtkTreeIter *iter,
                            const gchar *text,
                            gint *count,
                            gint n)
{
  XfceIconViewPrivate *priv = get_instance_private (icon_view);
  GtkTreePath *path;

  g_return_val_if_fail (XFCE_IS_ICON_VIEW (icon_view), FALSE);
  g_return_val_if_fail (GTK_IS_TREE_MODEL (model), FALSE);
  g_return_val_if_fail (count != NULL, FALSE);

  /* search for a matching item */
  do
    {
      if (!(*priv->search_equal_func) (model, priv->search_column, text,
                                       iter, priv->search_equal_data))
        {
          (*count) += 1;
          if (*count == n)
            {
              /* place cursor on the item and select it */
              path = gtk_tree_model_get_path (model, iter);
              xfce_icon_view_select_path (icon_view, path);
              xfce_icon_view_set_cursor (icon_view, path, NULL, FALSE);
              gtk_tree_path_free (path);
              return TRUE;
            }
        }
    }
  while (gtk_tree_model_iter_next (model, iter));

  /* no match */
  return FALSE;
}



static void
xfce_icon_view_search_move (GtkWidget *widget,
                            XfceIconView *icon_view,
                            gboolean move_up)
{
  XfceIconViewPrivate *priv = get_instance_private (icon_view);
  GtkTreeModel *model;
  const gchar *text;
  GtkTreeIter iter;
  gboolean retval;
  gint length;
  gint count = 0;

  /* determine the current text for the search entry */
  text = gtk_entry_get_text (GTK_ENTRY (priv->search_entry));
  if (G_UNLIKELY (text == NULL))
    return;

  /* if we already selected the first item, we cannot go up */
  if (move_up && priv->search_selected_iter == 1)
    return;

  /* determine the length of the search text */
  length = strlen (text);
  if (G_UNLIKELY (length < 1))
    return;

  /* unselect all items */
  xfce_icon_view_unselect_all (icon_view);

  /* verify that we have a valid model */
  model = xfce_icon_view_get_model (icon_view);
  if (G_UNLIKELY (model == NULL))
    return;

  /* determine the iterator to the first item */
  if (!gtk_tree_model_get_iter_first (model, &iter))
    return;

  /* first attempt to search */
  retval = xfce_icon_view_search_iter (icon_view, model, &iter, text, &count,
                                       move_up ? (priv->search_selected_iter - 1)
                                               : (priv->search_selected_iter + 1));

  /* check if we found something */
  if (G_LIKELY (retval))
    {
      /* match found */
      priv->search_selected_iter += move_up ? -1 : 1;
    }
  else
    {
      /* return to old iter */
      if (gtk_tree_model_get_iter_first (model, &iter))
        {
          count = 0;
          xfce_icon_view_search_iter (icon_view, model, &iter, text, &count,
                                      priv->search_selected_iter);
        }
    }
}



static gboolean
xfce_icon_view_search_start (XfceIconView *icon_view,
                             gboolean keybinding)
{
  XfceIconViewPrivate *priv = get_instance_private (icon_view);

  /* check if typeahead is enabled */
  if (G_UNLIKELY (!priv->enable_search && !keybinding))
    return FALSE;

  /* check if we already display the search window */
  if (priv->search_window != NULL
      && gtk_widget_get_visible (priv->search_window))
    return TRUE;

  /* we only start interactive search if we have focus,
   * we don't want to start interactive search if one of
   * our children has the focus.
   */
  if (!gtk_widget_has_focus (GTK_WIDGET (icon_view)))
    return FALSE;

  /* verify that we have a search column */
  if (G_UNLIKELY (priv->search_column < 0))
    return FALSE;

  xfce_icon_view_search_ensure_directory (icon_view);

  /* clear search entry if we were started by a keybinding */
  if (G_UNLIKELY (keybinding))
    gtk_entry_set_text (GTK_ENTRY (priv->search_entry), "");

  /* determine the position for the search dialog */
  (*priv->search_position_func) (icon_view, priv->search_window,
                                 priv->search_position_data);

  gtk_entry_grab_focus_without_selecting (GTK_ENTRY (priv->search_entry));

  /* display the search dialog */
  gtk_widget_show (priv->search_window);

  /* connect "changed" signal for the entry */
  if (G_UNLIKELY (priv->search_entry_changed_id == 0))
    {
      priv->search_entry_changed_id = g_signal_connect (
        G_OBJECT (priv->search_entry), "changed",
        G_CALLBACK (xfce_icon_view_search_init), icon_view);
    }

  /* start the search timeout */
  priv->search_timeout_id = gdk_threads_add_timeout_full (
    G_PRIORITY_LOW, XFCE_ICON_VIEW_SEARCH_DIALOG_TIMEOUT,
    xfce_icon_view_search_timeout, icon_view,
    xfce_icon_view_search_timeout_destroy);

  /* send focus-in event */
  xfce_gtk_widget_send_focus_change (priv->search_entry, TRUE);

  /* search first matching iter */
  xfce_icon_view_search_init (priv->search_entry, icon_view);

  return TRUE;
}



static gboolean
xfce_icon_view_search_equal_func (GtkTreeModel *model,
                                  gint column,
                                  const gchar *key,
                                  GtkTreeIter *iter,
                                  gpointer user_data)
{
  const gchar *str;
  gboolean retval = TRUE;
  GValue transformed = G_VALUE_INIT;
  GValue value = G_VALUE_INIT;
  gchar *case_normalized_string = NULL;
  gchar *case_normalized_key = NULL;
  gchar *normalized_string;
  gchar *normalized_key;

  /* determine the value for the column/iter */
  gtk_tree_model_get_value (model, iter, column, &value);

  /* try to transform the value to a string */
  g_value_init (&transformed, G_TYPE_STRING);
  if (!g_value_transform (&value, &transformed))
    {
      g_value_unset (&value);
      return TRUE;
    }
  g_value_unset (&value);

  /* check if we have a string value */
  str = g_value_get_string (&transformed);
  if (G_UNLIKELY (str == NULL))
    {
      g_value_unset (&transformed);
      return TRUE;
    }

  /* normalize the string and the key */
  normalized_string = g_utf8_normalize (str, -1, G_NORMALIZE_ALL);
  normalized_key = g_utf8_normalize (key, -1, G_NORMALIZE_ALL);

  /* check if we have normalized both string */
  if (G_LIKELY (normalized_string != NULL && normalized_key != NULL))
    {
      case_normalized_string = g_utf8_casefold (normalized_string, -1);
      case_normalized_key = g_utf8_casefold (normalized_key, -1);

      /* compare the casefolded strings */
      if (strncmp (case_normalized_key, case_normalized_string, strlen (case_normalized_key)) == 0)
        retval = FALSE;
    }

  /* cleanup */
  g_free (case_normalized_string);
  g_free (case_normalized_key);
  g_value_unset (&transformed);
  g_free (normalized_string);
  g_free (normalized_key);

  return retval;
}



static gboolean
xfce_icon_view_search_button_press_event (GtkWidget *widget,
                                          GdkEventButton *event,
                                          XfceIconView *icon_view)
{
  XfceIconViewPrivate *priv = get_instance_private (icon_view);

  g_return_val_if_fail (GTK_IS_WIDGET (widget), FALSE);
  g_return_val_if_fail (XFCE_IS_ICON_VIEW (icon_view), FALSE);

  /* hide the search dialog */
  xfce_icon_view_search_dialog_hide (widget, icon_view);

  if (event->window == priv->bin_window)
    xfce_icon_view_button_press_event (GTK_WIDGET (icon_view), event);

  return TRUE;
}



static gboolean
xfce_icon_view_search_delete_event (GtkWidget *widget,
                                    GdkEventAny *event,
                                    XfceIconView *icon_view)
{
  g_return_val_if_fail (GTK_IS_WIDGET (widget), FALSE);
  g_return_val_if_fail (XFCE_IS_ICON_VIEW (icon_view), FALSE);

  /* hide the search dialog */
  xfce_icon_view_search_dialog_hide (widget, icon_view);

  return TRUE;
}



static gboolean
xfce_icon_view_search_key_press_event (GtkWidget *widget,
                                       GdkEventKey *event,
                                       XfceIconView *icon_view)
{
  XfceIconViewPrivate *priv = get_instance_private (icon_view);
  gboolean retval = FALSE;

  g_return_val_if_fail (GTK_IS_WIDGET (widget), FALSE);
  g_return_val_if_fail (XFCE_IS_ICON_VIEW (icon_view), FALSE);


  /* close window and cancel the search */
  if (event->keyval == GDK_KEY_Escape || event->keyval == GDK_KEY_Tab)
    {
      xfce_icon_view_search_dialog_hide (widget, icon_view);
      return TRUE;
    }

  /* select previous matching iter */
  if (event->keyval == GDK_KEY_Up || event->keyval == GDK_KEY_KP_Up)
    {
      xfce_icon_view_search_move (widget, icon_view, TRUE);
      retval = TRUE;
    }

  if (((event->state & (GDK_CONTROL_MASK | GDK_SHIFT_MASK)) == (GDK_CONTROL_MASK | GDK_SHIFT_MASK))
      && (event->keyval == GDK_KEY_g || event->keyval == GDK_KEY_G))
    {
      xfce_icon_view_search_move (widget, icon_view, TRUE);
      retval = TRUE;
    }

  /* select next matching iter */
  if (event->keyval == GDK_KEY_Down || event->keyval == GDK_KEY_KP_Down)
    {
      xfce_icon_view_search_move (widget, icon_view, FALSE);
      retval = TRUE;
    }

  if (((event->state & (GDK_CONTROL_MASK | GDK_SHIFT_MASK)) == GDK_CONTROL_MASK)
      && (event->keyval == GDK_KEY_g || event->keyval == GDK_KEY_G))
    {
      xfce_icon_view_search_move (widget, icon_view, FALSE);
      retval = TRUE;
    }

  /* renew the flush timeout */
  if (retval && (priv->search_timeout_id != 0))
    {
      /* drop the previous timeout */
      g_source_remove (priv->search_timeout_id);

      /* schedule a new timeout */
      priv->search_timeout_id = gdk_threads_add_timeout_full (
        G_PRIORITY_LOW, XFCE_ICON_VIEW_SEARCH_DIALOG_TIMEOUT,
        xfce_icon_view_search_timeout, icon_view,
        xfce_icon_view_search_timeout_destroy);
    }

  return retval;
}



static gboolean
xfce_icon_view_search_scroll_event (GtkWidget *widget,
                                    GdkEventScroll *event,
                                    XfceIconView *icon_view)
{
  gboolean retval = TRUE;

  g_return_val_if_fail (GTK_IS_WIDGET (widget), FALSE);
  g_return_val_if_fail (XFCE_IS_ICON_VIEW (icon_view), FALSE);

  if (event->direction == GDK_SCROLL_UP)
    xfce_icon_view_search_move (widget, icon_view, TRUE);
  else if (event->direction == GDK_SCROLL_DOWN)
    xfce_icon_view_search_move (widget, icon_view, FALSE);
  else
    retval = FALSE;

  return retval;
}



static gboolean
xfce_icon_view_search_timeout (gpointer user_data)
{
  XfceIconView *icon_view = XFCE_ICON_VIEW (user_data);
  XfceIconViewPrivate *priv = get_instance_private (icon_view);

  xfce_icon_view_search_dialog_hide (priv->search_window, icon_view);

  return FALSE;
}



static void
xfce_icon_view_search_timeout_destroy (gpointer user_data)
{
  get_instance_private (user_data)->search_timeout_id = 0;
}



static void
xfce_icon_view_release_items (XfceIconView *icon_view)
{
  XfceIconViewPrivate *priv = get_instance_private (icon_view);
  GSequenceIter *item_iter;

  for (item_iter = g_sequence_get_begin_iter (priv->items);
       !g_sequence_iter_is_end (item_iter);
       item_iter = g_sequence_iter_next (item_iter))
    {
      g_free (XFCE_ICON_VIEW_ITEM (g_sequence_get (item_iter))->box);
      g_slice_free (XfceIconViewItem, g_sequence_get (item_iter));
    }
}

#define __XFCE_ICON_VIEW_C__
#include "libxfce4ui-visibility.c"
