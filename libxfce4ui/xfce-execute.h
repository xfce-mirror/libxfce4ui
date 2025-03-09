/*-
 * Copyright (c) 2005-2006 Benedikt Meurer <benny@xfce.org>.
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

#ifndef __XFCE_EXECUTE_H__
#define __XFCE_EXECUTE_H__

#include <glib.h>

G_BEGIN_DECLS

gboolean
xfce_execute_preferred_application (const gchar *category,
                                    const gchar *parameter,
                                    const gchar *working_directory,
                                    gchar **envp,
                                    GError **error);

gboolean
xfce_execute_terminal_shell (const gchar *command_line,
                             const gchar *working_directory,
                             gchar **envp,
                             GError **error);

G_END_DECLS

#endif /* !__XFCE_EXECUTE_H__ */
