/*
 * Copyright (c) 2007 The Xfce Development Team
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
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston,
 * MA 02110-1301 USA
 */

#ifndef __LIBXFCE4UI_PRIVATE_H__
#define __LIBXFCE4UI_PRIVATE_H__

#include <gtk/gtk.h>

G_BEGIN_DECLS

#ifndef NDEBUG
#define _libxfce4ui_assert(expr) g_assert (expr)
#define _libxfce4ui_assert_not_reached() g_assert_not_reached ()
#define _libxfce4ui_return_if_fail(expr) g_return_if_fail (expr)
#define _libxfce4ui_return_val_if_fail(expr, val) g_return_val_if_fail (expr, val)
#else
#define _libxfce4ui_assert(expr) \
  G_STMT_START { (void) 0; } \
  G_STMT_END
#define _libxfce4ui_assert_not_reached() \
  G_STMT_START { (void) 0; } \
  G_STMT_END
#define _libxfce4ui_return_if_fail(expr) \
  G_STMT_START { (void) 0; } \
  G_STMT_END
#define _libxfce4ui_return_val_if_fail(expr, val) \
  G_STMT_START { (void) 0; } \
  G_STMT_END
#endif

GdkPixbuf *
xfce_gdk_pixbuf_colorize (const GdkPixbuf *source,
                          const GdkColor *color) G_GNUC_MALLOC G_GNUC_WARN_UNUSED_RESULT;

GdkPixbuf *
xfce_gdk_pixbuf_spotlight (const GdkPixbuf *source) G_GNUC_MALLOC G_GNUC_WARN_UNUSED_RESULT;


G_END_DECLS

#endif /* !__LIBXFCE4UI_PRIVATE_H__ */
