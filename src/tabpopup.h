/* Metacity tab popup window */

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

#ifndef META_TABPOPUP_H
#define META_TABPOPUP_H

/* Don't include gtk.h or gdk.h here */
#include "common.h"
#include <X11/Xlib.h>
#include <glib.h>
#include <gdk-pixbuf/gdk-pixbuf.h>

typedef struct _MetaTabEntry MetaTabEntry;
typedef struct _MetaTabPopup MetaTabPopup;
typedef void *MetaTabEntryKey;

struct _MetaTabEntry
{
  MetaTabEntryKey  key;  
  const char      *title;
  GdkPixbuf       *icon;
  int              x, y, width, height;
  int              inner_x, inner_y, inner_width, inner_height;
  guint            blank : 1;
};

MetaTabPopup*   meta_ui_tab_popup_new          (const MetaTabEntry *entries,
                                                int                 screen_number,
                                                int                 entry_count,
                                                int                 width,
                                                gboolean            outline);
void            meta_ui_tab_popup_free         (MetaTabPopup       *popup);
void            meta_ui_tab_popup_set_showing  (MetaTabPopup       *popup,
                                                gboolean            showing);
void            meta_ui_tab_popup_forward      (MetaTabPopup       *popup);
void            meta_ui_tab_popup_backward     (MetaTabPopup       *popup);
MetaTabEntryKey meta_ui_tab_popup_get_selected (MetaTabPopup      *popup);
void            meta_ui_tab_popup_select       (MetaTabPopup       *popup,
                                                MetaTabEntryKey     key);


#endif

