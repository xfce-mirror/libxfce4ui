/* vi:set expandtab sw=2 sts=2: */
/*
 * Copyright (c) 2021 Sergios - Anestis Kefalidis <sergioskefalidis@gmail.com>
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
 * License along with this library; if not, write to the
 * Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 * Boston, MA 02110-1301, USA.
 */

#ifndef __XFCE_SHORTCUTS_EDITOR_DIALOG_H__
#define __XFCE_SHORTCUTS_EDITOR_DIALOG_H__

#include <gtk/gtk.h>

#include "libxfce4ui/libxfce4ui.h"

G_BEGIN_DECLS

#define XFCE_TYPE_SHORTCUTS_EDITOR_DIALOG (xfce_shortcuts_editor_dialog_get_type ())
G_DECLARE_FINAL_TYPE (XfceShortcutsEditorDialog, xfce_shortcuts_editor_dialog, XFCE, SHORTCUTS_EDITOR_DIALOG, XfceTitledDialog)

GtkWidget *
xfce_shortcuts_editor_dialog_new (int argument_count,
                                  ...) G_GNUC_MALLOC;

GtkWidget *
xfce_shortcuts_editor_dialog_new_with_parent (GtkWindow *parent,
                                              int argument_count,
                                              ...) G_GNUC_MALLOC;

G_END_DECLS

#endif /* !__XFCE_SHORTCUTS_EDITOR_DIALOG_H__ */
