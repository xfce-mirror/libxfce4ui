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

#ifndef __XFCE_DIE_EDITOR_H__
#define __XFCE_DIE_EDITOR_H__

#include <gtk/gtk.h>

#include "xfce-die-enum-types.h"

G_BEGIN_DECLS

#define XFCE_TYPE_DIE_EDITOR (xfce_die_editor_get_type ())
G_DECLARE_FINAL_TYPE (XfceDieEditor, xfce_die_editor, XFCE, DIE_EDITOR, GtkGrid)

GtkWidget *
xfce_die_editor_new (void) G_GNUC_MALLOC;

gboolean
xfce_die_editor_get_complete (XfceDieEditor *editor);

XfceDieEditorMode
xfce_die_editor_get_mode (XfceDieEditor *editor);
void
xfce_die_editor_set_mode (XfceDieEditor *editor,
                          XfceDieEditorMode mode);

const gchar *
xfce_die_editor_get_name (XfceDieEditor *editor);
void
xfce_die_editor_set_name (XfceDieEditor *editor,
                          const gchar *name);

const gchar *
xfce_die_editor_get_comment (XfceDieEditor *editor);
void
xfce_die_editor_set_comment (XfceDieEditor *editor,
                             const gchar *comment);

const gchar *
xfce_die_editor_get_command (XfceDieEditor *editor);
void
xfce_die_editor_set_command (XfceDieEditor *editor,
                             const gchar *command);

const gchar *
xfce_die_editor_get_url (XfceDieEditor *editor);
void
xfce_die_editor_set_url (XfceDieEditor *editor,
                         const gchar *url);

const gchar *
xfce_die_editor_get_path (XfceDieEditor *editor);
void
xfce_die_editor_set_path (XfceDieEditor *editor,
                          const gchar *path);

const gchar *
xfce_die_editor_get_icon (XfceDieEditor *editor);
void
xfce_die_editor_set_icon (XfceDieEditor *editor,
                          const gchar *icon);

gboolean
xfce_die_editor_get_snotify (XfceDieEditor *editor);
void
xfce_die_editor_set_snotify (XfceDieEditor *editor,
                             gboolean snotify);

gboolean
xfce_die_editor_get_terminal (XfceDieEditor *editor);
void
xfce_die_editor_set_terminal (XfceDieEditor *editor,
                              gboolean terminal);

G_END_DECLS

#endif /* !__XFCE_DIE_EDITOR_H__ */
