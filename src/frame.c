/* Metacity X window decorations */

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
#include "frame.h"
#include "errors.h"
#include "keybindings.h"

#define EVENT_MASK (SubstructureRedirectMask |                     \
                    StructureNotifyMask | SubstructureNotifyMask | \
                    ExposureMask |                                 \
                    ButtonPressMask | ButtonReleaseMask |          \
                    PointerMotionMask | PointerMotionHintMask |    \
                    EnterWindowMask | LeaveWindowMask |            \
                    FocusChangeMask |                              \
                    ColormapChangeMask)

void
meta_window_ensure_frame (MetaWindow *window)
{
  MetaFrame *frame;
  XSetWindowAttributes attrs;
  
  if (window->frame)
    return;

  /* See comment below for why this is required. */
  meta_display_grab (window->display);
  
  frame = g_new (MetaFrame, 1);

  frame->window = window;
  frame->xwindow = None;

  frame->rect = window->rect;
  frame->child_x = 0;
  frame->child_y = 0;
  frame->bottom_height = 0;
  frame->right_width = 0;
  frame->current_cursor = 0;

  frame->mapped = FALSE;
  frame->need_reapply_frame_shape = TRUE;
  
  attrs.event_mask = EVENT_MASK;

  meta_verbose ("Framing window %s: visual %s default, depth %d default depth %d\n",
                window->desc,
                XVisualIDFromVisual (window->xvisual) ==
                XVisualIDFromVisual (window->screen->default_xvisual) ?
                "is" : "is not",
                window->depth, window->screen->default_depth);
  meta_verbose ("Frame geometry %d,%d  %dx%d\n",
                frame->rect.x, frame->rect.y,
                frame->rect.width, frame->rect.height);
  
  /* Default depth/visual handles clients with weird visuals; they can
   * always be children of the root depth/visual obviously, but
   * e.g. DRI games can't be children of a parent that has the same
   * visual as the client.
   */
  
  frame->xwindow = XCreateWindow (window->display->xdisplay,
                                  window->screen->xroot,
                                  frame->rect.x,
                                  frame->rect.y,
                                  frame->rect.width,
                                  frame->rect.height,
                                  0,
                                  window->screen->default_depth,
                                  CopyFromParent,
                                  window->screen->default_xvisual,
                                  CWEventMask,
                                  &attrs);

  /* So our UI can find the window ID */
  XFlush (window->display->xdisplay);
  
  meta_verbose ("Frame for %s is 0x%lx\n", frame->window->desc, frame->xwindow);
  
  meta_display_register_x_window (window->display, &frame->xwindow, window);

  /* Reparent the client window; it may be destroyed,
   * thus the error trap. We'll get a destroy notify later
   * and free everything. Comment in FVWM source code says
   * we need a server grab or the child can get its MapNotify
   * before we've finished reparenting and getting the decoration
   * window onscreen, so ensure_frame must be called with
   * a grab.
   */
  meta_error_trap_push (window->display);
  if (window->mapped)
    {
      window->mapped = FALSE; /* the reparent will unmap the window,
                               * we don't want to take that as a withdraw
                               */
      meta_topic (META_DEBUG_WINDOW_STATE,
                  "Incrementing unmaps_pending on %s for reparent\n", window->desc);
      window->unmaps_pending += 1;
    }
  /* window was reparented to this position */
  window->rect.x = 0;
  window->rect.y = 0;

  XReparentWindow (window->display->xdisplay,
                   window->xwindow,
                   frame->xwindow,
                   window->rect.x,
                   window->rect.y);
  /* FIXME handle this error */
  meta_error_trap_pop (window->display, FALSE);
  
  /* stick frame to the window */
  window->frame = frame;
  
  meta_ui_add_frame (window->screen->ui, frame->xwindow);

  if (window->title)
    meta_ui_set_frame_title (window->screen->ui,
                             window->frame->xwindow,
                             window->title);

  /* Move keybindings to frame instead of window */
  meta_window_grab_keys (window);

  /* Shape mask */
  meta_ui_apply_frame_shape (frame->window->screen->ui,
                             frame->xwindow,
                             frame->rect.width,
                             frame->rect.height,
                             frame->window->has_shape);
  frame->need_reapply_frame_shape = FALSE;
  
  meta_display_ungrab (window->display);
}

void
meta_window_destroy_frame (MetaWindow *window)
{
  MetaFrame *frame;
  
  if (window->frame == NULL)
    return;

  frame = window->frame;
  
  meta_ui_remove_frame (window->screen->ui, frame->xwindow);
  
  /* Unparent the client window; it may be destroyed,
   * thus the error trap.
   */
  meta_error_trap_push (window->display);
  if (window->mapped)
    {
      window->mapped = FALSE; /* Keep track of unmapping it, so we
                               * can identify a withdraw initiated
                               * by the client.
                               */
      meta_topic (META_DEBUG_WINDOW_STATE,
                  "Incrementing unmaps_pending on %s for reparent back to root\n", window->desc);
      window->unmaps_pending += 1;
    }
  XReparentWindow (window->display->xdisplay,
                   window->xwindow,
                   window->screen->xroot,
                   /* FIXME where to put it back depends on the gravity */
                   window->frame->rect.x,
                   window->frame->rect.y);
  meta_error_trap_pop (window->display, FALSE);

  meta_display_unregister_x_window (window->display,
                                    frame->xwindow);
  
  window->frame = NULL;

  /* Move keybindings to window instead of frame */
  meta_window_grab_keys (window);
  
  /* should we push an error trap? */
  XDestroyWindow (window->display->xdisplay, frame->xwindow);
  
  g_free (frame);
  
  /* Put our state back where it should be */
  meta_window_queue_calc_showing (window);
}


MetaFrameFlags
meta_frame_get_flags (MetaFrame *frame)
{
  MetaFrameFlags flags;

  flags = 0;

  if (frame->window->border_only)
    {
      ; /* FIXME this may disable the _function_ as well as decor
         * in some cases, which is sort of wrong.
         */
    }
  else
    {
      flags |= META_FRAME_ALLOWS_MENU;
      
      if (frame->window->has_close_func)
        flags |= META_FRAME_ALLOWS_DELETE;
      
      if (frame->window->has_maximize_func)
        flags |= META_FRAME_ALLOWS_MAXIMIZE;
      
      if (frame->window->has_minimize_func)
        flags |= META_FRAME_ALLOWS_MINIMIZE;
      
      if (frame->window->has_shade_func)
        flags |= META_FRAME_ALLOWS_SHADE;
    }  
  
  if (META_WINDOW_ALLOWS_MOVE (frame->window))
    flags |= META_FRAME_ALLOWS_MOVE;

  if (META_WINDOW_ALLOWS_HORIZONTAL_RESIZE (frame->window))
    flags |= META_FRAME_ALLOWS_HORIZONTAL_RESIZE;

  if (META_WINDOW_ALLOWS_VERTICAL_RESIZE (frame->window))
    flags |= META_FRAME_ALLOWS_VERTICAL_RESIZE;
  
  if (frame->window->has_focus)
    flags |= META_FRAME_HAS_FOCUS;

  if (frame->window->shaded)
    flags |= META_FRAME_SHADED;

  if (frame->window->on_all_workspaces)
    flags |= META_FRAME_STUCK;

  if (frame->window->maximized)
    flags |= META_FRAME_MAXIMIZED;

  if (frame->window->fullscreen)
    flags |= META_FRAME_FULLSCREEN;

  return flags;
}

void
meta_frame_calc_geometry (MetaFrame         *frame,
                          MetaFrameGeometry *geomp)
{
  MetaFrameGeometry geom;
  MetaWindow *window;

  window = frame->window;

  meta_ui_get_frame_geometry (window->screen->ui,
                              frame->xwindow,
                              &geom.top_height,
                              &geom.bottom_height,
                              &geom.left_width,
                              &geom.right_width);
  
  *geomp = geom;
}

static void
update_shape (MetaFrame *frame)
{
  if (frame->need_reapply_frame_shape)
    {
      meta_ui_apply_frame_shape (frame->window->screen->ui,
                                 frame->xwindow,
                                 frame->rect.width,
                                 frame->rect.height,
                                 frame->window->has_shape);
      frame->need_reapply_frame_shape = FALSE;
    }
}

void
meta_frame_sync_to_window (MetaFrame *frame,
                           int        resize_gravity,
                           gboolean   need_move,
                           gboolean   need_resize)
{
  if (!(need_move || need_resize))
    {
      update_shape (frame);
      return;
    }

  meta_topic (META_DEBUG_GEOMETRY,
              "Syncing frame geometry %d,%d %dx%d (SE: %d,%d)\n",
              frame->rect.x, frame->rect.y,
              frame->rect.width, frame->rect.height,
              frame->rect.x + frame->rect.width,
              frame->rect.y + frame->rect.height);

  /* set bg to none to avoid flicker */
  if (need_resize)
    {
      meta_ui_unflicker_frame_bg (frame->window->screen->ui,
                                  frame->xwindow,
                                  frame->rect.width,
                                  frame->rect.height);

      /* we need new shape if we're resized */
      frame->need_reapply_frame_shape = TRUE;
    }

  /* Done before the window resize, because doing it before means
   * part of the window being resized becomes unshaped, which may
   * be sort of hard to see with bg = None. If we did it after
   * window resize, part of the window being resized would become
   * shaped, which might be more visible.
   */
  update_shape (frame);
  
  if (need_move && need_resize)
    XMoveResizeWindow (frame->window->display->xdisplay,
                       frame->xwindow,
                       frame->rect.x,
                       frame->rect.y,
                       frame->rect.width,
                       frame->rect.height);
  else if (need_move)
    XMoveWindow (frame->window->display->xdisplay,
                 frame->xwindow,
                 frame->rect.x,
                 frame->rect.y);
  else if (need_resize)
    XResizeWindow (frame->window->display->xdisplay,
                   frame->xwindow,
                   frame->rect.width,
                   frame->rect.height);  

  if (need_resize)
    {
      meta_ui_reset_frame_bg (frame->window->screen->ui,
                              frame->xwindow);

      /* If we're interactively resizing the frame, repaint
       * it immediately so we don't start to lag.
       */
      if (frame->window->display->grab_window ==
          frame->window)
        meta_ui_repaint_frame (frame->window->screen->ui,
                               frame->xwindow);
    }
}

void
meta_frame_queue_draw (MetaFrame *frame)
{
  meta_ui_queue_frame_draw (frame->window->screen->ui,
                            frame->xwindow);
}

void meta_frame_set_screen_cursor (MetaFrame	*frame,
				   MetaCursor	cursor)
{
  Cursor xcursor;
  if (cursor == frame->current_cursor)
    return;
  frame->current_cursor = cursor;
  if (cursor == META_CURSOR_DEFAULT)
    XUndefineCursor (frame->window->display->xdisplay, frame->xwindow);
  else
    { 
      xcursor = meta_display_create_x_cursor (frame->window->display, cursor);
      XDefineCursor (frame->window->display->xdisplay, frame->xwindow, xcursor);
      XFreeCursor (frame->window->display->xdisplay, xcursor);
    }
}

