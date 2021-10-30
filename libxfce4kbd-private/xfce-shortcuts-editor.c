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



typedef struct {
  XfceShortcutsEditor *editor;
  XfceGtkActionEntry  *entry;
} ShortcutClickedData;

typedef struct {
  gboolean        in_use;
  GdkModifierType mods;
  guint           key;
  gchar          *current_path;
  gchar          *other_path;
} ShortcutInfo;

typedef struct {
  XfceGtkActionEntry *entries;
  size_t              size;
} ActionEntryArray;



static void     xfce_shortcuts_editor_finalize           (GObject             *object);
static void     xfce_shortcuts_editor_create_contents    (XfceShortcutsEditor *editor);
static void     xfce_shortcuts_editor_shortcut_clicked   (GtkWidget           *widget,
                                                          ShortcutClickedData *data);
static void     xfce_shortcuts_editor_shortcut_check     (gpointer             data,
                                                          const gchar         *path,
                                                          guint                key,
                                                          GdkModifierType      mods,
                                                          gboolean             changed);
static gboolean xfce_shortcuts_editor_validate_shortcut  (XfceShortcutDialog  *editor,
                                                          const gchar         *shortcut,
                                                          ShortcutClickedData *data);



struct _XfceShortcutsEditorClass
{
  GtkVBoxClass __parent__;
};

struct _XfceShortcutsEditor
{
  GtkVBox             __parent__;

  ActionEntryArray *arrays;
  size_t            arrays_count;
};



G_DEFINE_TYPE (XfceShortcutsEditor, xfce_shortcuts_editor, GTK_TYPE_BOX)



static void
xfce_shortcuts_editor_class_init (XfceShortcutsEditorClass *klass)
{
  GObjectClass *gobject_class;

  /* Make sure to use the translations from libxfce4ui */
  xfce_textdomain (GETTEXT_PACKAGE, PACKAGE_LOCALE_DIR, "UTF-8");

  gobject_class = G_OBJECT_CLASS (klass);
  gobject_class->finalize = xfce_shortcuts_editor_finalize;
}



static void
xfce_shortcuts_editor_init (XfceShortcutsEditor *editor)
{
  ;
}



static void
xfce_shortcuts_editor_finalize (GObject *object)
{
  XfceShortcutsEditor *editor = XFCE_SHORTCUTS_EDITOR (object);

  g_free (editor->arrays);

  (*G_OBJECT_CLASS (xfce_shortcuts_editor_parent_class)->finalize) (object);
}



GtkWidget*
xfce_shortcuts_editor_new (int argument_count, ...)
{
  GtkWidget *editor;
  va_list    argument_list;

  va_start (argument_list, argument_count);

  editor = xfce_shortcuts_editor_new_variadic (argument_count, argument_list);

  va_end (argument_list);

  return editor;
}



GtkWidget
*xfce_shortcuts_editor_new_variadic (int     argument_count,
                                     va_list argument_list)
{
  XfceShortcutsEditor *editor;

  if (argument_count % 2 != 1)
    return NULL;

  editor = g_object_new (XFCE_TYPE_SHORTCUTS_EDITOR, NULL);

  editor->arrays_count = (argument_count - 1) / 2;
  editor->arrays = g_malloc (sizeof (ActionEntryArray) * editor->arrays_count);

  for (int i = 0; i * 2 + 1 < argument_count; i++)
    {
      editor->arrays[i].entries = va_arg (argument_list, XfceGtkActionEntry*);
      editor->arrays[i].size = va_arg (argument_list, size_t);
    }

  xfce_shortcuts_editor_create_contents (editor);

  gtk_widget_show (GTK_WIDGET (editor));

  return GTK_WIDGET (editor);
}



void
free_data (gpointer  data,
           GClosure *closure)
{
  g_free (data);
}



static void
xfce_shortcuts_editor_create_contents (XfceShortcutsEditor *editor)
{
  GtkWidget *vbox;
  GtkWidget *frame;
  GtkWidget *swin;
  GtkWidget *grid;
  GtkWidget *label;
  GtkWidget *box;
  GtkWidget *shortcut_button;
  GtkWidget *discard_button;
  size_t     row;

  vbox = GTK_WIDGET (editor);

  frame = g_object_new (GTK_TYPE_FRAME, "border-width", 0, "shadow-type", GTK_SHADOW_ETCHED_IN, NULL);
  gtk_box_pack_start (GTK_BOX (vbox), frame, FALSE, TRUE, 0);
  gtk_widget_show (frame);

  swin = gtk_scrolled_window_new (NULL, NULL);
  gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (swin), GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
  gtk_scrolled_window_set_shadow_type (GTK_SCROLLED_WINDOW (swin), GTK_SHADOW_IN);
  gtk_widget_set_hexpand (swin, TRUE);
  gtk_widget_set_vexpand (swin, TRUE);
  gtk_container_add (GTK_CONTAINER (frame), swin);
  gtk_widget_show (swin);

  grid = gtk_grid_new ();
  gtk_grid_set_column_spacing (GTK_GRID (grid), 12);
  gtk_grid_set_row_spacing (GTK_GRID (grid), 6);
  gtk_widget_set_margin_top (GTK_WIDGET (grid), 6);
  gtk_widget_set_margin_start (GTK_WIDGET (grid), 12);
  gtk_container_add (GTK_CONTAINER (swin), grid);
  gtk_widget_show (grid);

  row = 0;
  for (size_t array_idx = 0; array_idx < editor->arrays_count; array_idx++)
    {
      for (size_t entry_idx = 0; entry_idx < editor->arrays[array_idx].size; entry_idx++)
        {
          ShortcutClickedData *data = malloc (sizeof (ShortcutClickedData));
          XfceGtkActionEntry   entry = editor->arrays[array_idx].entries[entry_idx];
          GtkAccelKey          key;
          gchar               *shortcut_text;

          label = gtk_label_new_with_mnemonic (entry.menu_item_label_text);
          gtk_widget_set_hexpand (label, TRUE);
          gtk_widget_set_halign (label, GTK_ALIGN_START);
          gtk_grid_attach (GTK_GRID (grid), label, 0, row, 1, 1);
          gtk_widget_show (label);

          box = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 2);
          gtk_grid_attach (GTK_GRID (grid), box, 1, row, 1, 1);
          gtk_widget_show (box);

          gtk_accel_map_lookup_entry (entry.accel_path, &key);
          shortcut_text = gtk_accelerator_get_label (key.accel_key, key.accel_mods);
          shortcut_button = gtk_button_new_with_label (key.accel_key > 0 ? shortcut_text : "...");
          g_free (shortcut_text);
          gtk_box_pack_start (GTK_BOX (box), shortcut_button, TRUE, TRUE, 0);
          gtk_widget_show (shortcut_button);

          data->editor = editor;
          data->entry = editor->arrays[array_idx].entries + entry_idx;
          g_signal_connect_data (G_OBJECT (shortcut_button), "clicked", G_CALLBACK (xfce_shortcuts_editor_shortcut_clicked), data, free_data, 0);

          discard_button = gtk_button_new_from_icon_name ("edit-delete", GTK_ICON_SIZE_BUTTON);
          gtk_box_pack_end (GTK_BOX (box), discard_button, FALSE, FALSE, 0);
          gtk_widget_show (discard_button);

          row++;
        }
    }
}



static void
xfce_shortcuts_editor_shortcut_check (gpointer       data,
                                      const gchar    *path,
                                      guint           key,
                                      GdkModifierType mods,
                                      gboolean        changed)
{
  ShortcutInfo *info = (ShortcutInfo*) data;
  if (info->in_use)
    return;

  info->in_use = info->mods == mods &&
                 info->key == key &&
                 g_strcmp0 (info->current_path, path) != 0;

  if (info->in_use)
    info->other_path = g_strdup (path);
}



static gboolean
xfce_shortcuts_editor_validate_shortcut (XfceShortcutDialog        *editor,
                                        const gchar                *shortcut,
                                        ShortcutClickedData        *data)
{
  GdkModifierType accel_mods;
  guint           accel_key;
  ShortcutInfo    info;
  gchar          *command, *message;

  g_return_val_if_fail (XFCE_IS_SHORTCUT_DIALOG (editor), FALSE);
  g_return_val_if_fail (shortcut != NULL, FALSE);

  /* Ignore empty shortcuts */
  if (G_UNLIKELY (g_utf8_strlen (shortcut, -1) == 0))
    return FALSE;

  /* Ignore raw 'Return' and 'space' since that may have been used to activate the shortcut row */
  if (G_UNLIKELY (g_utf8_collate (shortcut, "Return") == 0 ||
                  g_utf8_collate (shortcut, "space") == 0))
    return FALSE;

  gtk_accelerator_parse (shortcut, &accel_key, &accel_mods);

  info.in_use = FALSE;
  info.mods = accel_mods;
  info.key = accel_key;
  info.current_path = data->entry->accel_path;
  info.other_path = NULL;

  gtk_accel_map_foreach_unfiltered (&info, xfce_shortcuts_editor_shortcut_check);

  if (info.in_use)
    {
      command = g_strrstr (info.other_path, "/");
      command = command == NULL ?
                info.other_path :
                command + 1; /* skip leading slash */

      message = g_strdup_printf (_("This keyboard shortcut is currently used by: '%s'"),
                                 command);
      xfce_dialog_show_warning (GTK_WINDOW (gtk_widget_get_toplevel (GTK_WIDGET (editor))), message,
                                _("Keyboard shortcut already in use"));
      g_free (message);
    }
  else
    {
      if (accel_key > 0)
        gtk_accel_map_change_entry (data->entry->accel_path, accel_key, accel_mods, TRUE);
    }

  g_free (info.other_path);

  return !info.in_use;
}



static void
xfce_shortcuts_editor_shortcut_clicked (GtkWidget           *widget,
                                        ShortcutClickedData *data)
{
  GtkWidget       *dialog;
  gint             response;
  const gchar     *shortcut;
  GdkModifierType  accel_mods;
  guint            accel_key;
  gchar           *label;

  dialog = xfce_shortcut_dialog_new ("", data->entry->menu_item_label_text, "");

  g_signal_connect (dialog, "validate-shortcut", G_CALLBACK (xfce_shortcuts_editor_validate_shortcut), data);

  response = xfce_shortcut_dialog_run (XFCE_SHORTCUT_DIALOG (dialog), gtk_widget_get_toplevel (GTK_WIDGET (data->editor)));

  if (G_LIKELY (response == GTK_RESPONSE_OK))
    {
      shortcut = xfce_shortcut_dialog_get_shortcut (XFCE_SHORTCUT_DIALOG (dialog));
      gtk_accelerator_parse (shortcut, &accel_key, &accel_mods);

      label = gtk_accelerator_get_label (accel_key, accel_mods);
      gtk_button_set_label (GTK_BUTTON (widget), label);

      g_free (label);
    }

  gtk_widget_destroy (dialog);
}