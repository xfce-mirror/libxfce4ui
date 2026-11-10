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

#include "libxfce4ui-private.h"
#include "xfce-clipboard-manager-dbus.h"
#include "xfce-clipboard-manager.h"
#include "libxfce4ui-visibility.h"



typedef struct
{
  XfceDbusClipboardManager *skeleton;
  guint owner_id;
} XfceClipboardManagerPrivate;

static void
xfce_clipboard_manager_dispose (GObject *object);
static gboolean
xfce_clipboard_manager_handle_store (XfceDbusClipboardManager *skeleton,
                                     GDBusMethodInvocation *invocation,
                                     XfceClipboardManager *manager);



/**
 * XfceClipboardManager:
 *
 * An opaque structure representing a clipboard manager. Use
 * xfce_clipboard_manager_new() to get an instance for the current display.
 *
 * Since: 4.19.5
 **/
G_DEFINE_ABSTRACT_TYPE_WITH_PRIVATE (XfceClipboardManager, xfce_clipboard_manager, G_TYPE_OBJECT)



static void
xfce_clipboard_manager_class_init (XfceClipboardManagerClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

  object_class->dispose = xfce_clipboard_manager_dispose;
}



static void
xfce_clipboard_manager_init (XfceClipboardManager *manager)
{
}



static void
xfce_clipboard_manager_dispose (GObject *object)
{
  XfceClipboardManagerPrivate *priv = xfce_clipboard_manager_get_instance_private (XFCE_CLIPBOARD_MANAGER (object));

  if (priv->skeleton != NULL)
    {
      g_signal_handlers_disconnect_by_data (priv->skeleton, object);
      g_dbus_interface_skeleton_unexport (G_DBUS_INTERFACE_SKELETON (priv->skeleton));
      g_clear_object (&priv->skeleton);
    }

  if (priv->owner_id != 0)
    {
      g_bus_unown_name (priv->owner_id);
      priv->owner_id = 0;
    }

  G_OBJECT_CLASS (xfce_clipboard_manager_parent_class)->dispose (object);
}



static gboolean
xfce_clipboard_manager_handle_store (XfceDbusClipboardManager *skeleton,
                                     GDBusMethodInvocation *invocation,
                                     XfceClipboardManager *manager)
{
  XfceClipboardManagerClass *klass = XFCE_CLIPBOARD_MANAGER_GET_CLASS (manager);
  gboolean success = TRUE;

  if (klass->persist != NULL)
    success = klass->persist (manager);

  xfce_dbus_clipboard_manager_complete_store (skeleton, invocation, success);

  return TRUE;
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
 * xfce_clipboard_manager_register_dbus_object:
 * @manager: an #XfceClipboardManager
 * @connection: a #GDBusConnection
 * @error: return location for a #GError, or %NULL
 *
 * Registers the org.xfce.ClipboardManager interface for @manager on
 * @connection and requests the org.xfce.ClipboardManager bus name, so
 * that clients can request clipboard persistence with
 * xfce_clipboard_manager_store(). If the name is already owned, e.g. by
 * another instance, the request is queued until it is released. The
 * object and the name are released when @manager is disposed.
 *
 * Return value: %TRUE on success, %FALSE if @error is set.
 *
 * Since: 4.21.10
 **/
gboolean
xfce_clipboard_manager_register_dbus_object (XfceClipboardManager *manager,
                                             GDBusConnection *connection,
                                             GError **error)
{
  XfceClipboardManagerPrivate *priv;
  XfceDbusClipboardManager *skeleton;

  g_return_val_if_fail (XFCE_IS_CLIPBOARD_MANAGER (manager), FALSE);
  g_return_val_if_fail (G_IS_DBUS_CONNECTION (connection), FALSE);
  g_return_val_if_fail (error == NULL || *error == NULL, FALSE);

  priv = xfce_clipboard_manager_get_instance_private (manager);
  g_return_val_if_fail (priv->skeleton == NULL, FALSE);

  skeleton = xfce_dbus_clipboard_manager_skeleton_new ();
  g_signal_connect (skeleton, "handle-store", G_CALLBACK (xfce_clipboard_manager_handle_store), manager);

  if (!g_dbus_interface_skeleton_export (G_DBUS_INTERFACE_SKELETON (skeleton),
                                         connection,
                                         XFCE_CLIPBOARD_MANAGER_DBUS_PATH,
                                         error))
    {
      g_object_unref (skeleton);
      return FALSE;
    }

  priv->skeleton = skeleton;
  priv->owner_id = g_bus_own_name_on_connection (connection,
                                                 XFCE_CLIPBOARD_MANAGER_DBUS_NAME,
                                                 G_BUS_NAME_OWNER_FLAGS_NONE,
                                                 NULL, NULL, NULL, NULL);

  return TRUE;
}



#define __XFCE_CLIPBOARD_MANAGER_C__
#include "libxfce4ui-visibility.c"
