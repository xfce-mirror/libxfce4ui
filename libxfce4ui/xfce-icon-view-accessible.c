/*-
 * Copyright (c) 2012 Nick Schermer <nick@xfce.org>
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

/* Accessibility Support */

static gpointer accessible_parent_class;
static gpointer accessible_item_parent_class;
static GQuark accessible_private_data_quark = 0;

#define XFCE_TYPE_ICON_VIEW_ITEM_ACCESSIBLE (xfce_icon_view_item_accessible_get_type ())
G_DECLARE_FINAL_TYPE (XfceIconViewItemAccessible, xfce_icon_view_item_accessible, XFCE, ICON_VIEW_ITEM_ACCESSIBLE, AtkObject)

enum
{
  ACTION_ACTIVATE,
  LAST_ACTION
};

struct _XfceIconViewItemAccessible
{
  AtkObject __parent__;

  XfceIconViewItem *item;
  GtkWidget *widget;
  AtkStateSet *state_set;
  gchar *text;
  GtkTextBuffer *text_buffer;

  gchar *action_descriptions[LAST_ACTION];
  gchar *image_description;
  guint action_idle_handler;
};

#define accessible_item_index(item) (g_sequence_iter_get_position (item->item->item_iter))

static const gchar *const xfce_icon_view_item_accessible_action_names[] = {
  "activate",
  NULL
};

static const gchar *const xfce_icon_view_item_accessible_action_descriptions[] = {
  "Activate item",
  NULL
};

static gboolean
xfce_icon_view_item_accessible_is_showing (XfceIconViewItemAccessible *item);

static gboolean
xfce_icon_view_item_accessible_idle_do_action (gpointer data)
{
  XfceIconViewItemAccessible *item;
  XfceIconView *icon_view;
  GtkTreePath *path;

  item = XFCE_ICON_VIEW_ITEM_ACCESSIBLE (data);
  item->action_idle_handler = 0;

  if (item->widget != NULL)
    {
      icon_view = XFCE_ICON_VIEW (item->widget);
      path = gtk_tree_path_new_from_indices (accessible_item_index (item), -1);
      xfce_icon_view_item_activated (icon_view, path);
      gtk_tree_path_free (path);
    }

  return FALSE;
}

static gboolean
xfce_icon_view_item_accessible_action_do_action (AtkAction *action,
                                                 gint i)
{
  XfceIconViewItemAccessible *item;

  if (i < 0 || i >= LAST_ACTION)
    return FALSE;

  item = XFCE_ICON_VIEW_ITEM_ACCESSIBLE (action);

  if (!XFCE_IS_ICON_VIEW (item->widget))
    return FALSE;

  if (atk_state_set_contains_state (item->state_set, ATK_STATE_DEFUNCT))
    return FALSE;

  switch (i)
    {
    case ACTION_ACTIVATE:
      if (!item->action_idle_handler)
        item->action_idle_handler = gdk_threads_add_idle (xfce_icon_view_item_accessible_idle_do_action, item);
      break;
    default:
      g_assert_not_reached ();
      return FALSE;
    }
  return TRUE;
}

static gint
xfce_icon_view_item_accessible_action_get_n_actions (AtkAction *action)
{
  return LAST_ACTION;
}

static const gchar *
xfce_icon_view_item_accessible_action_get_description (AtkAction *action,
                                                       gint i)
{
  XfceIconViewItemAccessible *item;

  if (i < 0 || i >= LAST_ACTION)
    return NULL;

  item = XFCE_ICON_VIEW_ITEM_ACCESSIBLE (action);

  if (item->action_descriptions[i])
    return item->action_descriptions[i];
  else
    return xfce_icon_view_item_accessible_action_descriptions[i];
}

static const gchar *
xfce_icon_view_item_accessible_action_get_name (AtkAction *action,
                                                gint i)
{
  if (i < 0 || i >= LAST_ACTION)
    return NULL;

  return xfce_icon_view_item_accessible_action_names[i];
}

static gboolean
xfce_icon_view_item_accessible_action_set_description (AtkAction *action,
                                                       gint i,
                                                       const gchar *description)
{
  XfceIconViewItemAccessible *item;

  if (i < 0 || i >= LAST_ACTION)
    return FALSE;

  item = XFCE_ICON_VIEW_ITEM_ACCESSIBLE (action);

  g_free (item->action_descriptions[i]);

  item->action_descriptions[i] = g_strdup (description);

  return TRUE;
}

static void
atk_action_item_interface_init (AtkActionIface *iface)
{
  iface->do_action = xfce_icon_view_item_accessible_action_do_action;
  iface->get_n_actions = xfce_icon_view_item_accessible_action_get_n_actions;
  iface->get_description = xfce_icon_view_item_accessible_action_get_description;
  iface->get_name = xfce_icon_view_item_accessible_action_get_name;
  iface->set_description = xfce_icon_view_item_accessible_action_set_description;
}

static const gchar *
xfce_icon_view_item_accessible_image_get_image_description (AtkImage *image)
{
  XfceIconViewItemAccessible *item;

  item = XFCE_ICON_VIEW_ITEM_ACCESSIBLE (image);

  return item->image_description;
}

static gboolean
xfce_icon_view_item_accessible_image_set_image_description (AtkImage *image,
                                                            const gchar *description)
{
  XfceIconViewItemAccessible *item;

  item = XFCE_ICON_VIEW_ITEM_ACCESSIBLE (image);

  g_free (item->image_description);
  item->image_description = g_strdup (description);

  return TRUE;
}

static gboolean
get_pixbuf_box (XfceIconView *icon_view,
                XfceIconViewItem *item,
                GdkRectangle *box)
{
  XfceIconViewPrivate *priv = get_instance_private (icon_view);
  GList *l;
  gint n;
  GParamSpec *pspec;

  for (l = priv->cell_list, n = 0; l; l = l->next, n++)
    {
      XfceIconViewCellInfo *info = l->data;

      pspec = g_object_class_find_property (G_OBJECT_GET_CLASS (info->cell), "pixmap");
      if (pspec != NULL)
        {
          *box = item->box[n];
          return TRUE;
        }
    }

  return FALSE;
}

static gchar *
get_text (XfceIconView *icon_view,
          XfceIconViewItem *item)
{
  XfceIconViewPrivate *priv = get_instance_private (icon_view);
  GList *l;
  gchar *text;
  GParamSpec *text_spec;

  for (l = priv->cell_list; l; l = l->next)
    {
      XfceIconViewCellInfo *info = l->data;

      text_spec = g_object_class_find_property (G_OBJECT_GET_CLASS (info->cell), "text");
      if (text_spec != NULL
          && G_PARAM_SPEC_STRING (text_spec))
        {
          g_object_get (info->cell, "text", &text, NULL);
          return text;
        }
    }

  return NULL;
}

static void
xfce_icon_view_item_accessible_image_get_image_size (AtkImage *image,
                                                     gint *width,
                                                     gint *height)
{
  XfceIconViewItemAccessible *item;
  GdkRectangle box;

  item = XFCE_ICON_VIEW_ITEM_ACCESSIBLE (image);

  if (!XFCE_IS_ICON_VIEW (item->widget))
    return;

  if (atk_state_set_contains_state (item->state_set, ATK_STATE_DEFUNCT))
    return;

  if (get_pixbuf_box (XFCE_ICON_VIEW (item->widget), item->item, &box))
    {
      *width = box.width;
      *height = box.height;
    }
}

static void
xfce_icon_view_item_accessible_image_get_image_position (AtkImage *image,
                                                         gint *x,
                                                         gint *y,
                                                         AtkCoordType coord_type)
{
  XfceIconViewItemAccessible *item;
  GdkRectangle box;

  item = XFCE_ICON_VIEW_ITEM_ACCESSIBLE (image);

  if (!XFCE_IS_ICON_VIEW (item->widget))
    return;

  if (atk_state_set_contains_state (item->state_set, ATK_STATE_DEFUNCT))
    return;

  atk_component_get_extents (ATK_COMPONENT (image), x, y, NULL, NULL, coord_type);

  if (get_pixbuf_box (XFCE_ICON_VIEW (item->widget), item->item, &box))
    {
      *x += box.x - item->item->area.x;
      *y += box.y - item->item->area.y;
    }
}

static void
atk_image_item_interface_init (AtkImageIface *iface)
{
  iface->get_image_description = xfce_icon_view_item_accessible_image_get_image_description;
  iface->set_image_description = xfce_icon_view_item_accessible_image_set_image_description;
  iface->get_image_size = xfce_icon_view_item_accessible_image_get_image_size;
  iface->get_image_position = xfce_icon_view_item_accessible_image_get_image_position;
}

static gchar *
xfce_icon_view_item_accessible_text_get_text (AtkText *text,
                                              gint start_pos,
                                              gint end_pos)
{
  XfceIconViewItemAccessible *item;
  GtkTextIter start, end;
  GtkTextBuffer *buffer;

  item = XFCE_ICON_VIEW_ITEM_ACCESSIBLE (text);

  if (!XFCE_IS_ICON_VIEW (item->widget))
    return NULL;

  if (atk_state_set_contains_state (item->state_set, ATK_STATE_DEFUNCT))
    return NULL;

  buffer = item->text_buffer;
  gtk_text_buffer_get_iter_at_offset (buffer, &start, start_pos);
  if (end_pos < 0)
    gtk_text_buffer_get_end_iter (buffer, &end);
  else
    gtk_text_buffer_get_iter_at_offset (buffer, &end, end_pos);

  return gtk_text_buffer_get_text (buffer, &start, &end, FALSE);
}

static gunichar
xfce_icon_view_item_accessible_text_get_character_at_offset (AtkText *text,
                                                             gint offset)
{
  XfceIconViewItemAccessible *item;
  GtkTextIter start, end;
  GtkTextBuffer *buffer;
  gchar *string;
  gunichar unichar;

  item = XFCE_ICON_VIEW_ITEM_ACCESSIBLE (text);

  if (!XFCE_IS_ICON_VIEW (item->widget))
    return '\0';

  if (atk_state_set_contains_state (item->state_set, ATK_STATE_DEFUNCT))
    return '\0';

  buffer = item->text_buffer;
  if (offset >= gtk_text_buffer_get_char_count (buffer))
    return '\0';

  gtk_text_buffer_get_iter_at_offset (buffer, &start, offset);
  end = start;
  gtk_text_iter_forward_char (&end);
  string = gtk_text_buffer_get_slice (buffer, &start, &end, FALSE);
  unichar = g_utf8_get_char (string);
  g_free (string);

  return unichar;
}

static gchar *
xfce_icon_view_item_accessible_text_get_text_before_offset (AtkText *text,
                                                            gint offset,
                                                            AtkTextBoundary boundary_type,
                                                            gint *start_offset,
                                                            gint *end_offset)
{
  XfceIconViewItemAccessible *item;
  GtkTextIter start, end;
  GtkTextBuffer *buffer;

  item = XFCE_ICON_VIEW_ITEM_ACCESSIBLE (text);

  if (!XFCE_IS_ICON_VIEW (item->widget))
    return NULL;

  if (atk_state_set_contains_state (item->state_set, ATK_STATE_DEFUNCT))
    return NULL;

  buffer = item->text_buffer;

  if (!gtk_text_buffer_get_char_count (buffer))
    {
      *start_offset = 0;
      *end_offset = 0;
      return g_strdup ("");
    }
  gtk_text_buffer_get_iter_at_offset (buffer, &start, offset);

  end = start;

  switch (boundary_type)
    {
    case ATK_TEXT_BOUNDARY_CHAR:
      gtk_text_iter_backward_char (&start);
      break;
    case ATK_TEXT_BOUNDARY_WORD_START:
      if (!gtk_text_iter_starts_word (&start))
        gtk_text_iter_backward_word_start (&start);
      end = start;
      gtk_text_iter_backward_word_start (&start);
      break;
    case ATK_TEXT_BOUNDARY_WORD_END:
      if (gtk_text_iter_inside_word (&start) && !gtk_text_iter_starts_word (&start))
        gtk_text_iter_backward_word_start (&start);
      while (!gtk_text_iter_ends_word (&start))
        {
          if (!gtk_text_iter_backward_char (&start))
            break;
        }
      end = start;
      gtk_text_iter_backward_word_start (&start);
      while (!gtk_text_iter_ends_word (&start))
        {
          if (!gtk_text_iter_backward_char (&start))
            break;
        }
      break;
    case ATK_TEXT_BOUNDARY_SENTENCE_START:
      if (!gtk_text_iter_starts_sentence (&start))
        gtk_text_iter_backward_sentence_start (&start);
      end = start;
      gtk_text_iter_backward_sentence_start (&start);
      break;
    case ATK_TEXT_BOUNDARY_SENTENCE_END:
      if (gtk_text_iter_inside_sentence (&start) && !gtk_text_iter_starts_sentence (&start))
        gtk_text_iter_backward_sentence_start (&start);
      while (!gtk_text_iter_ends_sentence (&start))
        {
          if (!gtk_text_iter_backward_char (&start))
            break;
        }
      end = start;
      gtk_text_iter_backward_sentence_start (&start);
      while (!gtk_text_iter_ends_sentence (&start))
        {
          if (!gtk_text_iter_backward_char (&start))
            break;
        }
      break;
    case ATK_TEXT_BOUNDARY_LINE_START:
    case ATK_TEXT_BOUNDARY_LINE_END:
      break;
    }

  *start_offset = gtk_text_iter_get_offset (&start);
  *end_offset = gtk_text_iter_get_offset (&end);

  return gtk_text_buffer_get_text (buffer, &start, &end, FALSE);
}

static gchar *
xfce_icon_view_item_accessible_text_get_text_at_offset (AtkText *text,
                                                        gint offset,
                                                        AtkTextBoundary boundary_type,
                                                        gint *start_offset,
                                                        gint *end_offset)
{
  XfceIconViewItemAccessible *item;
  GtkTextIter start, end;
  GtkTextBuffer *buffer;

  item = XFCE_ICON_VIEW_ITEM_ACCESSIBLE (text);

  if (!XFCE_IS_ICON_VIEW (item->widget))
    return NULL;

  if (atk_state_set_contains_state (item->state_set, ATK_STATE_DEFUNCT))
    return NULL;

  buffer = item->text_buffer;

  if (!gtk_text_buffer_get_char_count (buffer))
    {
      *start_offset = 0;
      *end_offset = 0;
      return g_strdup ("");
    }
  gtk_text_buffer_get_iter_at_offset (buffer, &start, offset);

  end = start;

  switch (boundary_type)
    {
    case ATK_TEXT_BOUNDARY_CHAR:
      gtk_text_iter_forward_char (&end);
      break;
    case ATK_TEXT_BOUNDARY_WORD_START:
      if (!gtk_text_iter_starts_word (&start))
        gtk_text_iter_backward_word_start (&start);
      if (gtk_text_iter_inside_word (&end))
        gtk_text_iter_forward_word_end (&end);
      while (!gtk_text_iter_starts_word (&end))
        {
          if (!gtk_text_iter_forward_char (&end))
            break;
        }
      break;
    case ATK_TEXT_BOUNDARY_WORD_END:
      if (gtk_text_iter_inside_word (&start) && !gtk_text_iter_starts_word (&start))
        gtk_text_iter_backward_word_start (&start);
      while (!gtk_text_iter_ends_word (&start))
        {
          if (!gtk_text_iter_backward_char (&start))
            break;
        }
      gtk_text_iter_forward_word_end (&end);
      break;
    case ATK_TEXT_BOUNDARY_SENTENCE_START:
      if (!gtk_text_iter_starts_sentence (&start))
        gtk_text_iter_backward_sentence_start (&start);
      if (gtk_text_iter_inside_sentence (&end))
        gtk_text_iter_forward_sentence_end (&end);
      while (!gtk_text_iter_starts_sentence (&end))
        {
          if (!gtk_text_iter_forward_char (&end))
            break;
        }
      break;
    case ATK_TEXT_BOUNDARY_SENTENCE_END:
      if (gtk_text_iter_inside_sentence (&start) && !gtk_text_iter_starts_sentence (&start))
        gtk_text_iter_backward_sentence_start (&start);
      while (!gtk_text_iter_ends_sentence (&start))
        {
          if (!gtk_text_iter_backward_char (&start))
            break;
        }
      gtk_text_iter_forward_sentence_end (&end);
      break;
    case ATK_TEXT_BOUNDARY_LINE_START:
    case ATK_TEXT_BOUNDARY_LINE_END:
      break;
    }


  *start_offset = gtk_text_iter_get_offset (&start);
  *end_offset = gtk_text_iter_get_offset (&end);

  return gtk_text_buffer_get_text (buffer, &start, &end, FALSE);
}

static gchar *
xfce_icon_view_item_accessible_text_get_text_after_offset (AtkText *text,
                                                           gint offset,
                                                           AtkTextBoundary boundary_type,
                                                           gint *start_offset,
                                                           gint *end_offset)
{
  XfceIconViewItemAccessible *item;
  GtkTextIter start, end;
  GtkTextBuffer *buffer;

  item = XFCE_ICON_VIEW_ITEM_ACCESSIBLE (text);

  if (!XFCE_IS_ICON_VIEW (item->widget))
    return NULL;

  if (atk_state_set_contains_state (item->state_set, ATK_STATE_DEFUNCT))
    return NULL;

  buffer = item->text_buffer;

  if (!gtk_text_buffer_get_char_count (buffer))
    {
      *start_offset = 0;
      *end_offset = 0;
      return g_strdup ("");
    }
  gtk_text_buffer_get_iter_at_offset (buffer, &start, offset);

  end = start;

  switch (boundary_type)
    {
    case ATK_TEXT_BOUNDARY_CHAR:
      gtk_text_iter_forward_char (&start);
      gtk_text_iter_forward_chars (&end, 2);
      break;
    case ATK_TEXT_BOUNDARY_WORD_START:
      if (gtk_text_iter_inside_word (&end))
        gtk_text_iter_forward_word_end (&end);
      while (!gtk_text_iter_starts_word (&end))
        {
          if (!gtk_text_iter_forward_char (&end))
            break;
        }
      start = end;
      if (!gtk_text_iter_is_end (&end))
        {
          gtk_text_iter_forward_word_end (&end);
          while (!gtk_text_iter_starts_word (&end))
            {
              if (!gtk_text_iter_forward_char (&end))
                break;
            }
        }
      break;
    case ATK_TEXT_BOUNDARY_WORD_END:
      gtk_text_iter_forward_word_end (&end);
      start = end;
      if (!gtk_text_iter_is_end (&end))
        gtk_text_iter_forward_word_end (&end);
      break;
    case ATK_TEXT_BOUNDARY_SENTENCE_START:
      if (gtk_text_iter_inside_sentence (&end))
        gtk_text_iter_forward_sentence_end (&end);
      while (!gtk_text_iter_starts_sentence (&end))
        {
          if (!gtk_text_iter_forward_char (&end))
            break;
        }
      start = end;
      if (!gtk_text_iter_is_end (&end))
        {
          gtk_text_iter_forward_sentence_end (&end);
          while (!gtk_text_iter_starts_sentence (&end))
            {
              if (!gtk_text_iter_forward_char (&end))
                break;
            }
        }
      break;
    case ATK_TEXT_BOUNDARY_SENTENCE_END:
      gtk_text_iter_forward_sentence_end (&end);
      start = end;
      if (!gtk_text_iter_is_end (&end))
        gtk_text_iter_forward_sentence_end (&end);
      break;
    case ATK_TEXT_BOUNDARY_LINE_START:
    case ATK_TEXT_BOUNDARY_LINE_END:
      break;
    }
  *start_offset = gtk_text_iter_get_offset (&start);
  *end_offset = gtk_text_iter_get_offset (&end);

  return gtk_text_buffer_get_text (buffer, &start, &end, FALSE);
}

static gint
xfce_icon_view_item_accessible_text_get_character_count (AtkText *text)
{
  XfceIconViewItemAccessible *item;

  item = XFCE_ICON_VIEW_ITEM_ACCESSIBLE (text);

  if (!XFCE_IS_ICON_VIEW (item->widget))
    return 0;

  if (atk_state_set_contains_state (item->state_set, ATK_STATE_DEFUNCT))
    return 0;

  return gtk_text_buffer_get_char_count (item->text_buffer);
}

static void
xfce_icon_view_item_accessible_text_get_character_extents (AtkText *text,
                                                           gint offset,
                                                           gint *x,
                                                           gint *y,
                                                           gint *width,
                                                           gint *height,
                                                           AtkCoordType coord_type)
{
  XfceIconViewItemAccessible *item;

  item = XFCE_ICON_VIEW_ITEM_ACCESSIBLE (text);

  if (!XFCE_IS_ICON_VIEW (item->widget))
    return;

  if (atk_state_set_contains_state (item->state_set, ATK_STATE_DEFUNCT))
    return;
}

static gint
xfce_icon_view_item_accessible_text_get_offset_at_point (AtkText *text,
                                                         gint x,
                                                         gint y,
                                                         AtkCoordType coord_type)
{
  XfceIconViewItemAccessible *item;
  gint offset = 0;

  item = XFCE_ICON_VIEW_ITEM_ACCESSIBLE (text);

  if (!XFCE_IS_ICON_VIEW (item->widget))
    return -1;

  if (atk_state_set_contains_state (item->state_set, ATK_STATE_DEFUNCT))
    return -1;

  return offset;
}

static void
atk_text_item_interface_init (AtkTextIface *iface)
{
  iface->get_text = xfce_icon_view_item_accessible_text_get_text;
  iface->get_character_at_offset = xfce_icon_view_item_accessible_text_get_character_at_offset;
  iface->get_text_before_offset = xfce_icon_view_item_accessible_text_get_text_before_offset;
  iface->get_text_at_offset = xfce_icon_view_item_accessible_text_get_text_at_offset;
  iface->get_text_after_offset = xfce_icon_view_item_accessible_text_get_text_after_offset;
  iface->get_character_count = xfce_icon_view_item_accessible_text_get_character_count;
  iface->get_character_extents = xfce_icon_view_item_accessible_text_get_character_extents;
  iface->get_offset_at_point = xfce_icon_view_item_accessible_text_get_offset_at_point;
}

static void
xfce_icon_view_item_accessible_get_extents (AtkComponent *component,
                                            gint *x,
                                            gint *y,
                                            gint *width,
                                            gint *height,
                                            AtkCoordType coord_type)
{
  XfceIconViewItemAccessible *item;
  AtkObject *parent_obj;
  gint l_x, l_y;

  g_return_if_fail (XFCE_IS_ICON_VIEW_ITEM_ACCESSIBLE (component));

  item = XFCE_ICON_VIEW_ITEM_ACCESSIBLE (component);
  if (!GTK_IS_WIDGET (item->widget))
    return;

  if (atk_state_set_contains_state (item->state_set, ATK_STATE_DEFUNCT))
    return;

  *width = item->item->area.width;
  *height = item->item->area.height;
  if (xfce_icon_view_item_accessible_is_showing (item))
    {
      parent_obj = gtk_widget_get_accessible (item->widget);
      atk_component_get_extents (ATK_COMPONENT (parent_obj), &l_x, &l_y, NULL, NULL, coord_type);
      *x = l_x + item->item->area.x;
      *y = l_y + item->item->area.y;
    }
  else
    {
      *x = G_MININT;
      *y = G_MININT;
    }
}

static gboolean
xfce_icon_view_item_accessible_grab_focus (AtkComponent *component)
{
  XfceIconViewItemAccessible *item;
  GtkWidget *toplevel;

  g_return_val_if_fail (XFCE_IS_ICON_VIEW_ITEM_ACCESSIBLE (component), FALSE);

  item = XFCE_ICON_VIEW_ITEM_ACCESSIBLE (component);
  if (!GTK_IS_WIDGET (item->widget))
    return FALSE;

  gtk_widget_grab_focus (item->widget);
  xfce_icon_view_set_cursor_item (XFCE_ICON_VIEW (item->widget), item->item, -1);
  toplevel = gtk_widget_get_toplevel (GTK_WIDGET (item->widget));
  if (gtk_widget_is_toplevel (toplevel))
    gtk_window_present (GTK_WINDOW (toplevel));

  return TRUE;
}

static void
atk_component_item_interface_init (AtkComponentIface *iface)
{
  iface->get_extents = xfce_icon_view_item_accessible_get_extents;
  iface->grab_focus = xfce_icon_view_item_accessible_grab_focus;
}

static gboolean
xfce_icon_view_item_accessible_add_state (XfceIconViewItemAccessible *item,
                                          AtkStateType state_type,
                                          gboolean emit_signal)
{
  gboolean rc;

  rc = atk_state_set_add_state (item->state_set, state_type);
  /*
   * The signal should only be generated if the value changed,
   * not when the item is set up.  So states that are set
   * initially should pass FALSE as the emit_signal argument.
   */

  if (emit_signal)
    {
      atk_object_notify_state_change (ATK_OBJECT (item), state_type, TRUE);
      /* If state_type is ATK_STATE_VISIBLE, additional notification */
      if (state_type == ATK_STATE_VISIBLE)
        g_signal_emit_by_name (item, "visible-data-changed");
    }

  return rc;
}

static gboolean
xfce_icon_view_item_accessible_remove_state (XfceIconViewItemAccessible *item,
                                             AtkStateType state_type,
                                             gboolean emit_signal)
{
  if (atk_state_set_contains_state (item->state_set, state_type))
    {
      gboolean rc;

      rc = atk_state_set_remove_state (item->state_set, state_type);
      /*
       * The signal should only be generated if the value changed,
       * not when the item is set up.  So states that are set
       * initially should pass FALSE as the emit_signal argument.
       */

      if (emit_signal)
        {
          atk_object_notify_state_change (ATK_OBJECT (item), state_type, FALSE);
          /* If state_type is ATK_STATE_VISIBLE, additional notification */
          if (state_type == ATK_STATE_VISIBLE)
            g_signal_emit_by_name (item, "visible-data-changed");
        }

      return rc;
    }
  else
    return FALSE;
}

static gboolean
xfce_icon_view_item_accessible_is_showing (XfceIconViewItemAccessible *item)
{
  XfceIconViewPrivate *priv;
  XfceIconView *icon_view;
  GdkRectangle visible_rect;
  gboolean is_showing;
  GtkAllocation allocation;

  /*
   * An item is considered "SHOWING" if any part of the item is in the
   * visible rectangle.
   */

  if (!XFCE_IS_ICON_VIEW (item->widget))
    return FALSE;

  if (item->item == NULL)
    return FALSE;

  icon_view = XFCE_ICON_VIEW (item->widget);
  priv = get_instance_private (icon_view);
  visible_rect.x = 0;
  if (priv->hadjustment)
    visible_rect.x += gtk_adjustment_get_value (priv->hadjustment);
  visible_rect.y = 0;
  if (priv->vadjustment)
    visible_rect.y += gtk_adjustment_get_value (priv->vadjustment);

  gtk_widget_get_allocation (item->widget, &allocation);
  visible_rect.width = allocation.width;
  visible_rect.height = allocation.height;

  if (item->item->area.x + item->item->area.width < visible_rect.x
      || item->item->area.y + item->item->area.height < visible_rect.y
      || item->item->area.x > visible_rect.x + visible_rect.width
      || item->item->area.y > visible_rect.y + visible_rect.height)
    is_showing = FALSE;
  else
    is_showing = TRUE;

  return is_showing;
}

static gboolean
xfce_icon_view_item_accessible_set_visibility (XfceIconViewItemAccessible *item,
                                               gboolean emit_signal)
{
  if (xfce_icon_view_item_accessible_is_showing (item))
    return xfce_icon_view_item_accessible_add_state (item, ATK_STATE_SHOWING,
                                                     emit_signal);
  else
    return xfce_icon_view_item_accessible_remove_state (item, ATK_STATE_SHOWING,
                                                        emit_signal);
}

static void
xfce_icon_view_item_accessible_object_init (XfceIconViewItemAccessible *item)
{
  gint i;

  item->state_set = atk_state_set_new ();

  atk_state_set_add_state (item->state_set, ATK_STATE_ENABLED);
  atk_state_set_add_state (item->state_set, ATK_STATE_FOCUSABLE);
  atk_state_set_add_state (item->state_set, ATK_STATE_SENSITIVE);
  atk_state_set_add_state (item->state_set, ATK_STATE_SELECTABLE);
  atk_state_set_add_state (item->state_set, ATK_STATE_VISIBLE);

  for (i = 0; i < LAST_ACTION; i++)
    item->action_descriptions[i] = NULL;

  item->image_description = NULL;

  item->action_idle_handler = 0;
}

static void
xfce_icon_view_item_accessible_finalize (GObject *object)
{
  XfceIconViewItemAccessible *item;
  gint i;

  g_return_if_fail (XFCE_IS_ICON_VIEW_ITEM_ACCESSIBLE (object));

  item = XFCE_ICON_VIEW_ITEM_ACCESSIBLE (object);

  if (item->widget)
    g_object_remove_weak_pointer (G_OBJECT (item->widget), (gpointer) &item->widget);

  if (item->state_set)
    g_object_unref (item->state_set);

  if (item->text_buffer)
    g_object_unref (item->text_buffer);

  for (i = 0; i < LAST_ACTION; i++)
    g_free (item->action_descriptions[i]);

  g_free (item->image_description);

  if (item->action_idle_handler)
    {
      g_source_remove (item->action_idle_handler);
      item->action_idle_handler = 0;
    }

  G_OBJECT_CLASS (accessible_item_parent_class)->finalize (object);
}

static AtkObject *
xfce_icon_view_item_accessible_get_parent (AtkObject *obj)
{
  XfceIconViewItemAccessible *item;

  g_return_val_if_fail (XFCE_IS_ICON_VIEW_ITEM_ACCESSIBLE (obj), NULL);
  item = XFCE_ICON_VIEW_ITEM_ACCESSIBLE (obj);

  if (item->widget)
    return gtk_widget_get_accessible (item->widget);
  else
    return NULL;
}

static gint
xfce_icon_view_item_accessible_get_index_in_parent (AtkObject *obj)
{
  XfceIconViewItemAccessible *item;

  g_return_val_if_fail (XFCE_IS_ICON_VIEW_ITEM_ACCESSIBLE (obj), 0);
  item = XFCE_ICON_VIEW_ITEM_ACCESSIBLE (obj);

  return accessible_item_index (item);
}

static AtkStateSet *
xfce_icon_view_item_accessible_ref_state_set (AtkObject *obj)
{
  XfceIconViewPrivate *priv;
  XfceIconViewItemAccessible *item;
  XfceIconView *icon_view;

  item = XFCE_ICON_VIEW_ITEM_ACCESSIBLE (obj);
  g_return_val_if_fail (item->state_set, NULL);

  if (!item->widget)
    return NULL;

  icon_view = XFCE_ICON_VIEW (item->widget);
  priv = get_instance_private (icon_view);
  if (priv->cursor_item == item->item)
    atk_state_set_add_state (item->state_set, ATK_STATE_FOCUSED);
  else
    atk_state_set_remove_state (item->state_set, ATK_STATE_FOCUSED);
  if (item->item->selected)
    atk_state_set_add_state (item->state_set, ATK_STATE_SELECTED);
  else
    atk_state_set_remove_state (item->state_set, ATK_STATE_SELECTED);

  return g_object_ref (item->state_set);
}

static void
xfce_icon_view_item_accessible_class_init (AtkObjectClass *klass)
{
  GObjectClass *gobject_class;

  accessible_item_parent_class = g_type_class_peek_parent (klass);

  gobject_class = (GObjectClass *) klass;

  gobject_class->finalize = xfce_icon_view_item_accessible_finalize;

  klass->get_index_in_parent = xfce_icon_view_item_accessible_get_index_in_parent;
  klass->get_parent = xfce_icon_view_item_accessible_get_parent;
  klass->ref_state_set = xfce_icon_view_item_accessible_ref_state_set;
}

GType
xfce_icon_view_item_accessible_get_type (void)
{
  static GType type = 0;

  if (!type)
    {
      const GTypeInfo tinfo = {
        sizeof (XfceIconViewItemAccessibleClass),
        (GBaseInitFunc) NULL, /* base init */
        (GBaseFinalizeFunc) NULL, /* base finalize */
        (GClassInitFunc) (void (*) (void)) xfce_icon_view_item_accessible_class_init, /* class init */
        (GClassFinalizeFunc) NULL, /* class finalize */
        NULL, /* class data */
        sizeof (XfceIconViewItemAccessible), /* instance size */
        0, /* nb preallocs */
        (GInstanceInitFunc) (void (*) (void)) xfce_icon_view_item_accessible_object_init, /* instance init */
        NULL /* value table */
      };

      const GInterfaceInfo atk_component_info = {
        (GInterfaceInitFunc) (void (*) (void)) atk_component_item_interface_init,
        (GInterfaceFinalizeFunc) NULL,
        NULL
      };
      const GInterfaceInfo atk_action_info = {
        (GInterfaceInitFunc) (void (*) (void)) atk_action_item_interface_init,
        (GInterfaceFinalizeFunc) NULL,
        NULL
      };
      const GInterfaceInfo atk_image_info = {
        (GInterfaceInitFunc) (void (*) (void)) atk_image_item_interface_init,
        (GInterfaceFinalizeFunc) NULL,
        NULL
      };
      const GInterfaceInfo atk_text_info = {
        (GInterfaceInitFunc) (void (*) (void)) atk_text_item_interface_init,
        (GInterfaceFinalizeFunc) NULL,
        NULL
      };

      type = g_type_register_static (ATK_TYPE_OBJECT,
                                     I_ ("XfceIconViewItemAccessible"), &tinfo, 0);
      g_type_add_interface_static (type, ATK_TYPE_COMPONENT,
                                   &atk_component_info);
      g_type_add_interface_static (type, ATK_TYPE_ACTION,
                                   &atk_action_info);
      g_type_add_interface_static (type, ATK_TYPE_IMAGE,
                                   &atk_image_info);
      g_type_add_interface_static (type, ATK_TYPE_TEXT,
                                   &atk_text_info);
    }

  return type;
}

#define XFCE_TYPE_ICON_VIEW_ACCESSIBLE (xfce_icon_view_accessible_get_type ())
G_DECLARE_FINAL_TYPE (XfceIconViewAccessible, xfce_icon_view_accessible, XFCE, ICON_VIEW_ACCESSIBLE, AtkObject)

struct _XfceIconViewAccessible
{
  AtkObject __parent__;
};

typedef struct
{
  AtkObject *item;
  gint index;
} XfceIconViewItemAccessibleInfo;

typedef struct
{
  GList *items;

  GtkAdjustment *old_hadj;
  GtkAdjustment *old_vadj;

  GtkTreeModel *model;
} XfceIconViewAccessiblePrivate;

static XfceIconViewAccessiblePrivate *
xfce_icon_view_accessible_get_priv (AtkObject *accessible)
{
  return g_object_get_qdata (G_OBJECT (accessible),
                             accessible_private_data_quark);
}

static void
xfce_icon_view_item_accessible_info_new (AtkObject *accessible,
                                         AtkObject *item,
                                         gint idx)
{
  XfceIconViewItemAccessibleInfo *info;
  XfceIconViewItemAccessibleInfo *tmp_info;
  XfceIconViewAccessiblePrivate *priv;
  GList *items;

  info = g_new (XfceIconViewItemAccessibleInfo, 1);
  info->item = item;
  info->index = idx;

  priv = xfce_icon_view_accessible_get_priv (accessible);
  items = priv->items;
  while (items)
    {
      tmp_info = items->data;
      if (tmp_info->index > idx)
        break;
      items = items->next;
    }
  priv->items = g_list_insert_before (priv->items, items, info);
  priv->old_hadj = NULL;
  priv->old_vadj = NULL;
}

static gint
xfce_icon_view_accessible_get_n_children (AtkObject *accessible)
{
  GtkWidget *widget;

  widget = gtk_accessible_get_widget (GTK_ACCESSIBLE (accessible));
  if (!widget)
    return 0;

  return g_sequence_get_length (get_instance_private (widget)->items);
}

static AtkObject *
xfce_icon_view_accessible_find_child (AtkObject *accessible,
                                      gint idx)
{
  XfceIconViewAccessiblePrivate *priv;
  XfceIconViewItemAccessibleInfo *info;
  GList *items;

  priv = xfce_icon_view_accessible_get_priv (accessible);
  items = priv->items;

  while (items)
    {
      info = items->data;
      if (info->index == idx)
        return info->item;
      items = items->next;
    }
  return NULL;
}

static AtkObject *
xfce_icon_view_accessible_ref_child (AtkObject *accessible,
                                     gint idx)
{
  XfceIconViewPrivate *priv;
  XfceIconView *icon_view;
  GtkWidget *widget;
  GSequenceIter *iter;
  AtkObject *obj;
  XfceIconViewItemAccessible *a11y_item;

  widget = gtk_accessible_get_widget (GTK_ACCESSIBLE (accessible));
  if (!widget)
    return NULL;

  icon_view = XFCE_ICON_VIEW (widget);
  priv = get_instance_private (icon_view);
  iter = g_sequence_get_iter_at_pos (priv->items, idx);
  obj = NULL;
  if (iter)
    {
      XfceIconViewItem *item = g_sequence_get (iter);
      gint item_index = g_sequence_iter_get_position (item->item_iter);

      g_return_val_if_fail (item_index == idx, NULL);
      obj = xfce_icon_view_accessible_find_child (accessible, idx);
      if (!obj)
        {
          gchar *text;

          obj = g_object_new (xfce_icon_view_item_accessible_get_type (), NULL);
          xfce_icon_view_item_accessible_info_new (accessible,
                                                   obj,
                                                   idx);
          obj->role = ATK_ROLE_ICON;
          a11y_item = XFCE_ICON_VIEW_ITEM_ACCESSIBLE (obj);
          a11y_item->item = item;
          a11y_item->widget = widget;
          a11y_item->text_buffer = gtk_text_buffer_new (NULL);

          xfce_icon_view_set_cell_data (icon_view, item);
          text = get_text (icon_view, item);
          if (text)
            {
              gtk_text_buffer_set_text (a11y_item->text_buffer, text, -1);
              g_free (text);
            }

          xfce_icon_view_item_accessible_set_visibility (a11y_item, FALSE);
          g_object_add_weak_pointer (G_OBJECT (widget), (gpointer) & (a11y_item->widget));
        }
      g_object_ref (obj);
    }
  return obj;
}

static void
xfce_icon_view_accessible_traverse_items (XfceIconViewAccessible *view,
                                          GList *list)
{
  XfceIconViewAccessiblePrivate *priv;
  XfceIconViewItemAccessibleInfo *info;
  XfceIconViewItemAccessible *item;
  GList *items;

  priv = xfce_icon_view_accessible_get_priv (ATK_OBJECT (view));
  if (priv->items)
    {
      GtkWidget *widget;
      gboolean act_on_item;

      widget = gtk_accessible_get_widget (GTK_ACCESSIBLE (view));
      if (widget == NULL)
        return;

      items = priv->items;

      act_on_item = (list == NULL);

      while (items)
        {
          info = (XfceIconViewItemAccessibleInfo *) items->data;
          item = XFCE_ICON_VIEW_ITEM_ACCESSIBLE (info->item);

          if (act_on_item == FALSE && list == items)
            act_on_item = TRUE;

          if (act_on_item)
            xfce_icon_view_item_accessible_set_visibility (item, TRUE);

          items = items->next;
        }
    }
}

static void
xfce_icon_view_accessible_adjustment_changed (GtkAdjustment *adjustment,
                                              XfceIconView *icon_view)
{
  AtkObject *obj;
  XfceIconViewAccessible *view;

  /*
   * The scrollbars have changed
   */
  obj = gtk_widget_get_accessible (GTK_WIDGET (icon_view));
  view = XFCE_ICON_VIEW_ACCESSIBLE (obj);

  xfce_icon_view_accessible_traverse_items (view, NULL);
}

static void
xfce_icon_view_accessible_set_scroll_adjustments (GtkWidget *widget,
                                                  GtkAdjustment *hadj,
                                                  GtkAdjustment *vadj)
{
  AtkObject *atk_obj;
  XfceIconViewAccessiblePrivate *priv;

  atk_obj = gtk_widget_get_accessible (widget);
  priv = xfce_icon_view_accessible_get_priv (atk_obj);

  if (priv->old_hadj != hadj)
    {
      if (priv->old_hadj)
        {
          g_object_remove_weak_pointer (G_OBJECT (priv->old_hadj),
                                        (gpointer *) &priv->old_hadj);

          g_signal_handlers_disconnect_by_func (priv->old_hadj,
                                                (gpointer) xfce_icon_view_accessible_adjustment_changed,
                                                widget);
        }
      priv->old_hadj = hadj;
      if (priv->old_hadj)
        {
          g_object_add_weak_pointer (G_OBJECT (priv->old_hadj),
                                     (gpointer *) &priv->old_hadj);
          g_signal_connect (hadj,
                            "value-changed",
                            G_CALLBACK (xfce_icon_view_accessible_adjustment_changed),
                            widget);
        }
    }
  if (priv->old_vadj != vadj)
    {
      if (priv->old_vadj)
        {
          g_object_remove_weak_pointer (G_OBJECT (priv->old_vadj),
                                        (gpointer *) &priv->old_vadj);

          g_signal_handlers_disconnect_by_func (priv->old_vadj,
                                                (gpointer) xfce_icon_view_accessible_adjustment_changed,
                                                widget);
        }
      priv->old_vadj = vadj;
      if (priv->old_vadj)
        {
          g_object_add_weak_pointer (G_OBJECT (priv->old_vadj),
                                     (gpointer *) &priv->old_vadj);
          g_signal_connect (vadj,
                            "value-changed",
                            G_CALLBACK (xfce_icon_view_accessible_adjustment_changed),
                            widget);
        }
    }
}

static void
xfce_icon_view_accessible_model_row_changed (GtkTreeModel *tree_model,
                                             GtkTreePath *path,
                                             GtkTreeIter *iter,
                                             gpointer user_data)
{
  AtkObject *atk_obj;
  gint idx;
  GtkWidget *widget;
  XfceIconView *icon_view;
  XfceIconViewItem *item;
  XfceIconViewItemAccessible *a11y_item;
  const gchar *name;
  gchar *text;

  atk_obj = gtk_widget_get_accessible (GTK_WIDGET (user_data));
  idx = gtk_tree_path_get_indices (path)[0];
  a11y_item = XFCE_ICON_VIEW_ITEM_ACCESSIBLE (
    xfce_icon_view_accessible_find_child (atk_obj, idx));

  if (a11y_item)
    {
      widget = gtk_accessible_get_widget (GTK_ACCESSIBLE (atk_obj));
      icon_view = XFCE_ICON_VIEW (widget);
      item = a11y_item->item;

      name = atk_object_get_name (ATK_OBJECT (a11y_item));

      if (!name || strcmp (name, "") == 0)
        {
          xfce_icon_view_set_cell_data (icon_view, item);
          text = get_text (icon_view, item);
          if (text)
            {
              gtk_text_buffer_set_text (a11y_item->text_buffer, text, -1);
              g_free (text);
            }
        }
    }

  g_signal_emit_by_name (atk_obj, "visible-data-changed");

  return;
}

static void
xfce_icon_view_accessible_model_row_inserted (GtkTreeModel *tree_model,
                                              GtkTreePath *path,
                                              GtkTreeIter *iter,
                                              gpointer user_data)
{
  XfceIconViewAccessiblePrivate *priv;
  XfceIconViewItemAccessibleInfo *info;
  XfceIconViewAccessible *view;
  XfceIconViewItemAccessible *item;
  GList *items;
  GList *tmp_list;
  AtkObject *atk_obj;
  gint idx;

  idx = gtk_tree_path_get_indices (path)[0];
  atk_obj = gtk_widget_get_accessible (GTK_WIDGET (user_data));
  view = XFCE_ICON_VIEW_ACCESSIBLE (atk_obj);
  priv = xfce_icon_view_accessible_get_priv (atk_obj);

  items = priv->items;
  tmp_list = NULL;
  while (items)
    {
      info = items->data;
      item = XFCE_ICON_VIEW_ITEM_ACCESSIBLE (info->item);
      if (info->index != accessible_item_index (item))
        {
          if (info->index < idx)
            g_warning ("Unexpected index value on insertion %d %d", idx, info->index);

          if (tmp_list == NULL)
            tmp_list = items;

          info->index = accessible_item_index (item);
        }

      items = items->next;
    }
  xfce_icon_view_accessible_traverse_items (view, tmp_list);
  g_signal_emit_by_name (atk_obj, "children-changed::add",
                         idx, NULL, NULL);
  return;
}

static void
xfce_icon_view_accessible_model_row_deleted (GtkTreeModel *tree_model,
                                             GtkTreePath *path,
                                             gpointer user_data)
{
  XfceIconViewAccessiblePrivate *priv;
  XfceIconViewItemAccessibleInfo *info;
  XfceIconViewAccessible *view;
  XfceIconViewItemAccessible *item;
  GList *items;
  GList *tmp_list;
  GList *deleted_item;
  AtkObject *atk_obj;
  gint idx;

  idx = gtk_tree_path_get_indices (path)[0];
  atk_obj = gtk_widget_get_accessible (GTK_WIDGET (user_data));
  view = XFCE_ICON_VIEW_ACCESSIBLE (atk_obj);
  priv = xfce_icon_view_accessible_get_priv (atk_obj);

  items = priv->items;
  tmp_list = NULL;
  deleted_item = NULL;
  info = NULL;
  while (items)
    {
      info = items->data;
      item = XFCE_ICON_VIEW_ITEM_ACCESSIBLE (info->item);
      if (info->index == idx)
        {
          deleted_item = items;
        }
      if (info->index != accessible_item_index (item))
        {
          if (tmp_list == NULL)
            tmp_list = items;

          info->index = accessible_item_index (item);
        }

      items = items->next;
    }
  xfce_icon_view_accessible_traverse_items (view, tmp_list);
  if (deleted_item)
    {
      info = deleted_item->data;
      xfce_icon_view_item_accessible_add_state (XFCE_ICON_VIEW_ITEM_ACCESSIBLE (info->item), ATK_STATE_DEFUNCT, TRUE);
      g_signal_emit_by_name (atk_obj, "children-changed::remove",
                             idx, NULL, NULL);
      priv->items = g_list_remove_link (priv->items, deleted_item);
      g_free (info);
    }

  return;
}

static gint
xfce_icon_view_accessible_item_compare (XfceIconViewItemAccessibleInfo *i1,
                                        XfceIconViewItemAccessibleInfo *i2)
{
  return i1->index - i2->index;
}

static void
xfce_icon_view_accessible_model_rows_reordered (GtkTreeModel *tree_model,
                                                GtkTreePath *path,
                                                GtkTreeIter *iter,
                                                gint *new_order,
                                                gpointer user_data)
{
  XfceIconViewAccessiblePrivate *priv;
  XfceIconViewItemAccessibleInfo *info;
  XfceIconView *icon_view;
  XfceIconViewItemAccessible *item;
  GList *items;
  AtkObject *atk_obj;
  gint *order;
  gint length, i;
  GSequenceIter *item_iter;

  atk_obj = gtk_widget_get_accessible (GTK_WIDGET (user_data));
  icon_view = XFCE_ICON_VIEW (user_data);
  priv = xfce_icon_view_accessible_get_priv (atk_obj);

  length = gtk_tree_model_iter_n_children (tree_model, NULL);

  order = g_new (gint, length);
  for (i = 0; i < length; i++)
    order[new_order[i]] = i;

  items = priv->items;
  while (items)
    {
      info = items->data;
      item = XFCE_ICON_VIEW_ITEM_ACCESSIBLE (info->item);
      info->index = order[info->index];
      item_iter = g_sequence_get_iter_at_pos (get_instance_private (icon_view)->items, info->index);
      item->item = g_sequence_get (item_iter);
      items = items->next;
    }
  g_free (order);
  priv->items = g_list_sort (priv->items,
                             (GCompareFunc) xfce_icon_view_accessible_item_compare);

  return;
}

static void
xfce_icon_view_accessible_disconnect_model_signals (GtkTreeModel *model,
                                                    GtkWidget *widget)
{
  GObject *obj;

  obj = G_OBJECT (model);
  g_signal_handlers_disconnect_by_func (obj, (gpointer) xfce_icon_view_accessible_model_row_changed, widget);
  g_signal_handlers_disconnect_by_func (obj, (gpointer) xfce_icon_view_accessible_model_row_inserted, widget);
  g_signal_handlers_disconnect_by_func (obj, (gpointer) xfce_icon_view_accessible_model_row_deleted, widget);
  g_signal_handlers_disconnect_by_func (obj, (gpointer) xfce_icon_view_accessible_model_rows_reordered, widget);
}

static void
xfce_icon_view_accessible_connect_model_signals (XfceIconView *icon_view)
{
  GObject *obj;

  obj = G_OBJECT (get_instance_private (icon_view)->model);
  g_signal_connect_data (obj, "row-changed",
                         (GCallback) xfce_icon_view_accessible_model_row_changed,
                         icon_view, NULL, 0);
  g_signal_connect_data (obj, "row-inserted",
                         (GCallback) xfce_icon_view_accessible_model_row_inserted,
                         icon_view, NULL, G_CONNECT_AFTER);
  g_signal_connect_data (obj, "row-deleted",
                         (GCallback) xfce_icon_view_accessible_model_row_deleted,
                         icon_view, NULL, G_CONNECT_AFTER);
  g_signal_connect_data (obj, "rows-reordered",
                         (GCallback) xfce_icon_view_accessible_model_rows_reordered,
                         icon_view, NULL, G_CONNECT_AFTER);
}

static void
xfce_icon_view_accessible_clear_cache (XfceIconViewAccessiblePrivate *priv)
{
  XfceIconViewItemAccessibleInfo *info;
  GList *items;

  items = priv->items;
  while (items)
    {
      info = (XfceIconViewItemAccessibleInfo *) items->data;
      g_object_unref (info->item);
      g_free (items->data);
      items = items->next;
    }
  g_list_free (priv->items);
  priv->items = NULL;
}

static void
xfce_icon_view_accessible_notify_gtk (GObject *obj,
                                      GParamSpec *pspec)
{
  XfceIconView *icon_view;
  GtkWidget *widget;
  AtkObject *atk_obj;
  XfceIconViewAccessiblePrivate *priv;

  if (strcmp (pspec->name, "model") == 0)
    {
      widget = GTK_WIDGET (obj);
      atk_obj = gtk_widget_get_accessible (widget);
      priv = xfce_icon_view_accessible_get_priv (atk_obj);
      if (priv->model)
        {
          g_object_remove_weak_pointer (G_OBJECT (priv->model),
                                        (gpointer *) &priv->model);
          xfce_icon_view_accessible_disconnect_model_signals (priv->model, widget);
        }
      xfce_icon_view_accessible_clear_cache (priv);

      icon_view = XFCE_ICON_VIEW (obj);
      priv->model = get_instance_private (icon_view)->model;
      /* If there is no model the XfceIconView is probably being destroyed */
      if (priv->model)
        {
          g_object_add_weak_pointer (G_OBJECT (priv->model), (gpointer *) &priv->model);
          xfce_icon_view_accessible_connect_model_signals (icon_view);
        }
    }

  return;
}

static void
xfce_icon_view_accessible_initialize (AtkObject *accessible,
                                      gpointer data)
{
  XfceIconViewAccessiblePrivate *priv;
  XfceIconViewPrivate *view_priv;
  XfceIconView *icon_view;

  if (ATK_OBJECT_CLASS (accessible_parent_class)->initialize)
    ATK_OBJECT_CLASS (accessible_parent_class)->initialize (accessible, data);

  priv = g_new0 (XfceIconViewAccessiblePrivate, 1);
  g_object_set_qdata (G_OBJECT (accessible),
                      accessible_private_data_quark,
                      priv);

  icon_view = XFCE_ICON_VIEW (data);
  view_priv = get_instance_private (icon_view);
  if (view_priv->hadjustment)
    {
      priv->old_hadj = view_priv->hadjustment;
      g_object_add_weak_pointer (G_OBJECT (priv->old_hadj), (gpointer *) &priv->old_hadj);
      g_signal_connect (view_priv->hadjustment,
                        "value-changed",
                        G_CALLBACK (xfce_icon_view_accessible_adjustment_changed),
                        icon_view);
    }
  if (view_priv->vadjustment)
    {
      priv->old_vadj = view_priv->vadjustment;
      g_object_add_weak_pointer (G_OBJECT (priv->old_vadj), (gpointer *) &priv->old_vadj);
      g_signal_connect (view_priv->vadjustment,
                        "value-changed",
                        G_CALLBACK (xfce_icon_view_accessible_adjustment_changed),
                        icon_view);
    }
  g_signal_connect_after (data,
                          "set-scroll-adjustments",
                          G_CALLBACK (xfce_icon_view_accessible_set_scroll_adjustments),
                          NULL);
  g_signal_connect (data,
                    "notify",
                    G_CALLBACK (xfce_icon_view_accessible_notify_gtk),
                    NULL);

  priv->model = view_priv->model;
  if (priv->model)
    {
      g_object_add_weak_pointer (G_OBJECT (priv->model), (gpointer *) &priv->model);
      xfce_icon_view_accessible_connect_model_signals (icon_view);
    }

  accessible->role = ATK_ROLE_LAYERED_PANE;
}

static void
xfce_icon_view_accessible_finalize (GObject *object)
{
  XfceIconViewAccessiblePrivate *priv;

  priv = xfce_icon_view_accessible_get_priv (ATK_OBJECT (object));
  xfce_icon_view_accessible_clear_cache (priv);

  g_free (priv);

  G_OBJECT_CLASS (accessible_parent_class)->finalize (object);
}

static void
xfce_icon_view_accessible_destroyed (GtkWidget *widget,
                                     GtkAccessible *accessible)
{
  AtkObject *atk_obj;
  XfceIconViewAccessiblePrivate *priv;

  atk_obj = ATK_OBJECT (accessible);
  priv = xfce_icon_view_accessible_get_priv (atk_obj);
  if (priv->old_hadj)
    {
      g_object_remove_weak_pointer (G_OBJECT (priv->old_hadj),
                                    (gpointer *) &priv->old_hadj);

      g_signal_handlers_disconnect_by_func (priv->old_hadj,
                                            (gpointer) xfce_icon_view_accessible_adjustment_changed,
                                            widget);
      priv->old_hadj = NULL;
    }
  if (priv->old_vadj)
    {
      g_object_remove_weak_pointer (G_OBJECT (priv->old_vadj),
                                    (gpointer *) &priv->old_vadj);

      g_signal_handlers_disconnect_by_func (priv->old_vadj,
                                            (gpointer) xfce_icon_view_accessible_adjustment_changed,
                                            widget);
      priv->old_vadj = NULL;
    }
}

static void
xfce_icon_view_accessible_connect_widget_destroyed (GtkAccessible *accessible)
{
  if (gtk_accessible_get_widget (accessible))
    {
      g_signal_connect_after (gtk_accessible_get_widget (accessible),
                              "destroy",
                              G_CALLBACK (xfce_icon_view_accessible_destroyed),
                              accessible);
    }
  GTK_ACCESSIBLE_CLASS (accessible_parent_class)->connect_widget_destroyed (accessible);
}

static void
xfce_icon_view_accessible_class_init (AtkObjectClass *klass)
{
  GObjectClass *gobject_class;
  GtkAccessibleClass *accessible_class;

  accessible_parent_class = g_type_class_peek_parent (klass);

  gobject_class = (GObjectClass *) klass;
  accessible_class = (GtkAccessibleClass *) klass;

  gobject_class->finalize = xfce_icon_view_accessible_finalize;

  klass->get_n_children = xfce_icon_view_accessible_get_n_children;
  klass->ref_child = xfce_icon_view_accessible_ref_child;
  klass->initialize = xfce_icon_view_accessible_initialize;

  accessible_class->connect_widget_destroyed = xfce_icon_view_accessible_connect_widget_destroyed;

  accessible_private_data_quark = g_quark_from_static_string ("icon_view-accessible-private-data");
}

static AtkObject *
xfce_icon_view_accessible_ref_accessible_at_point (AtkComponent *component,
                                                   gint x,
                                                   gint y,
                                                   AtkCoordType coord_type)
{
  GtkWidget *widget;
  XfceIconView *icon_view;
  XfceIconViewItem *item;
  gint x_pos, y_pos;
  gint idx;

  widget = gtk_accessible_get_widget (GTK_ACCESSIBLE (component));
  if (widget == NULL)
    /* State is defunct */
    return NULL;

  icon_view = XFCE_ICON_VIEW (widget);
  atk_component_get_extents (component, &x_pos, &y_pos, NULL, NULL, coord_type);
  item = xfce_icon_view_get_item_at_coords (icon_view, x - x_pos, y - y_pos, TRUE, NULL);
  idx = g_sequence_iter_get_position (item->item_iter);
  if (item)
    return xfce_icon_view_accessible_ref_child (ATK_OBJECT (component), idx);

  return NULL;
}

static void
atk_component_interface_init (AtkComponentIface *iface)
{
  iface->ref_accessible_at_point = xfce_icon_view_accessible_ref_accessible_at_point;
}

static gboolean
xfce_icon_view_accessible_add_selection (AtkSelection *selection,
                                         gint i)
{
  GtkWidget *widget;
  XfceIconView *icon_view;
  XfceIconViewItem *item;
  GSequenceIter *iter;

  widget = gtk_accessible_get_widget (GTK_ACCESSIBLE (selection));
  if (widget == NULL)
    return FALSE;

  icon_view = XFCE_ICON_VIEW (widget);

  iter = g_sequence_get_iter_at_pos (get_instance_private (icon_view)->items, i);
  if (g_sequence_iter_is_end (iter))
    return FALSE;

  item = g_sequence_get (iter);
  if (!item)
    return FALSE;

  xfce_icon_view_select_item (icon_view, item);

  return TRUE;
}

static gboolean
xfce_icon_view_accessible_clear_selection (AtkSelection *selection)
{
  GtkWidget *widget;
  XfceIconView *icon_view;

  widget = gtk_accessible_get_widget (GTK_ACCESSIBLE (selection));
  if (widget == NULL)
    return FALSE;

  icon_view = XFCE_ICON_VIEW (widget);
  xfce_icon_view_unselect_all (icon_view);

  return TRUE;
}

static AtkObject *
xfce_icon_view_accessible_ref_selection (AtkSelection *selection,
                                         gint i)
{
  GSequenceIter *iter;
  GtkWidget *widget;
  XfceIconView *icon_view;
  XfceIconViewItem *item;
  gint idx;

  widget = gtk_accessible_get_widget (GTK_ACCESSIBLE (selection));
  if (widget == NULL)
    return NULL;

  icon_view = XFCE_ICON_VIEW (widget);

  idx = 0;
  for (iter = g_sequence_get_begin_iter (get_instance_private (icon_view)->items);
       !g_sequence_iter_is_end (iter);
       iter = g_sequence_iter_next (iter))
    {
      item = g_sequence_get (iter);
      if (item->selected)
        {
          if (i == 0)
            return atk_object_ref_accessible_child (gtk_widget_get_accessible (widget), idx);
          else
            i--;
        }
      idx++;
    }

  return NULL;
}

static gint
xfce_icon_view_accessible_get_selection_count (AtkSelection *selection)
{
  GtkWidget *widget;
  XfceIconView *icon_view;
  XfceIconViewItem *item;
  GSequenceIter *iter;
  gint count;

  widget = gtk_accessible_get_widget (GTK_ACCESSIBLE (selection));
  if (widget == NULL)
    return 0;

  icon_view = XFCE_ICON_VIEW (widget);

  count = 0;
  for (iter = g_sequence_get_begin_iter (get_instance_private (icon_view)->items);
       !g_sequence_iter_is_end (iter);
       iter = g_sequence_iter_next (iter))
    {
      item = g_sequence_get (iter);
      if (item->selected)
        count++;
    }

  return count;
}

static gboolean
xfce_icon_view_accessible_is_child_selected (AtkSelection *selection,
                                             gint i)
{
  GtkWidget *widget;
  XfceIconView *icon_view;
  XfceIconViewItem *item;
  GSequenceIter *iter;

  widget = gtk_accessible_get_widget (GTK_ACCESSIBLE (selection));
  if (widget == NULL)
    return FALSE;

  icon_view = XFCE_ICON_VIEW (widget);

  iter = g_sequence_get_iter_at_pos (get_instance_private (icon_view)->items, i);
  if (g_sequence_iter_is_end (iter))
    return FALSE;

  item = g_sequence_get (iter);
  if (!item)
    return FALSE;

  return item->selected;
}

static gboolean
xfce_icon_view_accessible_remove_selection (AtkSelection *selection,
                                            gint i)
{
  GtkWidget *widget;
  XfceIconView *icon_view;
  XfceIconViewItem *item;
  GSequenceIter *iter;
  gint count;

  widget = gtk_accessible_get_widget (GTK_ACCESSIBLE (selection));
  if (widget == NULL)
    return FALSE;

  icon_view = XFCE_ICON_VIEW (widget);

  count = 0;
  for (iter = g_sequence_get_begin_iter (get_instance_private (icon_view)->items);
       !g_sequence_iter_is_end (iter);
       iter = g_sequence_iter_next (iter))
    {
      item = g_sequence_get (iter);
      if (item->selected)
        {
          if (count == i)
            {
              xfce_icon_view_unselect_item (icon_view, item);
              return TRUE;
            }
          count++;
        }
    }

  return FALSE;
}

static gboolean
xfce_icon_view_accessible_select_all_selection (AtkSelection *selection)
{
  GtkWidget *widget;
  XfceIconView *icon_view;

  widget = gtk_accessible_get_widget (GTK_ACCESSIBLE (selection));
  if (widget == NULL)
    return FALSE;

  icon_view = XFCE_ICON_VIEW (widget);
  xfce_icon_view_select_all (icon_view);
  return TRUE;
}

static void
xfce_icon_view_accessible_selection_interface_init (AtkSelectionIface *iface)
{
  iface->add_selection = xfce_icon_view_accessible_add_selection;
  iface->clear_selection = xfce_icon_view_accessible_clear_selection;
  iface->ref_selection = xfce_icon_view_accessible_ref_selection;
  iface->get_selection_count = xfce_icon_view_accessible_get_selection_count;
  iface->is_child_selected = xfce_icon_view_accessible_is_child_selected;
  iface->remove_selection = xfce_icon_view_accessible_remove_selection;
  iface->select_all_selection = xfce_icon_view_accessible_select_all_selection;
}

GType
xfce_icon_view_accessible_get_type (void)
{
  static GType type = 0;

  if (!type)
    {
      GTypeInfo tinfo = {
        0, /* class size */
        (GBaseInitFunc) NULL, /* base init */
        (GBaseFinalizeFunc) NULL, /* base finalize */
        (GClassInitFunc) (void (*) (void)) xfce_icon_view_accessible_class_init,
        (GClassFinalizeFunc) NULL, /* class finalize */
        NULL, /* class data */
        0, /* instance size */
        0, /* nb preallocs */
        (GInstanceInitFunc) NULL, /* instance init */
        NULL /* value table */
      };
      const GInterfaceInfo atk_component_info = {
        (GInterfaceInitFunc) (void (*) (void)) atk_component_interface_init,
        (GInterfaceFinalizeFunc) NULL,
        NULL
      };
      const GInterfaceInfo atk_selection_info = {
        (GInterfaceInitFunc) (void (*) (void)) xfce_icon_view_accessible_selection_interface_init,
        (GInterfaceFinalizeFunc) NULL,
        NULL
      };

      /*
       * Figure out the size of the class and instance
       * we are deriving from
       */
      AtkObjectFactory *factory;
      GType derived_type;
      GTypeQuery query;
      GType derived_atk_type;

      derived_type = g_type_parent (GTK_TYPE_ICON_VIEW);
      factory = atk_registry_get_factory (atk_get_default_registry (),
                                          derived_type);
      derived_atk_type = atk_object_factory_get_accessible_type (factory);
      g_type_query (derived_atk_type, &query);
      tinfo.class_size = query.class_size;
      tinfo.instance_size = query.instance_size;

      type = g_type_register_static (derived_atk_type,
                                     I_ ("XfceIconViewAccessible"),
                                     &tinfo, 0);
      g_type_add_interface_static (type, ATK_TYPE_COMPONENT,
                                   &atk_component_info);
      g_type_add_interface_static (type, ATK_TYPE_SELECTION,
                                   &atk_selection_info);
    }
  return type;
}

static AtkObject *
xfce_icon_view_accessible_new (GObject *obj)
{
  AtkObject *accessible;

  g_return_val_if_fail (GTK_IS_WIDGET (obj), NULL);

  accessible = g_object_new (xfce_icon_view_accessible_get_type (), NULL);
  atk_object_initialize (accessible, obj);

  return accessible;
}

static GType
xfce_icon_view_accessible_factory_get_accessible_type (void)
{
  return xfce_icon_view_accessible_get_type ();
}

static AtkObject *
xfce_icon_view_accessible_factory_create_accessible (GObject *obj)
{
  return xfce_icon_view_accessible_new (obj);
}

static void
xfce_icon_view_accessible_factory_class_init (AtkObjectFactoryClass *klass)
{
  klass->create_accessible = xfce_icon_view_accessible_factory_create_accessible;
  klass->get_accessible_type = xfce_icon_view_accessible_factory_get_accessible_type;
}


static GType
xfce_icon_view_accessible_factory_get_type (void)
{
  static GType type = 0;

  if (!type)
    {
      const GTypeInfo tinfo = {
        sizeof (AtkObjectFactoryClass),
        NULL, /* base_init */
        NULL, /* base_finalize */
        (GClassInitFunc) (void (*) (void)) xfce_icon_view_accessible_factory_class_init,
        NULL, /* class_finalize */
        NULL, /* class_data */
        sizeof (AtkObjectFactory),
        0, /* n_preallocs */
        NULL, NULL
      };

      type = g_type_register_static (ATK_TYPE_OBJECT_FACTORY,
                                     I_ ("XfceIconViewAccessibleFactory"),
                                     &tinfo, 0);
    }
  return type;
}
