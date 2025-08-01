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

#include <gdk/gdkkeysyms.h>

#include "libxfce4ui/libxfce4ui.h"

#include "xfce-shortcut-dialog.h"
#include "xfce-shortcuts-editor.h"
#include "xfce-shortcuts.h"
#include "libxfce4kbd-private-visibility.h"



typedef struct
{
  XfceShortcutsEditor *editor;
  XfceGtkActionEntry *entry;
  size_t section_index;
  const gchar *displayed_label;
} ShortcutEditClickedData;

typedef struct
{
  XfceShortcutsEditor *editor;
  GtkWidget *shortcut_button;
  XfceGtkActionEntry *entry;
  size_t section_index;
} ShortcutOtherClickedData;

typedef struct
{
  gboolean in_use;
  GdkModifierType mods;
  guint key;
  const gchar *current_path;
  gchar *other_path;

  /* A set of all accel paths that are allowed to have the same key+modifiers as 'current_path'
   * If no overlap is allowed, this will be NULL. */
  GHashTable *paths_with_overlap_allowed; /* gchar *accel_path (no values, used as set) */

} ShortcutInfo;



static void
xfce_shortcuts_editor_finalize (GObject *object);
static void
xfce_shortcuts_editor_create_contents (XfceShortcutsEditor *editor);
static void
xfce_shortcuts_editor_shortcut_clicked (GtkWidget *widget,
                                        ShortcutEditClickedData *data);
static void
xfce_shortcuts_editor_shortcut_clear_clicked (GtkWidget *widget,
                                              ShortcutOtherClickedData *data);
static void
xfce_shortcuts_editor_shortcut_reset_clicked (GtkWidget *widget,
                                              ShortcutOtherClickedData *data);
static void
xfce_shortcuts_editor_shortcut_check (gpointer data,
                                      const gchar *path,
                                      guint key,
                                      GdkModifierType mods,
                                      gboolean changed);
static gboolean
xfce_shortcuts_editor_validate_shortcut (XfceShortcutDialog *editor,
                                         const gchar *shortcut,
                                         ShortcutEditClickedData *data);
static void
free_data (gpointer data,
           GClosure *closure);



struct _XfceShortcutsEditorClass
{
  GtkVBoxClass __parent__;
};

struct _XfceShortcutsEditor
{
  GtkVBox __parent__;

  XfceShortcutsEditorSection *arrays;
  size_t arrays_count;

  /* Each group in this list is an array of XfceShortcutsEditorSection indexes
   * (into 'arrays' above). Within the sections listed in a group, shortcuts
   * are allowed to overlap (key+modifiers can be the same).
   * A section can be in multiple groups.*/
  GList *groups_with_overlap_allowed; /* GArray<size_t arrays_index> */
};



G_DEFINE_TYPE (XfceShortcutsEditor, xfce_shortcuts_editor, GTK_TYPE_BOX)



static void
xfce_shortcuts_editor_class_init (XfceShortcutsEditorClass *klass)
{
  GObjectClass *gobject_class;

  /* Make sure to use the translations from libxfce4ui */
  bindtextdomain (GETTEXT_PACKAGE, PACKAGE_LOCALE_DIR);
#ifdef HAVE_BIND_TEXTDOMAIN_CODESET
  bind_textdomain_codeset (GETTEXT_PACKAGE, "UTF-8");
#endif

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

  for (GList *l = editor->groups_with_overlap_allowed; l != NULL; l = l->next)
    {
      g_array_free (l->data, TRUE);
    }
  g_list_free (editor->groups_with_overlap_allowed);

  for (size_t i = 0; i < editor->arrays_count; i++)
    g_free (editor->arrays[i].section_name);
  g_free (editor->arrays);

  (*G_OBJECT_CLASS (xfce_shortcuts_editor_parent_class)->finalize) (object);
}



/**
 * xfce_shortcuts_editor_new:
 * @argument_count : #int, the number of arguments, including this one.
 *
 * A variable arguments version of xfce_shortcuts_editor_new_variadic.
 *
 * Since: 4.17.2
 **/
GtkWidget *
xfce_shortcuts_editor_new (int argument_count, ...)
{
  GtkWidget *editor;
  va_list argument_list;

  va_start (argument_list, argument_count);

  editor = xfce_shortcuts_editor_new_variadic (argument_count, argument_list);

  va_end (argument_list);

  return editor;
}



/**
 * xfce_shortcuts_editor_new_array:
 * @sections   : an array of XfceShortcutsEditorSection triads (see
 *               xfce_shortcuts_editor_new_variadic for a description).
 * @n_sections : #int, the size of the array.
 *
 * A vectorized version of xfce_shortcuts_editor_new.
 *
 * Since: 4.17.5
 **/
GtkWidget *
xfce_shortcuts_editor_new_array (XfceShortcutsEditorSection *sections,
                                 int n_sections)
{
  XfceShortcutsEditor *editor;

  editor = g_object_new (XFCE_TYPE_SHORTCUTS_EDITOR, NULL);

  editor->arrays_count = n_sections;
  editor->arrays = g_new (XfceShortcutsEditorSection, n_sections);

  for (int i = 0; i < n_sections; i++)
    {
      editor->arrays[i].section_name = g_strdup (sections[i].section_name);
      editor->arrays[i].entries = sections[i].entries;
      editor->arrays[i].size = sections[i].size;
    }

  xfce_shortcuts_editor_create_contents (editor);

  gtk_widget_show (GTK_WIDGET (editor));

  return GTK_WIDGET (editor);
}



/**
 * xfce_shortcuts_editor_new_variadic:
 * @argument_count : #int, the number of arguments, including this one.
 * @argument_list : a #va_list containing the arguments
 *
 * Create a new Shortcuts Editor from XfceGtkActionEntry arrays.
 *
 * The @argument_list consists of triads of arguments.
 * The types of a triad are the following: (gchar*, XfceGtkActionEntry[], size_t).
 *
 * The first member of a triad is the name of the section in the editor. Specify
 * %NULL or an empty string so that no section appears.
 * The second member of a triad is the array of XfceGtkActionEntries which are
 * used to handle the shortcuts.
 * The third member is the size of that array.
 *
 * Since: 4.17.2
 **/
GtkWidget *
xfce_shortcuts_editor_new_variadic (int argument_count,
                                    va_list argument_list)
{
  XfceShortcutsEditor *editor;

  if (argument_count % 3 != 1)
    return NULL;

  editor = g_object_new (XFCE_TYPE_SHORTCUTS_EDITOR, NULL);

  editor->arrays_count = (argument_count - 1) / 3;
  editor->arrays = g_new (XfceShortcutsEditorSection, editor->arrays_count);

  for (int i = 0; i * 3 + 1 < argument_count; i++)
    {
      editor->arrays[i].section_name = g_strdup (va_arg (argument_list, gchar *));
      editor->arrays[i].entries = va_arg (argument_list, XfceGtkActionEntry *);
      editor->arrays[i].size = va_arg (argument_list, size_t);
    }

  xfce_shortcuts_editor_create_contents (editor);

  gtk_widget_show (GTK_WIDGET (editor));

  return GTK_WIDGET (editor);
}



/**
 * xfce_shortcuts_editor_add_overlap_group:
 * @editor: A #XfceShortcutsEditor.
 * @first_section_index: The first index in the array of #XfceShortcutsEditorSection.
 * @...: Other indexes that should be in the group, as ints, terminated with -1.
 *
 * Adds an overlap group to @editor, which consists of a list of
 * #XfceShortcutsEditorSection indexes. Within actions inside all sections of
 * the group, there may be duplicate shortcuts set.
 *
 * Don't forget the terminating -1 at the end of the variable list of indexes.
 **/
void
xfce_shortcuts_editor_add_overlap_group (XfceShortcutsEditor *editor,
                                         int first_section_index,
                                         ...)
{
  /* NB: This function takes ints instead of ssize_t because the compiler, at
   * the call site, will not know that the varargs are supposed to be ssize_t,
   * but will assume they are just ints.  Since those types are not the same
   * size, we'll end up with invalid data in the function.  Expecting the
   * caller to know this and cast to ssize_t is unreasonable.  But when we put
   * the values into the GArray, they need to be the correct type and size for
   * the array. */

  g_return_if_fail (XFCE_IS_SHORTCUTS_EDITOR (editor));
  g_return_if_fail (first_section_index >= 0);

  GArray *group_indexes = g_array_sized_new (FALSE, TRUE, sizeof (size_t), 2);
  size_t first_section_index_pos = first_section_index;
  g_array_append_val (group_indexes, first_section_index_pos);

  va_list argument_list;
  va_start (argument_list, first_section_index);

  for (int index = va_arg (argument_list, int); index >= 0; index = va_arg (argument_list, int))
    {
      size_t index_pos = index;
#ifndef G_DISABLE_CHECKS
      if (index_pos >= editor->arrays_count)
        {
          va_end (argument_list);
          g_return_if_fail (index_pos < editor->arrays_count);
          return; // Shut up gcc-analyzer
        }
#endif
      g_array_append_val (group_indexes, index_pos);
    }

  va_end (argument_list);

  editor->groups_with_overlap_allowed = g_list_prepend (editor->groups_with_overlap_allowed, group_indexes);
}



/**
 * xfce_shortcuts_editor_add_overlap_group_array:
 * @editor: A #XfceShortcutsEditor.
 * @section_indexes: An array of indexes.
 * @n_section_indexes: The count of elements in @section_indexes.
 *
 * Adds an overlap group to @editor, which consists of a list of
 * #XfceShortcutsEditorSection indexes.  Within actions inside all sections of
 * the group, there may be duplicate shortcuts set.
 **/
void
xfce_shortcuts_editor_add_overlap_group_array (XfceShortcutsEditor *editor,
                                               size_t *section_indexes,
                                               size_t n_section_indexes)
{
  g_return_if_fail (XFCE_IS_SHORTCUTS_EDITOR (editor));
  g_return_if_fail (section_indexes != NULL);
  g_return_if_fail (n_section_indexes > 0);
#ifndef G_DISABLE_CHECKS
  for (size_t i = 0; i < n_section_indexes; ++i)
    {
      g_return_if_fail (section_indexes[i] < editor->arrays_count);
    }
#endif

  GArray *group_indexes = g_array_sized_new (FALSE, FALSE, sizeof (size_t), n_section_indexes);
  memcpy (group_indexes->data, section_indexes, sizeof (*section_indexes) * n_section_indexes);
  group_indexes->len = n_section_indexes;
  editor->groups_with_overlap_allowed = g_list_prepend (editor->groups_with_overlap_allowed, group_indexes);
}



static void
free_data (gpointer data,
           GClosure *closure)
{
  g_free (data);
}



static void
xfce_shortcuts_editor_create_contents (XfceShortcutsEditor *editor)
{
  GtkAdjustment *adjustment;
  GtkWidget *vbox;
  GtkWidget *frame;
  GtkWidget *swin;
  GtkWidget *grid;
  GtkWidget *label;
  GtkWidget *box;
  GtkWidget *box2;
  GtkWidget *shortcut_button;
  GtkWidget *clear_button;
  GtkWidget *reset_button;
  size_t row;

  vbox = GTK_WIDGET (editor);

  frame = g_object_new (GTK_TYPE_FRAME, "border-width", 0, "shadow-type", GTK_SHADOW_NONE, NULL);
  gtk_box_pack_start (GTK_BOX (vbox), frame, FALSE, TRUE, 0);
  gtk_widget_show (frame);

  swin = gtk_scrolled_window_new (NULL, NULL);
  gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (swin), GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
  gtk_widget_set_hexpand (swin, TRUE);
  gtk_widget_set_vexpand (swin, TRUE);
  gtk_container_add (GTK_CONTAINER (frame), swin);
  gtk_widget_show (swin);

  adjustment = gtk_scrolled_window_get_hadjustment (GTK_SCROLLED_WINDOW (swin));
  gtk_adjustment_set_value (adjustment, 0);
  gtk_scrolled_window_set_hadjustment (GTK_SCROLLED_WINDOW (swin), adjustment);

  grid = gtk_grid_new ();
  gtk_grid_set_column_spacing (GTK_GRID (grid), 12);
  gtk_grid_set_row_spacing (GTK_GRID (grid), 6);
  gtk_widget_set_margin_top (GTK_WIDGET (grid), 6);
  gtk_widget_set_margin_start (GTK_WIDGET (grid), 12);
  gtk_widget_set_margin_end (GTK_WIDGET (grid), 16);
  gtk_container_add (GTK_CONTAINER (swin), grid);
  gtk_widget_show (grid);

  row = 0;
  for (size_t array_idx = 0; array_idx < editor->arrays_count; array_idx++)
    {
      gchar *markup;

      /* leave an empty row before each section */
      if (array_idx != 0)
        {
          label = gtk_label_new ("");
          gtk_grid_attach (GTK_GRID (grid), label, 0, row, 2, 1);
          gtk_widget_show (label);
          row++;
        }

      /* section name */
      if (!xfce_str_is_empty (editor->arrays[array_idx].section_name))
        {
          label = gtk_label_new ("");
          markup = g_strconcat ("<b>", editor->arrays[array_idx].section_name, "</b>", NULL);
          gtk_label_set_markup (GTK_LABEL (label), markup);
          g_free (markup);
          gtk_widget_set_vexpand (label, TRUE);
          gtk_widget_set_hexpand (label, TRUE);
          gtk_widget_set_halign (label, GTK_ALIGN_START);
          gtk_grid_attach (GTK_GRID (grid), label, 0, row, 1, 1);
          gtk_widget_show (label);
          row++;
        }

      /* section shortcut entries */
      for (size_t entry_idx = 0; entry_idx < editor->arrays[array_idx].size; entry_idx++)
        {
          ShortcutEditClickedData *data;
          ShortcutOtherClickedData *clear_data;
          ShortcutOtherClickedData *reset_data;
          XfceGtkActionEntry entry;
          GtkAccelKey key;
          gchar *shortcut_text;

          entry = editor->arrays[array_idx].entries[entry_idx];

          /* ignore entries that don't do something when activated */
          if (entry.callback == NULL)
            continue;

          /* ignore entries that don't have an accelerator path */
          if (entry.accel_path == NULL || g_strcmp0 (entry.accel_path, "") == 0)
            continue;

          data = g_new (ShortcutEditClickedData, 1);
          clear_data = g_new (ShortcutOtherClickedData, 1);
          reset_data = g_new (ShortcutOtherClickedData, 1);

          label = gtk_label_new_with_mnemonic (entry.menu_item_label_text);
          data->displayed_label = gtk_label_get_text (GTK_LABEL (label));
          gtk_label_set_ellipsize (GTK_LABEL (label), PANGO_ELLIPSIZE_END);
          gtk_widget_set_tooltip_text (label, data->displayed_label);
          gtk_widget_set_hexpand (label, TRUE);
          gtk_widget_set_halign (label, GTK_ALIGN_START);
          gtk_widget_set_margin_start (label, 10);
          gtk_grid_attach (GTK_GRID (grid), label, 0, row, 1, 1);
          gtk_widget_show (label);

          box = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 4);
          gtk_grid_attach (GTK_GRID (grid), box, 1, row, 1, 1);
          gtk_widget_show (box);

          /* contains the edit shortcut button and the clear button */
          box2 = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 0);
          gtk_box_pack_start (GTK_BOX (box), box2, TRUE, TRUE, 0);
          gtk_widget_show (box2);
          gtk_style_context_add_class (gtk_widget_get_style_context (box2), "linked");

          /* edit shortcut button */
          gtk_accel_map_lookup_entry (entry.accel_path, &key);
          shortcut_text = gtk_accelerator_get_label (key.accel_key, key.accel_mods);
          shortcut_button = gtk_button_new_with_label (key.accel_key > 0 ? shortcut_text : "...");
          gtk_widget_set_tooltip_text (shortcut_button, _("The current shortcut. Click to edit..."));
          g_free (shortcut_text);
          gtk_box_pack_start (GTK_BOX (box2), shortcut_button, TRUE, TRUE, 0);
          gtk_widget_show (shortcut_button);

          data->editor = editor;
          data->entry = editor->arrays[array_idx].entries + entry_idx;
          data->section_index = array_idx;
          g_signal_connect_data (G_OBJECT (shortcut_button), "clicked", G_CALLBACK (xfce_shortcuts_editor_shortcut_clicked), data, free_data, 0);

          /* clear button */
          clear_button = gtk_button_new_from_icon_name ("edit-clear", GTK_ICON_SIZE_BUTTON);
          gtk_widget_set_tooltip_text (clear_button, _("Clear the shortcut"));
          gtk_box_pack_end (GTK_BOX (box2), clear_button, FALSE, TRUE, 0);
          gtk_widget_show (clear_button);

          clear_data->editor = editor;
          clear_data->shortcut_button = shortcut_button;
          clear_data->entry = editor->arrays[array_idx].entries + entry_idx;
          clear_data->section_index = array_idx;
          g_signal_connect_data (G_OBJECT (clear_button), "clicked", G_CALLBACK (xfce_shortcuts_editor_shortcut_clear_clicked), clear_data, free_data, 0);

          /* reset button */
          reset_button = gtk_button_new_from_icon_name ("edit-undo", GTK_ICON_SIZE_BUTTON);
          gtk_widget_set_tooltip_text (reset_button, _("Restore the default shortcut"));
          gtk_box_pack_end (GTK_BOX (box), reset_button, FALSE, FALSE, 0);
          gtk_widget_show (reset_button);

          reset_data->editor = editor;
          reset_data->shortcut_button = shortcut_button;
          reset_data->entry = editor->arrays[array_idx].entries + entry_idx;
          reset_data->section_index = array_idx;
          g_signal_connect_data (G_OBJECT (reset_button), "clicked", G_CALLBACK (xfce_shortcuts_editor_shortcut_reset_clicked), reset_data, free_data, 0);

          row++;
        }
    }
}



static inline gboolean
shortcut_in_overlap_group (ShortcutInfo *info,
                           const gchar *path)
{
  if (info->paths_with_overlap_allowed == NULL)
    return FALSE;

  return g_hash_table_contains (info->paths_with_overlap_allowed, path);
}



static void
xfce_shortcuts_editor_shortcut_check (gpointer data,
                                      const gchar *path,
                                      guint key,
                                      GdkModifierType mods,
                                      gboolean changed)
{
  ShortcutInfo *info = (ShortcutInfo *) data;
  if (info->in_use)
    return;

  info->in_use = info->mods == mods
                 && info->key == key
                 && g_strcmp0 (info->current_path, path) != 0
                 && !shortcut_in_overlap_group (info, path);

  if (info->in_use)
    info->other_path = g_strdup (path);
}



static GHashTable *
build_paths_with_overlap_allowed_set (XfceShortcutsEditor *editor,
                                      size_t changed_section_index)
{
  if (editor->groups_with_overlap_allowed != NULL)
    {
      GHashTable *paths_with_overlap_allowed = NULL;
      GHashTable *sections_with_overlap_allowed = g_hash_table_new (g_direct_hash, g_direct_equal);

      /* First build a list of section indexes that this accelerator's section can overlap with */
      for (GList *l = editor->groups_with_overlap_allowed; l != NULL; l = l->next)
        {
          GArray *overlap_allowed_group = l->data;

          gboolean overlap_allowed = FALSE;
          for (gsize group_idx = 0; group_idx < overlap_allowed_group->len; ++group_idx)
            {
              size_t section_index = g_array_index (overlap_allowed_group, size_t, group_idx);
              if (section_index == changed_section_index)
                {
                  overlap_allowed = TRUE;
                  break;
                }
            }

          if (overlap_allowed)
            {
              for (gsize group_idx = 0; group_idx < overlap_allowed_group->len; ++group_idx)
                {
                  size_t section_index = g_array_index (overlap_allowed_group, size_t, group_idx);

                  /* Dont allow overlap within our own section */
                  if (changed_section_index != section_index)
                    g_hash_table_add (sections_with_overlap_allowed, GUINT_TO_POINTER (section_index));
                }
            }
        }

      paths_with_overlap_allowed = g_hash_table_new (g_str_hash, g_str_equal);

      /* Insert paths that are allowed to overlap with this accelerator into 'paths_with_overlap_allowed' */
      for (size_t array_idx = 0; array_idx < editor->arrays_count; ++array_idx)
        {
          XfceShortcutsEditorSection *section = &editor->arrays[array_idx];
          for (size_t entry_idx = 0; entry_idx < section->size; ++entry_idx)
            {
              gchar *accel_path = (gchar *) section->entries[entry_idx].accel_path;
              if (accel_path != NULL && g_hash_table_contains (sections_with_overlap_allowed, GUINT_TO_POINTER (array_idx)))
                {
                  g_hash_table_add (paths_with_overlap_allowed, accel_path);
                }
            }
        }

      g_hash_table_destroy (sections_with_overlap_allowed);

      return paths_with_overlap_allowed;
    }
  else
    {
      return NULL;
    }
}



static gboolean
xfce_shortcuts_editor_validate_shortcut (XfceShortcutDialog *editor,
                                         const gchar *shortcut,
                                         ShortcutEditClickedData *data)
{
  GdkModifierType accel_mods;
  guint accel_key;
  ShortcutInfo info;
  gchar *command, *message;

  g_return_val_if_fail (XFCE_IS_SHORTCUT_DIALOG (editor), FALSE);
  g_return_val_if_fail (shortcut != NULL, FALSE);

  /* Ignore empty shortcuts */
  if (G_UNLIKELY (g_utf8_strlen (shortcut, -1) == 0))
    return FALSE;

  gtk_accelerator_parse (shortcut, &accel_key, &accel_mods);
  if (accel_key == 0)
    return FALSE;

  info.in_use = FALSE;
  info.mods = accel_mods;
  info.key = accel_key;
  info.current_path = data->entry->accel_path;
  info.other_path = NULL;
  info.paths_with_overlap_allowed = build_paths_with_overlap_allowed_set (data->editor, data->section_index);

  gtk_accel_map_foreach_unfiltered (&info, xfce_shortcuts_editor_shortcut_check);

  g_clear_pointer (&info.paths_with_overlap_allowed, g_hash_table_destroy);

  if (info.in_use)
    {
      command = g_strrstr (info.other_path, "/");
      command = command == NULL ? info.other_path : command + 1; /* skip leading slash */

      message = g_strdup_printf (_("This keyboard shortcut is currently used by: '%s'"),
                                 command);
      xfce_dialog_show_warning (GTK_WINDOW (gtk_widget_get_toplevel (GTK_WIDGET (editor))), message,
                                _("Keyboard shortcut already in use"));
      g_free (message);
      g_free (info.other_path);

      return FALSE;
    }
  else if (!gtk_accel_map_change_entry (data->entry->accel_path, accel_key, accel_mods, TRUE))
    return FALSE;

  return TRUE;
}



static void
xfce_shortcuts_editor_shortcut_clicked (GtkWidget *widget,
                                        ShortcutEditClickedData *data)
{
  GtkWidget *dialog;
  gint response;
  const gchar *shortcut;
  GdkModifierType accel_mods;
  guint accel_key;
  gchar *label;

  dialog = xfce_shortcut_dialog_new ("", data->displayed_label, "");

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



static void
xfce_shortcuts_editor_shortcut_clear_clicked (GtkWidget *widget,
                                              ShortcutOtherClickedData *data)
{
  if (gtk_accel_map_change_entry (data->entry->accel_path, 0, 0, TRUE))
    gtk_button_set_label (GTK_BUTTON (data->shortcut_button), "...");
}



static void
xfce_shortcuts_editor_shortcut_reset_clicked (GtkWidget *widget,
                                              ShortcutOtherClickedData *data)
{
  GdkModifierType accel_mods;
  guint accel_key;
  gchar *label;
  ShortcutInfo info;
  gchar *command, *message;

  gtk_accelerator_parse (data->entry->default_accelerator, &accel_key, &accel_mods);

  /* check if the default accelerator is used by another action */
  info.in_use = FALSE;
  info.mods = accel_mods;
  info.key = accel_key;
  info.current_path = data->entry->accel_path;
  info.other_path = NULL;
  info.paths_with_overlap_allowed = build_paths_with_overlap_allowed_set (data->editor, data->section_index);

  gtk_accel_map_foreach_unfiltered (&info, xfce_shortcuts_editor_shortcut_check);

  g_clear_pointer (&info.paths_with_overlap_allowed, g_hash_table_destroy);

  /* an empty default accelerator is always available */
  if (g_strcmp0 (data->entry->default_accelerator, "") != 0 && info.in_use == TRUE)
    {
      command = g_strrstr (info.other_path, "/");
      command = command == NULL ? info.other_path : command + 1; /* skip leading slash */

      message = g_strdup_printf (_("This keyboard shortcut is currently used by: '%s'"),
                                 command);
      xfce_dialog_show_warning (GTK_WINDOW (gtk_widget_get_toplevel (widget)), message,
                                _("Keyboard shortcut already in use"));
      g_free (message);
    }
  /* if the default accelerator is available change to that */
  else if (gtk_accel_map_change_entry (data->entry->accel_path, accel_key, accel_mods, TRUE))
    {
      if (accel_key > 0)
        label = gtk_accelerator_get_label (accel_key, accel_mods);
      else
        label = g_strdup ("...");
      gtk_button_set_label (GTK_BUTTON (data->shortcut_button), label);

      g_free (label);
    }

  g_free (info.other_path);
}

#define __XFCE_SHORTCUTS_EDITOR_C__
#include "libxfce4kbd-private-visibility.c"
