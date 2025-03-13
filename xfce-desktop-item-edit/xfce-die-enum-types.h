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

#ifndef __XFCE_DIE_ENUM_TYPES_H__
#define __XFCE_DIE_ENUM_TYPES_H__

#include <glib.h>

G_BEGIN_DECLS

#define XFCE_TYPE_DIE_EDITOR_MODE (xfce_die_editor_mode_get_type ())

/**
 * XfceDieEditorMode:
 * @XFCE_DIE_EDITOR_MODE_APPLICATION : application launcher editing.
 * @XFCE_DIE_EDITOR_MODE_LINK        : link editing.
 * @XFCE_DIE_EDITOR_MODE_DIRECTORY   : menu directory editing.
 *
 * Editing mode for xfce-desktop-item-edit
 *
 **/
typedef enum
{
  XFCE_DIE_EDITOR_MODE_APPLICATION,
  XFCE_DIE_EDITOR_MODE_LINK,
  XFCE_DIE_EDITOR_MODE_DIRECTORY
} XfceDieEditorMode;

GType
xfce_die_editor_mode_get_type (void) G_GNUC_CONST;

G_END_DECLS

#endif /* !__XFCE_DIE_ENUM_TYPES_H__ */
