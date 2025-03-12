/*-
 * Copyright (c) 2006 Benedikt Meurer <benny@xfce.org>.
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
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston,
 * MA 02110-1301 USA
 */

#ifndef __XFCE_DIE_DESKTOP_MODEL_H__
#define __XFCE_DIE_DESKTOP_MODEL_H__

#include <glib-object.h>
#include <gtk/gtk.h>

G_BEGIN_DECLS

#define XFCE_TYPE_DIE_DESKTOP_MODEL (xfce_die_desktop_model_get_type ())
G_DECLARE_FINAL_TYPE (XfceDieDesktopModel, xfce_die_desktop_model, XFCE, DIE_DESKTOP_MODEL, GObject)

/**
 * XfceDieDesktopModelColumn:
 * @XFCE_DIE_DESKTOP_MODEL_COLUMN_ABSTRACT : the column with the markup text for the renderer.
 * @XFCE_DIE_DESKTOP_MODEL_COLUMN_COMMAND  : the column with the application command.
 * @XFCE_DIE_DESKTOP_MODEL_COLUMN_COMMENT  : the column with the application comment.
 * @XFCE_DIE_DESKTOP_MODEL_COLUMN_ICON     : the column with the application icon.
 * @XFCE_DIE_DESKTOP_MODEL_COLUMN_NAME     : the column with the application name.
 * @XFCE_DIE_DESKTOP_MODEL_COLUMN_SNOTIFY  : the column with the applications StartupNotify setting.
 * @XFCE_DIE_DESKTOP_MODEL_COLUMN_TERMINAL : the column with the applications Terminal setting.
 *
 * The columns provided by the #XfceDieDesktopModel.
 **/
typedef enum /*< enum >*/
{
  XFCE_DIE_DESKTOP_MODEL_COLUMN_ABSTRACT,
  XFCE_DIE_DESKTOP_MODEL_COLUMN_COMMAND,
  XFCE_DIE_DESKTOP_MODEL_COLUMN_COMMENT,
  XFCE_DIE_DESKTOP_MODEL_COLUMN_ICON,
  XFCE_DIE_DESKTOP_MODEL_COLUMN_NAME,
  XFCE_DIE_DESKTOP_MODEL_COLUMN_SNOTIFY,
  XFCE_DIE_DESKTOP_MODEL_COLUMN_TERMINAL,
  XFCE_DIE_DESKTOP_MODEL_N_COLUMNS,
} XfceDieDesktopModelColumn;

XfceDieDesktopModel *
xfce_die_desktop_model_new (void) G_GNUC_MALLOC;
gboolean
xfce_die_desktop_model_match_func (GtkEntryCompletion *completion,
                                   const gchar *key,
                                   GtkTreeIter *iter,
                                   gpointer user_data);

G_END_DECLS

#endif /* !__XFCE_DIE_DESKTOP_MODEL_H__ */
