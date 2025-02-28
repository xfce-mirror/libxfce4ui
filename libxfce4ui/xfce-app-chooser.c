
#include <gtk/gtk.h>
#include <stdio.h>

#include "app_chooser.h"
#include "libxfce4ui-resources.h"


static void
xfce_app_chooser_dialog_class_init (XfceAppChooserDialogClass *klass);

static void
xfce_app_chooser_dialog_init (XfceAppChooserDialog *dialog);

static void
xfce_app_chooser_dialog_dispose (GObject *object);

static void
xfce_app_chooser_dialog_get_property (GObject    *object,
                                      guint       prop_id,
                                      GValue     *value,
                                      GParamSpec *pspec);

static void
xfce_app_chooser_dialog_set_property (GObject      *object,
                                      guint         prop_id,
                                      const GValue *value,
                                      GParamSpec   *pspec);

static void
xfce_app_chooser_dialog_update_header (XfceAppChooserDialog *dialog);

static void
xfce_app_chooser_model_reload (XfceAppChooserDialog *dialog);

static gboolean
xfce_app_chooser_dialog_button_press_event (GtkWidget           *tree_view,
                                          GdkEventButton      *event,
                                          XfceAppChooserDialog *dialog);

static gboolean
xfce_app_chooser_dialog_context_menu (XfceAppChooserDialog *dialog);

static void
xfce_app_chooser_dialog_row_activated (GtkTreeView         *treeview,
                                     GtkTreePath         *path,
                                     GtkTreeViewColumn   *column,
                                     XfceAppChooserDialog *dialog);

static void
xfce_app_chooser_dialog_selection_changed (GtkTreeSelection    *selection,
                                            XfceAppChooserDialog *dialog);

static void
xfce_app_chooser_dialog_notify_expanded (GtkExpander         *expander,
                                        GParamSpec          *pspec,
                                        XfceAppChooserDialog *dialog);

static void
xfce_app_chooser_dialog_update_accept (XfceAppChooserDialog *dialog);

static void
xfce_app_chooser_dialog_browse_clicked (GtkWidget           *button,
                                        XfceAppChooserDialog *dialog);

static void
xfce_app_chooser_dialog_accept_clicked (XfceAppChooserDialog *dialog);

static void
xfce_app_chooser_dialog_cancel_clicked (GtkWidget           *button,
                                        XfceAppChooserDialog *dialog);

static void
xfce_app_chooser_dialog_action_forget (XfceAppChooserDialog *dialog);

static void
xfce_app_chooser_dialog_action_remove (XfceAppChooserDialog *dialog);

static gboolean
xfce_app_chooser_dialog_selection_func (GtkTreeSelection *selection,
                                      GtkTreeModel     *model,
                                      GtkTreePath      *path,
                                      gboolean          path_currently_selected,
                                      gpointer          user_data);


/* Property identifiers */
enum
{
  PROP_0,
  PROP_MIME,
  PROP_SELECTED_APP,
};

/**
 * XfceAppChooserModelColumn:
 * @XFCE_APP_CHOOSER_MODEL_COLUMN_NAME        : the name of the application.
 * @XFCE_APP_CHOOSER_MODEL_COLUMN_ICON        : the name or absolute path of the application's icon.
 * @XFCE_APP_CHOOSER_MODEL_COLUMN_APPLICATION : the #GAppInfo object.
 * @XFCE_APP_CHOOSER_MODEL_COLUMN_STYLE       : custom font style.
 * @XFCE_APP_CHOOSER_MODEL_N_COLUMNS          : the number of columns in #XFCE_APPChooserModel.
 *
 * The identifiers for the columns provided by the #ThunarChooserModel.
 **/
typedef enum
{
  XFCE_APP_CHOOSER_MODEL_COLUMN_NAME,
  XFCE_APP_CHOOSER_MODEL_COLUMN_ICON,
  XFCE_APP_CHOOSER_MODEL_COLUMN_APPLICATION,
  XFCE_APP_CHOOSER_MODEL_COLUMN_ATTRS,
  XFCE_APP_CHOOSER_MODEL_N_COLUMNS,
} XfceAppChooserModelColumn;



struct _XfceAppChooserDialogClass
{
  GtkWindow __parent__;
};

struct _XfceAppChooserDialog
{
  GtkWindow __parent__;

  gchar       *mime;

  GAppInfo    *selected_app;

  GtkWidget *header_image;
  GtkWidget *header_label;
  GtkWidget *tree_view;
  GtkTreeStore *model;
  GtkWidget *column;
  GtkWidget *custom_expander;
  GtkWidget *custom_entry;
  GtkWidget *custom_button;
  GtkWidget *default_button;
  GtkWidget *cancel_button;
  GtkWidget *accept_button;
};

G_DEFINE_TYPE (XfceAppChooserDialog, xfce_app_chooser_dialog, GTK_TYPE_WINDOW)

static void
xfce_app_chooser_dialog_class_init (XfceAppChooserDialogClass *klass)
{
  GtkWindowClass *gtkwindow_class;
  GtkWidgetClass *gtkwidget_class;
  GObjectClass   *gobject_class;

  GResource *resource = xfrec_get_resource();
  gtkwidget_class = GTK_WIDGET_CLASS (klass);

  gtk_widget_class_set_template_from_resource (
                                            gtkwidget_class,
                                            "/org/xfce/libxfce4ui/xfce-app_chooser.ui");

  gtk_widget_class_bind_template_child (gtkwidget_class, XfceAppChooserDialog, header_image);
  gtk_widget_class_bind_template_child (gtkwidget_class, XfceAppChooserDialog, header_label);
  gtk_widget_class_bind_template_child (gtkwidget_class, XfceAppChooserDialog, tree_view);
  gtk_widget_class_bind_template_child (gtkwidget_class, XfceAppChooserDialog, column);
  gtk_widget_class_bind_template_child (gtkwidget_class, XfceAppChooserDialog, custom_expander);
  gtk_widget_class_bind_template_child (gtkwidget_class, XfceAppChooserDialog, custom_entry);
  gtk_widget_class_bind_template_child (gtkwidget_class, XfceAppChooserDialog, custom_button);
  gtk_widget_class_bind_template_child (gtkwidget_class, XfceAppChooserDialog, default_button);
  gtk_widget_class_bind_template_child (gtkwidget_class, XfceAppChooserDialog, cancel_button);
  gtk_widget_class_bind_template_child (gtkwidget_class, XfceAppChooserDialog, accept_button);

  gtk_widget_class_bind_template_callback (GTK_WIDGET_CLASS (klass), xfce_app_chooser_dialog_button_press_event);
  gtk_widget_class_bind_template_callback (GTK_WIDGET_CLASS (klass), xfce_app_chooser_dialog_context_menu);
  gtk_widget_class_bind_template_callback (GTK_WIDGET_CLASS (klass), xfce_app_chooser_dialog_row_activated);
  gtk_widget_class_bind_template_callback (GTK_WIDGET_CLASS (klass), xfce_app_chooser_dialog_selection_changed);
  gtk_widget_class_bind_template_callback (GTK_WIDGET_CLASS (klass), xfce_app_chooser_dialog_notify_expanded);
  gtk_widget_class_bind_template_callback (GTK_WIDGET_CLASS (klass), xfce_app_chooser_dialog_update_accept);
  gtk_widget_class_bind_template_callback (GTK_WIDGET_CLASS (klass), xfce_app_chooser_dialog_browse_clicked);
  gtk_widget_class_bind_template_callback (GTK_WIDGET_CLASS (klass), xfce_app_chooser_dialog_accept_clicked);
  gtk_widget_class_bind_template_callback (GTK_WIDGET_CLASS (klass), xfce_app_chooser_dialog_cancel_clicked);

  gobject_class = G_OBJECT_CLASS (klass);
  gobject_class->dispose = xfce_app_chooser_dialog_dispose;
  gobject_class->get_property = xfce_app_chooser_dialog_get_property;
  gobject_class->set_property = xfce_app_chooser_dialog_set_property;

  gtkwindow_class = GTK_WINDOW_CLASS (klass);

  return;
}

static void
xfce_app_chooser_dialog_init (XfceAppChooserDialog *dialog)
{
  GtkCellRenderer* renderer;
  
  gtk_widget_init_template (GTK_WIDGET (dialog));
  

  dialog->model = gtk_tree_store_new(XFCE_APP_CHOOSER_MODEL_N_COLUMNS,
                                    G_TYPE_STRING,
                                    G_TYPE_ICON,
                                    G_TYPE_APP_INFO,
                                    PANGO_TYPE_ATTR_LIST);

  gtk_tree_view_set_model( GTK_TREE_VIEW(dialog->tree_view), GTK_TREE_MODEL(dialog->model));
  renderer = gtk_cell_renderer_pixbuf_new ();

  g_object_set (G_OBJECT (renderer), "stock-size", GTK_ICON_SIZE_BUTTON, NULL);
  gtk_tree_view_column_pack_start (GTK_TREE_VIEW_COLUMN (dialog->column), renderer, FALSE);
  gtk_tree_view_column_set_attributes (GTK_TREE_VIEW_COLUMN(dialog->column), renderer,
                                       "gicon", XFCE_APP_CHOOSER_MODEL_COLUMN_ICON,
                                       NULL);
  renderer = gtk_cell_renderer_text_new ();
  gtk_tree_view_column_pack_start (GTK_TREE_VIEW_COLUMN(dialog->column), renderer, TRUE);
  gtk_tree_view_column_set_attributes (GTK_TREE_VIEW_COLUMN(dialog->column), renderer,
                                       "attributes", XFCE_APP_CHOOSER_MODEL_COLUMN_ATTRS,
                                       "text", XFCE_APP_CHOOSER_MODEL_COLUMN_NAME,
                                       NULL);

  GtkTreeSelection *selection = gtk_tree_view_get_selection (GTK_TREE_VIEW (dialog->tree_view));
  gtk_tree_selection_set_select_function (selection, xfce_app_chooser_dialog_selection_func, dialog, NULL);
}



static void
xfce_app_chooser_dialog_dispose (GObject *object)
{
  XfceAppChooserDialog *dialog = XFCE_APP_CHOOSER_DIALOG (object);

  (*G_OBJECT_CLASS (xfce_app_chooser_dialog_parent_class)->dispose) (object);
}

static void
xfce_app_chooser_dialog_get_property (GObject    *object,
                                      guint       prop_id,
                                      GValue     *value,
                                      GParamSpec *pspec)
{
  XfceAppChooserDialog *dialog = XFCE_APP_CHOOSER_DIALOG (object);

  switch (prop_id)
    {
    case PROP_MIME:
      g_value_set_object (value, xfce_app_chooser_dialog_get_mime (dialog));
      break;

    case PROP_SELECTED_APP:
      g_value_set_object (value, xfce_app_chooser_dialog_get_selected_app (dialog));
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
    }
}



static void
xfce_app_chooser_dialog_set_property (GObject      *object,
                                      guint         prop_id,
                                      const GValue *value,
                                      GParamSpec   *pspec)
{
  XfceAppChooserDialog *dialog = XFCE_APP_CHOOSER_DIALOG (object);
  switch (prop_id)
  {
  case PROP_MIME:
    xfce_app_chooser_dialog_set_mime (dialog, g_value_get_object (value));
    break;

  case PROP_SELECTED_APP:
    G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
    break;

  default:
    G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
    break;
  }

}

static gboolean
xfce_app_chooser_dialog_button_press_event (GtkWidget           *tree_view,
                                          GdkEventButton      *event,
                                          XfceAppChooserDialog *dialog)
{
  GtkTreeSelection *selection;
  GtkTreePath      *path;

  /* check if we should popup the context menu */
  if (G_LIKELY (event->button == 3 && event->type == GDK_BUTTON_PRESS))
    {
      /* determine the path for the clicked row */
      if (gtk_tree_view_get_path_at_pos (GTK_TREE_VIEW (tree_view), event->x, event->y, &path, NULL, NULL, NULL))
        {
          /* be sure to select exactly this row... */
          selection = gtk_tree_view_get_selection (GTK_TREE_VIEW (tree_view));
          gtk_tree_selection_unselect_all (selection);
          gtk_tree_selection_select_path (selection, path);
          gtk_tree_path_free (path);

          /* ...and popup the context menu */
          return xfce_app_chooser_dialog_context_menu (dialog);
        }
    }

  return FALSE;
}

static gboolean
xfce_app_chooser_dialog_context_menu (XfceAppChooserDialog *dialog)
{
  GtkTreeSelection *selection;
  GtkTreeModel     *model;
  GtkTreeIter       iter;
  GtkWidget        *item;
  GtkWidget        *menu;
  GAppInfo         *app_info;


  /* determine the selected row */
  selection = gtk_tree_view_get_selection (GTK_TREE_VIEW (dialog->tree_view));
  if (!gtk_tree_selection_get_selected (selection, &model, &iter))
    return FALSE;

  /* determine the app info for the row */
  gtk_tree_model_get (GTK_TREE_MODEL(dialog->model), &iter, XFCE_APP_CHOOSER_MODEL_COLUMN_APPLICATION, &app_info, -1);
  if (G_UNLIKELY (app_info == NULL))
    return FALSE;


  /* prepare the popup menu */
  menu = gtk_menu_new ();

  /* append the "Remove Launcher" item */
  item = gtk_menu_item_new_with_mnemonic (_("_Delete Custom Launcher"));
  gtk_widget_set_sensitive (item, g_app_info_can_delete (app_info));
  g_signal_connect_swapped (G_OBJECT (item), "activate", G_CALLBACK (xfce_app_chooser_dialog_action_remove), dialog);
  gtk_menu_shell_append (GTK_MENU_SHELL (menu), item);
  gtk_widget_show (item);

  /* append the "Forget Association" item */
  item = gtk_menu_item_new_with_mnemonic (_("_Remove From Recommended Apps"));
  gtk_widget_set_sensitive (item, g_app_info_can_remove_supports_type (app_info));
  g_signal_connect_swapped (G_OBJECT (item), "activate", G_CALLBACK (xfce_app_chooser_dialog_action_forget), dialog);
  gtk_menu_shell_append (GTK_MENU_SHELL (menu), item);
  gtk_widget_show (item);

  /* run the menu (takes over the floating of menu) */
  gtk_menu_popup_at_pointer (GTK_MENU (menu), NULL);

  /* clean up */
  g_object_unref (app_info);

  return TRUE;
}

static void
xfce_app_chooser_dialog_action_remove (XfceAppChooserDialog *dialog)
{
  GtkTreeSelection *selection;
  GtkTreeModel     *model;
  GtkTreeIter       iter;
  const gchar      *name;
  GtkWidget        *message;
  GAppInfo         *app_info;
  GError           *error = NULL;
  gint              response;

  /* determine the selected row */
  selection = gtk_tree_view_get_selection (GTK_TREE_VIEW (dialog->tree_view));
  if (!gtk_tree_selection_get_selected (selection, &model, &iter))
    return;

  /* determine the app info for the row */
  gtk_tree_model_get (model, &iter, XFCE_APP_CHOOSER_MODEL_COLUMN_APPLICATION, &app_info, -1);
  if (G_UNLIKELY (app_info == NULL))
    return;

  if (g_app_info_can_delete (app_info))
    {
      /* determine the name of the app info */
      name = g_app_info_get_name (app_info);

      /* ask the user whether to remove the application launcher */
      message = gtk_message_dialog_new (GTK_WINDOW (dialog),
                                        GTK_DIALOG_DESTROY_WITH_PARENT
                                        | GTK_DIALOG_MODAL,
                                        GTK_MESSAGE_QUESTION,
                                        GTK_BUTTONS_NONE,
                                        _("Are you sure that you want to remove \"%s\"?"), name);
      gtk_window_set_title (GTK_WINDOW (message), _("Remove application launcher"));
      gtk_dialog_add_buttons (GTK_DIALOG (message),
                              _("_Cancel"), GTK_RESPONSE_CANCEL,
                                _("_Remove"), GTK_RESPONSE_YES,
                              NULL);
      gtk_dialog_set_default_response (GTK_DIALOG (message), GTK_RESPONSE_YES);
      gtk_message_dialog_format_secondary_text (GTK_MESSAGE_DIALOG (message), _("This will remove the application launcher that appears in the file "
                                                                                "context menu, but will not uninstall the application itself.\n\n"
                                                                                "You can only remove application launchers that were created using "
                                                                                "the custom command box in the \"Open With\" dialog of the file "
                                                                                "manager."));
      response = gtk_dialog_run (GTK_DIALOG (message));
      gtk_widget_destroy (message);

      /* check if the user confirmed the removal */
      if (G_LIKELY (response == GTK_RESPONSE_YES))
        {
        }
    }

  /* cleanup */
  g_object_unref (app_info);
}




/**
 * thunar_chooser_dialog_set_file:
 * @dialog : a #ThunarChooserDialog.
 * @file   : a #ThunarFile or %NULL.
 *
 * Associates @dialog with @file.
 **/
static void
xfce_app_chooser_dialog_set_mime (XfceAppChooserDialog *dialog,
                                  gchar          *mime)
{

  dialog->mime = mime;
  xfce_app_chooser_model_reload(XFCE_APP_CHOOSER_DIALOG(dialog));

  /* update the header */
  xfce_app_chooser_dialog_update_header (dialog);
}


static gint
compare_app_infos (gconstpointer a,
                   gconstpointer b)
{
  return g_app_info_equal (G_APP_INFO (a), G_APP_INFO (b)) ? 0 : 1;
}


static gint
sort_app_infos (gconstpointer a,
                gconstpointer b)
{
  return g_utf8_collate (g_app_info_get_name (G_APP_INFO (a)),
                         g_app_info_get_name (G_APP_INFO (b)));
}


static void
xfce_app_chooser_model_append (GtkTreeStore *model,
                                const gchar *title,
                                const gchar *icon_name,
                                GList *app_infos);

static void
xfce_app_chooser_model_reload (XfceAppChooserDialog *dialog)
{
  GtkTreeIter parent_iter;
  GtkTreeIter child_iter;
  GList *fallback;
  GList *all;
  GList *lp;
  GList *other = NULL;
  GList *recommended;
  GList *default_app = NULL;

  gtk_tree_store_clear (GTK_TREE_STORE (dialog->model));

  /* get default application for this type and append it in @default_app */
  default_app = g_list_prepend (default_app, g_app_info_get_default_for_type (dialog->mime, FALSE));

  /* If default application was already selected, then display it in Treeview */
  if (default_app->data)
    {
      xfce_app_chooser_model_append (GTK_TREE_STORE(dialog->model),
                                   _("Default Application"),
                                   "org.xfce.settings.default-applications",
                                   default_app);

    }

  /* check if we have any applications for this type */
  recommended = g_app_info_get_recommended_for_type (dialog->mime);

  /* append them as recommended */
  recommended = g_list_sort (recommended, sort_app_infos);
  xfce_app_chooser_model_append (GTK_TREE_STORE(dialog->model),
                               _("Recommended Applications"),
                               "org.xfce.settings.default-applications",
                               recommended);

  fallback = g_app_info_get_fallback_for_type (dialog->mime);

  /* append the other applications */
  fallback = g_list_sort (fallback, sort_app_infos);
  xfce_app_chooser_model_append (GTK_TREE_STORE(dialog->model),
                               _("Other Applications"),
                               "gnome-applications",
                               fallback);

  all = g_app_info_get_all ();
  for (lp = all; lp != NULL; lp = lp->next)
    {
          if ((g_list_find_custom (recommended,
                              lp->data,
                              compare_app_infos)
          == NULL) && (g_list_find_custom (fallback,
                              lp->data,
                              compare_app_infos)
          == NULL))
        {
          other = g_list_prepend (other, lp->data);
    }
  }

  /* append the other applications */
  other = g_list_sort (other, sort_app_infos);
  xfce_app_chooser_model_append (GTK_TREE_STORE(dialog->model),
                               _("All Applications"),
                               "gnome-applications",
                               other);

  if (default_app->data != NULL)
    g_object_unref (default_app->data);
  g_list_free (default_app);

  g_list_free_full (recommended, g_object_unref);
  g_list_free_full (all, g_object_unref);

  g_list_free (other);
  gtk_tree_view_expand_all(GTK_TREE_VIEW(dialog->tree_view));

}

static void
xfce_app_chooser_model_append (GtkTreeStore *model,
                                const gchar *title,
                                const gchar *icon_name,
                                GList *app_infos)
{
  GIcon *icon;
  GtkTreeIter child_iter;
  GtkTreeIter parent_iter;
  GList *li;
  PangoAttrList *attrs;

  attrs = pango_attr_list_new ();
  pango_attr_list_insert (attrs, pango_attr_weight_new (PANGO_WEIGHT_BOLD));

  icon = g_themed_icon_new (icon_name);
  gtk_tree_store_append (model, &parent_iter, NULL);
  gtk_tree_store_set (model, &parent_iter,
                      XFCE_APP_CHOOSER_MODEL_COLUMN_NAME, title,
                      XFCE_APP_CHOOSER_MODEL_COLUMN_ICON, icon,
                      XFCE_APP_CHOOSER_MODEL_COLUMN_ATTRS, attrs,
                       -1);
  g_object_unref (G_OBJECT (icon));
  pango_attr_list_unref (attrs);

  if (G_LIKELY (app_infos != NULL))
    {
      /* insert the program items */
      for (li = app_infos; li != NULL; li = li->next)
        {
          /* append the tree row with the program data */
          gtk_tree_store_append (model, &child_iter, &parent_iter);
          gtk_tree_store_set (model, &child_iter,
                              XFCE_APP_CHOOSER_MODEL_COLUMN_NAME, g_app_info_get_name (li->data),
                              XFCE_APP_CHOOSER_MODEL_COLUMN_ICON, g_app_info_get_icon (li->data),
                              XFCE_APP_CHOOSER_MODEL_COLUMN_APPLICATION, li->data,
                              -1);
        }
    }
  else
    {
      attrs = pango_attr_list_new ();
      pango_attr_list_insert (attrs, pango_attr_style_new (PANGO_STYLE_ITALIC));

      /* tell the user that we don't have any applications for this category */
      gtk_tree_store_append (model, &child_iter, &parent_iter);
      gtk_tree_store_set (model, &child_iter,
                          XFCE_APP_CHOOSER_MODEL_COLUMN_NAME, _("None available"),
                          XFCE_APP_CHOOSER_MODEL_COLUMN_ATTRS, attrs,
                           -1);
      pango_attr_list_unref (attrs);
    }
}

static void
xfce_app_chooser_dialog_row_activated (GtkTreeView         *treeview,
                                     GtkTreePath         *path,
                                     GtkTreeViewColumn   *column,
                                     XfceAppChooserDialog *dialog)
{
  GtkTreeModel *model;
  GtkTreeIter   iter;
  GValue        value = G_VALUE_INIT;

  /* determine the current chooser model */
  model = gtk_tree_view_get_model (treeview);
  if (G_UNLIKELY (model == NULL))
    return;

  /* determine the application for the tree path */
  gtk_tree_model_get_iter (model, &iter, path);
  gtk_tree_model_get_value (model, &iter, XFCE_APP_CHOOSER_MODEL_COLUMN_APPLICATION, &value);

  /* check if the row refers to a valid application */
  if (G_LIKELY (g_value_get_object (&value) != NULL))
    {
      /* emit the accept dialog response */
      xfce_app_chooser_dialog_accept_clicked(dialog);
    }
  else if (gtk_tree_view_row_expanded (treeview, path))
    {
      /* collapse the path that were double clicked */
      gtk_tree_view_collapse_row (treeview, path);
    }
  else
    {
      /* expand the path that were double clicked */
      gtk_tree_view_expand_to_path (treeview, path);
    }

  /* cleanup */
  g_value_unset (&value);
}

static void
xfce_app_chooser_dialog_notify_expanded (GtkExpander         *expander,
                                        GParamSpec          *pspec,
                                        XfceAppChooserDialog *dialog)
{
  GtkTreeSelection *selection;

  /* clear the application selection whenever the expander
   * is expanded to avoid confusion for the user.
   */
  if (gtk_expander_get_expanded (expander))
    {
      selection = gtk_tree_view_get_selection (GTK_TREE_VIEW (dialog->tree_view));
      gtk_tree_selection_unselect_all (selection);
    }

  /* update the sensitivity of the "Ok"/"Open" button */
  xfce_app_chooser_dialog_update_accept (dialog);
}


static void
xfce_app_chooser_dialog_update_accept (XfceAppChooserDialog *dialog)
{
  GtkTreeSelection *selection;
  GtkTreeModel     *model;
  GtkTreeIter       iter;
  const gchar      *text;
  gboolean          sensitive = FALSE;
  GValue            value = G_VALUE_INIT;


  if (gtk_expander_get_expanded (GTK_EXPANDER (dialog->custom_expander)))
    {
      /* check if the user entered a valid custom command */
      text = gtk_entry_get_text (GTK_ENTRY (dialog->custom_entry));
      sensitive = (text != NULL && *text != '\0');
    }
  else
    {
      selection = gtk_tree_view_get_selection (GTK_TREE_VIEW (dialog->tree_view));
      if (gtk_tree_selection_get_selected (selection, &model, &iter))
        {
          /* check if the selected row refers to a valid application */
          gtk_tree_model_get_value (model, &iter, XFCE_APP_CHOOSER_MODEL_COLUMN_APPLICATION, &value);
          sensitive = (g_value_get_object (&value) != NULL);
          g_value_unset (&value);
        }
    }

  /* update the "Ok"/"Open" button sensitivity */
  gtk_widget_set_sensitive (GTK_WIDGET (dialog->accept_button), sensitive);
}


static void
xfce_app_chooser_dialog_update_header (XfceAppChooserDialog *dialog)
{
  const gchar *content_type;
  GIcon       *icon;
  gchar       *description;
  gchar       *text;

  description = g_content_type_get_description (dialog->mime);

  icon = g_content_type_get_icon (dialog->mime);
  gtk_image_set_from_gicon (GTK_IMAGE (dialog->header_image), icon, GTK_ICON_SIZE_DIALOG);
  g_object_unref (icon);

  /* update the header label */
  text = g_strdup_printf (_("Open files of type \"%s\" with:"),
                          dialog->mime);
  gtk_label_set_markup (GTK_LABEL (dialog->header_label), text);
  g_free (text);

  /* update the "Browse..." tooltip */
  gtk_widget_set_tooltip_text(dialog->custom_button,
                                 g_strdup_printf(_("Browse the file system to select an "
                                   "application to open files of type \"%s\"."),
                                 dialog->mime));

  /* update the "Use as default for this kind of file" tooltip */
  gtk_widget_set_tooltip_text(dialog->default_button,
                                 g_strdup_printf (_("Change the default application for files "
                                   "of type \"%s\" to the selected application."),
                                 description));

  /* cleanup */
  g_free (description);
}

static void
xfce_app_chooser_dialog_selection_changed (GtkTreeSelection    *selection,
                                            XfceAppChooserDialog *dialog)
{
  GAppInfo     *app_info;
  GtkTreeModel *model;
  const gchar  *exec;
  GtkTreeIter   iter;

  /* determine the iterator for the selected row */
  if (gtk_tree_selection_get_selected (selection, &model, &iter))
    {
      /* determine the app info for the selected row */
      gtk_tree_model_get (model, &iter, XFCE_APP_CHOOSER_MODEL_COLUMN_APPLICATION, &app_info, -1);

      /* determine the command for the app info */
      exec = g_app_info_get_executable (app_info);
      if (G_LIKELY (exec != NULL && g_utf8_validate (exec, -1, NULL)))
        {
          /* setup the command as default for the custom command box */
          gtk_entry_set_text (GTK_ENTRY (dialog->custom_entry), exec);
        }

      /* cleanup */
      g_object_unref (app_info);
    }

  /* update the sensitivity of the "Ok"/"Open" button */
  xfce_app_chooser_dialog_update_accept (dialog);
}

static gboolean
xfce_app_chooser_dialog_selection_func (GtkTreeSelection *selection,
                                      GtkTreeModel     *model,
                                      GtkTreePath      *path,
                                      gboolean          path_currently_selected,
                                      gpointer          user_data)
{
  GtkTreeIter iter;
  gboolean    permitted = TRUE;
  GValue      value = G_VALUE_INIT;

  /* we can always change the selection if the path is already selected */
  if (G_UNLIKELY (!path_currently_selected))
    {
      /* check if there's an application for the path */
      gtk_tree_model_get_iter (model, &iter, path);
      gtk_tree_model_get_value (model, &iter, XFCE_APP_CHOOSER_MODEL_COLUMN_APPLICATION, &value);
      permitted = (g_value_get_object (&value) != NULL);
      g_value_unset (&value);
    }

  return permitted;
}

static void
xfce_app_chooser_dialog_browse_clicked (GtkWidget           *button,
                                        XfceAppChooserDialog *dialog)
{
  GtkFileFilter *filter;
  GtkWidget     *chooser;
  gchar         *filename;
  gchar         *filename_escaped;
  gchar         *s;

  chooser = gtk_file_chooser_dialog_new (_("Select an Application"),
                                         GTK_WINDOW (dialog),
                                         GTK_FILE_CHOOSER_ACTION_OPEN,
                                           _("_Cancel"), GTK_RESPONSE_CANCEL,
                                             _("_Open"), GTK_RESPONSE_ACCEPT,
                                         NULL);
  gtk_file_chooser_set_local_only (GTK_FILE_CHOOSER (chooser), TRUE);

  /* add file chooser filters */
  filter = gtk_file_filter_new ();
  gtk_file_filter_set_name (filter, _("All Files"));
  gtk_file_filter_add_pattern (filter, "*");
  gtk_file_chooser_add_filter (GTK_FILE_CHOOSER (chooser), filter);

  filter = gtk_file_filter_new ();
  gtk_file_filter_set_name (filter, _("Executable Files"));
  gtk_file_filter_add_mime_type (filter, "application/x-csh");
  gtk_file_filter_add_mime_type (filter, "application/x-executable");
  gtk_file_filter_add_mime_type (filter, "application/x-perl");
  gtk_file_filter_add_mime_type (filter, "application/x-python");
  gtk_file_filter_add_mime_type (filter, "application/x-ruby");
  gtk_file_filter_add_mime_type (filter, "application/x-shellscript");
  gtk_file_filter_add_pattern (filter, "*.pl");
  gtk_file_filter_add_pattern (filter, "*.py");
  gtk_file_filter_add_pattern (filter, "*.rb");
  gtk_file_filter_add_pattern (filter, "*.sh");
  gtk_file_chooser_add_filter (GTK_FILE_CHOOSER (chooser), filter);
  gtk_file_chooser_set_filter (GTK_FILE_CHOOSER (chooser), filter);

  filter = gtk_file_filter_new ();
  gtk_file_filter_set_name (filter, _("Perl Scripts"));
  gtk_file_filter_add_mime_type (filter, "application/x-perl");
  gtk_file_filter_add_pattern (filter, "*.pl");
  gtk_file_chooser_add_filter (GTK_FILE_CHOOSER (chooser), filter);

  filter = gtk_file_filter_new ();
  gtk_file_filter_set_name (filter, _("Python Scripts"));
  gtk_file_filter_add_mime_type (filter, "application/x-python");
  gtk_file_filter_add_pattern (filter, "*.py");
  gtk_file_chooser_add_filter (GTK_FILE_CHOOSER (chooser), filter);

  filter = gtk_file_filter_new ();
  gtk_file_filter_set_name (filter, _("Ruby Scripts"));
  gtk_file_filter_add_mime_type (filter, "application/x-ruby");
  gtk_file_filter_add_pattern (filter, "*.rb");
  gtk_file_chooser_add_filter (GTK_FILE_CHOOSER (chooser), filter);

  filter = gtk_file_filter_new ();
  gtk_file_filter_set_name (filter, _("Shell Scripts"));
  gtk_file_filter_add_mime_type (filter, "application/x-csh");
  gtk_file_filter_add_mime_type (filter, "application/x-shellscript");
  gtk_file_filter_add_pattern (filter, "*.sh");
  gtk_file_chooser_add_filter (GTK_FILE_CHOOSER (chooser), filter);

  /* use the bindir as default folder */
  gchar* BINDIR="/usr/bin";
  gtk_file_chooser_set_current_folder (GTK_FILE_CHOOSER (chooser), BINDIR);

  /* setup the currently selected file */
  filename = gtk_editable_get_chars (GTK_EDITABLE (dialog->custom_entry), 0, -1);
  if (G_LIKELY (filename != NULL))
    {
      /* use only the first argument */
      s = strchr (filename, ' ');
      if (G_UNLIKELY (s != NULL))
        *s = '\0';

      /* check if we have a file name */
      if (G_LIKELY (*filename != '\0'))
        {
          /* check if the filename is not an absolute path */
          if (G_LIKELY (!g_path_is_absolute (filename)))
            {
              /* try to lookup the filename in $PATH */
              s = g_find_program_in_path (filename);
              if (G_LIKELY (s != NULL))
                {
                  /* use the absolute path instead */
                  g_free (filename);
                  filename = s;
                }
            }

          /* check if we have an absolute path now */
          if (G_LIKELY (g_path_is_absolute (filename)))
            gtk_file_chooser_set_filename (GTK_FILE_CHOOSER (chooser), filename);
        }

      /* release the filename */
      g_free (filename);
    }

  /* run the chooser dialog */
  if (gtk_dialog_run (GTK_DIALOG (chooser)) == GTK_RESPONSE_ACCEPT)
    {
      filename = gtk_file_chooser_get_filename (GTK_FILE_CHOOSER (chooser));
      gtk_entry_set_text (GTK_ENTRY (dialog->custom_entry), filename);
      g_free (filename);
    }

  gtk_widget_destroy (chooser);
}

static void
xfce_app_chooser_dialog_accept_clicked (XfceAppChooserDialog *dialog)
{
  GdkAppLaunchContext *context;
  GtkTreeSelection    *selection;
  GtkTreeModel        *model;
  GtkTreeIter          iter;
  const gchar         *content_type;
  GAppInfo            *app_info = NULL;
  gboolean             succeed = TRUE;
  GError              *error = NULL;
  const gchar         *custom_command;
  gchar               *name;
  GList                list;
  GList               *all_apps, *lp;
  GdkScreen           *screen;
  GAppInfo            *default_app = NULL;
  

  /* determine the application that was chosen by the user */
  if (!gtk_expander_get_expanded (GTK_EXPANDER (dialog->custom_expander)))
    {
      selection = gtk_tree_view_get_selection (GTK_TREE_VIEW (dialog->tree_view));
      if (gtk_tree_selection_get_selected (selection, &model, &iter))
        gtk_tree_model_get (model, &iter, XFCE_APP_CHOOSER_MODEL_COLUMN_APPLICATION, &app_info, -1);
    }
  else
    {
      custom_command = gtk_entry_get_text (GTK_ENTRY (dialog->custom_entry));

      /* determine the name of the custom command */
      name = g_path_get_basename (custom_command);

      /* try to add an application for the custom command */
      app_info = g_app_info_create_from_commandline (custom_command, name, G_APP_INFO_CREATE_NONE, &error);

      /* cleanup */
      g_free (name);

      /* verify the application */
      if (G_UNLIKELY (app_info == NULL))
        {
          /* display an error to the user Failed to add new application */
          /* release the error */
          g_error_free (error);
          return;
        }

      /* Check if that application already exists in our list */
      all_apps = g_app_info_get_all ();
      for (lp = all_apps; lp != NULL; lp = lp->next)
        {
          if (g_strcmp0 (g_app_info_get_name (lp->data), g_app_info_get_name (app_info)) == 0 && g_strcmp0 (g_app_info_get_commandline (lp->data), g_app_info_get_commandline (app_info)) == 0)
            {
              /* Re-use existing app-info instead of adding the same one again */
              g_object_unref (app_info);
              app_info = g_object_ref (lp->data);
              break;
            }
        }
      g_list_free_full (all_apps, g_object_unref);
    }

  /* verify that we have a valid application */
  if (G_UNLIKELY (app_info == NULL))
    return;

  dialog->selected_app = app_info;

  default_app = g_app_info_get_default_for_type (dialog->mime, FALSE);

  /* check if we should also set the application as default or
   * if application is opened first time, set it as default application */
  if (gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (dialog->default_button))
      || default_app == NULL)
    {
      /* remember the application as default for these kind of file */
      succeed = g_app_info_set_as_default_for_type (app_info, dialog->mime, &error);

      /* verify that we were successful */
      if (G_UNLIKELY (!succeed))
        {
          /* display an error to the user Failed to set default application */
          /* release the error */
          g_error_free (error);
        }

    }
  else
    {
      /* simply try to set the app as last used for this type (we do not show any errors here) */
      g_app_info_set_as_last_used_for_type (app_info, dialog->mime, NULL);
    }

  if (default_app != NULL)
    g_object_unref (default_app);

  /* cleanup */
  g_object_unref (app_info);

  gtk_widget_destroy(GTK_WIDGET(dialog));
}

static void
xfce_app_chooser_dialog_cancel_clicked (GtkWidget           *button,
                                        XfceAppChooserDialog *dialog)
{
  gtk_widget_destroy(GTK_WIDGET(dialog));
}


static void
xfce_app_chooser_dialog_action_forget (XfceAppChooserDialog *dialog)
{
  GtkTreeSelection *selection;
  GtkTreeModel     *model;
  GtkTreeIter       iter;
  const gchar      *name;
  GtkWidget        *message;
  GAppInfo         *app_info;
  const gchar      *content_type;
  GError           *error = NULL;
  gint              response;


  /* determine the selected row */
  selection = gtk_tree_view_get_selection (GTK_TREE_VIEW (dialog->tree_view));
  if (!gtk_tree_selection_get_selected (selection, &model, &iter))
    return;

  /* determine the app info for the row */
  gtk_tree_model_get (model, &iter, XFCE_APP_CHOOSER_MODEL_COLUMN_APPLICATION, &app_info, -1);
  if (G_UNLIKELY (app_info == NULL))
    return;

  if (g_app_info_can_remove_supports_type (app_info))
    {
      /* determine the name of the app info */
      name = g_app_info_get_name (app_info);

      /* ask the user whether to forget the application launcher */
      message = gtk_message_dialog_new (GTK_WINDOW (dialog),
                                        GTK_DIALOG_DESTROY_WITH_PARENT
                                        | GTK_DIALOG_MODAL,
                                        GTK_MESSAGE_QUESTION,
                                        GTK_BUTTONS_NONE,
                                        _("Are you sure that you want to forget \"%s\"?"), name);
      gtk_window_set_title (GTK_WINDOW (message), _("Forget application launcher"));
      gtk_dialog_add_buttons (GTK_DIALOG (message),
                              _("_Cancel"), GTK_RESPONSE_CANCEL,
                                _("_Forget"), GTK_RESPONSE_YES,
                              NULL);
      gtk_dialog_set_default_response (GTK_DIALOG (message), GTK_RESPONSE_YES);
      gtk_message_dialog_format_secondary_text (GTK_MESSAGE_DIALOG (message), _("This will dissociate the application launcher for this file type, "
                                                                                "but will not uninstall or remove the application launcher itself."));
      response = gtk_dialog_run (GTK_DIALOG (message));
      gtk_widget_destroy (message);

      /* check if the user confirmed */
      if (G_LIKELY (response == GTK_RESPONSE_YES))
        {
          /* Dont support this mime-type any more with that application */
          g_app_info_remove_supports_type (app_info, dialog->mime, NULL);
        }
    }

  /* cleanup */
  g_object_unref (app_info);
}

GAppInfo*
xfce_app_chooser_dialog_get_selected_app(XfceAppChooserDialog *dialog)
{
  return dialog->selected_app;
}

gchar*
xfce_app_chooser_dialog_get_mime(XfceAppChooserDialog *dialog)
{
  return dialog->mime;
}

GtkWidget*
xfce_app_chooser_dialog_new (GtkWindow    *parent)
{
  GtkWidget* dialog=g_object_new(XFCE_TYPE_APP_CHOOSER_DIALOG, NULL, "parent", parent);
  if (parent != NULL)
  {
    gtk_window_set_modal(GTK_WINDOW(dialog), TRUE);
    gtk_window_set_transient_for(GTK_WINDOW(dialog), GTK_WINDOW(parent));
  }
  return dialog;
}

GtkWidget*
xfce_app_chooser_dialog_new_for_mime (GtkWindow *parent, gchar *mime)
{
  GtkWidget* dialog=g_object_new(XFCE_TYPE_APP_CHOOSER_DIALOG, NULL, "parent", parent);
  if (parent != NULL)
  {
    gtk_window_set_modal(GTK_WINDOW(dialog), TRUE);
    gtk_window_set_transient_for(GTK_WINDOW(dialog), GTK_WINDOW(parent));
  }
  xfce_app_chooser_dialog_set_mime(XFCE_APP_CHOOSER_DIALOG(dialog), mime);
  return dialog;
}
