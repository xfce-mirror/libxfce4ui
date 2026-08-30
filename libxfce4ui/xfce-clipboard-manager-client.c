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
 * data before exiting. On X11 these delegate to GTK/GDK; on Wayland they
 * use D-Bus to reach the clipboard manager, e.g. in xfsettingsd.
 **/

#include <gio/gio.h>
#include <gtk/gtk.h>

#ifdef ENABLE_X11
#include <gdk/gdkx.h>
#endif

#ifdef ENABLE_WAYLAND
#include <gdk/gdkwayland.h>
#endif

#include "xfce-clipboard-manager-client.h"
#include "libxfce4ui-visibility.h"



#define CLIPBOARD_DBUS_NAME      "org.xfce.SettingsDaemon"
#define CLIPBOARD_DBUS_PATH      "/org/xfce/ClipboardManager"
#define CLIPBOARD_DBUS_INTERFACE "org.xfce.ClipboardManager"



#ifdef ENABLE_WAYLAND

typedef struct
{
  GMainLoop *loop;
  gboolean success;
} StoreData;



static void
clipboard_store_cb (GObject *source,
                    GAsyncResult *res,
                    gpointer user_data)
{
  StoreData *data = user_data;
  GVariant *result;
  GError *error = NULL;

  result = g_dbus_connection_call_finish (G_DBUS_CONNECTION (source), res, &error);
  if (result != NULL)
    {
      g_variant_get (result, "(b)", &data->success);
      g_variant_unref (result);
    }
  else
    {
      g_warning ("Clipboard store failed: %s", error->message);
      g_error_free (error);
    }

  g_main_loop_quit (data->loop);
}
#endif



/**
 * xfce_clipboard_manager_supports_persistence:
 *
 * Checks whether clipboard persistence is available in the current session.
 *
 * On X11, delegates to gdk_display_supports_clipboard_persistence().
 * On Wayland, checks whether the org.xfce.ClipboardManager D-Bus interface
 * has an owner (i.e. xfsettingsd is running with clipboard support enabled).
 *
 * Return value: %TRUE if clipboard persistence is available.
 *
 * Since: 4.21.10
 **/
gboolean
xfce_clipboard_manager_supports_persistence (void)
{
  GdkDisplay *display = gdk_display_get_default ();

#ifdef ENABLE_X11
  if (GDK_IS_X11_DISPLAY (display))
    return gdk_display_supports_clipboard_persistence (display);
#endif

#ifdef ENABLE_WAYLAND
  if (GDK_IS_WAYLAND_DISPLAY (display))
    {
      GDBusConnection *conn;
      GVariant *result;
      GError *error = NULL;
      gboolean has_owner = FALSE;

      conn = g_bus_get_sync (G_BUS_TYPE_SESSION, NULL, &error);
      if (G_UNLIKELY (error))
        {
          g_warning ("Failed to connect to session bus: %s", error->message);
          g_error_free (error);
          return FALSE;
        }

      result = g_dbus_connection_call_sync (conn,
                                            "org.freedesktop.DBus",
                                            "/org/freedesktop/DBus",
                                            "org.freedesktop.DBus",
                                            "NameHasOwner",
                                            g_variant_new ("(s)", CLIPBOARD_DBUS_NAME),
                                            G_VARIANT_TYPE ("(b)"),
                                            G_DBUS_CALL_FLAGS_NONE,
                                            1000,
                                            NULL,
                                            &error);
      if (result != NULL)
        {
          g_variant_get (result, "(b)", &has_owner);
          g_variant_unref (result);
        }
      else
        {
          g_warning ("Failed to check clipboard manager: %s", error->message);
          g_error_free (error);
          has_owner = FALSE;
        }

      g_object_unref (conn);
      return has_owner;
    }
#endif

  return FALSE;
}



/**
 * xfce_clipboard_manager_store:
 *
 * Persists the current clipboard content so it survives app exit.
 *
 * On X11, delegates to gtk_clipboard_store().
 * On Wayland, makes a synchronous D-Bus call to the clipboard manager
 * using Xfce custom interface, which reads and caches the current selection.
 *
 * Return value: %TRUE on success, %FALSE if persistence failed or no
 * manager is available.
 *
 * Since: 4.21.10
 **/
gboolean
xfce_clipboard_manager_store (void)
{
  GdkDisplay *display = gdk_display_get_default ();

#ifdef ENABLE_X11
  if (GDK_IS_X11_DISPLAY (display))
    {
      gtk_clipboard_store (gtk_clipboard_get (GDK_SELECTION_CLIPBOARD));
      return TRUE;
    }
#endif

#ifdef ENABLE_WAYLAND
  if (GDK_IS_WAYLAND_DISPLAY (display))
    {
      GDBusConnection *conn;
      GError *error = NULL;
      StoreData data = { .loop = NULL, .success = FALSE };

      conn = g_bus_get_sync (G_BUS_TYPE_SESSION, NULL, &error);
      if (G_UNLIKELY (error))
        {
          g_warning ("Failed to connect to session bus: %s", error->message);
          g_error_free (error);
          return FALSE;
        }

      /* Async + nested main loop: a sync call would block GTK's Wayland
       * event processing, deadlocking the clipboard data transfer */
      data.loop = g_main_loop_new (NULL, FALSE);

      g_dbus_connection_call (conn,
                              CLIPBOARD_DBUS_NAME,
                              CLIPBOARD_DBUS_PATH,
                              CLIPBOARD_DBUS_INTERFACE,
                              "Store",
                              NULL,
                              G_VARIANT_TYPE ("(b)"),
                              G_DBUS_CALL_FLAGS_NONE,
                              10000,
                              NULL,
                              clipboard_store_cb,
                              &data);

      g_main_loop_run (data.loop);
      g_main_loop_unref (data.loop);
      g_object_unref (conn);
      return data.success;
    }
#endif

  return FALSE;
}



#define __XFCE_CLIPBOARD_MANAGER_CLIENT_C__
#include "libxfce4ui-visibility.c"
