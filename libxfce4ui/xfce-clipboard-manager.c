/*
 * Copyright (c) 2026 The Xfce Development Team
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

/**
 * SECTION:xfce-clipboard-manager
 * @title: XfceClipboardManager
 * @short_description: Abstract clipboard manager
 * @stability: Stable
 * @include: libxfce4ui/libxfce4ui.h
 *
 * Abstract base class for windowing-system-specific clipboard managers.
 * Use xfce_clipboard_manager_new() to create the appropriate backend
 * for the current display.
 **/

#include <gtk/gtk.h>

#ifdef ENABLE_X11
#include <gdk/gdkx.h>
#include "xfce-clipboard-manager-x11.h"
#endif

#ifdef ENABLE_WAYLAND
#include <gdk/gdkwayland.h>
#include "xfce-clipboard-manager-wayland.h"
#endif

#include "xfce-clipboard-manager.h"
#include "libxfce4ui-visibility.h"



/**
 * XfceClipboardManager:
 *
 * An opaque structure representing a clipboard manager. Use
 * xfce_clipboard_manager_new() to get an instance for the current display.
 *
 * Since: 4.19.5
 **/
G_DEFINE_ABSTRACT_TYPE (XfceClipboardManager, xfce_clipboard_manager, G_TYPE_OBJECT)



static void
xfce_clipboard_manager_class_init (XfceClipboardManagerClass *klass)
{
}



static void
xfce_clipboard_manager_init (XfceClipboardManager *manager)
{
}



/**
 * xfce_clipboard_manager_new:
 * @replace: whether or not to replace an existing clipboard manager
 *           (only meaningful on X11, ignored on Wayland)
 *
 * Creates a new clipboard manager for the current display backend.
 *
 * Return value: (nullable) (transfer full): A new #XfceClipboardManager
 * instance or %NULL on failure.
 *
 * Since: 4.19.5
 **/
XfceClipboardManager *
xfce_clipboard_manager_new (gboolean replace)
{
  GdkDisplay *display = gdk_display_get_default ();

#ifdef ENABLE_WAYLAND
  if (GDK_IS_WAYLAND_DISPLAY (display))
    return XFCE_CLIPBOARD_MANAGER (xfce_clipboard_manager_wayland_new ());
#endif

#ifdef ENABLE_X11
  if (GDK_IS_X11_DISPLAY (display))
    return XFCE_CLIPBOARD_MANAGER (xfce_clipboard_manager_x11_new (replace));
#endif

  return NULL;
}



/**
 * xfce_clipboard_manager_persist:
 * @manager: an #XfceClipboardManager
 *
 * Persists the current clipboard data. On Wayland, reads from the
 * active offer and becomes selection owner. On X11, this is a no-op
 * since the ICCCM protocol handles persistence via SAVE_TARGETS.
 *
 * Return value: %TRUE on success.
 *
 * Since: 4.21.10
 **/
gboolean
xfce_clipboard_manager_persist (XfceClipboardManager *manager)
{
  XfceClipboardManagerClass *klass;

  g_return_val_if_fail (XFCE_IS_CLIPBOARD_MANAGER (manager), FALSE);

  klass = XFCE_CLIPBOARD_MANAGER_GET_CLASS (manager);
  if (klass->persist != NULL)
    return klass->persist (manager);

  return TRUE;
}



#define __XFCE_CLIPBOARD_MANAGER_C__
#include "libxfce4ui-visibility.c"
