/* Metacity interface for talking to GTK+ UI module */

/* 
 * Copyright (C) 2002 Havoc Pennington
 * stock icon code Copyright (C) 2002 Jorn Baayen <jorn@nl.linux.org>
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
#include "ui.h"
#include "frames.h"
#include "util.h"
#include "menu.h"
#include "core.h"
#include "theme.h"

#include "eggaccelerators.h"

#include "inlinepixbufs.h"

#include <pango/pangox.h>

#include <string.h>

static void meta_stock_icons_init (void);

struct _MetaUI
{
  Display *xdisplay;
  Screen *xscreen;
  MetaFrames *frames;
};

void
meta_ui_init (int *argc, char ***argv)
{
  if (!gtk_init_check (argc, argv))
    meta_fatal ("Unable to open X display %s\n", XDisplayName (NULL));

  {
    /* FIXME hackaround for Pango opening a separate display
     * connection and doing a server grab while we have a grab
     * on the primary display connection. This forces Pango to
     * go ahead and do its font cache before we try to grab
     * the server.
     */  
    PangoFontMetrics *metrics;
    PangoLanguage *lang;
    PangoContext *context;
    PangoFontDescription *font_desc;

    context = gdk_pango_context_get ();
    lang = gtk_get_default_language ();
    font_desc = pango_font_description_from_string ("Sans 12");
    metrics = pango_context_get_metrics (context, font_desc, lang);    
    
    pango_font_metrics_unref (metrics);
    pango_font_description_free (font_desc);
    g_object_unref (G_OBJECT (context));
  }

  meta_stock_icons_init ();
}

Display*
meta_ui_get_display (const char *name)
{
  if (name == NULL)
    return gdk_display;
  else
    return NULL;
}

typedef struct _EventFunc EventFunc;

struct _EventFunc
{
  MetaEventFunc func;
  gpointer data;
};

static GdkFilterReturn
filter_func (GdkXEvent *xevent,
             GdkEvent *event,
             gpointer data)
{
  EventFunc *ef;

  ef = data;

  if ((* ef->func) (xevent, ef->data))
    return GDK_FILTER_REMOVE;
  else
    return GDK_FILTER_CONTINUE;
}

static EventFunc *ef = NULL;

void
meta_ui_add_event_func (Display       *xdisplay,
                        MetaEventFunc  func,
                        gpointer       data)
{
  g_return_if_fail (ef == NULL);

  ef = g_new (EventFunc, 1);
  ef->func = func;
  ef->data = data;

  gdk_window_add_filter (NULL, filter_func, ef);
}

/* removal is by data due to proxy function */
void
meta_ui_remove_event_func (Display       *xdisplay,
                           MetaEventFunc  func,
                           gpointer       data)
{
  g_return_if_fail (ef != NULL);
  
  gdk_window_remove_filter (NULL, filter_func, ef);

  g_free (ef);
  ef = NULL;
}

MetaUI*
meta_ui_new (Display *xdisplay,
             Screen  *screen)
{
  MetaUI *ui;

  ui = g_new (MetaUI, 1);
  ui->xdisplay = xdisplay;
  ui->xscreen = screen;

  g_assert (xdisplay == gdk_display);
  ui->frames = meta_frames_new (XScreenNumberOfScreen (screen));
  gtk_widget_realize (GTK_WIDGET (ui->frames));
  
  return ui;
}

void
meta_ui_free (MetaUI *ui)
{
  gtk_widget_destroy (GTK_WIDGET (ui->frames));

  g_free (ui);
}

void
meta_ui_get_frame_geometry (MetaUI *ui,
                            Window frame_xwindow,
                            int *top_height, int *bottom_height,
                            int *left_width, int *right_width)
{
  meta_frames_get_geometry (ui->frames, frame_xwindow,
                            top_height, bottom_height,
                            left_width, right_width);
}


void
meta_ui_add_frame (MetaUI *ui,
                   Window  xwindow)
{
  meta_frames_manage_window (ui->frames, xwindow);
}

void
meta_ui_remove_frame (MetaUI *ui,
                      Window  xwindow)
{
  meta_frames_unmanage_window (ui->frames, xwindow);
}

void
meta_ui_map_frame   (MetaUI *ui,
                     Window  xwindow)
{
  GdkWindow *window;

  window = gdk_xid_table_lookup (xwindow);

  if (window)
    gdk_window_show_unraised (window);
}

void
meta_ui_unmap_frame (MetaUI *ui,
                     Window  xwindow)
{
  GdkWindow *window;

  window = gdk_xid_table_lookup (xwindow);

  if (window)
    gdk_window_hide (window);
}

void
meta_ui_unflicker_frame_bg (MetaUI *ui,
                            Window  xwindow,
                            int     target_width,
                            int     target_height)
{
  meta_frames_unflicker_bg (ui->frames, xwindow,
                            target_width, target_height);
}

void
meta_ui_repaint_frame (MetaUI *ui,
                       Window xwindow)
{
  meta_frames_repaint_frame (ui->frames, xwindow);
}

void
meta_ui_reset_frame_bg (MetaUI *ui,
                        Window xwindow)
{
  meta_frames_reset_bg (ui->frames, xwindow);
}

void
meta_ui_apply_frame_shape  (MetaUI  *ui,
                            Window   xwindow,
                            int      new_window_width,
                            int      new_window_height,
                            gboolean window_has_shape)
{
  meta_frames_apply_shapes (ui->frames, xwindow,
                            new_window_width, new_window_height,
                            window_has_shape);
}

void
meta_ui_queue_frame_draw (MetaUI *ui,
                          Window xwindow)
{
  meta_frames_queue_draw (ui->frames, xwindow);
}


void
meta_ui_set_frame_title (MetaUI     *ui,
                         Window      xwindow,
                         const char *title)
{
  meta_frames_set_title (ui->frames, xwindow, title);
}

MetaWindowMenu*
meta_ui_window_menu_new  (MetaUI             *ui,
                          Window              client_xwindow,
                          MetaMenuOp          ops,
                          MetaMenuOp          insensitive,
                          int                 active_workspace,
                          int                 n_workspaces,
                          MetaWindowMenuFunc  func,
                          gpointer            data)
{
  return meta_window_menu_new (ui->frames,
                               ops, insensitive,
                               client_xwindow,
                               active_workspace,
                               n_workspaces,
                               func, data);
}

void
meta_ui_window_menu_popup (MetaWindowMenu     *menu,
                           int                 root_x,
                           int                 root_y,
                           int                 button,
                           guint32             timestamp)
{
  meta_window_menu_popup (menu, root_x, root_y, button, timestamp);
}

void
meta_ui_window_menu_free (MetaWindowMenu *menu)
{
  meta_window_menu_free (menu);
}

struct _MetaImageWindow
{
  GtkWidget *window;
  GdkPixmap *pixmap;
};

MetaImageWindow*
meta_image_window_new (Display *xdisplay,
                       int      screen_number,
                       int      max_width,
                       int      max_height)
{
  MetaImageWindow *iw;
  
  iw = g_new (MetaImageWindow, 1);
  iw->window = gtk_window_new (GTK_WINDOW_POPUP);

#ifdef HAVE_GTK_MULTIHEAD
  {
    GdkDisplay *gdisplay;
    GdkScreen *gscreen;
    
    gdisplay = gdk_x11_lookup_xdisplay (xdisplay);
    gscreen = gdk_display_get_screen (gdisplay, screen_number);
    
    gtk_window_set_screen (GTK_WINDOW (iw->window), gscreen);
  }
#endif
 
  gtk_widget_realize (iw->window);
  iw->pixmap = gdk_pixmap_new (iw->window->window,
                               max_width, max_height,
                               -1);
  
  gtk_widget_set_size_request (iw->window, 1, 1);
  gtk_widget_set_double_buffered (iw->window, FALSE);
  gtk_widget_set_app_paintable (iw->window, TRUE);
  
  return iw;
}

void
meta_image_window_free (MetaImageWindow *iw)
{
  gtk_widget_destroy (iw->window);
  g_object_unref (G_OBJECT (iw->pixmap));
  g_free (iw);
}

void
meta_image_window_set_showing  (MetaImageWindow *iw,
                                gboolean         showing)
{
  if (showing)
    gtk_widget_show_all (iw->window);
  else
    {
      gtk_widget_hide (iw->window);
      meta_core_increment_event_serial (gdk_display);
    }
}

void
meta_image_window_set (MetaImageWindow *iw,
                       GdkPixbuf       *pixbuf,
                       int              x,
                       int              y)
{
  /* We use a back pixmap to avoid having to handle exposes, because
   * it's really too slow for large clients being minimized, etc.
   * and this way flicker is genuinely zero.
   */

  gdk_pixbuf_render_to_drawable (pixbuf,
                                 iw->pixmap,
                                 iw->window->style->black_gc,
                                 0, 0,
                                 0, 0,
                                 gdk_pixbuf_get_width (pixbuf),
                                 gdk_pixbuf_get_height (pixbuf),
                                 GDK_RGB_DITHER_NORMAL,
                                 0, 0);
  
  gdk_window_set_back_pixmap (iw->window->window,
                              iw->pixmap,
                              FALSE);
  
  gdk_window_move_resize (iw->window->window,
                          x, y,
                          gdk_pixbuf_get_width (pixbuf),
                          gdk_pixbuf_get_height (pixbuf));

  gdk_window_clear (iw->window->window);
}

static GdkColormap*
get_cmap (GdkPixmap *pixmap)
{
  GdkColormap *cmap;

  cmap = gdk_drawable_get_colormap (pixmap);
  if (cmap)
    g_object_ref (G_OBJECT (cmap));

  if (cmap == NULL)
    {
      if (gdk_drawable_get_depth (pixmap) == 1)
        {
          meta_verbose ("Using NULL colormap for snapshotting bitmap\n");
          cmap = NULL;
        }
      else
        {
          meta_verbose ("Using system cmap to snapshot pixmap\n");
#ifdef HAVE_GTK_MULTIHEAD
          cmap = gdk_screen_get_system_colormap (gdk_drawable_get_screen (pixmap));
#else
          cmap = gdk_colormap_get_system ();
#endif
          g_object_ref (G_OBJECT (cmap));
        }
    }

  /* Be sure we aren't going to blow up due to visual mismatch */
  if (cmap &&
      (gdk_colormap_get_visual (cmap)->depth !=
       gdk_drawable_get_depth (pixmap)))
    {
      cmap = NULL;
      meta_verbose ("Switching back to NULL cmap because of depth mismatch\n");
    }
  
  return cmap;
}

GdkPixbuf*
meta_gdk_pixbuf_get_from_window (GdkPixbuf   *dest,
                                 Window       xwindow,
                                 int          src_x,
                                 int          src_y,
                                 int          dest_x,
                                 int          dest_y,
                                 int          width,
                                 int          height)
{
  GdkDrawable *drawable;
  GdkPixbuf *retval;
  GdkColormap *cmap;
  
  retval = NULL;
  
  drawable = gdk_xid_table_lookup (xwindow);

  if (drawable)
    g_object_ref (G_OBJECT (drawable));
  else
    drawable = gdk_window_foreign_new (xwindow);

  cmap = get_cmap (drawable);
  
  retval = gdk_pixbuf_get_from_drawable (dest,
                                         drawable,
                                         cmap,
                                         src_x, src_y,
                                         dest_x, dest_y,
                                         width, height);

  if (cmap)
    g_object_unref (G_OBJECT (cmap));
  g_object_unref (G_OBJECT (drawable));

  return retval;
}

GdkPixbuf*
meta_gdk_pixbuf_get_from_pixmap (GdkPixbuf   *dest,
                                 Pixmap       xpixmap,
                                 int          src_x,
                                 int          src_y,
                                 int          dest_x,
                                 int          dest_y,
                                 int          width,
                                 int          height)
{
  GdkDrawable *drawable;
  GdkPixbuf *retval;
  GdkColormap *cmap;
  
  retval = NULL;
  
  drawable = gdk_xid_table_lookup (xpixmap);

  if (drawable)
    g_object_ref (G_OBJECT (drawable));
  else
    drawable = gdk_pixmap_foreign_new (xpixmap);

  cmap = get_cmap (drawable);
  
  retval = gdk_pixbuf_get_from_drawable (dest,
                                         drawable,
                                         cmap,
                                         src_x, src_y,
                                         dest_x, dest_y,
                                         width, height);

  if (cmap)
    g_object_unref (G_OBJECT (cmap));
  g_object_unref (G_OBJECT (drawable));

  return retval;
}

void
meta_ui_push_delay_exposes (MetaUI *ui)
{
  meta_frames_push_delay_exposes (ui->frames);
}

void
meta_ui_pop_delay_exposes  (MetaUI *ui)
{
  meta_frames_pop_delay_exposes (ui->frames);
}

#ifdef HAVE_GDK_PIXBUF_NEW_FROM_STREAM
#define gdk_pixbuf_new_from_inline gdk_pixbuf_new_from_stream
#endif

GdkPixbuf*
meta_ui_get_default_window_icon (MetaUI *ui)
{
  static GdkPixbuf *default_icon = NULL;

  if (default_icon == NULL)
    {
      GdkPixbuf *base;

      base = gdk_pixbuf_new_from_inline (-1, default_icon_data,
                                         FALSE,
                                         NULL);

      g_assert (base);

      default_icon = gdk_pixbuf_scale_simple (base,
                                              META_ICON_WIDTH,
                                              META_ICON_HEIGHT,
                                              GDK_INTERP_BILINEAR);

      g_object_unref (G_OBJECT (base));
    }

  g_object_ref (G_OBJECT (default_icon));
  
  return default_icon;
}

GdkPixbuf*
meta_ui_get_default_mini_icon (MetaUI *ui)
{
  static GdkPixbuf *default_icon = NULL;

  if (default_icon == NULL)
    {
      GdkPixbuf *base;

      base = gdk_pixbuf_new_from_inline (-1, default_icon_data,
                                         FALSE,
                                         NULL);

      g_assert (base);

      default_icon = gdk_pixbuf_scale_simple (base,
                                              META_MINI_ICON_WIDTH,
                                              META_MINI_ICON_HEIGHT,
                                              GDK_INTERP_BILINEAR);

      g_object_unref (G_OBJECT (base));
    }

  g_object_ref (G_OBJECT (default_icon));
  
  return default_icon;
}

gboolean
meta_ui_window_should_not_cause_focus (Display *xdisplay,
                                       Window   xwindow)
{
  GdkWindow *window;

  window = gdk_xid_table_lookup (xwindow);

  /* we shouldn't cause focus if we're an override redirect
   * toplevel which is not foreign
   */
  if (window && gdk_window_get_type (window) == GDK_WINDOW_TEMP)
    return TRUE;
  else
    return FALSE;
}

char*
meta_text_property_to_utf8 (Display             *xdisplay,
                            const XTextProperty *prop)
{
  char **list;
  int count;
  char *retval;
  
  list = NULL;

  count = gdk_text_property_to_utf8_list (gdk_x11_xatom_to_atom (prop->encoding),
                                          prop->format,
                                          prop->value,
                                          prop->nitems,
                                          &list);

  if (count == 0)
    return NULL;

  retval = list[0];
  list[0] = g_strdup (""); /* something to free */
  
  g_strfreev (list);

  return retval;
}

void
meta_ui_set_current_theme (const char *name,
                           gboolean    force_reload)
{
  meta_theme_set_current (name, force_reload);
}

gboolean
meta_ui_have_a_theme (void)
{
  return meta_theme_get_current () != NULL;
}

gboolean
meta_ui_parse_accelerator (const char          *accel,
                           unsigned int        *keysym,
                           MetaVirtualModifier *mask)
{
  EggVirtualModifierType gdk_mask = 0;
  guint gdk_sym = 0;
  
  *keysym = 0;
  *mask = 0;

  if (strcmp (accel, "disabled") == 0)
    return TRUE;
  
  if (!egg_accelerator_parse_virtual (accel, &gdk_sym, &gdk_mask))
    return FALSE;

  if (gdk_sym == None)
    return FALSE;
  
  if (gdk_mask & EGG_VIRTUAL_RELEASE_MASK) /* we don't allow this */
    return FALSE;
  
  *keysym = gdk_sym;

  if (gdk_mask & EGG_VIRTUAL_SHIFT_MASK)
    *mask |= META_VIRTUAL_SHIFT_MASK;
  if (gdk_mask & EGG_VIRTUAL_CONTROL_MASK)
    *mask |= META_VIRTUAL_CONTROL_MASK;
  if (gdk_mask & EGG_VIRTUAL_ALT_MASK)
    *mask |= META_VIRTUAL_ALT_MASK;
  if (gdk_mask & EGG_VIRTUAL_MOD2_MASK)
    *mask |= META_VIRTUAL_MOD2_MASK;
  if (gdk_mask & EGG_VIRTUAL_MOD3_MASK)
    *mask |= META_VIRTUAL_MOD3_MASK;
  if (gdk_mask & EGG_VIRTUAL_MOD4_MASK)
    *mask |= META_VIRTUAL_MOD4_MASK;
  if (gdk_mask & EGG_VIRTUAL_MOD5_MASK)
    *mask |= META_VIRTUAL_MOD5_MASK;
  if (gdk_mask & EGG_VIRTUAL_SUPER_MASK)
    *mask |= META_VIRTUAL_SUPER_MASK;
  if (gdk_mask & EGG_VIRTUAL_HYPER_MASK)
    *mask |= META_VIRTUAL_HYPER_MASK;
  if (gdk_mask & EGG_VIRTUAL_META_MASK)
    *mask |= META_VIRTUAL_META_MASK;
  
  return TRUE;
}

gboolean
meta_ui_parse_modifier (const char          *accel,
                        MetaVirtualModifier *mask)
{
  EggVirtualModifierType gdk_mask = 0;
  guint gdk_sym = 0;
  
  *mask = 0;

  if (strcmp (accel, "disabled") == 0)
    return TRUE;
  
  if (!egg_accelerator_parse_virtual (accel, &gdk_sym, &gdk_mask))
    return FALSE;

  if (gdk_sym != None)
    return FALSE;
  
  if (gdk_mask & EGG_VIRTUAL_RELEASE_MASK) /* we don't allow this */
    return FALSE;

  if (gdk_mask & EGG_VIRTUAL_SHIFT_MASK)
    *mask |= META_VIRTUAL_SHIFT_MASK;
  if (gdk_mask & EGG_VIRTUAL_CONTROL_MASK)
    *mask |= META_VIRTUAL_CONTROL_MASK;
  if (gdk_mask & EGG_VIRTUAL_ALT_MASK)
    *mask |= META_VIRTUAL_ALT_MASK;
  if (gdk_mask & EGG_VIRTUAL_MOD2_MASK)
    *mask |= META_VIRTUAL_MOD2_MASK;
  if (gdk_mask & EGG_VIRTUAL_MOD3_MASK)
    *mask |= META_VIRTUAL_MOD3_MASK;
  if (gdk_mask & EGG_VIRTUAL_MOD4_MASK)
    *mask |= META_VIRTUAL_MOD4_MASK;
  if (gdk_mask & EGG_VIRTUAL_MOD5_MASK)
    *mask |= META_VIRTUAL_MOD5_MASK;
  if (gdk_mask & EGG_VIRTUAL_SUPER_MASK)
    *mask |= META_VIRTUAL_SUPER_MASK;
  if (gdk_mask & EGG_VIRTUAL_HYPER_MASK)
    *mask |= META_VIRTUAL_HYPER_MASK;
  if (gdk_mask & EGG_VIRTUAL_META_MASK)
    *mask |= META_VIRTUAL_META_MASK;
  
  return TRUE;
}

gboolean
meta_ui_window_is_widget (MetaUI *ui,
                          Window  xwindow)
{
  GdkWindow *window;

  window = gdk_xid_table_lookup (xwindow);

  if (window &&
      gdk_window_get_window_type (window) != GDK_WINDOW_FOREIGN)
    {
      void *user_data = NULL;
      gdk_window_get_user_data (window, &user_data);
      return user_data != NULL;
    }
  else
    return FALSE;
}

/* stock icon code Copyright (C) 2002 Jorn Baayen <jorn@nl.linux.org> */
typedef struct
{
  char *stock_id;
  const guint8 *icon_data;
} MetaStockIcon;

static void
meta_stock_icons_init (void)
{
  GtkIconFactory *factory;
  int i;

  MetaStockIcon items[] =
  {
    { METACITY_STOCK_DELETE,   stock_delete_data   },
    { METACITY_STOCK_MINIMIZE, stock_minimize_data },
    { METACITY_STOCK_MAXIMIZE, stock_maximize_data }
  };

  factory = gtk_icon_factory_new ();
  gtk_icon_factory_add_default (factory);

  for (i = 0; i < (gint) G_N_ELEMENTS (items); i++)
    {
      GtkIconSet *icon_set;
      GdkPixbuf *pixbuf;

      pixbuf = gdk_pixbuf_new_from_inline (-1, items[i].icon_data,
					   FALSE,
					   NULL);

      icon_set = gtk_icon_set_new_from_pixbuf (pixbuf);
      gtk_icon_factory_add (factory, items[i].stock_id, icon_set);
      gtk_icon_set_unref (icon_set);
		
      g_object_unref (G_OBJECT (pixbuf));
    }

  g_object_unref (G_OBJECT (factory));
}
