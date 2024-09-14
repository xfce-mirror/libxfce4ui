/* -*- Mode: C; tab-width: 8; indent-tabs-mode: nil; c-basic-offset: 8 -*-
 *
 * Copyright (C) 2007 Matthias Clasen
 * Copyright (C) 2007 Anders Carlsson
 * Copyright (C) 2007 Rodrigo Moya
 * Copyright (C) 2007 William Jon McCann <mccann@jhu.edu>
 * Copyright (C) 2011 Nick Schermer <nick@xfce.org>
 * Copyright (c) 2024 Gaël Bonithon <gael@xfce.org>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 *
 */

/**
 * SECTION:xfce-clipboard-manager
 * @title: XfceClipboardManager
 * @short_description: X11 clipboard manager
 * @stability: Stable
 * @include: libxfce4ui/libxfce4ui.h
 *
 * X11 clipboard manager which ensures persistence in the sense of
 * gdk_display_supports_clipboard_persistence().
 **/

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <X11/Xatom.h>
#include <X11/Xlib.h>
#include <gdk/gdkx.h>
#include <gtk/gtk.h>

#include "xfce-clipboard-manager.h"
#include "libxfce4ui-alias.h"

struct _XfceClipboardManager
{
  GObject __parent__;

  guint start_idle_id;
  Display *display;
  Window window;
  Time timestamp;

  GSList *contents;
  GSList *conversions;
  GdkPixbuf *image;
  GBytes *bytes;
  gboolean is_image_available;

  Window requestor;
  Atom property;
  Time time;
};

typedef struct
{
  guchar *data;
  gulong length;
  Atom target;
  Atom type;
  gint format;
  gint refcount;
} TargetData;

typedef struct
{
  Atom target;
  TargetData *data;
  Atom property;
  Window requestor;
  gint offset;
} IncrConversion;

static void
xfce_clipboard_manager_finalize (GObject *object);
static void
clipboard_manager_watch_cb (XfceClipboardManager *manager,
                            Window window,
                            Bool is_start,
                            long mask,
                            void *cb_data);

static gulong SELECTION_MAX_SIZE = 0;

static Atom XA_ATOM_PAIR = None;
static Atom XA_CLIPBOARD_MANAGER = None;
static Atom XA_CLIPBOARD = None;
static Atom XA_DELETE = None;
static Atom XA_INCR = None;
static Atom XA_INSERT_PROPERTY = None;
static Atom XA_INSERT_SELECTION = None;
static Atom XA_MANAGER = None;
static Atom XA_MULTIPLE = None;
static Atom XA_NULL = None;
static Atom XA_SAVE_TARGETS = None;
static Atom XA_TARGETS = None;
static Atom XA_TIMESTAMP = None;



G_DEFINE_TYPE (XfceClipboardManager, xfce_clipboard_manager, G_TYPE_OBJECT)



static void
xfce_clipboard_manager_class_init (XfceClipboardManagerClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

  object_class->finalize = xfce_clipboard_manager_finalize;
}

static void
xfce_clipboard_manager_init (XfceClipboardManager *manager)
{
  manager->display = GDK_DISPLAY_XDISPLAY (gdk_display_get_default ());
}

/* We need to use reference counting for the target data, since we may
 * need to keep the data around after loosing the CLIPBOARD ownership
 * to complete incremental transfers.
 */
static TargetData *
target_data_ref (TargetData *data)
{
  data->refcount++;
  return data;
}

static void
target_data_unref (TargetData *data)
{
  data->refcount--;
  if (data->refcount == 0)
    {
      g_free (data->data);
      g_slice_free (TargetData, data);
    }
}

static void
conversion_free (IncrConversion *rdata)
{
  if (rdata->data)
    target_data_unref (rdata->data);
  g_slice_free (IncrConversion, rdata);
}

static void
xfce_clipboard_manager_finalize (GObject *object)
{
  XfceClipboardManager *manager = XFCE_CLIPBOARD_MANAGER (object);

  if (manager->window != None)
    {
      clipboard_manager_watch_cb (manager,
                                  manager->window,
                                  False,
                                  0,
                                  NULL);
      XDestroyWindow (manager->display, manager->window);
      manager->window = None;
    }

  if (manager->conversions != NULL)
    {
      g_slist_free_full (manager->conversions, (GDestroyNotify) conversion_free);
      manager->conversions = NULL;
    }

  if (manager->contents != NULL)
    {
      g_slist_free_full (manager->contents, (GDestroyNotify) target_data_unref);
      manager->contents = NULL;
    }

  if (manager->image != NULL)
    {
      g_object_unref (manager->image);
      g_bytes_unref (manager->bytes);
      manager->image = NULL;
      manager->bytes = NULL;
    }

  if (manager->start_idle_id != 0)
    g_source_remove (manager->start_idle_id);

  G_OBJECT_CLASS (xfce_clipboard_manager_parent_class)->finalize (object);
}

static void
send_selection_notify (XfceClipboardManager *manager,
                       Bool success)
{
  XSelectionEvent notify;

  notify.type = SelectionNotify;
  notify.serial = 0;
  notify.send_event = True;
  notify.display = manager->display;
  notify.requestor = manager->requestor;
  notify.selection = XA_CLIPBOARD_MANAGER;
  notify.target = XA_SAVE_TARGETS;
  notify.property = success ? manager->property : None;
  notify.time = manager->time;

  gdk_x11_display_error_trap_push (gdk_display_get_default ());

  XSendEvent (manager->display,
              manager->requestor,
              False,
              NoEventMask,
              (XEvent *) &notify);
  XSync (manager->display, False);

  if (gdk_x11_display_error_trap_pop (gdk_display_get_default ()) != 0)
    {
      g_critical ("Failed to notify clipboard selection");
    }
}

static void
finish_selection_request (XfceClipboardManager *manager,
                          XEvent *xev,
                          Bool success)
{
  XSelectionEvent notify;

  notify.type = SelectionNotify;
  notify.serial = 0;
  notify.send_event = True;
  notify.display = xev->xselectionrequest.display;
  notify.requestor = xev->xselectionrequest.requestor;
  notify.selection = xev->xselectionrequest.selection;
  notify.target = xev->xselectionrequest.target;
  notify.property = success ? xev->xselectionrequest.property : None;
  notify.time = xev->xselectionrequest.time;

  gdk_x11_display_error_trap_push (gdk_display_get_default ());

  XSendEvent (xev->xselectionrequest.display,
              xev->xselectionrequest.requestor,
              False, NoEventMask, (XEvent *) &notify);
  XSync (manager->display, False);

  if (gdk_x11_display_error_trap_pop (gdk_display_get_default ()) != 0)
    {
      g_critical ("Failed to send selection request");
    }
}

static int
clipboard_bytes_per_item (int format)
{
  switch (format)
    {
    case 8:
      return sizeof (char);
    case 16:
      return sizeof (short);
    case 32:
      return sizeof (long);
    default:;
    }

  return 0;
}

static void
save_targets (XfceClipboardManager *manager,
              Atom *targets,
              int nitems)
{
  gint nout, i;
  Atom *multiple;
  TargetData *tdata;

  multiple = g_new (Atom, 2 * nitems);

  nout = 0;
  for (i = 0; i < nitems; i++)
    {
      if (targets[i] != XA_TARGETS
          && targets[i] != XA_MULTIPLE
          && targets[i] != XA_DELETE
          && targets[i] != XA_INSERT_PROPERTY
          && targets[i] != XA_INSERT_SELECTION
          && targets[i] != XA_PIXMAP)
        {
          tdata = g_slice_new (TargetData);
          tdata->data = NULL;
          tdata->length = 0;
          tdata->target = targets[i];
          tdata->type = None;
          tdata->format = 0;
          tdata->refcount = 1;
          manager->contents = g_slist_prepend (manager->contents, tdata);

          multiple[nout++] = targets[i];
          multiple[nout++] = targets[i];
        }
    }

  XFree (targets);

  XChangeProperty (manager->display, manager->window,
                   XA_MULTIPLE, XA_ATOM_PAIR,
                   32, PropModeReplace, (const guchar *) multiple, nout);
  g_free (multiple);

  XConvertSelection (manager->display, XA_CLIPBOARD,
                     XA_MULTIPLE, XA_MULTIPLE,
                     manager->window, manager->time);
}

static int
find_content_target (TargetData *tdata,
                     Atom *target)
{
  return !(tdata->target == *target);
}

static int
find_content_type (TargetData *tdata,
                   Atom *type)
{
  return !(tdata->type == *type);
}

static int
find_conversion_requestor (IncrConversion *rdata,
                           XEvent *xev)
{
  return !(rdata->requestor == xev->xproperty.window
           && rdata->property == xev->xproperty.atom);
}

static void
get_property (TargetData *tdata,
              XfceClipboardManager *manager)
{
  Atom type;
  gint format;
  gulong length;
  gulong remaining;
  guchar *data;

  XGetWindowProperty (manager->display,
                      manager->window,
                      tdata->target,
                      0,
                      0x1FFFFFFF,
                      True,
                      AnyPropertyType,
                      &type,
                      &format,
                      &length,
                      &remaining,
                      &data);

  if (type == None)
    {
      manager->contents = g_slist_remove (manager->contents, tdata);
      g_slice_free (TargetData, tdata);
    }
  else if (type == XA_INCR)
    {
      tdata->type = type;
      tdata->length = 0;
      XFree (data);
    }
  else
    {
      tdata->type = type;
      tdata->data = data;
      tdata->length = length * clipboard_bytes_per_item (format);
      tdata->format = format;
    }
}

static Bool
receive_incrementally (XfceClipboardManager *manager,
                       XEvent *xev)
{
  GSList *list;
  TargetData *tdata;
  Atom type;
  gint format;
  gulong length, nitems, remaining;
  guchar *data;

  if (xev->xproperty.window != manager->window)
    return False;

  list = g_slist_find_custom (manager->contents,
                              &xev->xproperty.atom,
                              (GCompareFunc) find_content_target);

  if (!list)
    return False;

  tdata = (TargetData *) list->data;

  if (tdata->type != XA_INCR)
    return False;

  XGetWindowProperty (xev->xproperty.display,
                      xev->xproperty.window,
                      xev->xproperty.atom,
                      0, 0x1FFFFFFF, True, AnyPropertyType,
                      &type, &format, &nitems, &remaining, &data);

  length = nitems * clipboard_bytes_per_item (format);
  if (length == 0)
    {
      tdata->type = type;
      tdata->format = format;

      if (!g_slist_find_custom (manager->contents,
                                &XA_INCR, (GCompareFunc) find_content_type))
        {
          /* all incremental transfers done */
          send_selection_notify (manager, True);
          manager->requestor = None;
        }

      XFree (data);
    }
  else
    {
      if (!tdata->data)
        {
          tdata->data = data;
          tdata->length = length;
        }
      else
        {
          tdata->data = g_realloc (tdata->data, tdata->length + length + 1);
          memcpy (tdata->data + tdata->length, data, length + 1);
          tdata->length += length;
          XFree (data);
        }
    }

  return True;
}

static Bool
send_incrementally (XfceClipboardManager *manager,
                    XEvent *xev)
{
  GSList *list;
  IncrConversion *rdata;
  gulong length;
  gulong items;
  gulong bytes;
  guchar *data;

  list = g_slist_find_custom (manager->conversions, xev,
                              (GCompareFunc) find_conversion_requestor);
  if (list == NULL)
    return False;

  rdata = (IncrConversion *) list->data;

  data = rdata->data->data + rdata->offset;
  length = rdata->data->length - rdata->offset;
  if (length > SELECTION_MAX_SIZE)
    length = SELECTION_MAX_SIZE;

  rdata->offset += length;

  bytes = clipboard_bytes_per_item (rdata->data->format);
  items = bytes == 0 ? 0 : length / bytes;

  XChangeProperty (manager->display, rdata->requestor,
                   rdata->property, rdata->data->type,
                   rdata->data->format, PropModeAppend,
                   data, items);

  if (length == 0)
    {
      clipboard_manager_watch_cb (manager, rdata->requestor, False,
                                  PropertyChangeMask, NULL);
      manager->conversions = g_slist_remove (manager->conversions, rdata);
      conversion_free (rdata);
    }

  return True;
}

static void
convert_clipboard_manager (XfceClipboardManager *manager,
                           XEvent *xev)
{
  Atom type = None;
  gint format;
  gulong nitems;
  gulong remaining;
  Atom *targets = NULL;
  Atom targets2[3];
  gint n_targets;

  if (xev->xselectionrequest.target == XA_SAVE_TARGETS)
    {
      if (manager->requestor != None || manager->contents != NULL)
        {
          /* We're in the middle of a conversion request, or own
           * the CLIPBOARD already
           */
          finish_selection_request (manager, xev, False);
        }
      else
        {
          gdk_x11_display_error_trap_push (gdk_display_get_default ());

          clipboard_manager_watch_cb (manager,
                                      xev->xselectionrequest.requestor,
                                      True,
                                      StructureNotifyMask,
                                      NULL);
          XSelectInput (manager->display,
                        xev->xselectionrequest.requestor,
                        StructureNotifyMask);
          XSync (manager->display, False);

          if (gdk_x11_display_error_trap_pop (gdk_display_get_default ()) != Success)
            return;

          gdk_x11_display_error_trap_push (gdk_display_get_default ());

          if (xev->xselectionrequest.property != None)
            {
              XGetWindowProperty (manager->display,
                                  xev->xselectionrequest.requestor,
                                  xev->xselectionrequest.property,
                                  0, 0x1FFFFFFF, False, XA_ATOM,
                                  &type, &format, &nitems, &remaining,
                                  (guchar **) &targets);

              if (gdk_x11_display_error_trap_pop (gdk_display_get_default ()) != Success)
                {
                  if (targets)
                    XFree (targets);

                  return;
                }
            }

          manager->requestor = xev->xselectionrequest.requestor;
          manager->property = xev->xselectionrequest.property;
          manager->time = xev->xselectionrequest.time;

          if (type == None)
            XConvertSelection (manager->display, XA_CLIPBOARD,
                               XA_TARGETS, XA_TARGETS,
                               manager->window, manager->time);
          else
            save_targets (manager, targets, nitems);
        }
    }
  else if (xev->xselectionrequest.target == XA_TIMESTAMP)
    {
      XChangeProperty (manager->display,
                       xev->xselectionrequest.requestor,
                       xev->xselectionrequest.property,
                       XA_INTEGER, 32, PropModeReplace,
                       (guchar *) &manager->timestamp, 1);

      finish_selection_request (manager, xev, True);
    }
  else if (xev->xselectionrequest.target == XA_TARGETS)
    {
      n_targets = 0;
      targets2[n_targets++] = XA_TARGETS;
      targets2[n_targets++] = XA_TIMESTAMP;
      targets2[n_targets++] = XA_SAVE_TARGETS;

      XChangeProperty (manager->display,
                       xev->xselectionrequest.requestor,
                       xev->xselectionrequest.property,
                       XA_ATOM, 32, PropModeReplace,
                       (guchar *) targets2, n_targets);

      finish_selection_request (manager, xev, True);
    }
  else
    {
      finish_selection_request (manager, xev, False);
    }
}

static void
convert_clipboard_target (IncrConversion *rdata,
                          XfceClipboardManager *manager)
{
  TargetData *tdata;
  Atom *targets;
  gint n_targets;
  GSList *list;
  gulong items;
  gulong bytes;
  XWindowAttributes atts;

  if (rdata->target == XA_TARGETS)
    {
      n_targets = g_slist_length (manager->contents) + 2;
      targets = g_new (Atom, n_targets);

      n_targets = 0;
      targets[n_targets++] = XA_TARGETS;
      targets[n_targets++] = XA_MULTIPLE;

      for (list = manager->contents; list; list = list->next)
        {
          tdata = (TargetData *) list->data;
          targets[n_targets++] = tdata->target;
        }

      XChangeProperty (manager->display, rdata->requestor,
                       rdata->property,
                       XA_ATOM, 32, PropModeReplace,
                       (guchar *) targets, n_targets);
      g_free (targets);
    }
  else
    {
      /* Convert from stored CLIPBOARD data */
      list = g_slist_find_custom (manager->contents,
                                  &rdata->target,
                                  (GCompareFunc) find_content_target);

      /* We got a target that we don't support */
      if (!list)
        return;

      tdata = (TargetData *) list->data;
      if (tdata->type == XA_INCR)
        {
          /* we haven't completely received this target yet  */
          rdata->property = None;
          return;
        }

      rdata->data = target_data_ref (tdata);
      bytes = clipboard_bytes_per_item (tdata->format);
      items = bytes == 0 ? 0 : tdata->length / bytes;
      if (tdata->length <= SELECTION_MAX_SIZE)
        XChangeProperty (manager->display, rdata->requestor,
                         rdata->property,
                         tdata->type, tdata->format, PropModeReplace,
                         tdata->data, items);
      else
        {
          /* start incremental transfer */
          rdata->offset = 0;

          gdk_x11_display_error_trap_push (gdk_display_get_default ());

          XGetWindowAttributes (manager->display, rdata->requestor, &atts);

          clipboard_manager_watch_cb (manager, rdata->requestor,
                                      True, PropertyChangeMask,
                                      NULL);

          XSelectInput (manager->display, rdata->requestor,
                        atts.your_event_mask | PropertyChangeMask);

          XChangeProperty (manager->display, rdata->requestor,
                           rdata->property,
                           XA_INCR, 32, PropModeReplace,
                           (guchar *) &items, 1);

          XSync (manager->display, False);

          if (gdk_x11_display_error_trap_pop (gdk_display_get_default ()) != 0)
            {
              g_critical ("Failed to transfer clipboard contents");
            }
        }
    }
}

static void
collect_incremental (IncrConversion *rdata,
                     XfceClipboardManager *manager)
{
  if (rdata->offset >= 0)
    manager->conversions = g_slist_prepend (manager->conversions, rdata);
  else
    conversion_free (rdata);
}

static void
convert_clipboard (XfceClipboardManager *manager,
                   XEvent *xev)
{
  GSList *list;
  GSList *conversions = NULL;
  IncrConversion *rdata;
  Atom type = None;
  gint format;
  gulong i, nitems;
  gulong remaining;
  Atom *multiple;

  if (xev->xselectionrequest.target == XA_MULTIPLE)
    {
      XGetWindowProperty (xev->xselectionrequest.display,
                          xev->xselectionrequest.requestor,
                          xev->xselectionrequest.property,
                          0, 0x1FFFFFFF, False, XA_ATOM_PAIR,
                          &type, &format, &nitems, &remaining,
                          (guchar **) &multiple);

      if (type != XA_ATOM_PAIR || nitems == 0)
        {
          if (multiple)
            g_free (multiple);
          return;
        }

      for (i = 0; i < nitems; i += 2)
        {
          rdata = g_slice_new (IncrConversion);
          rdata->requestor = xev->xselectionrequest.requestor;
          rdata->target = multiple[i];
          rdata->property = multiple[i + 1];
          rdata->data = NULL;
          rdata->offset = -1;
          conversions = g_slist_prepend (conversions, rdata);
        }
    }
  else
    {
      multiple = NULL;

      rdata = g_slice_new (IncrConversion);
      rdata->requestor = xev->xselectionrequest.requestor;
      rdata->target = xev->xselectionrequest.target;
      rdata->property = xev->xselectionrequest.property;
      rdata->data = NULL;
      rdata->offset = -1;
      conversions = g_slist_prepend (conversions, rdata);
    }

  g_slist_foreach (conversions, (GFunc) convert_clipboard_target, manager);

  if (conversions != NULL && conversions->next == NULL
      && ((IncrConversion *) conversions->data)->property == None)
    {
      finish_selection_request (manager, xev, False);
    }
  else
    {
      if (multiple)
        {
          i = 0;
          for (list = conversions; list; list = list->next)
            {
              rdata = (IncrConversion *) list->data;
              multiple[i++] = rdata->target;
              multiple[i++] = rdata->property;
            }
          XChangeProperty (xev->xselectionrequest.display,
                           xev->xselectionrequest.requestor,
                           xev->xselectionrequest.property,
                           XA_ATOM_PAIR, 32, PropModeReplace,
                           (guchar *) multiple, nitems);
        }
      finish_selection_request (manager, xev, True);
    }

  g_slist_foreach (conversions, (GFunc) collect_incremental, manager);
  g_slist_free (conversions);

  g_free (multiple);
}

static Bool
clipboard_manager_process_event (XfceClipboardManager *manager,
                                 XEvent *xev)
{
  Atom type;
  gint format;
  gulong nitems;
  gulong remaining;
  Atom *targets = NULL;
  GSList *tmp;

  if (manager->is_image_available)
    return False;

  switch (xev->xany.type)
    {
    case DestroyNotify:
      if (xev->xdestroywindow.window == manager->requestor)
        {
          g_slist_free_full (manager->contents, (GDestroyNotify) target_data_unref);
          manager->contents = NULL;

          clipboard_manager_watch_cb (manager,
                                      manager->requestor,
                                      False,
                                      0,
                                      NULL);
          manager->requestor = None;
        }
      break;

    case PropertyNotify:
      if (xev->xproperty.state == PropertyNewValue)
        {
          return receive_incrementally (manager, xev);
        }
      else
        {
          return send_incrementally (manager, xev);
        }
      break;

    case SelectionClear:
      if (xev->xany.window != manager->window)
        return False;

      if (xev->xselectionclear.selection == XA_CLIPBOARD_MANAGER)
        {
          /* We lost the manager selection */
          if (manager->contents)
            {
              g_slist_free_full (manager->contents, (GDestroyNotify) target_data_unref);
              manager->contents = NULL;

              XSetSelectionOwner (manager->display,
                                  XA_CLIPBOARD,
                                  None, manager->time);
            }

          return True;
        }
      if (xev->xselectionclear.selection == XA_CLIPBOARD)
        {
          /* We lost the clipboard selection */
          g_slist_free_full (manager->contents, (GDestroyNotify) target_data_unref);
          manager->contents = NULL;
          clipboard_manager_watch_cb (manager,
                                      manager->requestor,
                                      False,
                                      0,
                                      NULL);
          manager->requestor = None;

          return True;
        }
      break;

    case SelectionNotify:
      if (xev->xany.window != manager->window)
        return False;

      if (xev->xselection.selection == XA_CLIPBOARD)
        {
          /* a CLIPBOARD conversion is done */
          if (xev->xselection.property == XA_TARGETS)
            {
              XGetWindowProperty (xev->xselection.display,
                                  xev->xselection.requestor,
                                  xev->xselection.property,
                                  0, 0x1FFFFFFF, True, XA_ATOM,
                                  &type, &format, &nitems, &remaining,
                                  (guchar **) &targets);

              save_targets (manager, targets, nitems);
            }
          else if (xev->xselection.property == XA_MULTIPLE)
            {
              tmp = g_slist_copy (manager->contents);
              g_slist_foreach (tmp, (GFunc) get_property, manager);
              g_slist_free (tmp);

              manager->time = xev->xselection.time;
              XSetSelectionOwner (manager->display, XA_CLIPBOARD,
                                  manager->window, manager->time);

              if (manager->property != None)
                XChangeProperty (manager->display,
                                 manager->requestor,
                                 manager->property,
                                 XA_ATOM, 32, PropModeReplace,
                                 (guchar *) &XA_NULL, 1);

              if (!g_slist_find_custom (manager->contents,
                                        &XA_INCR, (GCompareFunc) find_content_type))
                {
                  /* all transfers done */
                  send_selection_notify (manager, True);
                  clipboard_manager_watch_cb (manager,
                                              manager->requestor,
                                              False,
                                              0,
                                              NULL);
                  manager->requestor = None;
                }
            }
          else if (xev->xselection.property == None)
            {
              send_selection_notify (manager, False);
              clipboard_manager_watch_cb (manager,
                                          manager->requestor,
                                          False,
                                          0,
                                          NULL);
              manager->requestor = None;
            }

          return True;
        }
      break;

    case SelectionRequest:
      if (xev->xany.window != manager->window)
        {
          return False;
        }

      if (xev->xselectionrequest.selection == XA_CLIPBOARD_MANAGER)
        {
          convert_clipboard_manager (manager, xev);
          return True;
        }
      else if (xev->xselectionrequest.selection == XA_CLIPBOARD)
        {
          convert_clipboard (manager, xev);
          return True;
        }
      break;

    default:;
    }

  return False;
}

static GdkFilterReturn
clipboard_manager_event_filter (GdkXEvent *xevent,
                                GdkEvent *event,
                                XfceClipboardManager *manager)
{
  if (clipboard_manager_process_event (manager, (XEvent *) xevent))
    {
      return GDK_FILTER_REMOVE;
    }
  else
    {
      return GDK_FILTER_CONTINUE;
    }
}

static void
clipboard_manager_watch_cb (XfceClipboardManager *manager,
                            Window window,
                            Bool is_start,
                            long mask,
                            void *cb_data)
{
  GdkWindow *gdkwin;
  GdkDisplay *display;

  display = gdk_display_get_default ();
  gdkwin = gdk_x11_window_lookup_for_display (display, window);

  if (is_start)
    {
      if (gdkwin == NULL)
        {
          gdkwin = gdk_x11_window_foreign_new_for_display (display, window);
        }
      else
        {
          g_object_ref (gdkwin);
        }

      gdk_window_add_filter (gdkwin,
                             (GdkFilterFunc) clipboard_manager_event_filter,
                             manager);
    }
  else
    {
      if (gdkwin == NULL)
        {
          return;
        }
      gdk_window_remove_filter (gdkwin,
                                (GdkFilterFunc) clipboard_manager_event_filter,
                                manager);
      g_object_unref (gdkwin);
    }
}

static void
init_atoms (Display *display)
{
  gulong max_request_size;

  if (SELECTION_MAX_SIZE > 0)
    return;

  XA_ATOM_PAIR = XInternAtom (display, "ATOM_PAIR", False);
  XA_CLIPBOARD_MANAGER = XInternAtom (display, "CLIPBOARD_MANAGER", False);
  XA_CLIPBOARD = XInternAtom (display, "CLIPBOARD", False);
  XA_DELETE = XInternAtom (display, "DELETE", False);
  XA_INCR = XInternAtom (display, "INCR", False);
  XA_INSERT_PROPERTY = XInternAtom (display, "INSERT_PROPERTY", False);
  XA_INSERT_SELECTION = XInternAtom (display, "INSERT_SELECTION", False);
  XA_MANAGER = XInternAtom (display, "MANAGER", False);
  XA_MULTIPLE = XInternAtom (display, "MULTIPLE", False);
  XA_NULL = XInternAtom (display, "NULL", False);
  XA_SAVE_TARGETS = XInternAtom (display, "SAVE_TARGETS", False);
  XA_TARGETS = XInternAtom (display, "TARGETS", False);
  XA_TIMESTAMP = XInternAtom (display, "TIMESTAMP", False);

  max_request_size = XExtendedMaxRequestSize (display);
  if (max_request_size == 0)
    max_request_size = XMaxRequestSize (display);

  SELECTION_MAX_SIZE = max_request_size - 100;
  if (SELECTION_MAX_SIZE > 262144)
    SELECTION_MAX_SIZE = 262144;
}

typedef struct _XfceTimestamp
{
  Window window;
  Atom atom;
} XfceTimestamp;

static Bool
timestamp_predicate (Display *xdisplay,
                     XEvent *xevent,
                     XPointer arg)
{
  XfceTimestamp *ts = (XfceTimestamp *) (gpointer) arg;

  return xevent->type == PropertyNotify
         && xevent->xproperty.window == ts->window
         && xevent->xproperty.atom == ts->atom;
}

static Time
get_server_time (Display *xdisplay,
                 Window window)
{
  XfceTimestamp ts;
  guchar c = 'a';
  XEvent xevent;

  /* get the current xserver timestamp */
  ts.atom = XInternAtom (xdisplay, "_TIMESTAMP_PROP", False);
  ts.window = window;
  XChangeProperty (xdisplay, window, ts.atom, ts.atom, 8, PropModeReplace, &c, 1);
  XIfEvent (xdisplay, &xevent, timestamp_predicate, (XPointer) &ts);

  return xevent.xproperty.time;
}

static gboolean
clipboard_manager_start (XfceClipboardManager *manager,
                         gboolean replace)
{
  XClientMessageEvent xev;

  init_atoms (manager->display);

  /* check if there is a clipboard manager running */
  if (!replace
      && XGetSelectionOwner (manager->display, XA_CLIPBOARD_MANAGER))
    {
      return FALSE;
    }

  manager->contents = NULL;
  manager->conversions = NULL;
  manager->requestor = None;

  manager->window = XCreateSimpleWindow (manager->display,
                                         DefaultRootWindow (manager->display),
                                         0, 0, 10, 10, 0,
                                         WhitePixel (manager->display,
                                                     DefaultScreen (manager->display)),
                                         WhitePixel (manager->display,
                                                     DefaultScreen (manager->display)));
  clipboard_manager_watch_cb (manager,
                              manager->window,
                              True,
                              PropertyChangeMask,
                              NULL);
  XSelectInput (manager->display,
                manager->window,
                PropertyChangeMask);
  manager->timestamp = get_server_time (manager->display, manager->window);

  XSetSelectionOwner (manager->display,
                      XA_CLIPBOARD_MANAGER,
                      manager->window,
                      manager->timestamp);

  /* Check to see if we managed to claim the selection. If not,
   * we treat it as if we got it then immediately lost it
   */
  if (XGetSelectionOwner (manager->display, XA_CLIPBOARD_MANAGER) == manager->window)
    {
      xev.type = ClientMessage;
      xev.window = DefaultRootWindow (manager->display);
      xev.message_type = XA_MANAGER;
      xev.format = 32;
      xev.data.l[0] = manager->timestamp;
      xev.data.l[1] = XA_CLIPBOARD_MANAGER;
      xev.data.l[2] = manager->window;
      xev.data.l[3] = 0; /* manager specific data */
      xev.data.l[4] = 0; /* manager specific data */

      XSendEvent (manager->display,
                  DefaultRootWindow (manager->display),
                  False,
                  StructureNotifyMask,
                  (XEvent *) &xev);
    }
  else
    {
      clipboard_manager_watch_cb (manager,
                                  manager->window,
                                  False,
                                  0,
                                  NULL);
    }

  manager->start_idle_id = 0;

  return TRUE;
}

static void
owner_change (GtkClipboard *clipboard,
              GdkEvent *event,
              XfceClipboardManager *manager)
{
  GdkAtom *targets;
  gint n_targets;

  manager->is_image_available = FALSE;
  if (gtk_clipboard_wait_for_targets (clipboard, &targets, &n_targets))
    {
      /* we replace the contents of the clipboard to take ownership of the image below,
       * so we must not do this if there are other formats available */
      manager->is_image_available = gtk_targets_include_image (targets, n_targets, FALSE)
                                    && !gtk_targets_include_text (targets, n_targets)
                                    && !gtk_targets_include_uri (targets, n_targets);
      g_free (targets);
    }

  if (manager->is_image_available)
    {
      GdkPixbuf *image = gtk_clipboard_wait_for_image (clipboard);
      if (image != NULL)
        {
          GBytes *bytes = gdk_pixbuf_read_pixel_bytes (image);
          if (manager->image == NULL || !g_bytes_equal (bytes, manager->bytes))
            {
              if (manager->image != NULL)
                {
                  g_object_unref (manager->image);
                  g_bytes_unref (manager->bytes);
                }
              manager->image = g_object_ref (image);
              manager->bytes = g_bytes_ref (bytes);
              gtk_clipboard_set_image (clipboard, image);
            }
          g_object_unref (image);
          g_bytes_unref (bytes);
        }
    }
  else if (manager->image != NULL)
    {
      g_object_unref (manager->image);
      g_bytes_unref (manager->bytes);
      manager->image = NULL;
      manager->bytes = NULL;
    }
}

/**
 * xfce_clipboard_manager_new: (constructor)
 * @replace: whether or not to replace an existing clipboard manager
 *
 * Return value: (nullable) (transfer full): A new #XfceClipboardManager instance
 * or %NULL if @replace is %FALSE and a clipboard manager is already running.
 *
 * Since: 4.19.5
 **/
XfceClipboardManager *
xfce_clipboard_manager_new (gboolean replace)
{
  XfceClipboardManager *manager = g_object_new (XFCE_TYPE_CLIPBOARD_MANAGER, NULL);
  if (!clipboard_manager_start (manager, replace))
    {
      g_object_unref (manager);
      return NULL;
    }

  g_signal_connect (gtk_clipboard_get (GDK_SELECTION_CLIPBOARD), "owner-change", G_CALLBACK (owner_change), manager);

  return manager;
}

#define __XFCE_CLIPBOARD_MANAGER_C__
#include "libxfce4ui-aliasdef.c"
