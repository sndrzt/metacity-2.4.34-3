/* Metacity resizing-terminal-window feedback */

/* 
 * Copyright (C) 2001 Havoc Pennington
 * 
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of the
 * License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA
 * 02111-1307, USA.
 */

#include <config.h>
#include "resizepopup.h"
#include "util.h"
#include <gtk/gtkwindow.h>
#include <gtk/gtklabel.h>
#include <gtk/gtkframe.h>
#include <gtk/gtkmain.h>
#include <gdk/gdkx.h>

struct _MetaResizePopup
{
  GtkWidget *size_window;
  GtkWidget *size_label;
  Display *display;
  int screen_number;  

  int vertical_size;
  int horizontal_size;
  
  gboolean showing;
  
  int resize_gravity;
  int x;
  int y;
  int width;
  int height;
  int width_inc;
  int height_inc;
  int min_width;
  int min_height;
  int frame_left;
  int frame_right;
  int frame_top;
  int frame_bottom;
  int tick_origin_x;
  int tick_origin_y;
};

MetaResizePopup*
meta_ui_resize_popup_new (Display *display,
                          int      screen_number)
{
  MetaResizePopup *popup;

  popup = g_new0 (MetaResizePopup, 1);

  popup->resize_gravity = -1;
  popup->display = display;
  popup->screen_number = screen_number;
  
  return popup;
}

void
meta_ui_resize_popup_free (MetaResizePopup *popup)
{
  g_return_if_fail (popup != NULL);
  
  if (popup->size_window)
    gtk_widget_destroy (popup->size_window);
  
  g_free (popup);
}

static void
ensure_size_window (MetaResizePopup *popup)
{
  GtkWidget *frame;
  
  if (popup->size_window)
    return;
  
  popup->size_window = gtk_window_new (GTK_WINDOW_POPUP);
#ifdef HAVE_GTK_MULTIHEAD
  gtk_window_set_screen (GTK_WINDOW (popup->size_window),
			 gdk_display_get_screen (gdk_x11_lookup_xdisplay (popup->display),
						 popup->screen_number));
#endif
  
  /* never shrink the size window */
  gtk_window_set_resizable (GTK_WINDOW (popup->size_window),
                            TRUE);
  
  frame = gtk_frame_new (NULL);
  gtk_frame_set_shadow_type (GTK_FRAME (frame), GTK_SHADOW_OUT);

  gtk_container_add (GTK_CONTAINER (popup->size_window), frame);

  popup->size_label = gtk_label_new ("");
  gtk_misc_set_padding (GTK_MISC (popup->size_label), 3, 3);

  gtk_container_add (GTK_CONTAINER (frame), popup->size_label);

  gtk_widget_show_all (frame);
}

static void
update_size_window (MetaResizePopup *popup)
{
  char *str;
  int x, y;
  int width, height;
  
  g_return_if_fail (popup->size_window != NULL);
  
  str = g_strdup_printf (_("%d x %d"),
                         popup->horizontal_size,
                         popup->vertical_size);

  gtk_label_set_text (GTK_LABEL (popup->size_label), str);

  g_free (str);

  gtk_window_get_size (GTK_WINDOW (popup->size_window), &width, &height);

  x = popup->x + (popup->width - width) / 2;
  y = popup->y + (popup->height - height) / 2;
  
  if (GTK_WIDGET_REALIZED (popup->size_window))
    {
      /* using move_resize to avoid jumpiness */
      gdk_window_move_resize (popup->size_window->window,
                              x, y,
                              width, height);
    }
  else
    {
      gtk_window_move   (GTK_WINDOW (popup->size_window),
                         x, y);
    }
}

static void
sync_showing (MetaResizePopup *popup)
{
  if (popup->showing)
    {
      if (popup->size_window)
        gtk_widget_show (popup->size_window);
      
      if (popup->size_window && GTK_WIDGET_REALIZED (popup->size_window))
        gdk_window_raise (popup->size_window->window);
    }
  else
    {
      if (popup->size_window)
        gtk_widget_hide (popup->size_window);
    }
}

void
meta_ui_resize_popup_set (MetaResizePopup *popup,
                          int              resize_gravity,
                          int              x,
                          int              y,
                          int              width,
                          int              height,
                          int              base_width,
                          int              base_height,
                          int              min_width,
                          int              min_height,
                          int              width_inc,
                          int              height_inc,
                          int              frame_left,
                          int              frame_right,
                          int              frame_top,
                          int              frame_bottom)
{
  gboolean need_update_size;
  int display_w, display_h;
  
  g_return_if_fail (popup != NULL);

  need_update_size = FALSE;
  
  display_w = width - base_width;
  if (width_inc > 0)
    display_w /= width_inc;

  display_h = height - base_height;
  if (height_inc > 0)
    display_h /= height_inc;
  
  if (popup->x != x ||
      popup->y != y ||
      popup->width != width ||
      popup->height != height ||
      display_w != popup->horizontal_size ||
      display_h != popup->vertical_size)
    need_update_size = TRUE;
  
  popup->resize_gravity = resize_gravity;
  popup->x = x;
  popup->y = y;
  popup->width = width;
  popup->height = height;
  popup->min_width = min_width;
  popup->min_height = min_height;
  popup->width_inc = width_inc;
  popup->height_inc = height_inc;
  popup->frame_left = frame_left;
  popup->frame_right = frame_right;
  popup->frame_top = frame_top;
  popup->frame_bottom = frame_bottom;
  popup->vertical_size = display_h;
  popup->horizontal_size = display_w;
  
  if (need_update_size)
    {
      ensure_size_window (popup);
      update_size_window (popup);
    }
      
  sync_showing (popup);
}

void
meta_ui_resize_popup_set_showing  (MetaResizePopup *popup,
                                   gboolean         showing)
{
  g_return_if_fail (popup != NULL);
  
  if (showing == popup->showing)
    return;

  popup->showing = !!showing;

  if (popup->showing)
    {
      ensure_size_window (popup);
      update_size_window (popup);
    }
  
  sync_showing (popup);
}
