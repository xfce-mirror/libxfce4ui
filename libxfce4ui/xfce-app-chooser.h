
#include <gtk/gtk.h>

#ifndef __XFCE_APP_CHOOSER_DIALOG_H__
#define __XFCE_APP_CHOOSER_DIALOG_H__


G_BEGIN_DECLS;

#define XFCE_TYPE_APP_CHOOSER_DIALOG (xfce_app_chooser_dialog_get_type ())

G_DECLARE_FINAL_TYPE (XfceAppChooserDialog, xfce_app_chooser_dialog, XFCE, APP_CHOOSER_DIALOG, GtkWindow)

GType
xfce_app_chooser_dialog_get_type (void) G_GNUC_CONST;

gchar*
xfce_app_chooser_dialog_get_mime(XfceAppChooserDialog *dialog);

static void
xfce_app_chooser_dialog_set_mime(XfceAppChooserDialog *dialog, gchar* mime);

GAppInfo*
xfce_app_chooser_dialog_get_selected_app(XfceAppChooserDialog *dialog);

GtkWidget*
xfce_app_chooser_dialog_new (GtkWindow    *parent);

GtkWidget*
xfce_app_chooser_dialog_new_for_mime (GtkWindow    *parent, gchar *mime);

G_END_DECLS;

#endif /* !__XFCE_APP_CHOOSER_DIALOG_H__ */
