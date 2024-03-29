/* Metacity window groups */

/* 
 * Copyright (C) 2002 Red Hat Inc.
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
#include "util.h"
#include "group-private.h"
#include "group-props.h"
#include "window.h"

static MetaGroup*
meta_group_new (MetaDisplay *display,
                Window       group_leader)
{
  MetaGroup *group;
#define N_INITIAL_PROPS 3
  Atom initial_props[N_INITIAL_PROPS];
  int i;
  
  g_assert (N_INITIAL_PROPS == (int) G_N_ELEMENTS (initial_props));
  
  group = g_new0 (MetaGroup, 1);

  group->display = display;
  group->windows = NULL;
  group->group_leader = group_leader;
  group->refcount = 1; /* owned by caller, hash table has only weak ref */
  
  if (display->groups_by_leader == NULL)
    display->groups_by_leader = g_hash_table_new (meta_unsigned_long_hash,
                                                  meta_unsigned_long_equal);

  g_assert (g_hash_table_lookup (display->groups_by_leader, &group_leader) == NULL);

  g_hash_table_insert (display->groups_by_leader,
                       &group->group_leader,
                       group);

  /* Fill these in the order we want them to be gotten */
  i = 0;
  initial_props[i++] = display->atom_wm_client_machine;
  initial_props[i++] = display->atom_net_wm_pid;
  initial_props[i++] = display->atom_net_startup_id;
  g_assert (N_INITIAL_PROPS == i);
  
  meta_group_reload_properties (group, initial_props, N_INITIAL_PROPS);

  meta_topic (META_DEBUG_GROUPS,
              "Created new group with leader 0x%lx\n",
              group->group_leader);
  
  return group;
}

static void
meta_group_unref (MetaGroup *group)
{
  g_return_if_fail (group->refcount > 0);

  group->refcount -= 1;
  if (group->refcount == 0)
    {
      meta_topic (META_DEBUG_GROUPS,
                  "Destroying group with leader 0x%lx\n",
                  group->group_leader);      
      
      g_assert (group->display->groups_by_leader != NULL);
      
      g_hash_table_remove (group->display->groups_by_leader,
                           &group->group_leader);

      /* mop up hash table, this is how it gets freed on display close */
      if (g_hash_table_size (group->display->groups_by_leader) == 0)
        {
          g_hash_table_destroy (group->display->groups_by_leader);
          group->display->groups_by_leader = NULL;
        }

      g_free (group->wm_client_machine);
      g_free (group->startup_id);
      
      g_free (group);
    }
}

MetaGroup*
meta_window_get_group (MetaWindow *window)
{
  if (window->unmanaging)
    return NULL;
  
  if (window->cached_group == NULL)
    {
      MetaGroup *group;

      /* use window->xwindow if no window->xgroup_leader */
      
      group = NULL;
      
      if (window->display->groups_by_leader)
        {
          if (window->xgroup_leader != None)
            group = g_hash_table_lookup (window->display->groups_by_leader,
                                         &window->xgroup_leader);
          else
            group = g_hash_table_lookup (window->display->groups_by_leader,
                                         &window->xwindow);
        }
      
      if (group != NULL)
        {
          window->cached_group = group;
          group->refcount += 1;
        }
      else
        {
          if (window->xgroup_leader != None)
            group = meta_group_new (window->display,
                                    window->xgroup_leader);
          else
            group = meta_group_new (window->display,
                                    window->xwindow);
          
          window->cached_group = group;
        }

      window->cached_group->windows = g_slist_prepend (window->cached_group->windows,
                                                       window);

      meta_topic (META_DEBUG_GROUPS,
                  "Adding %s to group with leader 0x%lx\n",
                  window->desc, group->group_leader);
    }

  return window->cached_group;
}

static void
remove_window_from_group (MetaWindow *window)
{
  if (window->cached_group != NULL)
    {
      meta_topic (META_DEBUG_GROUPS,
                  "Removing %s from group with leader 0x%lx\n",
                  window->desc, window->cached_group->group_leader);
      
      window->cached_group->windows =
        g_slist_remove (window->cached_group->windows,
                        window);
      meta_group_unref (window->cached_group);
      window->cached_group = NULL;
    }
}

void
meta_window_group_leader_changed (MetaWindow *window)
{
  remove_window_from_group (window);
  meta_window_get_group (window);
}

void
meta_window_shutdown_group (MetaWindow *window)
{
  remove_window_from_group (window);
}

MetaGroup*
meta_display_lookup_group (MetaDisplay *display,
                           Window       group_leader)
{
  MetaGroup *group;
  
  group = NULL;
  
  if (display->groups_by_leader)
    group = g_hash_table_lookup (display->groups_by_leader,
                                 &group_leader);

  return group;
}

GSList*
meta_group_list_windows (MetaGroup *group)
{
  return g_slist_copy (group->windows);
}

void
meta_group_update_layers (MetaGroup *group)
{
  GSList *tmp;
  GSList *frozen_stacks;
  
  if (group->windows == NULL)
    return;

  frozen_stacks = NULL;
  tmp = group->windows;
  while (tmp != NULL)
    {
      MetaWindow *window = tmp->data;

      /* we end up freezing the same stack a lot of times,
       * but doesn't hurt anything. have to handle
       * groups that span 2 screens.
       */
      meta_stack_freeze (window->screen->stack);
      frozen_stacks = g_slist_prepend (frozen_stacks, window->screen->stack);

      meta_stack_update_layer (window->screen->stack,
                               window);
      
      tmp = tmp->next;
    }

  tmp = frozen_stacks;
  while (tmp != NULL)
    {
      meta_stack_thaw (tmp->data);
      tmp = tmp->next;
    }

  g_slist_free (frozen_stacks);
}

const char*
meta_group_get_startup_id (MetaGroup *group)
{
  return group->startup_id;
}

gboolean
meta_group_property_notify (MetaGroup  *group,
                            XEvent     *event)
{
  meta_group_reload_property (group,
                              event->xproperty.atom);

  return TRUE;
}
