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

#ifndef __XFCE_DIE_COMMAND_ENTRY_H__
#define __XFCE_DIE_COMMAND_ENTRY_H__

#include <gtk/gtk.h>

G_BEGIN_DECLS

#define XFCE_TYPE_DIE_COMMAND_ENTRY (xfce_die_command_entry_get_type ())
G_DECLARE_FINAL_TYPE (XfceDieCommandEntry, xfce_die_command_entry, XFCE, DIE_COMMAND_ENTRY, GtkBox)

GtkWidget *
xfce_die_command_entry_new (void) G_GNUC_MALLOC;
const gchar *
xfce_die_command_entry_get_text (XfceDieCommandEntry *command_entry);
void
xfce_die_command_entry_set_text (XfceDieCommandEntry *command_entry,
                                 const gchar *text);
GtkWidget *
xfce_die_command_entry_get_text_entry (XfceDieCommandEntry *command_entry);

G_END_DECLS

#endif /* !__XFCE_DIE_COMMAND_ENTRY_H__ */
