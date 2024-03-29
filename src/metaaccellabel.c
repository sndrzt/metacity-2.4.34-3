/* Metacity hacked-up GtkAccelLabel */
/* Copyright (C) 2002 Red Hat, Inc. */
/* GTK - The GIMP Toolkit
 * Copyright (C) 1995-1997 Peter Mattis, Spencer Kimball and Josh MacDonald
 *
 * MetaAccelLabel: GtkLabel with accelerator monitoring facilities.
 * Copyright (C) 1998 Tim Janik
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.	 See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.
 */

/*
 * Modified by the GTK+ Team and others 1997-2001.  See the AUTHORS
 * file for a list of people on the GTK+ Team.  See the ChangeLog
 * files for a list of changes.  These files are distributed with
 * GTK+ at ftp://ftp.gtk.org/pub/gtk/.
 */

#include <config.h>
#include "metaaccellabel.h"
#include <gtk/gtkmain.h>
#include <gtk/gtkaccelmap.h>
#include <string.h>
#include "util.h"

static void     meta_accel_label_class_init   (MetaAccelLabelClass *klass);
static void     meta_accel_label_init         (MetaAccelLabel      *accel_label);
static void     meta_accel_label_destroy      (GtkObject           *object);
static void     meta_accel_label_finalize     (GObject             *object);
static void     meta_accel_label_size_request (GtkWidget           *widget,
                                               GtkRequisition      *requisition);
static gboolean meta_accel_label_expose_event (GtkWidget           *widget,
                                               GdkEventExpose      *event);

static void  meta_accel_label_update          (MetaAccelLabel *accel_label);
static int   meta_accel_label_get_accel_width (MetaAccelLabel *accel_label);


static MetaAccelLabelClass *accel_label_class = NULL;
static GtkLabelClass *parent_class = NULL;


GtkType
meta_accel_label_get_type (void)
{
  static GtkType accel_label_type = 0;

  if (!accel_label_type)
    {
      static const GtkTypeInfo accel_label_info =
      {
	"MetaAccelLabel",
	sizeof (MetaAccelLabel),
	sizeof (MetaAccelLabelClass),
	(GtkClassInitFunc) meta_accel_label_class_init,
	(GtkObjectInitFunc) meta_accel_label_init,
        /* reserved_1 */ NULL,
	/* reserved_2 */ NULL,
	(GtkClassInitFunc) NULL,
      };

      accel_label_type = gtk_type_unique (GTK_TYPE_LABEL, &accel_label_info);
    }

  return accel_label_type;
}

static void
meta_accel_label_class_init (MetaAccelLabelClass *class)
{
  GObjectClass *gobject_class = G_OBJECT_CLASS (class);
  GtkObjectClass *object_class = GTK_OBJECT_CLASS (class);
  GtkWidgetClass *widget_class = GTK_WIDGET_CLASS (class);

  accel_label_class = class;
  parent_class = g_type_class_peek_parent (class);

  gobject_class->finalize = meta_accel_label_finalize;

  object_class->destroy = meta_accel_label_destroy;

  widget_class->size_request = meta_accel_label_size_request;
  widget_class->expose_event = meta_accel_label_expose_event;

  class->signal_quote1 = g_strdup ("<:");
  class->signal_quote2 = g_strdup (":>");
  /* This is the text that should appear next to menu accelerators
   * that use the shift key. If the text on this key isn't typically
   * translated on keyboards used for your language, don't translate
   * this.
   */
  class->mod_name_shift = g_strdup (_("Shift"));
  /* This is the text that should appear next to menu accelerators
   * that use the control key. If the text on this key isn't typically
   * translated on keyboards used for your language, don't translate
   * this.
   */
  class->mod_name_control = g_strdup (_("Ctrl"));
  /* This is the text that should appear next to menu accelerators
   * that use the alt key. If the text on this key isn't typically
   * translated on keyboards used for your language, don't translate
   * this.
   */
  class->mod_name_alt = g_strdup (_("Alt"));
  /* This is the text that should appear next to menu accelerators
   * that use the meta key. If the text on this key isn't typically
   * translated on keyboards used for your language, don't translate
   * this.
   */
  class->mod_name_meta = g_strdup (_("Meta"));
  /* This is the text that should appear next to menu accelerators
   * that use the super key. If the text on this key isn't typically
   * translated on keyboards used for your language, don't translate
   * this.
   */
  class->mod_name_super = g_strdup (_("Super"));
  /* This is the text that should appear next to menu accelerators
   * that use the hyper key. If the text on this key isn't typically
   * translated on keyboards used for your language, don't translate
   * this.
   */
  class->mod_name_hyper = g_strdup (_("Hyper"));
  /* This is the text that should appear next to menu accelerators
   * that use the mod2 key. If the text on this key isn't typically
   * translated on keyboards used for your language, don't translate
   * this.
   */
  class->mod_name_mod2 = g_strdup (_("Mod2"));
  /* This is the text that should appear next to menu accelerators
   * that use the mod3 key. If the text on this key isn't typically
   * translated on keyboards used for your language, don't translate
   * this.
   */
  class->mod_name_mod3 = g_strdup (_("Mod3"));
  /* This is the text that should appear next to menu accelerators
   * that use the mod4 key. If the text on this key isn't typically
   * translated on keyboards used for your language, don't translate
   * this.
   */
  class->mod_name_mod4 = g_strdup (_("Mod4"));
  /* This is the text that should appear next to menu accelerators
   * that use the mod5 key. If the text on this key isn't typically
   * translated on keyboards used for your language, don't translate
   * this.
   */
  class->mod_name_mod5 = g_strdup (_("Mod5"));

  class->mod_separator = g_strdup ("+");
  class->accel_seperator = g_strdup (" / ");
  class->latin1_to_char = TRUE;
}

static void
meta_accel_label_init (MetaAccelLabel *accel_label)
{
  accel_label->accel_padding = 3;
  accel_label->accel_string = NULL;

  meta_accel_label_update (accel_label);
}

GtkWidget*
meta_accel_label_new_with_mnemonic (const gchar *string)
{
  MetaAccelLabel *accel_label;

  g_return_val_if_fail (string != NULL, NULL);

  accel_label = g_object_new (META_TYPE_ACCEL_LABEL, NULL);

  gtk_label_set_text_with_mnemonic (GTK_LABEL (accel_label), string);

  return GTK_WIDGET (accel_label);
}

static void
meta_accel_label_destroy (GtkObject *object)
{
  MetaAccelLabel *accel_label = META_ACCEL_LABEL (object);


  g_free (accel_label->accel_string);
  accel_label->accel_string = NULL;

  accel_label->accel_mods = 0;
  accel_label->accel_key = 0;

  GTK_OBJECT_CLASS (parent_class)->destroy (object);
}

static void
meta_accel_label_finalize (GObject *object)
{
  MetaAccelLabel *accel_label = META_ACCEL_LABEL (object);

  g_free (accel_label->accel_string);

  G_OBJECT_CLASS (parent_class)->finalize (object);
}

void
meta_accel_label_set_accelerator (MetaAccelLabel         *accel_label,
                                  guint                   accelerator_key,
                                  MetaVirtualModifier     accelerator_mods)
{
  g_return_if_fail (META_IS_ACCEL_LABEL (accel_label));

  if (accelerator_key != accel_label->accel_key ||
      accelerator_mods != accel_label->accel_mods)
    {
      accel_label->accel_mods = accelerator_mods;
      accel_label->accel_key = accelerator_key;

      meta_accel_label_update (accel_label);
    }
}

static int
meta_accel_label_get_accel_width (MetaAccelLabel *accel_label)
{
  g_return_val_if_fail (META_IS_ACCEL_LABEL (accel_label), 0);

  return (accel_label->accel_string_width +
	  (accel_label->accel_string_width ? accel_label->accel_padding : 0));
}

static void
meta_accel_label_size_request (GtkWidget	     *widget,
			      GtkRequisition *requisition)
{
  MetaAccelLabel *accel_label = META_ACCEL_LABEL (widget);
  PangoLayout *layout;
  gint width;

  if (GTK_WIDGET_CLASS (parent_class)->size_request)
    GTK_WIDGET_CLASS (parent_class)->size_request (widget, requisition);

  layout = gtk_widget_create_pango_layout (widget, accel_label->accel_string);
  pango_layout_get_pixel_size (layout, &width, NULL);
  accel_label->accel_string_width = width;

  g_object_unref (G_OBJECT (layout));
}

static gboolean
meta_accel_label_expose_event (GtkWidget      *widget,
			      GdkEventExpose *event)
{
  MetaAccelLabel *accel_label = META_ACCEL_LABEL (widget);
  GtkMisc *misc = GTK_MISC (accel_label);
  PangoLayout *layout;

  if (GTK_WIDGET_DRAWABLE (accel_label))
    {
      int ac_width;

      ac_width = meta_accel_label_get_accel_width (accel_label);

      if (widget->allocation.width >= widget->requisition.width + ac_width)
	{
	  int x;
	  int y;

	  widget->allocation.width -= ac_width;
	  if (GTK_WIDGET_CLASS (parent_class)->expose_event)
	    GTK_WIDGET_CLASS (parent_class)->expose_event (widget, event);
	  widget->allocation.width += ac_width;

	  x = widget->allocation.x + widget->allocation.width - misc->xpad - ac_width;

	  y = (widget->allocation.y * (1.0 - misc->yalign) +
	       (widget->allocation.y + widget->allocation.height -
		(widget->requisition.height - misc->ypad * 2)) *
	       misc->yalign) + 1.5;

	  layout = gtk_widget_create_pango_layout (widget, accel_label->accel_string);

          gtk_paint_layout (widget->style,
                            widget->window,
                            GTK_WIDGET_STATE (widget),
			    FALSE,
                            &event->area,
                            widget,
                            "accellabel",
                            x, y,
                            layout);

          g_object_unref (G_OBJECT (layout));
	}
      else
	{
	  if (GTK_WIDGET_CLASS (parent_class)->expose_event)
	    GTK_WIDGET_CLASS (parent_class)->expose_event (widget, event);
	}
    }

  return FALSE;
}

static void
meta_accel_label_update (MetaAccelLabel *accel_label)
{
  MetaAccelLabelClass *class;
  GString *gstring;
  gboolean seen_mod = FALSE;
  gunichar ch;

  g_return_if_fail (META_IS_ACCEL_LABEL (accel_label));

  class = META_ACCEL_LABEL_GET_CLASS (accel_label);

  g_free (accel_label->accel_string);
  accel_label->accel_string = NULL;

  gstring = g_string_new (accel_label->accel_string);
  g_string_append (gstring, gstring->len ? class->accel_seperator : "   ");

  if (accel_label->accel_mods & META_VIRTUAL_SHIFT_MASK)
    {
      g_string_append (gstring, class->mod_name_shift);
      seen_mod = TRUE;
    }
  if (accel_label->accel_mods & META_VIRTUAL_CONTROL_MASK)
    {
      if (seen_mod)
        g_string_append (gstring, class->mod_separator);
      g_string_append (gstring, class->mod_name_control);
      seen_mod = TRUE;
    }
  if (accel_label->accel_mods & META_VIRTUAL_ALT_MASK)
    {
      if (seen_mod)
        g_string_append (gstring, class->mod_separator);
      g_string_append (gstring, class->mod_name_alt);
      seen_mod = TRUE;
    }
  if (accel_label->accel_mods & META_VIRTUAL_META_MASK)
    {
      if (seen_mod)
        g_string_append (gstring, class->mod_separator);
      g_string_append (gstring, class->mod_name_meta);
      seen_mod = TRUE;
    }
  if (accel_label->accel_mods & META_VIRTUAL_SUPER_MASK)
    {
      if (seen_mod)
        g_string_append (gstring, class->mod_separator);
      g_string_append (gstring, class->mod_name_super);
      seen_mod = TRUE;
    }
  if (accel_label->accel_mods & META_VIRTUAL_HYPER_MASK)
    {
      if (seen_mod)
        g_string_append (gstring, class->mod_separator);
      g_string_append (gstring, class->mod_name_hyper);
      seen_mod = TRUE;
    }
  if (accel_label->accel_mods & META_VIRTUAL_MOD2_MASK)
    {
      if (seen_mod)
        g_string_append (gstring, class->mod_separator);
      g_string_append (gstring, class->mod_name_mod2);
      seen_mod = TRUE;
    }
  if (accel_label->accel_mods & META_VIRTUAL_MOD3_MASK)
    {
      if (seen_mod)
        g_string_append (gstring, class->mod_separator);
      g_string_append (gstring, class->mod_name_mod3);
      seen_mod = TRUE;
    }
  if (accel_label->accel_mods & META_VIRTUAL_MOD4_MASK)
    {
      if (seen_mod)
        g_string_append (gstring, class->mod_separator);
      g_string_append (gstring, class->mod_name_mod4);
      seen_mod = TRUE;
    }
  if (accel_label->accel_mods & META_VIRTUAL_MOD5_MASK)
    {
      if (seen_mod)
        g_string_append (gstring, class->mod_separator);
      g_string_append (gstring, class->mod_name_mod5);
      seen_mod = TRUE;
    }

  if (seen_mod)
    g_string_append (gstring, class->mod_separator);

  ch = gdk_keyval_to_unicode (accel_label->accel_key);
  if (ch && (g_unichar_isgraph (ch) || ch == ' ') &&
      (ch < 0x80 || class->latin1_to_char))
    {
      switch (ch)
        {
        case ' ':
          g_string_append (gstring, "Space");
          break;
        case '\\':
          g_string_append (gstring, "Backslash");
          break;
        default:
          g_string_append_unichar (gstring, g_unichar_toupper (ch));
          break;
        }
    }
  else
    {
      gchar *tmp;

      tmp = gtk_accelerator_name (accel_label->accel_key, 0);
      if (tmp[0] != 0 && tmp[1] == 0)
        tmp[0] = g_ascii_toupper (tmp[0]);
      g_string_append (gstring, tmp);
      g_free (tmp);
    }

  g_free (accel_label->accel_string);
  accel_label->accel_string = gstring->str;
  g_string_free (gstring, FALSE);

  g_assert (accel_label->accel_string);
  /* accel_label->accel_string = g_strdup ("-/-"); */

  gtk_widget_queue_resize (GTK_WIDGET (accel_label));
}
