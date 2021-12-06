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

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <gdk/gdkkeysyms.h>
#include <gdk/gdk.h>
#include <gdk/gdkx.h>
#include <gtk/gtk.h>

#include <libxfce4util/libxfce4util.h>
#include <libxfce4ui/libxfce4ui.h>

#include <libxfce4kbd-private/xfce-shortcuts.h>
#include <libxfce4kbd-private/xfce-shortcut-dialog.h>
#include <libxfce4kbd-private/xfce-shortcuts-editor.h>
#include <libxfce4kbd-private/xfce-shortcuts-editor-dialog.h>



struct _XfceShortcutsEditorDialogClass
{
  XfceTitledDialogClass __parent__;
};

struct _XfceShortcutsEditorDialog
{
  XfceTitledDialog    __parent__;
};



G_DEFINE_TYPE (XfceShortcutsEditorDialog, xfce_shortcuts_editor_dialog, XFCE_TYPE_TITLED_DIALOG)



static void
xfce_shortcuts_editor_dialog_class_init (XfceShortcutsEditorDialogClass *klass)
{
  /* Make sure to use the translations from libxfce4ui */
  xfce_textdomain (GETTEXT_PACKAGE, PACKAGE_LOCALE_DIR, "UTF-8");
}



static void
xfce_shortcuts_editor_dialog_init (XfceShortcutsEditorDialog *dialog)
{
  ;
}



/**
 * xfce_shortcuts_editor_dialog_new:
 * @argument_count : #int, the number of arguments, including this one.
 *
 * Returns a dialog that includes a XfceShortcutsEditor.
 * See xfce_shortcuts_editor_new_variadic for the expected types of the variable argument list.
 *
 * Since: 4.17.2
 **/
GtkWidget*
xfce_shortcuts_editor_dialog_new (int argument_count, ...)
{
  XfceShortcutsEditorDialog *dialog;
  va_list                    argument_list;
  GdkGeometry                windowProperties;

  va_start (argument_list, argument_count);

  dialog = g_object_new (XFCE_TYPE_SHORTCUTS_EDITOR_DIALOG, NULL);
  gtk_window_set_title (GTK_WINDOW (dialog), _("Shortcuts Editor"));
  gtk_box_pack_start (GTK_BOX (gtk_dialog_get_content_area (GTK_DIALOG (dialog))), xfce_shortcuts_editor_new_variadic (argument_count, argument_list), TRUE, TRUE, 0);
  gtk_widget_show (GTK_WIDGET (dialog));

  windowProperties.min_width = 500;
  windowProperties.min_height = 600;
  gtk_window_set_geometry_hints(GTK_WINDOW(dialog), NULL, &windowProperties, GDK_HINT_MIN_SIZE);

  va_end (argument_list);

  return GTK_WIDGET (dialog);
}