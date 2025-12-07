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

#ifndef __XFCE_ITEM_LIST_VIEW_H__
#define __XFCE_ITEM_LIST_VIEW_H__

#include <gtk/gtk.h>
#include <libxfce4ui/xfce-item-list-model.h>

G_BEGIN_DECLS

/**
 * XFCE_MENU_ATTRIBUTE_MOVEMENT:
 *
 * Attribute with boolean type. If %TRUE, then the menu item is responsible for changing the order of rows.
 * Buttons with this attribute will appear on the right side of the #XfceItemListView widget.
 *
 * Since: 4.21.3
 **/
#define XFCE_MENU_ATTRIBUTE_MOVEMENT "movement"

/**
 * XFCE_MENU_ATTRIBUTE_TOOLTIP:
 *
 * Attribute with string type. The format is the same as gtk_widget_set_tooltip_text().
 *
 * Since: 4.21.3
 **/
#define XFCE_MENU_ATTRIBUTE_TOOLTIP "tooltip"

/**
 * XFCE_MENU_ATTRIBUTE_HIDE_IN_CONTEXT_MENU:
 *
 * Attribute with boolean type. If %TRUE, the menu item will not appear in context menus.
 *
 * Since: 4.21.3
 **/
#define XFCE_MENU_ATTRIBUTE_HIDE_IN_CONTEXT_MENU "hide-in-context-menu"

/**
 * XFCE_MENU_ATTRIBUTE_HIDE_IN_BUTTONS:
 *
 * Boolean type attribute. If %TRUE, the menu item will not appear as a button.
 *
 * Since: 4.21.4
 **/
#define XFCE_MENU_ATTRIBUTE_HIDE_IN_BUTTONS "hide-in-buttons"

#define XFCE_TYPE_ITEM_LIST_VIEW (xfce_item_list_view_get_type ())
G_DECLARE_FINAL_TYPE (XfceItemListView, xfce_item_list_view, XFCE, ITEM_LIST_VIEW, GtkBox)

GtkWidget *
xfce_item_list_view_new (XfceItemListModel *model) G_GNUC_MALLOC;

XfceItemListModel *
xfce_item_list_view_get_model (XfceItemListView *view);

void
xfce_item_list_view_set_model (XfceItemListView *view,
                               XfceItemListModel *model);

GMenu *
xfce_item_list_view_get_menu (XfceItemListView *view);

GtkWidget *
xfce_item_list_view_get_tree_view (XfceItemListView *view);

void
xfce_item_list_view_set_label_visibility (XfceItemListView *view,
                                          gboolean visibility);

gint
xfce_item_list_view_get_selected_items (XfceItemListView *view,
                                        gint **items);

G_END_DECLS

#endif /* !__XFCE_ITEM_LIST_VIEW_H__ */
