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

#ifndef __XFCE_DIE_COMMAND_MODEL_H__
#define __XFCE_DIE_COMMAND_MODEL_H__

#include <glib-object.h>

G_BEGIN_DECLS

#define XFCE_TYPE_DIE_COMMAND_MODEL (xfce_die_command_model_get_type ())
G_DECLARE_FINAL_TYPE (XfceDieCommandModel, xfce_die_command_model, XFCE, DIE_COMMAND_MODEL, GObject)

/**
 * XfceDieCommandModelColumn:
 * @XFCE_DIE_COMMAND_MODEL_COLUMN_NAME : the column with the file name.
 *
 * The columns provided by the #XfceDieCommandModel.
 **/
typedef enum /*< enum >*/
{
  XFCE_DIE_COMMAND_MODEL_COLUMN_NAME,
  XFCE_DIE_COMMAND_MODEL_N_COLUMNS,
} XfceDieCommandModelColumn;

XfceDieCommandModel *
xfce_die_command_model_new (void) G_GNUC_MALLOC;

G_END_DECLS

#endif /* !__XFCE_DIE_COMMAND_MODEL_H__ */
