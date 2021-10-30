/* vi:set expandtab sw=2 sts=2: */
/*
 * Copyright (c) 2008 Jannis Pohlmann <jannis@xfce.org>
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
  GObjectClass *gobject_class;

  /* Make sure to use the translations from libxfce4ui */
  xfce_textdomain (GETTEXT_PACKAGE, PACKAGE_LOCALE_DIR, "UTF-8");

  gobject_class = G_OBJECT_CLASS (klass);
}



static void
xfce_shortcuts_editor_dialog_init (XfceShortcutsEditorDialog *dialog)
{
  ;
}



GtkWidget*
xfce_shortcuts_editor_dialog_new (XfceGtkActionEntry *entries,
                                  size_t              entries_count)
{
  XfceShortcutsEditorDialog *dialog;

  dialog = g_object_new (XFCE_TYPE_SHORTCUTS_EDITOR_DIALOG, NULL);
  gtk_window_set_title (GTK_WINDOW (dialog), "Shortcuts Editor");
  gtk_box_pack_start (GTK_BOX (gtk_dialog_get_content_area (GTK_DIALOG (dialog))), xfce_shortcuts_editor_new (entries, entries_count), TRUE, TRUE, 0);

  gtk_widget_show (GTK_WIDGET (dialog));

  return GTK_WIDGET (dialog);
}