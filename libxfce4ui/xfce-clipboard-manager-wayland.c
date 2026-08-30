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
 * SECTION:xfce-clipboard-manager-wayland
 * @title: XfceClipboardManagerWayland
 * @short_description: Wayland clipboard manager
 * @stability: Stable
 * @include: libxfce4ui/libxfce4ui.h
 *
 * Wayland clipboard manager using the ext-data-control-v1 protocol.
 * Tracks the current selection source and, on explicit Store() request,
 * reads and caches its data, then re-serves it as the new selection owner.
 **/

#include <errno.h>
#include <fcntl.h>
#include <poll.h>
#include <string.h>
#include <unistd.h>

#include <gio/gunixoutputstream.h>
#include <gdk/gdkwayland.h>
#include <gtk/gtk.h>

#include "ext-data-control-v1-client.h"
#include "xfce-clipboard-manager-wayland.h"
#include "libxfce4ui-visibility.h"



typedef struct
{
  gchar *mime_type;
  GBytes *data;
} MimeData;



struct _XfceClipboardManagerWayland
{
  GObject __parent__;

  struct wl_display *wl_display;
  struct wl_registry *wl_registry;
  struct ext_data_control_manager_v1 *dc_manager;
  struct ext_data_control_device_v1 *dc_device;

  struct ext_data_control_offer_v1 *incoming_offer;
  GPtrArray *incoming_mime_types;

  struct ext_data_control_offer_v1 *active_offer;
  GPtrArray *active_mime_types;

  GPtrArray *cached_contents;
  struct ext_data_control_source_v1 *dc_source;
};



static void xfce_clipboard_manager_wayland_finalize (GObject *object);

static void registry_global (void *data, struct wl_registry *registry,
                             uint32_t id, const char *interface, uint32_t version);
static void registry_global_remove (void *data, struct wl_registry *registry, uint32_t id);

static void device_data_offer (void *data, struct ext_data_control_device_v1 *device,
                               struct ext_data_control_offer_v1 *offer);
static void device_selection (void *data, struct ext_data_control_device_v1 *device,
                              struct ext_data_control_offer_v1 *offer);
static void device_finished (void *data, struct ext_data_control_device_v1 *device);
static void device_primary_selection (void *data, struct ext_data_control_device_v1 *device,
                                      struct ext_data_control_offer_v1 *offer);

static void offer_offer (void *data, struct ext_data_control_offer_v1 *offer,
                         const char *mime_type);

static void source_send (void *data, struct ext_data_control_source_v1 *source,
                         const char *mime_type, int32_t fd);
static void source_cancelled (void *data, struct ext_data_control_source_v1 *source);



static const struct wl_registry_listener registry_listener = {
  .global = registry_global,
  .global_remove = registry_global_remove,
};

static const struct ext_data_control_device_v1_listener device_listener = {
  .data_offer = device_data_offer,
  .selection = device_selection,
  .finished = device_finished,
  .primary_selection = device_primary_selection,
};

static const struct ext_data_control_offer_v1_listener offer_listener = {
  .offer = offer_offer,
};

static const struct ext_data_control_source_v1_listener source_listener = {
  .send = source_send,
  .cancelled = source_cancelled,
};



G_DEFINE_FINAL_TYPE (XfceClipboardManagerWayland, xfce_clipboard_manager_wayland, G_TYPE_OBJECT)



static void
xfce_clipboard_manager_wayland_class_init (XfceClipboardManagerWaylandClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);
  object_class->finalize = xfce_clipboard_manager_wayland_finalize;
}



static void
xfce_clipboard_manager_wayland_init (XfceClipboardManagerWayland *manager)
{
}



static void
clear_incoming_offer (XfceClipboardManagerWayland *manager)
{
  if (manager->incoming_offer != NULL)
    {
      ext_data_control_offer_v1_destroy (manager->incoming_offer);
      manager->incoming_offer = NULL;
    }
  g_clear_pointer (&manager->incoming_mime_types, g_ptr_array_unref);
}



static void
clear_active_offer (XfceClipboardManagerWayland *manager)
{
  if (manager->active_offer != NULL)
    {
      ext_data_control_offer_v1_destroy (manager->active_offer);
      manager->active_offer = NULL;
    }
  g_clear_pointer (&manager->active_mime_types, g_ptr_array_unref);
}



static void
clear_cached_contents (XfceClipboardManagerWayland *manager)
{
  g_clear_pointer (&manager->cached_contents, g_ptr_array_unref);
}



static void
clear_source (XfceClipboardManagerWayland *manager)
{
  if (manager->dc_source != NULL)
    {
      ext_data_control_source_v1_destroy (manager->dc_source);
      manager->dc_source = NULL;
    }
}



static void
xfce_clipboard_manager_wayland_finalize (GObject *object)
{
  XfceClipboardManagerWayland *manager = XFCE_CLIPBOARD_MANAGER_WAYLAND (object);

  clear_source (manager);
  clear_cached_contents (manager);
  clear_active_offer (manager);
  clear_incoming_offer (manager);

  if (manager->dc_device != NULL)
    ext_data_control_device_v1_destroy (manager->dc_device);
  if (manager->dc_manager != NULL)
    ext_data_control_manager_v1_destroy (manager->dc_manager);
  if (manager->wl_registry != NULL)
    wl_registry_destroy (manager->wl_registry);

  G_OBJECT_CLASS (xfce_clipboard_manager_wayland_parent_class)->finalize (object);
}



static void
mime_data_free (MimeData *md)
{
  g_free (md->mime_type);
  g_clear_pointer (&md->data, g_bytes_unref);
  g_slice_free (MimeData, md);
}



static GPtrArray *
mime_types_array_new (void)
{
  return g_ptr_array_new_with_free_func (g_free);
}



static GPtrArray *
cached_contents_array_new (void)
{
  return g_ptr_array_new_with_free_func ((GDestroyNotify) mime_data_free);
}



static GBytes *
read_all_from_fd (int fd, int timeout_ms)
{
  GByteArray *buf = g_byte_array_new ();
  guint8 chunk[65536];
  gssize nread;
  struct pollfd pfd = { .fd = fd, .events = POLLIN };

  while (TRUE)
    {
      int ret = poll (&pfd, 1, timeout_ms);
      if (ret == 0)
        {
          g_warning ("Clipboard read timed out after %d ms", timeout_ms);
          break;
        }
      else if (ret < 0)
        {
          if (errno == EINTR)
            continue;
          g_warning ("Clipboard poll error: %s", g_strerror (errno));
          break;
        }

      nread = read (fd, chunk, sizeof (chunk));
      if (nread > 0)
        {
          g_byte_array_append (buf, chunk, nread);
        }
      else if (nread == 0)
        {
          break;
        }
      else
        {
          if (errno == EINTR)
            continue;
          g_warning ("Clipboard read error: %s", g_strerror (errno));
          break;
        }
    }

  close (fd);
  return g_byte_array_free_to_bytes (buf);
}



static void
write_all_to_fd (int fd, const guint8 *data, gsize len)
{
  GOutputStream *stream = g_unix_output_stream_new (fd, TRUE);
  GError *error = NULL;

  if (!g_output_stream_write_all (stream, data, len, NULL, NULL, &error))
    {
      g_warning ("Clipboard write error: %s", error->message);
      g_error_free (error);
    }

  g_object_unref (stream);
}



static GBytes *
receive_mime_data (XfceClipboardManagerWayland *manager,
                   struct ext_data_control_offer_v1 *offer,
                   const gchar *mime_type)
{
  int fds[2];

  if (pipe (fds) < 0)
    {
      g_warning ("Failed to create pipe: %s", g_strerror (errno));
      return g_bytes_new (NULL, 0);
    }
  fcntl (fds[0], F_SETFD, FD_CLOEXEC);
  fcntl (fds[1], F_SETFD, FD_CLOEXEC);

  ext_data_control_offer_v1_receive (offer, mime_type, fds[1]);
  close (fds[1]);

  wl_display_flush (manager->wl_display);

  return read_all_from_fd (fds[0], 2000);
}



static gboolean
mime_type_in_list (const gchar *mime, const gchar **list)
{
  for (; *list != NULL; list++)
    {
      if (g_strcmp0 (mime, *list) == 0)
        return TRUE;
    }
  return FALSE;
}



static gboolean
cache_selection (XfceClipboardManagerWayland *manager)
{
  struct ext_data_control_offer_v1 *offer;
  GPtrArray *mime_types;
  GPtrArray *contents;

  /* X11 atoms (meaningless on Wayland), ICO variants (GTK offers but
   * can't produce), and misc container aliases */
  static const gchar *skip_types[] = {
    "SAVE_TARGETS", "TARGETS", "MULTIPLE", "DELETE",
    "INSERT_PROPERTY", "INSERT_SELECTION", "TIMESTAMP",
    "COMPOUND_TEXT", "UTF8_STRING", "STRING", "TEXT",
    "image/x-icon", "image/x-ico", "image/x-win-bitmap",
    "image/vnd.microsoft.icon", "application/ico",
    "image/ico", "image/icon", "text/ico",
    "audio/x-riff",
    NULL
  };

  offer = manager->active_offer;
  mime_types = manager->active_mime_types;

  if (offer == NULL || mime_types == NULL || mime_types->len == 0)
    return FALSE;

  contents = cached_contents_array_new ();

  for (guint i = 0; i < mime_types->len; i++)
    {
      const gchar *mime = g_ptr_array_index (mime_types, i);
      gboolean skip = FALSE;
      GBytes *data;
      gsize size;

      if (mime_type_in_list (mime, skip_types))
        skip = TRUE;

      /* Skip duplicates */
      if (!skip)
        {
          for (guint j = 0; j < contents->len; j++)
            {
              MimeData *existing = g_ptr_array_index (contents, j);
              if (g_strcmp0 (existing->mime_type, mime) == 0)
                {
                  skip = TRUE;
                  break;
                }
            }
        }

      /* Skip image/x-* and image/vnd.* aliases when a canonical
       * image/ form is already cached */
      if (!skip
          && (g_str_has_prefix (mime, "image/x-")
              || g_str_has_prefix (mime, "image/vnd.")))
        {
          for (guint j = 0; j < contents->len; j++)
            {
              MimeData *existing = g_ptr_array_index (contents, j);
              if (g_str_has_prefix (existing->mime_type, "image/")
                  && !g_str_has_prefix (existing->mime_type, "image/x-")
                  && !g_str_has_prefix (existing->mime_type, "image/vnd."))
                {
                  skip = TRUE;
                  break;
                }
            }
        }

      if (skip)
        continue;

      data = receive_mime_data (manager, offer, mime);
      size = g_bytes_get_size (data);

      if (size > 0)
        {
          MimeData *md = g_slice_new (MimeData);
          md->mime_type = g_strdup (mime);
          md->data = data;
          g_ptr_array_add (contents, md);
        }
      else
        g_bytes_unref (data);
    }

  if (contents->len == 0)
    {
      g_ptr_array_unref (contents);
      return FALSE;
    }

  clear_cached_contents (manager);
  manager->cached_contents = contents;

  return TRUE;
}



static void
become_selection_owner (XfceClipboardManagerWayland *manager)
{
  if (manager->cached_contents == NULL || manager->cached_contents->len == 0)
    return;

  clear_source (manager);

  manager->dc_source = ext_data_control_manager_v1_create_data_source (manager->dc_manager);
  ext_data_control_source_v1_add_listener (manager->dc_source, &source_listener, manager);

  for (guint i = 0; i < manager->cached_contents->len; i++)
    {
      MimeData *md = g_ptr_array_index (manager->cached_contents, i);
      ext_data_control_source_v1_offer (manager->dc_source, md->mime_type);
    }

  ext_data_control_device_v1_set_selection (manager->dc_device, manager->dc_source);
  wl_display_flush (manager->wl_display);
}



static void
registry_global (void *data,
                 struct wl_registry *registry,
                 uint32_t id,
                 const char *interface,
                 uint32_t version)
{
  XfceClipboardManagerWayland *manager = data;

  if (g_strcmp0 (ext_data_control_manager_v1_interface.name, interface) == 0)
    {
      manager->dc_manager = wl_registry_bind (registry, id,
                                              &ext_data_control_manager_v1_interface,
                                              MIN ((uint32_t) ext_data_control_manager_v1_interface.version, version));
    }
}



static void
registry_global_remove (void *data,
                        struct wl_registry *registry,
                        uint32_t id)
{
}



static void
device_data_offer (void *data,
                   struct ext_data_control_device_v1 *device,
                   struct ext_data_control_offer_v1 *offer)
{
  XfceClipboardManagerWayland *manager = data;

  clear_incoming_offer (manager);

  manager->incoming_offer = offer;
  manager->incoming_mime_types = mime_types_array_new ();
  ext_data_control_offer_v1_add_listener (offer, &offer_listener, manager);
}



static void
device_selection (void *data,
                  struct ext_data_control_device_v1 *device,
                  struct ext_data_control_offer_v1 *offer)
{
  XfceClipboardManagerWayland *manager = data;

  if (offer == NULL)
    {
      clear_active_offer (manager);
      if (manager->cached_contents != NULL && manager->cached_contents->len > 0)
        become_selection_owner (manager);
      return;
    }

  /* We own the selection — ignore our own events */
  if (manager->dc_source != NULL)
    {
      clear_incoming_offer (manager);
      return;
    }

  clear_active_offer (manager);

  manager->active_offer = manager->incoming_offer;
  manager->active_mime_types = manager->incoming_mime_types;
  manager->incoming_offer = NULL;
  manager->incoming_mime_types = NULL;
}



static void
device_finished (void *data,
                 struct ext_data_control_device_v1 *device)
{
  XfceClipboardManagerWayland *manager = data;

  clear_active_offer (manager);
  clear_incoming_offer (manager);

  if (manager->dc_device != NULL)
    {
      ext_data_control_device_v1_destroy (manager->dc_device);
      manager->dc_device = NULL;
    }
}



static void
device_primary_selection (void *data,
                          struct ext_data_control_device_v1 *device,
                          struct ext_data_control_offer_v1 *offer)
{
  XfceClipboardManagerWayland *manager = data;

  /* Clear incoming_offer set by the preceding data_offer event —
   * we don't manage primary selection */
  if (offer != NULL)
    clear_incoming_offer (manager);
}



static void
offer_offer (void *data,
             struct ext_data_control_offer_v1 *offer,
             const char *mime_type)
{
  XfceClipboardManagerWayland *manager = data;

  if (manager->incoming_mime_types != NULL)
    g_ptr_array_add (manager->incoming_mime_types, g_strdup (mime_type));
}



static void
source_send (void *data,
             struct ext_data_control_source_v1 *source,
             const char *mime_type,
             int32_t fd)
{
  XfceClipboardManagerWayland *manager = data;

  if (manager->cached_contents == NULL)
    {
      close (fd);
      return;
    }

  for (guint i = 0; i < manager->cached_contents->len; i++)
    {
      MimeData *md = g_ptr_array_index (manager->cached_contents, i);
      if (g_strcmp0 (md->mime_type, mime_type) == 0)
        {
          gsize size;
          const guint8 *bytes_data = g_bytes_get_data (md->data, &size);
          write_all_to_fd (fd, bytes_data, size);
          return;
        }
    }

  close (fd);
}



static void
source_cancelled (void *data,
                  struct ext_data_control_source_v1 *source)
{
  XfceClipboardManagerWayland *manager = data;

  if (manager->dc_source == source)
    {
      ext_data_control_source_v1_destroy (manager->dc_source);
      manager->dc_source = NULL;
      clear_cached_contents (manager);
    }
  else
    ext_data_control_source_v1_destroy (source);
}



/**
 * xfce_clipboard_manager_wayland_new:
 *
 * Creates a new Wayland clipboard manager that uses the
 * ext-data-control-v1 protocol to persist clipboard data.
 *
 * Return value: (nullable) (transfer full): A new #XfceClipboardManagerWayland
 * instance or %NULL if the protocol is not available.
 *
 * Since: 4.21.10
 **/
XfceClipboardManagerWayland *
xfce_clipboard_manager_wayland_new (void)
{
  XfceClipboardManagerWayland *manager;
  GdkDisplay *display;
  struct wl_seat *seat;

  display = gdk_display_get_default ();
  if (!GDK_IS_WAYLAND_DISPLAY (display))
    {
      g_warning ("Cannot create Wayland clipboard manager: not a Wayland display");
      return NULL;
    }

  manager = g_object_new (XFCE_TYPE_CLIPBOARD_MANAGER_WAYLAND, NULL);

  manager->wl_display = gdk_wayland_display_get_wl_display (display);
  manager->wl_registry = wl_display_get_registry (manager->wl_display);
  wl_registry_add_listener (manager->wl_registry, &registry_listener, manager);
  wl_display_roundtrip (manager->wl_display);

  if (manager->dc_manager == NULL)
    {
      g_warning ("Compositor does not support ext-data-control-v1: clipboard persistence unavailable");
      g_object_unref (manager);
      return NULL;
    }

  seat = gdk_wayland_seat_get_wl_seat (gdk_display_get_default_seat (display));
  manager->dc_device = ext_data_control_manager_v1_get_data_device (manager->dc_manager, seat);
  ext_data_control_device_v1_add_listener (manager->dc_device, &device_listener, manager);

  wl_display_roundtrip (manager->wl_display);

  return manager;
}



/**
 * xfce_clipboard_manager_wayland_store:
 * @manager: an #XfceClipboardManagerWayland
 *
 * Explicitly reads and caches the current clipboard selection, then
 * sets this manager as the new selection source to persist the data.
 * Called by the D-Bus Store() handler in clipboard manager, e.g. xfsettingsd.
 *
 * Return value: %TRUE if data was successfully cached and ownership taken.
 *
 * Since: 4.21.10
 **/
gboolean
xfce_clipboard_manager_wayland_store (XfceClipboardManagerWayland *manager)
{
  g_return_val_if_fail (XFCE_IS_CLIPBOARD_MANAGER_WAYLAND (manager), FALSE);

  /* Ensure active_offer reflects the latest selection event */
  wl_display_roundtrip (manager->wl_display);

  if (manager->active_offer == NULL || manager->active_mime_types == NULL)
    {
      if (manager->cached_contents != NULL && manager->cached_contents->len > 0)
        return TRUE;
      return FALSE;
    }

  if (!cache_selection (manager))
    return FALSE;

  become_selection_owner (manager);
  return TRUE;
}



#define __XFCE_CLIPBOARD_MANAGER_WAYLAND_C__
#include "libxfce4ui-visibility.c"
