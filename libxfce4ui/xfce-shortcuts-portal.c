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

/**
 * SECTION:xfce-shortcuts-portal
 * @title: XfceShortcutsPortal
 * @short_description: global keyboard shortcuts through the desktop portal
 * @include: libxfce4ui/libxfce4ui.h
 *
 * #XfceShortcutsPortal registers keys through the
 * org.freedesktop.portal.GlobalShortcuts portal and emits
 * #XfceShortcutsPortal::activated when one of them is pressed. This is
 * useful on Wayland, where applications cannot grab keys themselves.
 *
 * Each instance uses its own session bus connection, so several
 * instances in the same process, e.g. panel plugins, can register with
 * different application IDs.
 *
 * Since: 4.21.10
 **/

#include <gio/gio.h>

#include "xfce-shortcuts-portal.h"
#include "libxfce4ui-visibility.h"

#define PORTAL_BUS_NAME "org.freedesktop.portal.Desktop"
#define PORTAL_PATH "/org/freedesktop/portal/desktop"
#define SHORTCUTS_INTERFACE "org.freedesktop.portal.GlobalShortcuts"



static void
xfce_shortcuts_portal_get_property (GObject *object,
                                    guint prop_id,
                                    GValue *value,
                                    GParamSpec *pspec);
static void
xfce_shortcuts_portal_set_property (GObject *object,
                                    guint prop_id,
                                    const GValue *value,
                                    GParamSpec *pspec);
static void
xfce_shortcuts_portal_constructed (GObject *object);
static void
xfce_shortcuts_portal_dispose (GObject *object);
static void
xfce_shortcuts_portal_finalize (GObject *object);
static void
xfce_shortcuts_portal_start_session (XfceShortcutsPortal *portal);
static void
xfce_shortcuts_portal_close_session (XfceShortcutsPortal *portal);
static void
xfce_shortcuts_portal_bind_keys (XfceShortcutsPortal *portal);
static void
xfce_shortcuts_portal_watch_response (XfceShortcutsPortal *portal,
                                      const gchar *token);
static void
xfce_shortcuts_portal_stop_watching (XfceShortcutsPortal *portal);



enum
{
  PROP_0,
  PROP_APP_ID,
};

enum
{
  ACTIVATED,
  LAST_SIGNAL,
};

struct _XfceShortcutsPortal
{
  GObject parent;

  gchar *app_id;
  GDBusConnection *connection;
  GCancellable *cancellable;
  gchar **keys;
  gchar *session;
  gboolean ready;
  guint generation;
  guint token;
  guint activated_id;
  guint response_id;
};

static guint portal_signals[LAST_SIGNAL];



G_DEFINE_FINAL_TYPE (XfceShortcutsPortal, xfce_shortcuts_portal, G_TYPE_OBJECT)



static void
xfce_shortcuts_portal_class_init (XfceShortcutsPortalClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

  object_class->get_property = xfce_shortcuts_portal_get_property;
  object_class->set_property = xfce_shortcuts_portal_set_property;
  object_class->constructed = xfce_shortcuts_portal_constructed;
  object_class->dispose = xfce_shortcuts_portal_dispose;
  object_class->finalize = xfce_shortcuts_portal_finalize;

  /**
   * XfceShortcutsPortal:app-id:
   *
   * The application ID registered with the portal. It should match
   * the name of an installed desktop file, without the ".desktop"
   * suffix.
   *
   * Since: 4.21.10
   **/
  g_object_class_install_property (object_class, PROP_APP_ID,
                                   g_param_spec_string ("app-id", NULL, NULL, NULL,
                                                        G_PARAM_READWRITE
                                                          | G_PARAM_CONSTRUCT_ONLY
                                                          | G_PARAM_STATIC_STRINGS));

  /**
   * XfceShortcutsPortal::activated:
   * @portal: the #XfceShortcutsPortal
   * @key: the key that was pressed, as given to
   *       xfce_shortcuts_portal_set_keys()
   *
   * Emitted when one of the registered keys is pressed.
   *
   * Since: 4.21.10
   **/
  portal_signals[ACTIVATED] = g_signal_new ("activated",
                                            G_TYPE_FROM_CLASS (klass),
                                            G_SIGNAL_RUN_LAST,
                                            0, NULL, NULL, NULL,
                                            G_TYPE_NONE, 1, G_TYPE_STRING);
}



static void
xfce_shortcuts_portal_init (XfceShortcutsPortal *portal)
{
  portal->cancellable = g_cancellable_new ();
}



static void
xfce_shortcuts_portal_get_property (GObject *object,
                                    guint prop_id,
                                    GValue *value,
                                    GParamSpec *pspec)
{
  XfceShortcutsPortal *portal = XFCE_SHORTCUTS_PORTAL (object);

  switch (prop_id)
    {
    case PROP_APP_ID:
      g_value_set_string (value, portal->app_id);
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
    }
}



static void
xfce_shortcuts_portal_set_property (GObject *object,
                                    guint prop_id,
                                    const GValue *value,
                                    GParamSpec *pspec)
{
  XfceShortcutsPortal *portal = XFCE_SHORTCUTS_PORTAL (object);

  switch (prop_id)
    {
    case PROP_APP_ID:
      portal->app_id = g_value_dup_string (value);
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
    }
}



static void
xfce_shortcuts_portal_activated (GDBusConnection *connection,
                                 const gchar *sender,
                                 const gchar *path,
                                 const gchar *interface,
                                 const gchar *signal,
                                 GVariant *parameters,
                                 gpointer data)
{
  XfceShortcutsPortal *portal = data;
  const gchar *session;
  const gchar *key;

  g_variant_get (parameters, "(&o&st@a{sv})", &session, &key, NULL, NULL);
  if (g_strcmp0 (session, portal->session) == 0)
    g_signal_emit (portal, portal_signals[ACTIVATED], 0, key);
}



static void
xfce_shortcuts_portal_registered (GObject *source,
                                  GAsyncResult *result,
                                  gpointer data)
{
  XfceShortcutsPortal *portal;
  GVariant *reply;
  GError *error = NULL;

  reply = g_dbus_connection_call_finish (G_DBUS_CONNECTION (source), result, &error);
  if (reply == NULL)
    {
      gboolean cancelled = g_error_matches (error, G_IO_ERROR, G_IO_ERROR_CANCELLED);

      g_error_free (error);
      if (cancelled)
        return;
    }
  else
    g_variant_unref (reply);

  portal = data;
  portal->ready = TRUE;
  if (portal->keys != NULL && portal->keys[0] != NULL)
    xfce_shortcuts_portal_start_session (portal);
}



static void
xfce_shortcuts_portal_connected (GObject *source,
                                 GAsyncResult *result,
                                 gpointer data)
{
  XfceShortcutsPortal *portal;
  GDBusConnection *connection;

  connection = g_dbus_connection_new_for_address_finish (result, NULL);
  if (connection == NULL)
    return;

  portal = data;
  portal->connection = connection;
  portal->activated_id = g_dbus_connection_signal_subscribe (connection, PORTAL_BUS_NAME,
                                                             SHORTCUTS_INTERFACE, "Activated",
                                                             PORTAL_PATH, NULL,
                                                             G_DBUS_SIGNAL_FLAGS_NONE,
                                                             xfce_shortcuts_portal_activated,
                                                             portal, NULL);
  g_dbus_connection_call (connection, PORTAL_BUS_NAME, PORTAL_PATH,
                          "org.freedesktop.host.portal.Registry", "Register",
                          g_variant_new ("(s@a{sv})", portal->app_id != NULL ? portal->app_id : "",
                                         g_variant_new_array (G_VARIANT_TYPE ("{sv}"), NULL, 0)),
                          NULL, G_DBUS_CALL_FLAGS_NONE, -1, portal->cancellable,
                          xfce_shortcuts_portal_registered, portal);
}



static void
xfce_shortcuts_portal_constructed (GObject *object)
{
  XfceShortcutsPortal *portal = XFCE_SHORTCUTS_PORTAL (object);
  gchar *address;

  G_OBJECT_CLASS (xfce_shortcuts_portal_parent_class)->constructed (object);

  address = g_dbus_address_get_for_bus_sync (G_BUS_TYPE_SESSION, NULL, NULL);
  if (address != NULL)
    g_dbus_connection_new_for_address (address,
                                       G_DBUS_CONNECTION_FLAGS_AUTHENTICATION_CLIENT
                                         | G_DBUS_CONNECTION_FLAGS_MESSAGE_BUS_CONNECTION,
                                       NULL, portal->cancellable,
                                       xfce_shortcuts_portal_connected, portal);
  g_free (address);
}



static void
xfce_shortcuts_portal_dispose (GObject *object)
{
  XfceShortcutsPortal *portal = XFCE_SHORTCUTS_PORTAL (object);

  g_cancellable_cancel (portal->cancellable);
  if (portal->connection != NULL)
    {
      xfce_shortcuts_portal_close_session (portal);
      if (portal->activated_id != 0)
        g_dbus_connection_signal_unsubscribe (portal->connection, portal->activated_id);
      portal->activated_id = 0;
      g_dbus_connection_flush_sync (portal->connection, NULL, NULL);
      g_clear_object (&portal->connection);
    }

  G_OBJECT_CLASS (xfce_shortcuts_portal_parent_class)->dispose (object);
}



static void
xfce_shortcuts_portal_finalize (GObject *object)
{
  XfceShortcutsPortal *portal = XFCE_SHORTCUTS_PORTAL (object);

  g_object_unref (portal->cancellable);
  g_strfreev (portal->keys);
  g_free (portal->app_id);

  G_OBJECT_CLASS (xfce_shortcuts_portal_parent_class)->finalize (object);
}



static gchar *
xfce_shortcuts_portal_next_token (XfceShortcutsPortal *portal)
{
  return g_strdup_printf ("xfce%u_%u", portal->generation, ++portal->token);
}



static void
xfce_shortcuts_portal_response (GDBusConnection *connection,
                                const gchar *sender,
                                const gchar *path,
                                const gchar *interface,
                                const gchar *signal,
                                GVariant *parameters,
                                gpointer data)
{
  XfceShortcutsPortal *portal = data;
  guint32 response;
  GVariant *results;
  const gchar *session = NULL;

  xfce_shortcuts_portal_stop_watching (portal);
  g_variant_get (parameters, "(u@a{sv})", &response, &results);
  if (response == 0 && portal->session == NULL
      && g_variant_lookup (results, "session_handle", "&s", &session))
    {
      portal->session = g_strdup (session);
      xfce_shortcuts_portal_bind_keys (portal);
    }
  g_variant_unref (results);
}



static void
xfce_shortcuts_portal_start_session (XfceShortcutsPortal *portal)
{
  GVariantBuilder options;
  gchar *token = xfce_shortcuts_portal_next_token (portal);
  gchar *session_token = xfce_shortcuts_portal_next_token (portal);

  xfce_shortcuts_portal_watch_response (portal, token);
  g_variant_builder_init (&options, G_VARIANT_TYPE_VARDICT);
  g_variant_builder_add (&options, "{sv}", "handle_token", g_variant_new_string (token));
  g_variant_builder_add (&options, "{sv}", "session_handle_token",
                         g_variant_new_string (session_token));
  g_dbus_connection_call (portal->connection, PORTAL_BUS_NAME, PORTAL_PATH,
                          SHORTCUTS_INTERFACE, "CreateSession",
                          g_variant_new ("(a{sv})", &options), NULL,
                          G_DBUS_CALL_FLAGS_NONE, -1, portal->cancellable, NULL, NULL);
  g_free (token);
  g_free (session_token);
}



static void
xfce_shortcuts_portal_close_session (XfceShortcutsPortal *portal)
{
  xfce_shortcuts_portal_stop_watching (portal);
  if (portal->session == NULL)
    return;

  g_dbus_connection_call (portal->connection, PORTAL_BUS_NAME, portal->session,
                          "org.freedesktop.portal.Session", "Close", NULL, NULL,
                          G_DBUS_CALL_FLAGS_NONE, -1, NULL, NULL, NULL);
  g_clear_pointer (&portal->session, g_free);
}



static void
xfce_shortcuts_portal_bind_keys (XfceShortcutsPortal *portal)
{
  GVariantBuilder shortcuts;
  GVariantBuilder options;
  gchar *token = xfce_shortcuts_portal_next_token (portal);

  g_variant_builder_init (&shortcuts, G_VARIANT_TYPE ("a(sa{sv})"));
  for (guint i = 0; portal->keys != NULL && portal->keys[i] != NULL; i++)
    {
      GVariantBuilder properties;

      g_variant_builder_init (&properties, G_VARIANT_TYPE_VARDICT);
      g_variant_builder_add (&properties, "{sv}", "description",
                             g_variant_new_string (portal->keys[i]));
      g_variant_builder_add (&properties, "{sv}", "preferred_trigger",
                             g_variant_new_string (portal->keys[i]));
      g_variant_builder_add (&shortcuts, "(sa{sv})", portal->keys[i], &properties);
    }
  g_variant_builder_init (&options, G_VARIANT_TYPE_VARDICT);
  g_variant_builder_add (&options, "{sv}", "handle_token", g_variant_new_string (token));
  g_dbus_connection_call (portal->connection, PORTAL_BUS_NAME, PORTAL_PATH,
                          SHORTCUTS_INTERFACE, "BindShortcuts",
                          g_variant_new ("(oa(sa{sv})sa{sv})", portal->session, &shortcuts, "",
                                         &options),
                          NULL, G_DBUS_CALL_FLAGS_NONE, -1, portal->cancellable, NULL, NULL);
  g_free (token);
}



static void
xfce_shortcuts_portal_watch_response (XfceShortcutsPortal *portal,
                                      const gchar *token)
{
  gchar *sender = g_strdup (g_dbus_connection_get_unique_name (portal->connection) + 1);
  gchar *path;

  g_strdelimit (sender, ".", '_');
  path = g_strdup_printf (PORTAL_PATH "/request/%s/%s", sender, token);
  xfce_shortcuts_portal_stop_watching (portal);
  portal->response_id = g_dbus_connection_signal_subscribe (portal->connection, PORTAL_BUS_NAME,
                                                            "org.freedesktop.portal.Request",
                                                            "Response", path, NULL,
                                                            G_DBUS_SIGNAL_FLAGS_NONE,
                                                            xfce_shortcuts_portal_response,
                                                            portal, NULL);
  g_free (path);
  g_free (sender);
}



static void
xfce_shortcuts_portal_stop_watching (XfceShortcutsPortal *portal)
{
  if (portal->response_id != 0)
    g_dbus_connection_signal_unsubscribe (portal->connection, portal->response_id);
  portal->response_id = 0;
}



/**
 * xfce_shortcuts_portal_new:
 * @app_id: (nullable): the application ID to register with the portal
 *
 * Creates a new #XfceShortcutsPortal. No key is registered until
 * xfce_shortcuts_portal_set_keys() is called.
 *
 * Return value: (transfer full): a new #XfceShortcutsPortal.
 *
 * Since: 4.21.10
 **/
XfceShortcutsPortal *
xfce_shortcuts_portal_new (const gchar *app_id)
{
  return g_object_new (XFCE_TYPE_SHORTCUTS_PORTAL, "app-id", app_id, NULL);
}



/**
 * xfce_shortcuts_portal_set_keys:
 * @portal: an #XfceShortcutsPortal
 * @keys: (nullable) (array zero-terminated=1): the keys to register,
 *        e.g. "XF86AudioRaiseVolume", or %NULL to register none
 *
 * Replaces the registered keys with @keys. Each key is used both as the
 * shortcut ID and as the preferred trigger.
 *
 * Since: 4.21.10
 **/
void
xfce_shortcuts_portal_set_keys (XfceShortcutsPortal *portal,
                                const gchar *const *keys)
{
  g_return_if_fail (XFCE_IS_SHORTCUTS_PORTAL (portal));

  g_strfreev (portal->keys);
  portal->keys = g_strdupv ((gchar **) keys);
  portal->generation++;
  if (!portal->ready)
    return;

  xfce_shortcuts_portal_close_session (portal);
  if (portal->keys != NULL && portal->keys[0] != NULL)
    xfce_shortcuts_portal_start_session (portal);
}

#define __XFCE_SHORTCUTS_PORTAL_C__
#include "libxfce4ui-visibility.c"
