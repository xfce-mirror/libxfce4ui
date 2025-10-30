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

#include <libxfce4util/libxfce4util.h>

#include "libxfce4ui/libxfce4ui.h"

#include "xfce-die-command-entry.h"
#include "xfce-die-desktop-model.h"
#include "xfce-die-editor.h"

#define MAX_WIDTH 80



/* Property identifiers */
enum
{
  PROP_0,
  PROP_COMPLETE,
  PROP_MODE,
  PROP_NAME,
  PROP_COMMENT,
  PROP_COMMAND,
  PROP_URL,
  PROP_ICON,
  PROP_PATH,
  PROP_SNOTIFY,
  PROP_TERMINAL,
};



static void
xfce_die_editor_finalize (GObject *object);
static void
xfce_die_editor_get_property (GObject *object,
                              guint prop_id,
                              GValue *value,
                              GParamSpec *pspec);
static void
xfce_die_editor_set_property (GObject *object,
                              guint prop_id,
                              const GValue *value,
                              GParamSpec *pspec);
static void
xfce_die_editor_icon_clicked (GtkWidget *button,
                              XfceDieEditor *editor);
static void
xfce_die_editor_path_clicked (GtkWidget *button,
                              XfceDieEditor *editor);
static gboolean
xfce_die_editor_match_selected (GtkEntryCompletion *completion,
                                GtkTreeModel *model,
                                GtkTreeIter *iter,
                                gpointer user_data);
static void
xfce_die_editor_cell_data_func (GtkCellLayout *cell_layout,
                                GtkCellRenderer *renderer,
                                GtkTreeModel *model,
                                GtkTreeIter *iter,
                                gpointer user_data);



struct _XfceDieEditor
{
  GtkGrid __parent__;

  GtkWidget *name_entry;
  GtkWidget *comment_entry;
  GtkWidget *command_entry;
  GtkWidget *url_entry;
  GtkWidget *path_entry;
  GtkWidget *icon_button;
  XfceDieEditorMode mode;
  gchar *name;
  gchar *comment;
  gchar *command;
  gchar *url;
  gchar *icon;
  gchar *path;
  guint snotify : 1;
  guint terminal : 1;
};



G_DEFINE_TYPE (XfceDieEditor, xfce_die_editor, GTK_TYPE_GRID)



static void
xfce_die_editor_class_init (XfceDieEditorClass *klass)
{
  GObjectClass *gobject_class;

  gobject_class = G_OBJECT_CLASS (klass);
  gobject_class->finalize = xfce_die_editor_finalize;
  gobject_class->get_property = xfce_die_editor_get_property;
  gobject_class->set_property = xfce_die_editor_set_property;

  /**
   * XfceDieEditor:complete:
   *
   * %TRUE if the values entered into the #XfceDieEditor are
   * complete for the given mode.
   **/
  g_object_class_install_property (gobject_class,
                                   PROP_COMPLETE,
                                   g_param_spec_boolean ("complete",
                                                         "complete",
                                                         "complete",
                                                         FALSE,
                                                         G_PARAM_READABLE | G_PARAM_STATIC_STRINGS));

  /**
   * XfceDieEditor:mode:
   *
   * The #XfceDieEditorMode for this editor.
   **/
  g_object_class_install_property (gobject_class,
                                   PROP_MODE,
                                   g_param_spec_enum ("mode", "mode", "mode",
                                                      XFCE_TYPE_DIE_EDITOR_MODE,
                                                      XFCE_DIE_EDITOR_MODE_APPLICATION,
                                                      G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS | G_PARAM_CONSTRUCT));

  /**
   * XfceDieEditor:name:
   *
   * The name of the desktop item edited by this editor.
   **/
  g_object_class_install_property (gobject_class,
                                   PROP_NAME,
                                   g_param_spec_string ("name",
                                                        "name",
                                                        "name",
                                                        NULL,
                                                        G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));

  /**
   * XfceDieEditor:comment:
   *
   * The commment of the desktop item edited by this editor.
   **/
  g_object_class_install_property (gobject_class,
                                   PROP_COMMENT,
                                   g_param_spec_string ("comment",
                                                        "comment",
                                                        "comment",
                                                        NULL,
                                                        G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));

  /**
   * XfceDieEditor:command:
   *
   * The command of the desktop item edited by this editor, only
   * valid if the mode is %XFCE_DIE_EDITOR_MODE_APPLICATION.
   **/
  g_object_class_install_property (gobject_class,
                                   PROP_COMMAND,
                                   g_param_spec_string ("command",
                                                        "command",
                                                        "command",
                                                        NULL,
                                                        G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));

  /**
   * XfceDieEditor:url:
   *
   * The URL of the desktop item edited by this editor, only valid
   * if the mode is %XFCE_DIE_EDITOR_MODE_LINK.
   **/
  g_object_class_install_property (gobject_class,
                                   PROP_URL,
                                   g_param_spec_string ("url",
                                                        "url",
                                                        "url",
                                                        NULL,
                                                        G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));

  /**
   * XfceDieEditor:icon:
   *
   * The icon of the desktop item edited by this editor.
   **/
  g_object_class_install_property (gobject_class,
                                   PROP_ICON,
                                   g_param_spec_string ("icon",
                                                        "icon",
                                                        "icon",
                                                        NULL,
                                                        G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));

  /**
   * XfceDieEditor:path:
   *
   * The path of the desktop item edited by this editor.
   **/
  g_object_class_install_property (gobject_class,
                                   PROP_PATH,
                                   g_param_spec_string ("path",
                                                        "path",
                                                        "path",
                                                        NULL,
                                                        G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));

  /**
   * XfceDieEditor:snotify:
   *
   * Whether the desktop item edited by this editor should use startup
   * notification, only valid if mode is %XFCE_DIE_EDITOR_MODE_APPLICATION.
   **/
  g_object_class_install_property (gobject_class,
                                   PROP_SNOTIFY,
                                   g_param_spec_boolean ("snotify",
                                                         "snotify",
                                                         "snotify",
                                                         FALSE,
                                                         G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));

  /**
   * XfceDieEditor:terminal:
   *
   * Whether the desktop item edited by this editor should have its command
   * run in a terminal, only valid if mode is %XFCE_DIE_EDITOR_MODE_APPLICATION.
   **/
  g_object_class_install_property (gobject_class,
                                   PROP_TERMINAL,
                                   g_param_spec_boolean ("terminal",
                                                         "terminal",
                                                         "terminal",
                                                         FALSE,
                                                         G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));
}



static gboolean
xfce_die_true_if_application (GBinding *binding,
                              const GValue *src_value,
                              GValue *dst_value,
                              gpointer user_data)
{
  g_value_set_boolean (dst_value, (g_value_get_enum (src_value) == XFCE_DIE_EDITOR_MODE_APPLICATION));
  return TRUE;
}



static gboolean
xfce_die_true_if_link (GBinding *binding,
                       const GValue *src_value,
                       GValue *dst_value,
                       gpointer user_data)
{
  g_value_set_boolean (dst_value, (g_value_get_enum (src_value) == XFCE_DIE_EDITOR_MODE_LINK));
  return TRUE;
}



static void
xfce_die_editor_init (XfceDieEditor *editor)
{
  GtkWidget *button;
  GtkWidget *entry;
  GtkWidget *label;
  GtkWidget *image;
  GtkWidget *box;
  gint row;

  /* start with sane defaults */
  editor->mode = XFCE_DIE_EDITOR_MODE_LINK;
  editor->command = g_strdup ("");
  editor->comment = g_strdup ("");
  editor->icon = g_strdup ("");
  editor->name = g_strdup ("");
  editor->url = g_strdup ("");
  editor->path = g_strdup ("");

  /* configure the table */
  gtk_grid_set_column_spacing (GTK_GRID (editor), 12);
  gtk_grid_set_row_spacing (GTK_GRID (editor), 6);

  row = 0;

  /* TRANSLATORS: Label in "Create Launcher"/"Create Link" dialog, make sure to avoid mnemonic conflicts */
  label = gtk_label_new_with_mnemonic (_("_Name:"));
  gtk_label_set_use_markup (GTK_LABEL (label), TRUE);
  g_object_set (label, "xalign", 1.0f, "yalign", 0.5f, NULL);
  gtk_grid_attach (GTK_GRID (editor), label, 0, row, 1, 1);
  gtk_widget_show (label);

  editor->name_entry = gtk_entry_new ();
  gtk_entry_set_activates_default (GTK_ENTRY (editor->name_entry), TRUE);
  g_object_bind_property (G_OBJECT (editor), "name",
                          G_OBJECT (editor->name_entry), "text",
                          G_BINDING_BIDIRECTIONAL | G_BINDING_SYNC_CREATE);
  gtk_grid_attach (GTK_GRID (editor), editor->name_entry, 1, row, 1, 1);
  g_object_set (editor->name_entry, "hexpand", TRUE, NULL);
  gtk_label_set_mnemonic_widget (GTK_LABEL (label), editor->name_entry);
  gtk_widget_show (editor->name_entry);

  row += 1;

  /* TRANSLATORS: Label in "Create Launcher"/"Create Link" dialog, make sure to avoid mnemonic conflicts */
  label = gtk_label_new_with_mnemonic (_("C_omment:"));
  gtk_label_set_use_markup (GTK_LABEL (label), TRUE);
  g_object_set (label, "xalign", 1.0f, "yalign", 0.5f, NULL);
  gtk_grid_attach (GTK_GRID (editor), label, 0, row, 1, 1);
  gtk_widget_show (label);

  editor->comment_entry = entry = gtk_entry_new ();
  gtk_entry_set_activates_default (GTK_ENTRY (entry), TRUE);
  g_object_bind_property (G_OBJECT (editor), "comment",
                          G_OBJECT (entry), "text",
                          G_BINDING_BIDIRECTIONAL | G_BINDING_SYNC_CREATE);
  gtk_grid_attach (GTK_GRID (editor), entry, 1, row, 1, 1);
  g_object_set (entry, "hexpand", TRUE, NULL);
  gtk_label_set_mnemonic_widget (GTK_LABEL (label), entry);
  gtk_widget_show (entry);

  row += 1;

  /* TRANSLATORS: Label in "Create Launcher" dialog, make sure to avoid mnemonic conflicts */
  label = gtk_label_new_with_mnemonic (_("Comm_and:"));
  gtk_label_set_use_markup (GTK_LABEL (label), TRUE);
  g_object_set (label, "xalign", 1.0f, "yalign", 0.5f, NULL);
  g_object_bind_property_full (editor, "mode", label, "visible",
                               G_BINDING_SYNC_CREATE,
                               xfce_die_true_if_application, NULL,
                               NULL, NULL);
  gtk_grid_attach (GTK_GRID (editor), label, 0, row, 1, 1);

  editor->command_entry = entry = xfce_die_command_entry_new ();
  g_object_bind_property (G_OBJECT (editor), "command",
                          G_OBJECT (entry), "text",
                          G_BINDING_BIDIRECTIONAL | G_BINDING_SYNC_CREATE);
  g_object_bind_property_full (editor, "mode", entry, "visible",
                               G_BINDING_SYNC_CREATE,
                               xfce_die_true_if_application, NULL,
                               NULL, NULL);
  gtk_grid_attach (GTK_GRID (editor), entry, 1, row, 1, 1);
  g_object_set (entry, "hexpand", TRUE, NULL);
  gtk_label_set_mnemonic_widget (GTK_LABEL (label), entry);

  row += 1;

  /* TRANSLATORS: Label in "Create Link" dialog, make sure to avoid mnemonic conflicts */
  label = gtk_label_new_with_mnemonic (_("_URL:"));
  gtk_label_set_use_markup (GTK_LABEL (label), TRUE);
  g_object_set (label, "xalign", 1.0f, "yalign", 0.5f, NULL);
  g_object_bind_property_full (editor, "mode", label, "visible",
                               G_BINDING_SYNC_CREATE,
                               xfce_die_true_if_link, NULL,
                               NULL, NULL);
  gtk_grid_attach (GTK_GRID (editor), label, 0, row, 1, 1);

  editor->url_entry = entry = gtk_entry_new ();
  gtk_entry_set_activates_default (GTK_ENTRY (entry), TRUE);
  g_object_bind_property (G_OBJECT (editor), "url",
                          G_OBJECT (entry), "text",
                          G_BINDING_BIDIRECTIONAL | G_BINDING_SYNC_CREATE);
  g_object_bind_property_full (editor, "mode", entry, "visible",
                               G_BINDING_SYNC_CREATE,
                               xfce_die_true_if_link, NULL,
                               NULL, NULL);
  gtk_grid_attach (GTK_GRID (editor), entry, 1, row, 1, 1);
  g_object_set (entry, "hexpand", TRUE, NULL);
  gtk_label_set_mnemonic_widget (GTK_LABEL (label), entry);

  row += 1;

  /* TRANSLATORS: Label in "Create Launcher" dialog, make sure to avoid mnemonic conflicts */
  label = gtk_label_new_with_mnemonic (_("Working _Directory:"));
  gtk_label_set_use_markup (GTK_LABEL (label), TRUE);
  g_object_set (label, "xalign", 1.0f, "yalign", 0.5f, NULL);
  g_object_bind_property_full (editor, "mode", label, "visible",
                               G_BINDING_SYNC_CREATE,
                               xfce_die_true_if_application, NULL,
                               NULL, NULL);
  gtk_grid_attach (GTK_GRID (editor), label, 0, row, 1, 1);

  box = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 6);
  gtk_grid_attach (GTK_GRID (editor), box, 1, row, 1, 1);
  g_object_set (box, "hexpand", TRUE, NULL);
  g_object_bind_property_full (editor, "mode", box, "visible",
                               G_BINDING_SYNC_CREATE,
                               xfce_die_true_if_application, NULL,
                               NULL, NULL);

  editor->path_entry = entry = gtk_entry_new ();
  gtk_entry_set_activates_default (GTK_ENTRY (entry), TRUE);
  g_object_bind_property (G_OBJECT (editor), "path",
                          G_OBJECT (entry), "text",
                          G_BINDING_BIDIRECTIONAL | G_BINDING_SYNC_CREATE);
  gtk_box_pack_start (GTK_BOX (box), entry, TRUE, TRUE, 0);
  gtk_label_set_mnemonic_widget (GTK_LABEL (label), entry);
  gtk_widget_show (entry);

  button = gtk_button_new ();
  g_signal_connect (G_OBJECT (button), "clicked", G_CALLBACK (xfce_die_editor_path_clicked), editor);
  gtk_box_pack_start (GTK_BOX (box), button, FALSE, FALSE, 0);
  gtk_widget_show (button);

  image = gtk_image_new_from_icon_name ("folder", GTK_ICON_SIZE_MENU);
  gtk_container_add (GTK_CONTAINER (button), image);
  gtk_widget_show (image);

  row += 1;

  /* TRANSLATORS: Label in "Create Launcher"/"Create Link" dialog, make sure to avoid mnemonic conflicts */
  label = gtk_label_new_with_mnemonic (_("_Icon:"));
  gtk_label_set_use_markup (GTK_LABEL (label), TRUE);
  g_object_set (label, "xalign", 1.0f, "yalign", 0.5f, NULL);
  gtk_grid_attach (GTK_GRID (editor), label, 0, row, 1, 1);
  gtk_widget_show (label);

  editor->icon_button = gtk_button_new ();
  g_signal_connect (G_OBJECT (editor->icon_button), "clicked", G_CALLBACK (xfce_die_editor_icon_clicked), editor);
  gtk_label_set_mnemonic_widget (GTK_LABEL (label), editor->icon_button);
  gtk_widget_show (editor->icon_button);
  gtk_grid_attach (GTK_GRID (editor), editor->icon_button, 1, row, 1, 1);
  g_object_set (editor->icon_button, "expand", FALSE, "halign", GTK_ALIGN_START, "valign", GTK_ALIGN_CENTER, NULL);

  /* TRANSLATORS: Label for the icon button in "Create Launcher"/"Create Link" dialog if no icon selected */
  label = gtk_label_new (_("No icon"));
  gtk_container_add (GTK_CONTAINER (editor->icon_button), label);
  gtk_widget_show (label);

  row += 1;

  label = gtk_label_new (_("Options:"));
  gtk_label_set_use_markup (GTK_LABEL (label), TRUE);
  g_object_set (label, "xalign", 1.0f, "yalign", 0.5f, NULL);
  g_object_bind_property_full (editor, "mode", label, "visible",
                               G_BINDING_SYNC_CREATE,
                               xfce_die_true_if_application, NULL,
                               NULL, NULL);
  gtk_grid_attach (GTK_GRID (editor), label, 0, row, 1, 1);

  /* TRANSLATORS: Check button label in "Create Launcher" dialog, make sure to avoid mnemonic conflicts
   *              and sync your translations with the translations in Thunar and xfce4-panel.
   */
  button = gtk_check_button_new_with_mnemonic (_("Use _startup notification"));
  gtk_widget_set_tooltip_text (button, _("Select this option to enable startup notification when the command "
                                         "is run from the file manager or the menu. Not every application supports "
                                         "startup notification."));
  g_object_bind_property (G_OBJECT (editor), "snotify",
                          G_OBJECT (button), "active",
                          G_BINDING_BIDIRECTIONAL | G_BINDING_SYNC_CREATE);
  g_object_bind_property_full (editor, "mode", button, "visible",
                               G_BINDING_SYNC_CREATE,
                               xfce_die_true_if_application, NULL,
                               NULL, NULL);
  gtk_grid_attach (GTK_GRID (editor), button, 1, row, 1, 1);
  g_object_set (button, "hexpand", TRUE, NULL);

  row += 1;

  /* TRANSLATORS: Check button label in "Create Launcher" dialog, make sure to avoid mnemonic conflicts
   *              and sync your translations with the translations in Thunar and xfce4-panel.
   */
  button = gtk_check_button_new_with_mnemonic (_("Run in _terminal"));
  gtk_widget_set_tooltip_text (button, _("Select this option to run the command in a terminal window."));
  g_object_bind_property (G_OBJECT (editor), "terminal",
                          G_OBJECT (button), "active",
                          G_BINDING_BIDIRECTIONAL | G_BINDING_SYNC_CREATE);
  g_object_bind_property_full (editor, "mode", button, "visible",
                               G_BINDING_SYNC_CREATE,
                               xfce_die_true_if_application, NULL,
                               NULL, NULL);
  gtk_grid_attach (GTK_GRID (editor), button, 1, row, 1, 1);
  g_object_set (button, "hexpand", TRUE, NULL);
}



static void
xfce_die_editor_finalize (GObject *object)
{
  XfceDieEditor *editor = XFCE_DIE_EDITOR (object);

  /* cleanup */
  g_free (editor->command);
  g_free (editor->comment);
  g_free (editor->icon);
  g_free (editor->name);
  g_free (editor->url);
  g_free (editor->path);

  G_OBJECT_CLASS (xfce_die_editor_parent_class)->finalize (object);
}



static void
xfce_die_editor_get_property (GObject *object,
                              guint prop_id,
                              GValue *value,
                              GParamSpec *pspec)
{
  XfceDieEditor *editor = XFCE_DIE_EDITOR (object);

  switch (prop_id)
    {
    case PROP_COMPLETE:
      g_value_set_boolean (value, xfce_die_editor_get_complete (editor));
      break;

    case PROP_MODE:
      g_value_set_enum (value, xfce_die_editor_get_mode (editor));
      break;

    case PROP_NAME:
      g_value_set_string (value, xfce_die_editor_get_name (editor));
      break;

    case PROP_COMMENT:
      g_value_set_string (value, xfce_die_editor_get_comment (editor));
      break;

    case PROP_COMMAND:
      g_value_set_string (value, xfce_die_editor_get_command (editor));
      break;

    case PROP_URL:
      g_value_set_string (value, xfce_die_editor_get_url (editor));
      break;

    case PROP_PATH:
      g_value_set_string (value, xfce_die_editor_get_path (editor));
      break;

    case PROP_ICON:
      g_value_set_string (value, xfce_die_editor_get_icon (editor));
      break;

    case PROP_SNOTIFY:
      g_value_set_boolean (value, xfce_die_editor_get_snotify (editor));
      break;

    case PROP_TERMINAL:
      g_value_set_boolean (value, xfce_die_editor_get_terminal (editor));
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
    }
}



static void
xfce_die_editor_set_property (GObject *object,
                              guint prop_id,
                              const GValue *value,
                              GParamSpec *pspec)
{
  XfceDieEditor *editor = XFCE_DIE_EDITOR (object);

  switch (prop_id)
    {
    case PROP_MODE:
      xfce_die_editor_set_mode (editor, g_value_get_enum (value));
      break;

    case PROP_NAME:
      xfce_die_editor_set_name (editor, g_value_get_string (value));
      break;

    case PROP_COMMENT:
      xfce_die_editor_set_comment (editor, g_value_get_string (value));
      break;

    case PROP_COMMAND:
      xfce_die_editor_set_command (editor, g_value_get_string (value));
      break;

    case PROP_URL:
      xfce_die_editor_set_url (editor, g_value_get_string (value));
      break;

    case PROP_PATH:
      xfce_die_editor_set_path (editor, g_value_get_string (value));
      break;

    case PROP_ICON:
      xfce_die_editor_set_icon (editor, g_value_get_string (value));
      break;

    case PROP_SNOTIFY:
      xfce_die_editor_set_snotify (editor, g_value_get_boolean (value));
      break;

    case PROP_TERMINAL:
      xfce_die_editor_set_terminal (editor, g_value_get_boolean (value));
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
    }
}



static void
xfce_die_editor_icon_clicked (GtkWidget *button,
                              XfceDieEditor *editor)
{
  GtkWidget *toplevel;
  GtkWidget *chooser;
  gchar *icon;

  g_return_if_fail (GTK_IS_BUTTON (button));
  g_return_if_fail (XFCE_IS_DIE_EDITOR (editor));

  /* determine the toplevel widget */
  toplevel = gtk_widget_get_toplevel (button);
  if (toplevel == NULL || !gtk_widget_is_toplevel (toplevel))
    return;

  /* allocate the icon chooser dialog */
  chooser = xfce_icon_chooser_dialog_new (_("Select an icon"),
                                          GTK_WINDOW (toplevel), _("_Cancel"),
                                          GTK_RESPONSE_CANCEL, _("_OK"),
                                          GTK_RESPONSE_ACCEPT,
                                          NULL);
  gtk_dialog_set_default_response (GTK_DIALOG (chooser), GTK_RESPONSE_ACCEPT);

  /* check if we have an icon to set for the chooser */
  if (G_LIKELY (!xfce_str_is_empty (editor->icon)))
    xfce_icon_chooser_dialog_set_icon (XFCE_ICON_CHOOSER_DIALOG (chooser), editor->icon);

  /* run the chooser dialog */
  if (gtk_dialog_run (GTK_DIALOG (chooser)) == GTK_RESPONSE_ACCEPT)
    {
      /* remember the selected icon from the chooser */
      icon = xfce_icon_chooser_dialog_get_icon (XFCE_ICON_CHOOSER_DIALOG (chooser));
      xfce_die_editor_set_icon (editor, icon);
      g_free (icon);
    }

  /* destroy the chooser */
  gtk_widget_destroy (chooser);
}



static void
xfce_die_editor_path_clicked (GtkWidget *button,
                              XfceDieEditor *editor)
{
  GtkWidget *toplevel;
  GtkWidget *chooser;
  gchar *path;

  g_return_if_fail (GTK_IS_BUTTON (button));
  g_return_if_fail (XFCE_IS_DIE_EDITOR (editor));

  /* determine the toplevel widget */
  toplevel = gtk_widget_get_toplevel (button);
  if (toplevel == NULL || !gtk_widget_is_toplevel (toplevel))
    return;

  /* allocate the file chooser dialog */
  chooser = gtk_file_chooser_dialog_new (_("Select a working directory"),
                                         GTK_WINDOW (toplevel),
                                         GTK_FILE_CHOOSER_ACTION_SELECT_FOLDER, _("_Cancel"),
                                         GTK_RESPONSE_CANCEL, _("_OK"),
                                         GTK_RESPONSE_ACCEPT,
                                         NULL);
  gtk_dialog_set_default_response (GTK_DIALOG (chooser), GTK_RESPONSE_ACCEPT);

  /* check if we have a path to set for the chooser */
  if (G_LIKELY (!xfce_str_is_empty (editor->path)))
    gtk_file_chooser_set_current_folder (GTK_FILE_CHOOSER (chooser), editor->path);

  /* run the chooser dialog */
  if (gtk_dialog_run (GTK_DIALOG (chooser)) == GTK_RESPONSE_ACCEPT)
    {
      /* remember the selected path from the chooser */
      path = gtk_file_chooser_get_filename (GTK_FILE_CHOOSER (chooser));
      xfce_die_editor_set_path (editor, path);
      g_free (path);
    }

  /* destroy the chooser */
  gtk_widget_destroy (chooser);
}



static gboolean
xfce_die_editor_match_selected (GtkEntryCompletion *completion,
                                GtkTreeModel *model,
                                GtkTreeIter *iter,
                                gpointer user_data)
{
  XfceDieEditor *editor = XFCE_DIE_EDITOR (user_data);
  gboolean terminal;
  gboolean snotify;
  gchar *comment;
  gchar *command;
  gchar *icon;
  gchar *name;

  g_return_val_if_fail (GTK_IS_ENTRY_COMPLETION (completion), FALSE);
  g_return_val_if_fail (XFCE_IS_DIE_EDITOR (editor), FALSE);
  g_return_val_if_fail (GTK_IS_TREE_MODEL (model), FALSE);

  /* determine the attributes for the selected row */
  gtk_tree_model_get (model, iter,
                      XFCE_DIE_DESKTOP_MODEL_COLUMN_COMMENT, &comment,
                      XFCE_DIE_DESKTOP_MODEL_COLUMN_COMMAND, &command,
                      XFCE_DIE_DESKTOP_MODEL_COLUMN_ICON, &icon,
                      XFCE_DIE_DESKTOP_MODEL_COLUMN_NAME, &name,
                      XFCE_DIE_DESKTOP_MODEL_COLUMN_SNOTIFY, &snotify,
                      XFCE_DIE_DESKTOP_MODEL_COLUMN_TERMINAL, &terminal,
                      -1);

  /* apply the settings to the editor */
  xfce_die_editor_set_name (editor, name);
  xfce_die_editor_set_comment (editor, (comment != NULL) ? comment : "");
  xfce_die_editor_set_command (editor, command);
  xfce_die_editor_set_icon (editor, (icon != NULL) ? icon : "");
  xfce_die_editor_set_snotify (editor, snotify);
  xfce_die_editor_set_terminal (editor, terminal);
  xfce_die_editor_set_path (editor, "");

  /* cleanup */
  g_free (comment);
  g_free (command);
  g_free (icon);
  g_free (name);

  return TRUE;
}



static void
xfce_die_editor_cell_data_func (GtkCellLayout *cell_layout,
                                GtkCellRenderer *renderer,
                                GtkTreeModel *model,
                                GtkTreeIter *iter,
                                gpointer user_data)
{
  XfceDieEditor *editor = XFCE_DIE_EDITOR (user_data);
  GtkIconTheme *icon_theme;
  GdkPixbuf *pixbuf_scaled;
  GdkPixbuf *pixbuf = NULL;
  cairo_surface_t *surface = NULL;
  gchar *icon;
  gint pixbuf_width;
  gint pixbuf_height;
  gint scale_factor = gtk_widget_get_scale_factor (GTK_WIDGET (editor));

  /* determine the icon for the row */
  gtk_tree_model_get (model, iter, XFCE_DIE_DESKTOP_MODEL_COLUMN_ICON, &icon, -1);

  /* check the icon depending on the type */
  if (icon != NULL && g_path_is_absolute (icon))
    {
      /* try to load the icon from the file */
      pixbuf = gdk_pixbuf_new_from_file (icon, NULL);
    }
  else if (!xfce_str_is_empty (icon))
    {
      /* determine the appropriate icon theme */
      icon_theme = gtk_icon_theme_get_for_screen (gtk_widget_get_screen (GTK_WIDGET (editor)));

      /* try to load the named icon */
      pixbuf = gtk_icon_theme_load_icon_for_scale (icon_theme, icon, 16, scale_factor,
                                                   GTK_ICON_LOOKUP_FORCE_SIZE, NULL);
    }

  /* setup the icon button */
  if (G_LIKELY (pixbuf != NULL))
    {
      /* scale down the icon if required */
      pixbuf_width = gdk_pixbuf_get_width (pixbuf);
      pixbuf_height = gdk_pixbuf_get_height (pixbuf);
      if (G_UNLIKELY (pixbuf_width > 16 * scale_factor || pixbuf_height > 16 * scale_factor))
        {
          pixbuf_scaled = xfce_gdk_pixbuf_scale_ratio (pixbuf, 16 * scale_factor);
          g_object_unref (G_OBJECT (pixbuf));
          pixbuf = pixbuf_scaled;
        }

      surface = gdk_cairo_surface_create_from_pixbuf (
        pixbuf, scale_factor, gtk_widget_get_window (GTK_WIDGET (editor)));
      g_object_unref (G_OBJECT (pixbuf));
    }

  /* setup the pixbuf for the renderer */
  g_object_set (G_OBJECT (renderer), "surface", surface, NULL);

  /* cleanup */
  if (G_LIKELY (surface != NULL))
    cairo_surface_destroy (surface);
  g_free (icon);
}



/**
 * xfce_die_editor_new:
 *
 * Allocates a new #XfceDieEditor instance.
 *
 * Return value: the newly allocated #XfceDieEditor.
 **/
GtkWidget *
xfce_die_editor_new (void)
{
  return g_object_new (XFCE_TYPE_DIE_EDITOR, NULL);
}



/**
 * xfce_die_dialog_get_complete:
 * @editor : an #XfceDieEditor.
 *
 * Returns %TRUE if the values entered into the
 * @editor are complete.
 *
 * Return value: if @editor<!---->s values are
 *               complete.
 **/
gboolean
xfce_die_editor_get_complete (XfceDieEditor *editor)
{
  g_return_val_if_fail (XFCE_IS_DIE_EDITOR (editor), FALSE);

  /* the exact meaning of complete depends on the mode */
  switch (editor->mode)
    {
    case XFCE_DIE_EDITOR_MODE_APPLICATION:
      return (!xfce_str_is_empty (editor->name)
              && !xfce_str_is_empty (editor->command));

    case XFCE_DIE_EDITOR_MODE_LINK:
      return (!xfce_str_is_empty (editor->name)
              && !xfce_str_is_empty (editor->url));

    case XFCE_DIE_EDITOR_MODE_DIRECTORY:
      return !xfce_str_is_empty (editor->name);

    default:
      g_assert_not_reached ();
      return FALSE;
    }
}



/**
 * xfce_die_editor_get_mode:
 * @editor : an #XfceDieEditor.
 *
 * Returns the #XfceDieEditorMode for @editor.
 *
 * Return value: the #XfceDieEditorMode for @editor.
 **/
XfceDieEditorMode
xfce_die_editor_get_mode (XfceDieEditor *editor)
{
  g_return_val_if_fail (XFCE_IS_DIE_EDITOR (editor), XFCE_DIE_EDITOR_MODE_APPLICATION);
  return editor->mode;
}



/**
 * xfce_die_editor_set_mode:
 * @editor : an #XfceDieEditor.
 * @mode   : an #XfceDieEditorMode.
 *
 * Sets the mode for @editor to @mode.
 **/
void
xfce_die_editor_set_mode (XfceDieEditor *editor,
                          XfceDieEditorMode mode)
{
  XfceDieDesktopModel *desktop_model;
  GtkEntryCompletion *completion;
  GtkCellRenderer *renderer;

  g_return_if_fail (XFCE_IS_DIE_EDITOR (editor));

  /* check if we have a new mode here */
  if (G_LIKELY (editor->mode != mode))
    {
      /* apply the new mode */
      editor->mode = mode;

      /* enable name completion based on the mode */
      if (mode == XFCE_DIE_EDITOR_MODE_APPLICATION)
        {
          /* allocate a new completion for the name entry */
          completion = gtk_entry_completion_new ();
          gtk_entry_completion_set_inline_completion (completion, TRUE);
          gtk_entry_completion_set_minimum_key_length (completion, 3);
          gtk_entry_completion_set_popup_completion (completion, TRUE);
          g_signal_connect (G_OBJECT (completion), "match-selected",
                            G_CALLBACK (xfce_die_editor_match_selected), editor);
          gtk_entry_set_completion (GTK_ENTRY (editor->name_entry), completion);
          g_object_unref (G_OBJECT (completion));

          /* allocate the desktop application model */
          desktop_model = xfce_die_desktop_model_new ();
          gtk_entry_completion_set_match_func (completion, xfce_die_desktop_model_match_func, desktop_model, NULL);
          gtk_entry_completion_set_model (completion, GTK_TREE_MODEL (desktop_model));
          g_object_unref (G_OBJECT (desktop_model));

          /* add the icon renderer */
          renderer = gtk_cell_renderer_pixbuf_new ();
          gtk_cell_layout_pack_start (GTK_CELL_LAYOUT (completion), renderer, FALSE);
          gtk_cell_layout_set_cell_data_func (GTK_CELL_LAYOUT (completion), renderer,
                                              xfce_die_editor_cell_data_func, editor, NULL);

          /* add the text renderer */
          renderer = g_object_new (GTK_TYPE_CELL_RENDERER_TEXT, "ellipsize", PANGO_ELLIPSIZE_END, NULL);
          gtk_cell_layout_pack_start (GTK_CELL_LAYOUT (completion), renderer, TRUE);
          gtk_cell_layout_set_attributes (GTK_CELL_LAYOUT (completion), renderer,
                                          "markup", XFCE_DIE_DESKTOP_MODEL_COLUMN_ABSTRACT,
                                          NULL);
        }
      else
        {
          /* completion is disabled for links */
          gtk_entry_set_completion (GTK_ENTRY (editor->name_entry), NULL);
        }

      /* notify listeners */
      g_object_notify (G_OBJECT (editor), "complete");
      g_object_notify (G_OBJECT (editor), "mode");
    }
}



/**
 * xfce_die_editor_get_name:
 * @editor : an #XfceDieEditor.
 *
 * Returns the name for the @editor.
 *
 * Return value: the name for the @editor.
 **/
const gchar *
xfce_die_editor_get_name (XfceDieEditor *editor)
{
  g_return_val_if_fail (XFCE_IS_DIE_EDITOR (editor), NULL);
  return editor->name;
}



/**
 * xfce_die_editor_set_name:
 * @editor : an #XfceDieEditor.
 * @name   : the new name for @editor.
 *
 * Sets the name for @editor to @name.
 **/
void
xfce_die_editor_set_name (XfceDieEditor *editor,
                          const gchar *name)
{
  g_return_if_fail (XFCE_IS_DIE_EDITOR (editor));
  g_return_if_fail (g_utf8_validate (name, -1, NULL));

  /* check if we have a new name */
  if (g_strcmp0 (editor->name, name) != 0)
    {
      /* apply the new name */
      g_free (editor->name);
      editor->name = g_strdup (name);
      gtk_entry_set_width_chars (GTK_ENTRY (editor->name_entry),
                                 MIN (g_utf8_strlen (editor->name, -1), MAX_WIDTH));

      /* notify listeners */
      g_object_notify (G_OBJECT (editor), "complete");
      g_object_notify (G_OBJECT (editor), "name");
    }
}



/**
 * xfce_die_editor_get_comment:
 * @editor : an #XfceDieEditor.
 *
 * Returns the comment for @editor.
 *
 * Return value: the comment for @editor.
 **/
const gchar *
xfce_die_editor_get_comment (XfceDieEditor *editor)
{
  g_return_val_if_fail (XFCE_IS_DIE_EDITOR (editor), NULL);
  return editor->comment;
}



/**
 * xfce_die_editor_set_comment:
 * @editor  : an #XfceDieEditor.
 * @comment : the new comment for @editor.
 *
 * Sets the comment for @editor to @comment.
 **/
void
xfce_die_editor_set_comment (XfceDieEditor *editor,
                             const gchar *comment)
{
  g_return_if_fail (XFCE_IS_DIE_EDITOR (editor));
  g_return_if_fail (g_utf8_validate (comment, -1, NULL));

  /* check if we have a new comment here */
  if (g_strcmp0 (editor->comment, comment) != 0)
    {
      /* apply the new comment */
      g_free (editor->comment);
      editor->comment = g_strdup (comment);
      gtk_entry_set_width_chars (GTK_ENTRY (editor->comment_entry),
                                 MIN (g_utf8_strlen (editor->comment, -1), MAX_WIDTH));

      /* notify listeners */
      g_object_notify (G_OBJECT (editor), "comment");
    }
}



/**
 * xfce_die_editor_get_command:
 * @editor : an #XfceDieEditor.
 *
 * Returns the command for the @editor, which is only valid
 * if the mode is %XFCE_DIE_EDITOR_MODE_APPLICATION.
 *
 * Return value: the command for the @editor.
 **/
const gchar *
xfce_die_editor_get_command (XfceDieEditor *editor)
{
  g_return_val_if_fail (XFCE_IS_DIE_EDITOR (editor), NULL);
  return editor->command;
}



/**
 * xfce_die_editor_set_command:
 * @editor  : an #XfceDieEditor.
 * @command : the new command for @editor.
 *
 * Sets the command for @editor to @command.
 **/
void
xfce_die_editor_set_command (XfceDieEditor *editor,
                             const gchar *command)
{
  g_return_if_fail (XFCE_IS_DIE_EDITOR (editor));
  g_return_if_fail (g_utf8_validate (command, -1, NULL));

  /* check if we have a new command here */
  if (g_strcmp0 (editor->command, command) != 0)
    {
      /* apply the new command */
      g_free (editor->command);
      editor->command = g_strdup (command);
      gtk_entry_set_width_chars (GTK_ENTRY (xfce_die_command_entry_get_text_entry (XFCE_DIE_COMMAND_ENTRY (editor->command_entry))),
                                 MIN (g_utf8_strlen (editor->command, -1), MAX_WIDTH));

      /* notify listeners */
      g_object_notify (G_OBJECT (editor), "complete");
      g_object_notify (G_OBJECT (editor), "command");
    }
}



/**
 * xfce_die_editor_get_url:
 * @editor : an #XfceDieEditor.
 *
 * Returns the URL for @editor, which is only valid
 * if the mode is %XFCE_DIE_EDITOR_MODE_LINK.
 *
 * Return value: the URL for @editor.
 **/
const gchar *
xfce_die_editor_get_url (XfceDieEditor *editor)
{
  g_return_val_if_fail (XFCE_IS_DIE_EDITOR (editor), NULL);
  return editor->url;
}



/**
 * xfce_die_editor_set_url:
 * @editor : an #XfceDieEditor.
 * @url    : the new URL for @editor.
 *
 * Sets the URL for @editor to @url.
 **/
void
xfce_die_editor_set_url (XfceDieEditor *editor,
                         const gchar *url)
{
  g_return_if_fail (XFCE_IS_DIE_EDITOR (editor));
  g_return_if_fail (g_utf8_validate (url, -1, NULL));

  /* check if we have a new URL here */
  if (g_strcmp0 (editor->url, url) != 0)
    {
      /* apply the new URL */
      g_free (editor->url);
      editor->url = g_strdup (url);
      gtk_entry_set_width_chars (GTK_ENTRY (editor->url_entry),
                                 MIN (g_utf8_strlen (editor->url, -1), MAX_WIDTH));

      /* notify listeners */
      g_object_notify (G_OBJECT (editor), "complete");
      g_object_notify (G_OBJECT (editor), "url");
    }
}



/**
 * xfce_die_editor_get_path:
 * @editor : an #XfceDieEditor.
 *
 * Returns the path for @editor, which is only valid
 * if the mode is %XFCE_DIE_EDITOR_MODE_APPLICATION.
 *
 * Return value: the working directory for @editor.
 **/
const gchar *
xfce_die_editor_get_path (XfceDieEditor *editor)
{
  g_return_val_if_fail (XFCE_IS_DIE_EDITOR (editor), NULL);
  return editor->path;
}



/**
 * xfce_die_editor_set_path:
 * @editor : an #XfceDieEditor.
 * @path   : the new working directory for @editor.
 *
 * Sets the working directory for @editor to @url.
 **/
void
xfce_die_editor_set_path (XfceDieEditor *editor,
                          const gchar *path)
{
  g_return_if_fail (XFCE_IS_DIE_EDITOR (editor));
  g_return_if_fail (g_utf8_validate (path, -1, NULL));

  /* check if we have a new URL here */
  if (g_strcmp0 (editor->path, path) != 0)
    {
      /* apply the new URL */
      g_free (editor->path);
      editor->path = g_strdup (path);
      gtk_entry_set_width_chars (GTK_ENTRY (editor->path_entry),
                                 MIN (g_utf8_strlen (editor->path, -1), MAX_WIDTH));

      /* notify listeners */
      g_object_notify (G_OBJECT (editor), "path");
    }
}



/**
 * xfce_die_editor_get_icon:
 * @editor : an #XfceDieEditor.
 *
 * Returns the icon for the @editor.
 *
 * Return value: the icon for the @editor.
 **/
const gchar *
xfce_die_editor_get_icon (XfceDieEditor *editor)
{
  g_return_val_if_fail (XFCE_IS_DIE_EDITOR (editor), NULL);
  return editor->icon;
}



/**
 * xfce_die_editor_set_icon:
 * @editor : an #XfceDieEditor.
 * @icon   : the new icon for the @editor.
 *
 * Sets the icon for the @editor to @icon.
 **/
void
xfce_die_editor_set_icon (XfceDieEditor *editor,
                          const gchar *icon)
{
  GtkIconTheme *icon_theme;
  GdkPixbuf *pixbuf_scaled;
  GdkPixbuf *pixbuf = NULL;
  cairo_surface_t *surface;
  GtkWidget *image;
  GtkWidget *label;
  gint scale_factor;
  gint icon_size;
  gint pixbuf_width;
  gint pixbuf_height;

  g_return_if_fail (XFCE_IS_DIE_EDITOR (editor));
  g_return_if_fail (g_utf8_validate (icon, -1, NULL));

  /* check if we have a new icon here */
  if (g_strcmp0 (editor->icon, icon) != 0)
    {
      /* apply the new icon */
      g_free (editor->icon);
      editor->icon = g_strdup (icon);

      /* notify listeners */
      g_object_notify (G_OBJECT (editor), "icon");

      /* drop the previous icon button child */
      if (gtk_bin_get_child (GTK_BIN (editor->icon_button)) != NULL)
        gtk_widget_destroy (gtk_bin_get_child (GTK_BIN (editor->icon_button)));

      scale_factor = gtk_widget_get_scale_factor (GTK_WIDGET (editor));
      icon_size = 48;

      /* check the icon depending on the type */
      if (icon != NULL && g_path_is_absolute (icon))
        {
          /* try to load the icon from the file */
          pixbuf = gdk_pixbuf_new_from_file (icon, NULL);
        }
      else if (!xfce_str_is_empty (icon))
        {
          /* determine the appropriate icon theme */
          icon_theme = gtk_icon_theme_get_for_screen (gtk_widget_get_screen (GTK_WIDGET (editor)));

          /* try to load the named icon */
          pixbuf = gtk_icon_theme_load_icon_for_scale (icon_theme, icon, icon_size, scale_factor,
                                                       GTK_ICON_LOOKUP_FORCE_SIZE, NULL);
        }

      /* setup the icon button */
      if (G_LIKELY (pixbuf != NULL))
        {
          /* scale down the icon if required */
          pixbuf_width = gdk_pixbuf_get_width (pixbuf);
          pixbuf_height = gdk_pixbuf_get_height (pixbuf);
          if (G_UNLIKELY (pixbuf_width > icon_size * scale_factor
                          || pixbuf_height > icon_size * scale_factor))
            {
              pixbuf_scaled = xfce_gdk_pixbuf_scale_ratio (pixbuf, icon_size * scale_factor);
              g_object_unref (G_OBJECT (pixbuf));
              pixbuf = pixbuf_scaled;
            }
          surface = gdk_cairo_surface_create_from_pixbuf (
            pixbuf, scale_factor, gtk_widget_get_window (GTK_WIDGET (editor)));

          /* setup an image for the icon */
          image = gtk_image_new_from_surface (surface);
          gtk_container_add (GTK_CONTAINER (editor->icon_button), image);
          gtk_widget_show (image);

          /* release the pixbuf */
          g_object_unref (G_OBJECT (pixbuf));
          cairo_surface_destroy (surface);
        }
      else
        {
          /* setup a label to tell that no icon was selected */
          label = gtk_label_new (_("No icon"));
          gtk_container_add (GTK_CONTAINER (editor->icon_button), label);
          gtk_widget_show (label);
        }
    }
}



/**
 * xfce_die_editor_get_snotify:
 * @editor : an #XfceDieEditor.
 *
 * Returns %TRUE if @editor has enabled startup notification, which
 * is only valid if mode is %XFCE_DIE_EDITOR_MODE_APPLICATION.
 *
 * Return value: %TRUE if startup notification is enabled.
 **/
gboolean
xfce_die_editor_get_snotify (XfceDieEditor *editor)
{
  g_return_val_if_fail (XFCE_IS_DIE_EDITOR (editor), FALSE);
  return editor->snotify;
}



/**
 * xfce_die_editor_set_snotify:
 * @editor  : an #XfceDieEditor.
 * @snotify : %TRUE to enable startup notification.
 *
 * Set startup notification state for @editor to @snotify.
 **/
void
xfce_die_editor_set_snotify (XfceDieEditor *editor,
                             gboolean snotify)
{
  g_return_if_fail (XFCE_IS_DIE_EDITOR (editor));

  /* normalize the value */
  snotify = !!snotify;

  /* check if we have a new value */
  if (editor->snotify != snotify)
    {
      /* apply the new value */
      editor->snotify = snotify;

      /* notify listeners */
      g_object_notify (G_OBJECT (editor), "snotify");
    }
}



/**
 * xfce_die_editor_get_terminal:
 * @editor : an #XfceDieEditor.
 *
 * Returns %TRUE if the command should be run in a terminal, only valid
 * if mode for @editor is %XFCE_DIE_EDITOR_MODE_APPLICATION.
 *
 * Return value: %TRUE if command should be run in terminal.
 **/
gboolean
xfce_die_editor_get_terminal (XfceDieEditor *editor)
{
  g_return_val_if_fail (XFCE_IS_DIE_EDITOR (editor), FALSE);
  return editor->terminal;
}



/**
 * xfce_die_editor_set_terminal:
 * @editor   : an #XfceDieEditor.
 * @terminal : %TRUE to run command in terminal.
 *
 * Sets the run in terminal setting of @editor to @terminal.
 **/
void
xfce_die_editor_set_terminal (XfceDieEditor *editor,
                              gboolean terminal)
{
  g_return_if_fail (XFCE_IS_DIE_EDITOR (editor));

  /* normalize the value */
  terminal = !!terminal;

  /* check if we have a new value */
  if (editor->terminal != terminal)
    {
      /* apply the new value */
      editor->terminal = terminal;

      /* notify listeners */
      g_object_notify (G_OBJECT (editor), "terminal");
    }
}
