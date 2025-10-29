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

#ifndef __XFCE_ITEM_LIST_STORE_H__
#define __XFCE_ITEM_LIST_STORE_H__

#include <libxfce4ui/xfce-item-list-model.h>

G_BEGIN_DECLS

#define XFCE_TYPE_ITEM_LIST_STORE (xfce_item_list_store_get_type ())
G_DECLARE_FINAL_TYPE (XfceItemListStore, xfce_item_list_store, XFCE, ITEM_LIST_STORE, XfceItemListModel)

XfceItemListStore *
xfce_item_list_store_new (gint n_columns,
                          ...);

void
xfce_item_list_store_clear (XfceItemListStore *store);

gint
xfce_item_list_store_insert (XfceItemListStore *store,
                             gint index);

void
xfce_item_list_store_set (XfceItemListStore *store,
                          gint index,
                          ...);

gint
xfce_item_list_store_insert_with_values (XfceItemListStore *store,
                                         gint index,
                                         ...);

G_END_DECLS


#endif /* !__XFCE_ITEM_LIST_STORE_H__ */
