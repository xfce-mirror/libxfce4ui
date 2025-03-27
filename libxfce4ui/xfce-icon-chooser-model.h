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

#if !defined(_LIBXFCE4UI_INSIDE_LIBXFCE4UI_H) && !defined(LIBXFCE4UI_COMPILATION)
#error "Only <libxfce4ui/libxfce4ui.h> can be included directly, this file is not part of the public API."
#endif

#ifndef __XFCE_ICON_CHOOSER_MODEL_H__
#define __XFCE_ICON_CHOOSER_MODEL_H__

#include <gtk/gtk.h>

G_BEGIN_DECLS

#define XFCE_TYPE_ICON_CHOOSER_MODEL (xfce_icon_chooser_model_get_type ())
G_DECLARE_FINAL_TYPE (XfceIconChooserModel, xfce_icon_chooser_model, XFCE, ICON_CHOOSER_MODEL, GObject)

/**
 * XfceIconChooserContexts:
 *
 * The list of default contexts for the icon themes
 * according to the Icon Naming Spec, Version 0.7.
 **/
typedef enum
{
  /* the contexts provided by the model */
  XFCE_ICON_CHOOSER_CONTEXT_ACTIONS,
  XFCE_ICON_CHOOSER_CONTEXT_ANIMATIONS,
  XFCE_ICON_CHOOSER_CONTEXT_APPLICATIONS,
  XFCE_ICON_CHOOSER_CONTEXT_CATEGORIES,
  XFCE_ICON_CHOOSER_CONTEXT_DEVICES,
  XFCE_ICON_CHOOSER_CONTEXT_EMBLEMS,
  XFCE_ICON_CHOOSER_CONTEXT_EMOTES,
  XFCE_ICON_CHOOSER_CONTEXT_MIME_TYPES,
  XFCE_ICON_CHOOSER_CONTEXT_PLACES,
  XFCE_ICON_CHOOSER_CONTEXT_STATUS,
  XFCE_ICON_CHOOSER_CONTEXT_STOCK,
  XFCE_ICON_CHOOSER_CONTEXT_OTHER,
  XFCE_ICON_CHOOSER_N_CONTEXTS,

  /* not provided by the model (plus separators before them) */
  XFCE_ICON_CHOOSER_CONTEXT_ALL = XFCE_ICON_CHOOSER_CONTEXT_OTHER + 2,
  XFCE_ICON_CHOOSER_CONTEXT_FILE = XFCE_ICON_CHOOSER_CONTEXT_OTHER + 4,
  XFCE_ICON_CHOOSER_CONTEXT_NO_ICON = XFCE_ICON_CHOOSER_CONTEXT_OTHER + 6,
} XfceIconChooserContext;

/**
 * XfceIconChooserModelColumns:
 * @XFCE_ICON_CHOOSER_MODEL_COLUMN_CONTEXT      : the context of the icon.
 * @XFCE_ICON_CHOOSER_MODEL_COLUMN_ICON_NAME    : the name of the icon.
 * @XFCE_ICON_CHOOSER_MODEL_N_COLUMNS           : the number of columns.
 *
 * The columns provided by the #XfceIconChooserModel.
 **/
typedef enum
{
  XFCE_ICON_CHOOSER_MODEL_COLUMN_CONTEXT,
  XFCE_ICON_CHOOSER_MODEL_COLUMN_ICON_NAME,
  XFCE_ICON_CHOOSER_MODEL_N_COLUMNS,
} XfceIconChooserModelColumn;

G_GNUC_INTERNAL XfceIconChooserModel *
xfce_icon_chooser_model_get_for_widget (GtkWidget *widget) G_GNUC_WARN_UNUSED_RESULT;
G_GNUC_INTERNAL XfceIconChooserModel *
xfce_icon_chooser_model_get_for_icon_theme (GtkIconTheme *icon_theme) G_GNUC_WARN_UNUSED_RESULT;

G_GNUC_INTERNAL gboolean
xfce_icon_chooser_model_get_iter_for_icon_name (XfceIconChooserModel *model,
                                                GtkTreeIter *iter,
                                                const gchar *icon_name);

G_END_DECLS

#endif /* !__XFCE_ICON_CHOOSER_MODEL_H__ */
