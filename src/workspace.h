/* Metacity Workspaces */

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

#ifndef META_WORKSPACE_H
#define META_WORKSPACE_H

#include "window.h"

/* Negative to avoid conflicting with real workspace
 * numbers
 */
typedef enum
{
  META_MOTION_UP = -1,
  META_MOTION_DOWN = -2,
  META_MOTION_LEFT = -3,
  META_MOTION_RIGHT = -4
} MetaMotionDirection;

struct _MetaWorkspace
{
  MetaScreen *screen;
  
  GList *windows;
  
  MetaRectangle work_area;
  guint work_area_invalid : 1;
};

MetaWorkspace* meta_workspace_new           (MetaScreen    *screen);
void           meta_workspace_free          (MetaWorkspace *workspace);
void           meta_workspace_add_window    (MetaWorkspace *workspace,
                                             MetaWindow    *window);
void           meta_workspace_remove_window (MetaWorkspace *workspace,
                                             MetaWindow    *window);
void           meta_workspace_relocate_windows (MetaWorkspace *workspace,
                                                MetaWorkspace *new_home);
/* don't confuse with meta_window_visible_on_workspace() */
gboolean       meta_workspace_contains_window (MetaWorkspace *workspace,
                                               MetaWindow  *window);
void           meta_workspace_activate      (MetaWorkspace *workspace);
int            meta_workspace_index         (MetaWorkspace *workspace);
GList*         meta_workspace_list_windows  (MetaWorkspace *workspace);

void meta_workspace_invalidate_work_area (MetaWorkspace *workspace);
void meta_workspace_get_work_area        (MetaWorkspace *workspace,
                                          MetaRectangle *area);


MetaWorkspace* meta_workspace_get_neighbor (MetaWorkspace      *workspace,
                                            MetaMotionDirection direction);

const char* meta_workspace_get_name (MetaWorkspace *workspace);

#endif




