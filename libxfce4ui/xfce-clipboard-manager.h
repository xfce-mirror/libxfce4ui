/*
 * Copyright (c) 2024 Gaël Bonithon <gael@xfce.org>
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

#ifndef __XFCE_CLIPBOARD_MANAGER_H__
#define __XFCE_CLIPBOARD_MANAGER_H__

#include <glib-object.h>

G_BEGIN_DECLS

#define XFCE_TYPE_CLIPBOARD_MANAGER (xfce_clipboard_manager_get_type ())
G_DECLARE_FINAL_TYPE (XfceClipboardManager, xfce_clipboard_manager, XFCE, CLIPBOARD_MANAGER, GObject)

XfceClipboardManager *
xfce_clipboard_manager_new (gboolean replace);

G_END_DECLS

#endif /* __XFCE_CLIPBOARD_MANAGER_H__ */
