/*
 * Copyright (c) 2026 fulalas
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

#ifndef __XFCE_SHORTCUTS_PORTAL_H__
#define __XFCE_SHORTCUTS_PORTAL_H__

#include <glib-object.h>

G_BEGIN_DECLS

#define XFCE_TYPE_SHORTCUTS_PORTAL (xfce_shortcuts_portal_get_type ())
G_DECLARE_FINAL_TYPE (XfceShortcutsPortal, xfce_shortcuts_portal, XFCE, SHORTCUTS_PORTAL, GObject)

XfceShortcutsPortal *
xfce_shortcuts_portal_new (const gchar *app_id);

void
xfce_shortcuts_portal_set_keys (XfceShortcutsPortal *portal,
                                const gchar *const *keys);

G_END_DECLS

#endif
