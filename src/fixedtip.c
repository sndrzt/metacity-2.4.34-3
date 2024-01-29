/* Metacity fixed tooltip routine */

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
#include "fixedtip.h"

static GtkWidget *tip = NULL;
static GtkWidget *label = NULL;
static int screen_width = 0;
static int screen_height = 0;

static gint
expose_handler (GtkTooltips *tooltips)
{
  gtk_paint_flat_box (tip->style, tip->window,
                      GTK_STATE_NORMAL, GTK_SHADOW_OUT, 
                      NULL, tip, "tooltip",
                      0, 0, -1, -1);

  return FALSE;
}

void
meta_fixed_tip_show (Display *xdisplay, int screen_number,
                     int root_x, int root_y,
                     const char *markup_text)
{
  int w, h;
  
  if (tip == NULL)
    {      
      tip = gtk_window_new (GTK_WINDOW_POPUP);
#ifdef HAVE_GTK_MULTIHEAD
      {
        GdkScreen *gdk_screen;

        gdk_screen = gdk_display_get_screen (gdk_display_get_default (),
                                             screen_number);
        gtk_window_set_screen (GTK_WINDOW (tip),
                               gdk_screen);
        screen_width = gdk_screen_get_width (gdk_screen);
        screen_height = gdk_screen_get_height (gdk_screen);
      }
#else
      screen_width = gdk_screen_width ();
      screen_height = gdk_screen_height ();      
#endif
      
      gtk_widget_set_app_paintable (tip, TRUE);
      gtk_window_set_policy (GTK_WINDOW (tip), FALSE, FALSE, TRUE);
      gtk_widget_set_name (tip, "gtk-tooltips");
      gtk_container_set_border_width (GTK_CONTAINER (tip), 4);

      gtk_signal_connect_object (GTK_OBJECT (tip), 
				 "expose_event",
				 GTK_SIGNAL_FUNC (expose_handler),
                                 NULL);

      label = gtk_label_new (NULL);
      gtk_label_set_line_wrap (GTK_LABEL (label), TRUE);
      gtk_misc_set_alignment (GTK_MISC (label), 0.5, 0.5);
      gtk_widget_show (label);
      
      gtk_container_add (GTK_CONTAINER (tip), label);

      gtk_signal_connect (GTK_OBJECT (tip),
			  "destroy",
			  GTK_SIGNAL_FUNC (gtk_widget_destroyed),
			  &tip);
    }

  gtk_label_set_markup (GTK_LABEL (label), markup_text);
  
  /* FIXME should also handle Xinerama here, just to be
   * really cool
   */
  gtk_window_get_size (GTK_WINDOW (tip), &w, &h);
  if ((root_x + w) > screen_width)
    root_x -= (root_x + w) - screen_width;
  
  gtk_window_move (GTK_WINDOW (tip), root_x, root_y);

  gtk_widget_show (tip);
}

void
meta_fixed_tip_hide (void)
{
  if (tip)
    {
      gtk_widget_destroy (tip);
      tip = NULL;
    }
}
