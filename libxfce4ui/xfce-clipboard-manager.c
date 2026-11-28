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
#include "xfce-clipboard-manager-private.h"
#include "libxfce4ui-visibility.h"



typedef struct _XfceClipboardManagerPrivate
{
  XfceDBusClipboardManager *skeleton;
  guint owner_id;
} XfceClipboardManagerPrivate;

static void
xfce_clipboard_manager_dispose (GObject *object);
static gboolean
xfce_clipboard_manager_handle_store (XfceDBusClipboardManager *skeleton,
                                     GDBusMethodInvocation *invocation,
                                     XfceClipboardManager *manager);
static void
xfce_clipboard_manager_bus_acquired (GDBusConnection *connection,
                                     const gchar *name,
                                     gpointer user_data);
static void
xfce_clipboard_manager_own_name (XfceClipboardManager *manager);



/**
 * XfceClipboardManager:
 *
 * An opaque structure representing a clipboard manager. Use
 * xfce_clipboard_manager_new() to get an instance for the current display.
 *
 * Since: 4.19.5
 **/
G_DEFINE_ABSTRACT_TYPE_WITH_PRIVATE (XfceClipboardManager, xfce_clipboard_manager, G_TYPE_OBJECT)

G_DEFINE_ENUM_TYPE ("XfceClipboardManagerError",
                    xfce_clipboard_manager_error,
                    G_DEFINE_ENUM_VALUE (XFCE_CLIPBOARD_MANAGER_ERROR_NO_SELECTION, "NoSelection"),
                    G_DEFINE_ENUM_VALUE (XFCE_CLIPBOARD_MANAGER_ERROR_READ_FAILED, "ReadFailed"))



static const GDBusErrorEntry xfce_clipboard_manager_error_entries[] = {
  { XFCE_CLIPBOARD_MANAGER_ERROR_NO_SELECTION, XFCE_CLIPBOARD_MANAGER_DBUS_NAME ".Error.NoSelection" },
  { XFCE_CLIPBOARD_MANAGER_ERROR_READ_FAILED, XFCE_CLIPBOARD_MANAGER_DBUS_NAME ".Error.ReadFailed" },
};



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

  g_clear_handle_id (&priv->owner_id, g_bus_unown_name);

  G_OBJECT_CLASS (xfce_clipboard_manager_parent_class)->dispose (object);
}



static gboolean
xfce_clipboard_manager_handle_store (XfceDBusClipboardManager *skeleton,
                                     GDBusMethodInvocation *invocation,
                                     XfceClipboardManager *manager)
{
  XfceClipboardManagerClass *klass = XFCE_CLIPBOARD_MANAGER_GET_CLASS (manager);
  gboolean success = TRUE;
  GError *error = NULL;

  if (klass->persist != NULL)
    success = klass->persist (manager, &error);

  if (success)
    xfce_dbus_clipboard_manager_complete_store (skeleton, invocation);
  else
    g_dbus_method_invocation_take_error (invocation, error);

  return TRUE;
}



GQuark
xfce_clipboard_manager_error_get_quark (void)
{
  static volatile gsize quark = 0;

  g_dbus_error_register_error_domain ("xfce_clipboard_manager_error",
                                      &quark,
                                      xfce_clipboard_manager_error_entries,
                                      G_N_ELEMENTS (xfce_clipboard_manager_error_entries));

  return quark;
}



static void
xfce_clipboard_manager_bus_acquired (GDBusConnection *connection,
                                     const gchar *name,
                                     gpointer user_data)
{
  XfceClipboardManager *manager = XFCE_CLIPBOARD_MANAGER (user_data);
  XfceClipboardManagerPrivate *priv = xfce_clipboard_manager_get_instance_private (manager);
  GError *error = NULL;

  priv->skeleton = xfce_dbus_clipboard_manager_skeleton_new ();
  g_signal_connect (priv->skeleton, "handle-store", G_CALLBACK (xfce_clipboard_manager_handle_store), manager);

  if (!g_dbus_interface_skeleton_export (G_DBUS_INTERFACE_SKELETON (priv->skeleton),
                                         connection,
                                         XFCE_CLIPBOARD_MANAGER_DBUS_PATH,
                                         &error))
    {
      g_warning ("Failed to export %s: %s", XFCE_CLIPBOARD_MANAGER_DBUS_PATH, error->message);
      g_error_free (error);
      g_signal_handlers_disconnect_by_data (priv->skeleton, manager);
      g_clear_object (&priv->skeleton);
    }
}



static void
xfce_clipboard_manager_own_name (XfceClipboardManager *manager)
{
  XfceClipboardManagerPrivate *priv = xfce_clipboard_manager_get_instance_private (manager);

  priv->owner_id = g_bus_own_name (G_BUS_TYPE_SESSION,
                                   XFCE_CLIPBOARD_MANAGER_DBUS_NAME,
                                   G_BUS_NAME_OWNER_FLAGS_NONE,
                                   xfce_clipboard_manager_bus_acquired,
                                   NULL, NULL, manager, NULL);
}



/**
 * xfce_clipboard_manager_new:
 * @replace: whether or not to replace an existing clipboard manager
 *           (only meaningful on X11, ignored on Wayland)
 *
 * Creates a new clipboard manager for the current display backend.
 *
 * Ownership of the org.xfce.ClipboardManager name on the session bus
 * is also requested, so clients can, for example, request clipboard
 * persistence with xfce_clipboard_manager_store().
 * This is done asynchronously; if another process already owns the name,
 * the request is queued until it is released. The name is released when
 * the manager is disposed.
 *
 * Return value: (nullable) (transfer full): A new #XfceClipboardManager
 * instance or %NULL on failure.
 *
 * Since: 4.19.5
 **/
XfceClipboardManager *
xfce_clipboard_manager_new (gboolean replace)
{
  XfceClipboardManager *manager = NULL;

#ifdef ENABLE_WAYLAND
  if (GDK_IS_WAYLAND_DISPLAY (gdk_display_get_default ()))
    manager = XFCE_CLIPBOARD_MANAGER (xfce_clipboard_manager_wayland_new ());
#endif

#ifdef ENABLE_X11
  if (GDK_IS_X11_DISPLAY (gdk_display_get_default ()))
    manager = XFCE_CLIPBOARD_MANAGER (xfce_clipboard_manager_x11_new (replace));
#endif

  if (manager != NULL)
    xfce_clipboard_manager_own_name (manager);

  return manager;
}



#define __XFCE_CLIPBOARD_MANAGER_C__
#include "libxfce4ui-visibility.c"
