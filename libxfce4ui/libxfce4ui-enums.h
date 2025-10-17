/*
 * Copyright (c) 2023 Gaël Bonithon <gael@xfce.org>
 * Copyright (c) 2009 Brian Tarricone <brian@terricone.org>
 * Copyright (C) 1999 Olivier Fourdan <fourdan@xfce.org>
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
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston,
 * MA 02110-1301 USA
 */

#if !defined(_LIBXFCE4UI_INSIDE_LIBXFCE4UI_H) && !defined(LIBXFCE4UI_COMPILATION)
#error "Only <libxfce4ui/libxfce4ui.h> can be included directly, this file is not part of the public API."
#endif

#ifndef __LIBXFCE4UI_ENUMS_H__
#define __LIBXFCE4UI_ENUMS_H__

#include <glib.h>

G_BEGIN_DECLS

/**
 * XfceSmCLientErrorEnum:
 * @XFCE_SM_CLIENT_ERROR_FAILED: Failed to connect to the session manager.
 * @XFCE_SM_CLIENT_ERROR_INVALID_CLIENT: Session does not have a valid client id.
 *
 * Error codes returned by XfceSmCLient functions.
 **/
typedef enum
{
  XFCE_SM_CLIENT_ERROR_FAILED,
  XFCE_SM_CLIENT_ERROR_INVALID_CLIENT
} XfceSmCLientErrorEnum;

typedef enum
{
  XFCE_SM_CLIENT_RESTART_NORMAL = 0,
  XFCE_SM_CLIENT_RESTART_IMMEDIATELY,
} XfceSMClientRestartStyle;

typedef enum /*< skip >*/
{
  XFCE_SM_CLIENT_PRIORITY_HIGHEST = 0,
  XFCE_SM_CLIENT_PRIORITY_WM = 15,
  XFCE_SM_CLIENT_PRIORITY_CORE = 25,
  XFCE_SM_CLIENT_PRIORITY_DESKTOP = 35,
  XFCE_SM_CLIENT_PRIORITY_DEFAULT = 50,
  XFCE_SM_CLIENT_PRIORITY_LOWEST = 255,
} XfceSMClientPriority;

typedef enum
{
  XFCE_SM_CLIENT_SHUTDOWN_HINT_ASK = 0,
  XFCE_SM_CLIENT_SHUTDOWN_HINT_LOGOUT,
  XFCE_SM_CLIENT_SHUTDOWN_HINT_HALT,
  XFCE_SM_CLIENT_SHUTDOWN_HINT_REBOOT,
} XfceSMClientShutdownHint;

/**
 * XfceIconViewDropPosition:
 * @XFCE_ICON_VIEW_NO_DROP    : no drop indicator.
 * @XFCE_ICON_VIEW_DROP_INTO  : drop indicator on an item.
 * @XFCE_ICON_VIEW_DROP_LEFT  : drop indicator on the left of an item.
 * @XFCE_ICON_VIEW_DROP_RIGHT : drop indicator on the right of an item.
 * @XFCE_ICON_VIEW_DROP_ABOVE : drop indicator above an item.
 * @XFCE_ICON_VIEW_DROP_BELOW : drop indicator below an item.
 *
 * Specifies whether to display the drop indicator,
 * i.e. where to drop into the icon view.
 **/
typedef enum
{
  XFCE_ICON_VIEW_NO_DROP,
  XFCE_ICON_VIEW_DROP_INTO,
  XFCE_ICON_VIEW_DROP_LEFT,
  XFCE_ICON_VIEW_DROP_RIGHT,
  XFCE_ICON_VIEW_DROP_ABOVE,
  XFCE_ICON_VIEW_DROP_BELOW
} XfceIconViewDropPosition;

/**
 * XfceIconViewLayoutMode:
 * @XFCE_ICON_VIEW_LAYOUT_ROWS : layout items in rows.
 * @XFCE_ICON_VIEW_LAYOUT_COLS : layout items in columns.
 *
 * Specifies the layouting mode of an #XfceIconView. @XFCE_ICON_VIEW_LAYOUT_ROWS
 * is the default, which lays out items vertically in rows from top to bottom.
 * @XFCE_ICON_VIEW_LAYOUT_COLS lays out items horizontally in columns from left
 * to right.
 **/
typedef enum
{
  XFCE_ICON_VIEW_LAYOUT_ROWS,
  XFCE_ICON_VIEW_LAYOUT_COLS
} XfceIconViewLayoutMode;

/**
 * XfceItemListModelFlags:
 * @XFCE_ITEM_LIST_MODEL_NONE         : No additional features.
 * @XFCE_ITEM_LIST_MODEL_REORDERABLE  : The order of items can be changed.
 * @XFCE_ITEM_LIST_MODEL_EDITABLE     : An edit dialog can be opened for each item.
 * @XFCE_ITEM_LIST_MODEL_ADDABLE      : Items can be added.
 * @XFCE_ITEM_LIST_MODEL_REMOVABLE    : Items can be removed.
 * @XFCE_ITEM_LIST_MODEL_RESETTABLE   : A button "reset to default" will be offered.
 *
 * Specifies the model's capabilities
 *
 * Since: 4.21.2
 **/
typedef enum
{
  XFCE_ITEM_LIST_MODEL_NONE = 0,
  XFCE_ITEM_LIST_MODEL_REORDERABLE = 1 << 0,
  XFCE_ITEM_LIST_MODEL_EDITABLE = 1 << 1,
  XFCE_ITEM_LIST_MODEL_ADDABLE = 1 << 2,
  XFCE_ITEM_LIST_MODEL_REMOVABLE = 1 << 3,
  XFCE_ITEM_LIST_MODEL_RESETTABLE = 1 << 4,
} XfceItemListModelFlags;

/**
 * XfceItemListModelColumn:
 * @XFCE_ITEM_LIST_MODEL_COLUMN_ACTIVE    : boolean, required.
 * @XFCE_ITEM_LIST_MODEL_COLUMN_ACTIVABLE : boolean, required.
 * @XFCE_ITEM_LIST_MODEL_COLUMN_ICON      : #GIcon, required.
 * @XFCE_ITEM_LIST_MODEL_COLUMN_NAME      : string, markup, required.
 * @XFCE_ITEM_LIST_MODEL_COLUMN_TOOLTIP   : string, required.
 * @XFCE_ITEM_LIST_MODEL_COLUMN_EDITABLE  : boolean, required if the #XFCE_ITEM_LIST_MODEL_EDITABLE flag is set.
 * @XFCE_ITEM_LIST_MODEL_COLUMN_REMOVABLE : boolean, required if the #XFCE_ITEM_LIST_MODEL_REMOVABLE flag is set.
 * @XFCE_ITEM_LIST_MODEL_COLUMN_USER      : If you need to add your own columns, then your first column should have this value.
 *
 * Since: 4.21.2
 **/
typedef enum
{
  XFCE_ITEM_LIST_MODEL_COLUMN_ACTIVE,
  XFCE_ITEM_LIST_MODEL_COLUMN_ACTIVABLE,
  XFCE_ITEM_LIST_MODEL_COLUMN_ICON,
  XFCE_ITEM_LIST_MODEL_COLUMN_NAME,
  XFCE_ITEM_LIST_MODEL_COLUMN_TOOLTIP,
  XFCE_ITEM_LIST_MODEL_COLUMN_EDITABLE,
  XFCE_ITEM_LIST_MODEL_COLUMN_REMOVABLE,
  XFCE_ITEM_LIST_MODEL_COLUMN_USER,
} XfceItemListModelColumn;

G_END_DECLS

#endif /* __LIBXFCE4UI_ENUMS_H__ */
