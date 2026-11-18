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
 * SECTION:xfce-clipboard-manager-client
 * @title: Clipboard client API
 * @short_description: Functions for apps to check and trigger clipboard persistence
 * @stability: Stable
 * @include: libxfce4ui/libxfce4ui.h
 *
 * Free functions that apps (screenshooter, etc.) call to persist clipboard
 * data before exiting. On X11 these delegate to GTK/GDK.
 **/

#include <gtk/gtk.h>

#ifdef ENABLE_X11
#include <gdk/gdkx.h>
#endif

#include "xfce-clipboard-manager-client.h"
#include "libxfce4ui-visibility.h"



/**
 * xfce_clipboard_manager_supports_persistence:
 *
 * Checks whether clipboard persistence is available in the current session.
 *
 * On X11, delegates to gdk_display_supports_clipboard_persistence().
 *
 * Return value: %TRUE if clipboard persistence is available.
 *
 * Since: 4.21.10
 **/
gboolean
xfce_clipboard_manager_supports_persistence (void)
{
#ifdef ENABLE_X11
  GdkDisplay *display = gdk_display_get_default ();

  if (GDK_IS_X11_DISPLAY (display))
    return gdk_display_supports_clipboard_persistence (display);
#endif

  return FALSE;
}



/**
 * xfce_clipboard_manager_store:
 *
 * Persists the current clipboard content so it survives app exit.
 *
 * On X11, delegates to gtk_clipboard_store().
 *
 * Return value: %TRUE on success, %FALSE if persistence failed or no
 * manager is available.
 *
 * Since: 4.21.10
 **/
gboolean
xfce_clipboard_manager_store (void)
{
#ifdef ENABLE_X11
  GdkDisplay *display = gdk_display_get_default ();

  if (GDK_IS_X11_DISPLAY (display))
    {
      gtk_clipboard_store (gtk_clipboard_get (GDK_SELECTION_CLIPBOARD));
      return TRUE;
    }
#endif

  return FALSE;
}



#define __XFCE_CLIPBOARD_MANAGER_CLIENT_C__
#include "libxfce4ui-visibility.c"
