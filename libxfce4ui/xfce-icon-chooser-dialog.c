/*-
 * Copyright (c) 2005-2006 Benedikt Meurer <benny@xfce.org>
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

#ifdef HAVE_MEMORY_H
#include <memory.h>
#endif
#ifdef HAVE_STDARG_H
#include <stdarg.h>
#endif
#ifdef HAVE_STRING_H
#include <string.h>
#endif

#include <gdk/gdkkeysyms.h>
#include <libxfce4util/libxfce4util.h>

#include "xfce-cell-renderer-icon.h"
#include "xfce-gtk-extensions.h"
#include "xfce-icon-chooser-dialog.h"
#include "xfce-icon-chooser-model.h"
#include "xfce-icon-view.h"
#include "libxfce4ui-visibility.h"

#define FILTER_ENTRY_DELAY 300

/**
 * SECTION: xfce-icon-chooser-dialog
 * @title: XfceIconChooserDialog
 * @short_description: Dialog to select icons
 * @include: libxfce4ui/libxfce4ui.h
 * @see_also: <ulink type="http" href="http://library.gnome.org/devel/gtk/stable/GtkIconTheme.html">
 *            GtkIconTheme</ulink>
 *
 * The #XfceIconChooserDialog class provides an easy to use dialog to ask
 * the user to select either a named icon from the selected icon theme,
 * or an image file from the local file system.
 **/



static void
xfce_icon_chooser_dialog_finalize (GObject *object);
static void
xfce_icon_chooser_dialog_style_set (GtkWidget *widget,
                                    GtkStyle *previous_style);
static void
xfce_icon_chooser_dialog_screen_changed (GtkWidget *widget,
                                         GdkScreen *previous_screen);
static void
xfce_icon_chooser_dialog_close (GtkDialog *dialog);
static void
xfce_icon_chooser_dialog_set_model (XfceIconChooserDialog *dialog);
static gboolean
xfce_icon_chooser_dialog_separator_func (GtkTreeModel *model,
                                         GtkTreeIter *iter,
                                         gpointer user_data);
static gboolean
xfce_icon_chooser_dialog_visible_func (GtkTreeModel *model,
                                       GtkTreeIter *iter,
                                       gpointer user_data);
static gboolean
xfce_icon_chooser_dialog_start_interactive_search (XfceIconChooserDialog *dialog);
static void
xfce_icon_chooser_dialog_combo_changed (GtkWidget *combo,
                                        XfceIconChooserDialog *dialog);
static void
xfce_icon_chooser_dialog_entry_changed_delay (GtkEntry *entry,
                                              XfceIconChooserDialog *dialog);
static gboolean
xfce_icon_chooser_dialog_entry_changed (gpointer user_data);
static void
xfce_icon_chooser_dialog_entry_clear (GtkEntry *entry,
                                      GtkEntryIconPosition icon_pos,
                                      GdkEvent *event);
static void
xfce_icon_chooser_dialog_selection_changed (XfceIconChooserDialog *dialog);



/**
 * XfceIconChooserDialog:
 *
 * The #XfceIconChooserDialog class provides an easy to use dialog to ask
 * the user to select either a named icon from the selected icon theme,
 * or an image file from the local file system.
 **/
struct _XfceIconChooserDialog
{
  GtkDialog __parent__;

  GtkWidget *combo;
  GtkWidget *filter_entry;
  guint filter_entry_timeout_source_id;
  GtkWidget *icon_chooser;
  GtkWidget *file_chooser;
  gchar *casefolded_text;
};



static const gchar CONTEXT_TITLES[][80] = {
  /* XFCE_ICON_CHOOSER_CONTEXT_ACTIONS */
  N_ ("Action Icons"),
  /* XFCE_ICON_CHOOSER_CONTEXT_ANIMATIONS */
  N_ ("Animations"),
  /* XFCE_ICON_CHOOSER_CONTEXT_APPLICATIONS */
  N_ ("Application Icons"),
  /* XFCE_ICON_CHOOSER_CONTEXT_CATEGORIES */
  N_ ("Menu Icons"),
  /* XFCE_ICON_CHOOSER_CONTEXT_DEVICES */
  N_ ("Device Icons"),
  /* XFCE_ICON_CHOOSER_CONTEXT_EMBLEMS */
  N_ ("Emblems"),
  /* XFCE_ICON_CHOOSER_CONTEXT_EMOTES */
  N_ ("Emoticons"),
  /* XFCE_ICON_CHOOSER_CONTEXT_MIME_TYPES */
  N_ ("File Type Icons"),
  /* XFCE_ICON_CHOOSER_CONTEXT_PLACES */
  N_ ("Location Icons"),
  /* XFCE_ICON_CHOOSER_CONTEXT_STATUS */
  N_ ("Status Icons"),
  /* XFCE_ICON_CHOOSER_CONTEXT_STOCK */
  N_ ("Stock Icons"),
  /* XFCE_ICON_CHOOSER_CONTEXT_OTHER */
  N_ ("Uncategorized Icons"),
  /* separator */
  "#",
  /* XFCE_ICON_CHOOSER_CONTEXT_ALL */
  N_ ("All Icons"),
  /* separator */
  "#",
  /* XFCE_ICON_CHOOSER_CONTEXT_FILE */
  N_ ("Image Files"),
  /* separator */
  "#",
  /* XFCE_ICON_CHOOSER_CONTEXT_NO_ICON */
  N_ ("No Icon"),
};



G_DEFINE_TYPE (XfceIconChooserDialog, xfce_icon_chooser_dialog, GTK_TYPE_DIALOG)



static void
xfce_icon_chooser_dialog_class_init (XfceIconChooserDialogClass *klass)
{
  GObjectClass *gobject_class;
  GtkDialogClass *gtkdialog_class;
  GtkWidgetClass *gtkwidget_class;
  GtkBindingSet *binding_set;

  gobject_class = G_OBJECT_CLASS (klass);
  gobject_class->finalize = xfce_icon_chooser_dialog_finalize;

  gtkwidget_class = GTK_WIDGET_CLASS (klass);
  gtkwidget_class->style_set = xfce_icon_chooser_dialog_style_set;
  gtkwidget_class->screen_changed = xfce_icon_chooser_dialog_screen_changed;

  gtkdialog_class = GTK_DIALOG_CLASS (klass);
  gtkdialog_class->close = xfce_icon_chooser_dialog_close;

  /* make sure to use the translations from libxfce4ui */
  bindtextdomain (GETTEXT_PACKAGE, PACKAGE_LOCALE_DIR);
#ifdef HAVE_BIND_TEXTDOMAIN_CODESET
  bind_textdomain_codeset (GETTEXT_PACKAGE, "UTF-8");
#endif

  /* connect additional key bindings to the GtkDialog::close action signal */
  binding_set = gtk_binding_set_by_class (klass);
  gtk_binding_entry_add_signal (binding_set, GDK_KEY_w, GDK_CONTROL_MASK, "close", 0);
  gtk_binding_entry_add_signal (binding_set, GDK_KEY_W, GDK_CONTROL_MASK, "close", 0);
}



static void
xfce_icon_chooser_dialog_init (XfceIconChooserDialog *dialog)
{
  XfceIconChooserContext context;
  GtkCellRenderer *renderer;
  GtkFileFilter *filter;
  GtkWidget *scrolled_window;
  GtkWidget *label;
  GtkWidget *table;
  GtkWidget *vbox;

  gtk_window_set_default_size (GTK_WINDOW (dialog), 780, 560);

  /* add the main box */
  vbox = gtk_box_new (GTK_ORIENTATION_VERTICAL, 6);
  gtk_container_set_border_width (GTK_CONTAINER (vbox), 6);
  gtk_box_pack_start (GTK_BOX (gtk_dialog_get_content_area (GTK_DIALOG (dialog))), vbox, TRUE, TRUE, 0);
  gtk_widget_show (vbox);

  /* add the header table */
  table = gtk_grid_new ();
  gtk_grid_set_row_spacing (GTK_GRID (table), 6);
  gtk_grid_set_column_spacing (GTK_GRID (table), 12);
  gtk_box_pack_start (GTK_BOX (vbox), table, FALSE, FALSE, 0);
  gtk_widget_show (table);

  /* setup the context combo box */
  label = gtk_label_new_with_mnemonic (_("Select _icon from:"));
  g_object_set (label, "xalign", 0.0f, "yalign", 0.5f, NULL);
  gtk_grid_attach (GTK_GRID (table), label, 0, 0, 1, 1);
  gtk_widget_show (label);

  dialog->combo = gtk_combo_box_text_new ();
  for (context = 0; context < G_N_ELEMENTS (CONTEXT_TITLES); ++context)
    gtk_combo_box_text_append_text (GTK_COMBO_BOX_TEXT (dialog->combo), _(CONTEXT_TITLES[context]));
  gtk_combo_box_set_row_separator_func (GTK_COMBO_BOX (dialog->combo), xfce_icon_chooser_dialog_separator_func, dialog, NULL);
  g_signal_connect (G_OBJECT (dialog->combo), "changed", G_CALLBACK (xfce_icon_chooser_dialog_combo_changed), dialog);
  gtk_grid_attach (GTK_GRID (table), dialog->combo, 1, 0, 1, 1);
  g_object_set (dialog->combo, "hexpand", TRUE, NULL);
  gtk_label_set_mnemonic_widget (GTK_LABEL (label), dialog->combo);
  gtk_widget_show (dialog->combo);

  /* search filter */
  label = gtk_label_new_with_mnemonic (_("_Search icon:"));
  g_object_set (label, "xalign", 0.0f, "yalign", 0.5f, NULL);
  gtk_grid_attach (GTK_GRID (table), label, 0, 1, 1, 1);

  dialog->filter_entry_timeout_source_id = 0;
  dialog->filter_entry = gtk_entry_new ();
  g_object_bind_property (G_OBJECT (dialog->filter_entry), "visible", G_OBJECT (label), "visible", G_BINDING_SYNC_CREATE);
  gtk_grid_attach (GTK_GRID (table), dialog->filter_entry, 1, 1, 1, 1);
  g_object_set (dialog->filter_entry, "hexpand", TRUE, NULL);
  gtk_label_set_mnemonic_widget (GTK_LABEL (label), dialog->filter_entry);
  g_signal_connect (G_OBJECT (dialog->filter_entry), "changed", G_CALLBACK (xfce_icon_chooser_dialog_entry_changed_delay), dialog);
  gtk_entry_set_icon_from_icon_name (GTK_ENTRY (dialog->filter_entry), GTK_ENTRY_ICON_SECONDARY, "edit-clear");
  gtk_entry_set_icon_tooltip_text (GTK_ENTRY (dialog->filter_entry), GTK_ENTRY_ICON_SECONDARY, _("Clear search field"));
  gtk_entry_set_icon_sensitive (GTK_ENTRY (dialog->filter_entry), GTK_ENTRY_ICON_SECONDARY, FALSE);
  g_signal_connect (G_OBJECT (dialog->filter_entry), "icon-release", G_CALLBACK (xfce_icon_chooser_dialog_entry_clear), NULL);

  /* setup the scrolled window for the icon chooser */
  scrolled_window = gtk_scrolled_window_new (NULL, NULL);
  gtk_scrolled_window_set_shadow_type (GTK_SCROLLED_WINDOW (scrolled_window), GTK_SHADOW_ETCHED_IN);
  gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (scrolled_window), GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
  gtk_box_pack_start (GTK_BOX (vbox), scrolled_window, TRUE, TRUE, 0);
  gtk_widget_show (scrolled_window);

  /* setup the icon chooser (shown by default) */
  dialog->icon_chooser = xfce_icon_view_new ();
  g_object_bind_property (G_OBJECT (dialog->icon_chooser), "visible", G_OBJECT (scrolled_window), "visible", G_BINDING_SYNC_CREATE);
  g_signal_connect_swapped (dialog->icon_chooser, "item-activated", G_CALLBACK (gtk_window_activate_default), dialog);
  g_signal_connect_swapped (dialog->icon_chooser, "selection-changed", G_CALLBACK (xfce_icon_chooser_dialog_selection_changed), dialog);
  g_signal_connect_swapped (dialog->icon_chooser, "start-interactive-search", G_CALLBACK (xfce_icon_chooser_dialog_start_interactive_search), dialog);
  gtk_container_add (GTK_CONTAINER (scrolled_window), dialog->icon_chooser);
  gtk_widget_show (dialog->icon_chooser);

  /* setup the icon renderer */
  renderer = xfce_cell_renderer_icon_new ();
  gtk_cell_layout_pack_start (GTK_CELL_LAYOUT (dialog->icon_chooser), renderer, TRUE);
  gtk_cell_layout_set_attributes (GTK_CELL_LAYOUT (dialog->icon_chooser), renderer, "icon", XFCE_ICON_CHOOSER_MODEL_COLUMN_ICON_NAME, NULL);

  /* setup the text renderer */
  renderer = g_object_new (GTK_TYPE_CELL_RENDERER_TEXT,
                           "wrap-mode", PANGO_WRAP_WORD_CHAR,
                           "wrap-width", 104,
                           "xalign", 0.5f,
                           "alignment", PANGO_ALIGN_CENTER,
                           "yalign", 0.0f,
                           NULL);
  gtk_cell_layout_pack_start (GTK_CELL_LAYOUT (dialog->icon_chooser), renderer, FALSE);
  gtk_cell_layout_set_attributes (GTK_CELL_LAYOUT (dialog->icon_chooser), renderer, "text", XFCE_ICON_CHOOSER_MODEL_COLUMN_ICON_NAME, NULL);

  /* setup the file chooser (hidden by default) */
  dialog->file_chooser = gtk_file_chooser_widget_new (GTK_FILE_CHOOSER_ACTION_OPEN);
  gtk_file_chooser_set_local_only (GTK_FILE_CHOOSER (dialog->file_chooser), TRUE);
  xfce_gtk_file_chooser_add_thumbnail_preview (GTK_FILE_CHOOSER (dialog->file_chooser));
  gtk_file_chooser_set_current_folder (GTK_FILE_CHOOSER (dialog->file_chooser), DATADIR G_DIR_SEPARATOR_S "pixmaps");
  g_signal_connect_swapped (dialog->file_chooser, "file-activated", G_CALLBACK (gtk_window_activate_default), dialog);
  g_signal_connect_swapped (dialog->file_chooser, "selection-changed", G_CALLBACK (xfce_icon_chooser_dialog_selection_changed), dialog);
  gtk_box_pack_start (GTK_BOX (vbox), dialog->file_chooser, TRUE, TRUE, 0);

  /* add file chooser filters */
  filter = gtk_file_filter_new ();
  gtk_file_filter_set_name (filter, _("Image Files"));
  gtk_file_filter_add_pixbuf_formats (filter);
  gtk_file_chooser_add_filter (GTK_FILE_CHOOSER (dialog->file_chooser), filter);
  gtk_file_chooser_set_filter (GTK_FILE_CHOOSER (dialog->file_chooser), filter);

  /* update the model */
  xfce_icon_chooser_dialog_set_model (dialog);

  /* default to "All Icons", as the user will be able to narrow the search using the interactive search */
  gtk_combo_box_set_active (GTK_COMBO_BOX (dialog->combo), XFCE_ICON_CHOOSER_CONTEXT_ALL);

  /* start with interactive search */
  xfce_icon_chooser_dialog_start_interactive_search (dialog);
}



static void
xfce_icon_chooser_dialog_finalize (GObject *object)
{
  XfceIconChooserDialog *dialog = XFCE_ICON_CHOOSER_DIALOG (object);

  if (dialog->filter_entry_timeout_source_id != 0)
    {
      g_source_remove (dialog->filter_entry_timeout_source_id);
      dialog->filter_entry_timeout_source_id = 0;
    }
  g_free (dialog->casefolded_text);

  G_OBJECT_CLASS (xfce_icon_chooser_dialog_parent_class)->finalize (object);
}



static void
xfce_icon_chooser_dialog_style_set (GtkWidget *widget,
                                    GtkStyle *previous_style)
{
  /* call the parent's style_set method */
  if (GTK_WIDGET_CLASS (xfce_icon_chooser_dialog_parent_class)->style_set != NULL)
    (*GTK_WIDGET_CLASS (xfce_icon_chooser_dialog_parent_class)->style_set) (widget, previous_style);

  /* update the model */
  xfce_icon_chooser_dialog_set_model (XFCE_ICON_CHOOSER_DIALOG (widget));
}



static void
xfce_icon_chooser_dialog_screen_changed (GtkWidget *widget,
                                         GdkScreen *previous_screen)
{
  /* call the parent's screen_changed method */
  if (GTK_WIDGET_CLASS (xfce_icon_chooser_dialog_parent_class)->screen_changed != NULL)
    (*GTK_WIDGET_CLASS (xfce_icon_chooser_dialog_parent_class)->screen_changed) (widget, previous_screen);

  /* update the model */
  xfce_icon_chooser_dialog_set_model (XFCE_ICON_CHOOSER_DIALOG (widget));
}



static void
xfce_icon_chooser_dialog_close (GtkDialog *dialog)
{
  GdkEvent *event;

  /* verify that the dialog is realized */
  if (G_LIKELY (gtk_widget_get_realized (GTK_WIDGET (dialog))))
    {
      /* send a delete event to the dialog */
      event = gdk_event_new (GDK_DELETE);
      event->any.window = g_object_ref (gtk_widget_get_window (GTK_WIDGET (dialog)));
      event->any.send_event = TRUE;
      gtk_main_do_event (event);
      gdk_event_free (event);
    }
}



static void
xfce_icon_chooser_dialog_set_model (XfceIconChooserDialog *dialog)
{
  XfceIconChooserModel *model;
  GtkTreeModel *filter;

  /* determine the icon chooser model for the widget */
  model = xfce_icon_chooser_model_get_for_widget (GTK_WIDGET (dialog));

  /* check if we have a new model here */
  filter = xfce_icon_view_get_model (XFCE_ICON_VIEW (dialog->icon_chooser));
  if (filter == NULL
      || GTK_TREE_MODEL (model) != gtk_tree_model_filter_get_model (GTK_TREE_MODEL_FILTER (filter)))
    {
      /* setup a new filter for the model */
      filter = gtk_tree_model_filter_new (GTK_TREE_MODEL (model), NULL);
      gtk_tree_model_filter_set_visible_func (GTK_TREE_MODEL_FILTER (filter), xfce_icon_chooser_dialog_visible_func, dialog, NULL);
      xfce_icon_view_set_model (XFCE_ICON_VIEW (dialog->icon_chooser), filter);
      g_object_unref (G_OBJECT (filter));

      /* enable search on the display name */
      xfce_icon_view_set_search_column (XFCE_ICON_VIEW (dialog->icon_chooser), XFCE_ICON_CHOOSER_MODEL_COLUMN_ICON_NAME);
    }
  g_object_unref (G_OBJECT (model));
}



static gboolean
xfce_icon_chooser_dialog_separator_func (GtkTreeModel *model,
                                         GtkTreeIter *iter,
                                         gpointer user_data)
{
  gboolean separator;
  gchar *title;

  /* check if we have a separator here */
  gtk_tree_model_get (model, iter, 0, &title, -1);
  separator = (title[0] == '#' && title[1] == '\0');
  g_free (title);

  return separator;
}



static gboolean
xfce_icon_chooser_dialog_visible_func (GtkTreeModel *model,
                                       GtkTreeIter *iter,
                                       gpointer user_data)
{
  XfceIconChooserDialog *dialog = XFCE_ICON_CHOOSER_DIALOG (user_data);
  guint icon_chooser_context;
  guint item_context;
  gchar *normalized;
  gchar *name;
  gchar *name_casefolded;
  gboolean visible;

  /* check if we need to test the context */
  icon_chooser_context = gtk_combo_box_get_active (GTK_COMBO_BOX (dialog->combo));
  if (G_LIKELY (icon_chooser_context < XFCE_ICON_CHOOSER_CONTEXT_ALL))
    {
      /* determine the context for the iter... */
      gtk_tree_model_get (model, iter, XFCE_ICON_CHOOSER_MODEL_COLUMN_CONTEXT, &item_context, -1);

      /* ...and compare them */
      if (icon_chooser_context != item_context)
        return FALSE;
    }

  /* filter by string */
  if (dialog->casefolded_text != NULL)
    {
      gtk_tree_model_get (model, iter, XFCE_ICON_CHOOSER_MODEL_COLUMN_ICON_NAME, &name, -1);

      /* casefold the name */
      normalized = g_utf8_normalize (name, -1, G_NORMALIZE_ALL);
      g_free (name);
      name_casefolded = g_utf8_casefold (normalized, -1);
      g_free (normalized);

      visible = (strstr (name_casefolded, dialog->casefolded_text) != NULL);

      g_free (name_casefolded);

      return visible;
    }

  /* all icons should be shown */
  return TRUE;
}



static gboolean
xfce_icon_chooser_dialog_start_interactive_search (XfceIconChooserDialog *dialog)
{
  gtk_window_set_focus (GTK_WINDOW (dialog), dialog->filter_entry);

  return TRUE;
}



static void
xfce_icon_chooser_dialog_combo_changed (GtkWidget *combo,
                                        XfceIconChooserDialog *dialog)
{
  XfceIconChooserContext context;
  GtkTreeModel *model;
  GList *selected_items;

  /* determine the new active context */
  context = gtk_combo_box_get_active (GTK_COMBO_BOX (combo));
  if (context <= XFCE_ICON_CHOOSER_CONTEXT_ALL)
    {
      /* hide the file chooser/show the icon chooser */
      gtk_widget_hide (dialog->file_chooser);
      gtk_widget_show (dialog->icon_chooser);
      gtk_widget_show (dialog->filter_entry);

      /* need to re-filter with the new context */
      model = xfce_icon_view_get_model (XFCE_ICON_VIEW (dialog->icon_chooser));
      if (G_LIKELY (model != NULL))
        gtk_tree_model_filter_refilter (GTK_TREE_MODEL_FILTER (model));

      /* check if the icon chooser has a selected item */
      selected_items = xfce_icon_view_get_selected_items (XFCE_ICON_VIEW (dialog->icon_chooser));
      if (G_LIKELY (selected_items != NULL))
        {
          /* make sure the selected item is visible */
          xfce_icon_view_scroll_to_path (XFCE_ICON_VIEW (dialog->icon_chooser), selected_items->data, FALSE, 0.0f, 0.0f);
          g_list_free_full (selected_items, (GDestroyNotify) gtk_tree_path_free);
        }
    }
  else if (context == XFCE_ICON_CHOOSER_CONTEXT_FILE)
    {
      /* show the file chooser/hide the icon chooser */
      gtk_widget_show (dialog->file_chooser);
      gtk_widget_hide (dialog->icon_chooser);
      gtk_widget_hide (dialog->filter_entry);
    }
  else
    {
      /* only show context combo box */
      gtk_widget_hide (dialog->file_chooser);
      gtk_widget_hide (dialog->icon_chooser);
      gtk_widget_hide (dialog->filter_entry);
    }

  /* we certainly changed the selection this way */
  xfce_icon_chooser_dialog_selection_changed (dialog);
}



static void
xfce_icon_chooser_dialog_entry_changed_delay (GtkEntry *entry,
                                              XfceIconChooserDialog *dialog)
{
  if (dialog->filter_entry_timeout_source_id != 0)
    {
      g_source_remove (dialog->filter_entry_timeout_source_id);
      dialog->filter_entry_timeout_source_id = 0;
    }

  dialog->filter_entry_timeout_source_id = g_timeout_add (FILTER_ENTRY_DELAY,
                                                          xfce_icon_chooser_dialog_entry_changed,
                                                          dialog);
}



static gboolean
xfce_icon_chooser_dialog_entry_changed (gpointer user_data)
{
  XfceIconChooserDialog *dialog = XFCE_ICON_CHOOSER_DIALOG (user_data);
  const gchar *text;
  gchar *normalized;
  GtkTreeModel *model;

  g_free (dialog->casefolded_text);
  dialog->casefolded_text = NULL;

  text = gtk_entry_get_text (GTK_ENTRY (dialog->filter_entry));
  if (!xfce_str_is_empty (text))
    {
      /* case fold the search string */
      normalized = g_utf8_normalize (text, -1, G_NORMALIZE_ALL);
      dialog->casefolded_text = g_utf8_casefold (normalized, -1);
      g_free (normalized);
    }

  gtk_entry_set_icon_sensitive (GTK_ENTRY (dialog->filter_entry),
                                GTK_ENTRY_ICON_SECONDARY,
                                !xfce_str_is_empty (text));

  model = xfce_icon_view_get_model (XFCE_ICON_VIEW (dialog->icon_chooser));
  if (G_LIKELY (model != NULL))
    gtk_tree_model_filter_refilter (GTK_TREE_MODEL_FILTER (model));

  dialog->filter_entry_timeout_source_id = 0;

  return FALSE;
}



static void
xfce_icon_chooser_dialog_entry_clear (GtkEntry *entry,
                                      GtkEntryIconPosition icon_pos,
                                      GdkEvent *event)
{
  if (icon_pos == GTK_ENTRY_ICON_SECONDARY)
    gtk_entry_set_text (entry, "");
}



static void
xfce_icon_chooser_dialog_selection_changed (XfceIconChooserDialog *dialog)
{
  gchar *icon;

  /* check if we have a valid icon in the chooser */
  icon = xfce_icon_chooser_dialog_get_icon (dialog);

  /* update all response-ids that we recognize */
  gtk_dialog_set_response_sensitive (GTK_DIALOG (dialog), GTK_RESPONSE_ACCEPT, (icon != NULL));
  gtk_dialog_set_response_sensitive (GTK_DIALOG (dialog), GTK_RESPONSE_APPLY, (icon != NULL));
  gtk_dialog_set_response_sensitive (GTK_DIALOG (dialog), GTK_RESPONSE_OK, (icon != NULL));
  gtk_dialog_set_response_sensitive (GTK_DIALOG (dialog), GTK_RESPONSE_YES, (icon != NULL));

  /* cleanup */
  g_free (icon);
}



/**
 * xfce_icon_chooser_dialog_new:
 * @title             : title of the dialog, or %NULL.
 * @parent            : transient parent of the dialog, or %NULL.
 * @first_button_text : stock-id or text to go in the first button, or %NULL.
 * @...               : response-id for the first button, then additional (button, id) pairs,
 *                      ending with %NULL.
 *
 * Creates a new #XfceIconChooserDialog. This function is analogous to gtk_dialog_new_with_buttons().
 *
 * Returns: (transfer full): a new #XfceIconChooserDialog.
 *
 * Since: 4.21.0
 **/
GtkWidget *
xfce_icon_chooser_dialog_new (const gchar *title,
                              GtkWindow *parent,
                              const gchar *first_button_text,
                              ...)
{
  const gchar *button_text;
  GtkWidget *dialog;
  va_list var_args;

  g_return_val_if_fail (parent == NULL || GTK_IS_WINDOW (parent), NULL);

  dialog = g_object_new (XFCE_TYPE_ICON_CHOOSER_DIALOG,
                         "title", title,
                         NULL);

  if (G_LIKELY (parent != NULL))
    {
      gtk_window_set_destroy_with_parent (GTK_WINDOW (dialog), TRUE);
      gtk_window_set_modal (GTK_WINDOW (dialog), gtk_window_get_modal (parent));
      gtk_window_set_transient_for (GTK_WINDOW (dialog), parent);
    }

  va_start (var_args, first_button_text);
  for (button_text = first_button_text; button_text != NULL; button_text = va_arg (var_args, const gchar *))
    gtk_dialog_add_button (GTK_DIALOG (dialog), button_text, va_arg (var_args, gint));
  va_end (var_args);

  return dialog;
}



/**
 * xfce_icon_chooser_dialog_get_icon:
 * @dialog : an #XfceIconChooserDialog.
 *
 * Returns the currently selected icon for the @dialog. The selected icon can be either
 * a named icon (from the active icon theme) or the absolute path to an image file in the file system.
 * You can distinguish between those two icon types using the g_path_is_absolute() function. If no
 * icon is currently selected, %NULL will be returned.
 *
 * The caller is responsible to free the returned string using g_free() when no longer needed.
 *
 * Returns: (nullable) (transfer full): the currently selected icon for @dialog or %NULL if no icon is selected.
 *
 * Since: 4.21.0
 **/
gchar *
xfce_icon_chooser_dialog_get_icon (XfceIconChooserDialog *dialog)
{
  XfceIconChooserContext context;
  GtkTreeModel *model;
  GtkTreeIter iter;
  GList *selected_items;
  gchar *icon = NULL;

  g_return_val_if_fail (XFCE_IS_ICON_CHOOSER_DIALOG (dialog), NULL);

  /* determine the active context for the chooser */
  context = gtk_combo_box_get_active (GTK_COMBO_BOX (dialog->combo));
  if (context <= XFCE_ICON_CHOOSER_CONTEXT_ALL)
    {
      /* user is selecting a named icon, check if atleast one selected */
      selected_items = xfce_icon_view_get_selected_items (XFCE_ICON_VIEW (dialog->icon_chooser));
      if (G_LIKELY (selected_items != NULL))
        {
          /* determine the name of the first selected icon */
          model = xfce_icon_view_get_model (XFCE_ICON_VIEW (dialog->icon_chooser));
          if (gtk_tree_model_get_iter (model, &iter, selected_items->data))
            gtk_tree_model_get (model, &iter, XFCE_ICON_CHOOSER_MODEL_COLUMN_ICON_NAME, &icon, -1);

          /* release the list of selected items */
          g_list_free_full (selected_items, (GDestroyNotify) gtk_tree_path_free);
        }
    }
  else if (context == XFCE_ICON_CHOOSER_CONTEXT_FILE)
    {
      /* user is selecting an image file, so just return the absolute path to the image file */
      icon = gtk_file_chooser_get_filename (GTK_FILE_CHOOSER (dialog->file_chooser));
      if (icon != NULL && gdk_pixbuf_get_file_info (icon, NULL, NULL) == NULL)
        {
          /* whatever the user selected, it's not an icon that we support */
          g_free (icon);
          icon = NULL;
        }
    }
  else
    {
      /* unset icon by returning an empty string */
      icon = g_strdup ("");
    }

  return icon;
}



/**
 * xfce_icon_chooser_dialog_set_icon:
 * @dialog : an #XfceIconChooserDialog.
 * @icon                : the themed icon or the absolute path to an image file to select.
 *
 * Preselects the specified @icon in the @dialog, and returns %TRUE if the
 * @icon was successfully selected.
 *
 * Returns: %TRUE if the @icon was successfully preselected in the @dialog,
 *          %FALSE otherwise.
 *
 * Since: 4.21.0
 **/
gboolean
xfce_icon_chooser_dialog_set_icon (XfceIconChooserDialog *dialog,
                                   const gchar *icon)
{
  GtkTreeModel *filter;
  GtkTreeModel *model;
  GtkTreePath *filter_path;
  GtkTreePath *model_path;
  GtkTreeIter model_iter;
  guint context;

  g_return_val_if_fail (XFCE_IS_ICON_CHOOSER_DIALOG (dialog), FALSE);
  g_return_val_if_fail (icon != NULL, FALSE);

  /* check if we have a file or a named icon here */
  if (g_path_is_absolute (icon))
    {
      /* try to select the filename in the file chooser */
      if (gtk_file_chooser_set_filename (GTK_FILE_CHOOSER (dialog->file_chooser), icon))
        {
          /* need to display the file chooser with the newly selected filename */
          gtk_combo_box_set_active (GTK_COMBO_BOX (dialog->combo), XFCE_ICON_CHOOSER_CONTEXT_FILE);
          return TRUE;
        }
    }
  else
    {
      /* determine the real model and the filter for the model */
      filter = xfce_icon_view_get_model (XFCE_ICON_VIEW (dialog->icon_chooser));
      model = gtk_tree_model_filter_get_model (GTK_TREE_MODEL_FILTER (filter));

      /* lookup the named icon in the model */
      if (xfce_icon_chooser_model_get_iter_for_icon_name (XFCE_ICON_CHOOSER_MODEL (model), &model_iter, icon))
        {
          /* determine the path for the iterator in the real model */
          model_path = gtk_tree_model_get_path (model, &model_iter);
          if (G_LIKELY (model_path != NULL))
            {
              /* translate the path in the real model to a path in the filter */
              filter_path = gtk_tree_model_filter_convert_child_path_to_path (GTK_TREE_MODEL_FILTER (filter), model_path);
              if (G_UNLIKELY (filter_path == NULL))
                {
                  /* determine the context for the iterator in the real model */
                  gtk_tree_model_get (model, &model_iter, XFCE_ICON_CHOOSER_MODEL_COLUMN_CONTEXT, &context, -1);

                  /* switch the filter to the context for the item at the real iter */
                  gtk_combo_box_set_active (GTK_COMBO_BOX (dialog->combo), context);

                  /* now we should be able to determine the filter path */
                  filter_path = gtk_tree_model_filter_convert_child_path_to_path (GTK_TREE_MODEL_FILTER (filter), model_path);
                }

              /* check if we have a valid path in the filter */
              if (G_LIKELY (filter_path != NULL))
                {
                  /* select the icon and place the cursor on the newly selected icon */
                  xfce_icon_view_select_path (XFCE_ICON_VIEW (dialog->icon_chooser), filter_path);
                  xfce_icon_view_set_cursor (XFCE_ICON_VIEW (dialog->icon_chooser), filter_path, NULL, FALSE);
                  gtk_tree_path_free (filter_path);
                }

              /* release the path in the real model */
              gtk_tree_path_free (model_path);

              /* the icon was successfully selected if we have a filter path */
              return (filter_path != NULL);
            }
        }
    }

  return FALSE;
}

#define __XFCE_ICON_CHOOSER_DIALOG_C__
#include "libxfce4ui-visibility.c"
