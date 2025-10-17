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

#include "xfce-item-list-model.h"

G_BEGIN_DECLS

/**
 * XFCE_MENU_ATTRIBUTE_MNEMONIC:
 *
 * Attribute with string type. The format is the same as gtk_button_new_with_mnemonic().
 *
 * Since: 4.21.2
 **/
#define XFCE_MENU_ATTRIBUTE_MNEMONIC "mnemonic"

/**
 * XFCE_MENU_ATTRIBUTE_MOVEMENT:
 *
 * Attribute with boolean type. If TRUE, then the menu item is responsible for changing the order of rows.
 * Buttons with this attribute will appear on the right side of the #XfceItemListView widget.
 *
 * Since: 4.21.2
 **/
#define XFCE_MENU_ATTRIBUTE_MOVEMENT "movement"

/**
 * XFCE_MENU_ATTRIBUTE_TOOLTIP:
 *
 * Attribute with string type. The format is the same as gtk_widget_set_tooltip_text().
 *
 * Since: 4.21.2
 **/
#define XFCE_MENU_ATTRIBUTE_TOOLTIP "tooltip"

#define XFCE_TYPE_ITEM_LIST_VIEW (xfce_item_list_view_get_type ())
G_DECLARE_FINAL_TYPE (XfceItemListView, xfce_item_list_view, XFCE, ITEM_LIST_VIEW, GtkBox)

GtkWidget *
xfce_item_list_view_new (XfceItemListModel *model) G_GNUC_MALLOC;

GMenu *
xfce_item_list_view_get_menu (XfceItemListView *view);

GtkWidget *
xfce_item_list_view_get_tree_view (XfceItemListView *view);

G_END_DECLS

#endif /* !__XFCE_ITEM_LIST_VIEW_H__ */
