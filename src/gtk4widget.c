/* A Gtk Widget that inherits GtkFixed, but can be shrunk.
This file is only use when compiling with Gtk+ 3.

Copyright (C) 2011-2019 Free Software Foundation, Inc.

This file is part of GNU Emacs.

GNU Emacs is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or (at
your option) any later version.

GNU Emacs is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with GNU Emacs.  If not, see <https://www.gnu.org/licenses/>.  */

#include <config.h>

#include "lisp.h"
#include "frame.h"
#include "gtk4term.h"
#include "xwidget.h"

#include "gtk4widget.h"

/* Silence a bogus diagnostic; see GNOME bug 683906.  */
#if GNUC_PREREQ (4, 7, 0) && ! GLIB_CHECK_VERSION (2, 35, 7)
# pragma GCC diagnostic push
# pragma GCC diagnostic ignored "-Wunused-local-typedefs"
#endif

typedef struct _EmacsFixed EmacsFixed;
typedef struct _EmacsFixedClass EmacsFixedClass;

struct _EmacsFixedPrivate
{
  struct frame *f;
};


static void emacs_fixed_get_preferred_width  (GtkWidget *widget,
					      gint      *minimum,
					      gint      *natural);
static void emacs_fixed_get_preferred_height (GtkWidget *widget,
					      gint      *minimum,
					      gint      *natural);
static GType emacs_fixed_get_type (void);
/* static void             gtk_emacsfixed_native_interface_init (GtkNativeInterface  *iface); */

G_DEFINE_TYPE (EmacsFixed, emacs_fixed, GTK_TYPE_FIXED)

/* G_DEFINE_TYPE_WITH_CODE (EmacsFixed, emacs_fixed, GTK_TYPE_FIXED, */
/*			   G_IMPLEMENT_INTERFACE (GTK_TYPE_NATIVE, */
/*						  gtk_emacsfixed_native_interface_init)) */


static EmacsFixed *
EMACS_FIXED (GtkWidget *widget)
{
  return G_TYPE_CHECK_INSTANCE_CAST (widget, emacs_fixed_get_type (),
				     EmacsFixed);
}

#ifdef HAVE_XWIDGETS

static EmacsFixedClass *
EMACS_FIXED_GET_CLASS (GtkWidget *widget)
{
  return G_TYPE_INSTANCE_GET_CLASS (widget, emacs_fixed_get_type (),
				    EmacsFixedClass);
}

struct GtkFixedPrivateL
{
  GList *children;
};

static void
emacs_fixed_gtk_widget_size_allocate (GtkWidget *widget,
				      GtkAllocation *allocation)
{
  /* For xwidgets.

     This basically re-implements the base class method and adds an
     additional case for an xwidget view.

     It would be nicer if the bse class method could be called first,
     and the xview modification only would remain here. It wasn't
     possible to solve it that way yet.  */
  EmacsFixedClass *klass;
  GtkWidgetClass *parent_class;
  struct GtkFixedPrivateL *priv;

  klass = EMACS_FIXED_GET_CLASS (widget);
  parent_class = g_type_class_peek_parent (klass);
  parent_class->size_allocate (widget, allocation);

  priv = G_TYPE_INSTANCE_GET_PRIVATE (widget, GTK_TYPE_FIXED,
				      struct GtkFixedPrivateL);

  gtk_widget_set_allocation (widget, allocation);

  if (gtk_widget_get_has_window (widget))
    {
      if (gtk_widget_get_realized (widget))
	gdk_window_move_resize (gtk_widget_get_window (widget),
				allocation->x,
				allocation->y,
				allocation->width,
				allocation->height);
    }

  for (GList *children = priv->children; children; children = children->next)
    {
      GtkFixedChild *child = children->data;

      if (!gtk_widget_get_visible (child->widget))
	continue;

      GtkRequisition child_requisition;
      gtk_widget_get_preferred_size (child->widget, &child_requisition, NULL);

      GtkAllocation child_allocation;
      child_allocation.x = child->x;
      child_allocation.y = child->y;

      if (!gtk_widget_get_has_window (widget))
	{
	  child_allocation.x += allocation->x;
	  child_allocation.y += allocation->y;
	}

      child_allocation.width = child_requisition.width;
      child_allocation.height = child_requisition.height;

      struct xwidget_view *xv
	= g_object_get_data (G_OBJECT (child->widget), XG_XWIDGET_VIEW);
      if (xv)
	{
	  child_allocation.width = xv->clip_right;
	  child_allocation.height = xv->clip_bottom - xv->clip_top;
	}

      gtk_widget_size_allocate (child->widget, &child_allocation);
    }
}

#endif  /* HAVE_XWIDGETS */

#if 0 //def HAVE_GTK4
#define FRAME_CR_SURFACE(f)	((f)->output_data.gtk4->cr_surface)

static GdkSurface *
gtk_emacsfixed_native_get_surface (GtkNative *native)
{
  /* GtkWindow *self = GTK_WINDOW (native); */
  /* GtkWindowPrivate *priv = gtk_emacsfixed_get_instance_private (self); */

  /* return priv->surface; */

  GTK4_TRACE("snapshot____");
  EmacsFixed *fixed = EMACS_FIXED (native);
  EmacsFixedPrivate *priv = G_TYPE_INSTANCE_GET_PRIVATE (native, emacs_fixed_get_type (),
				      EmacsFixedPrivate);

  return FRAME_CR_SURFACE (priv->f);

  /* return NULL; */
}

static GskRenderer *
gtk_emacsfixed_native_get_renderer (GtkNative *native)
{
  /* GtkWindow *self = GTK_WINDOW (native); */
  /* GtkWindowPrivate *priv = gtk_emacsfixed_get_instance_private (self); */

  /* return priv->renderer; */
  return NULL;
}

static void
gtk_emacsfixed_native_get_surface_transform (GtkNative *native,
					 int       *x,
					 int       *y)
{
  GtkWindow *self = GTK_WINDOW (native);
  GtkStyleContext *context;
  GtkBorder margin, border, padding;

  context = gtk_widget_get_style_context (GTK_WIDGET (self));
  gtk_style_context_get_margin (context, &margin);
  gtk_style_context_get_border (context, &border);
  gtk_style_context_get_padding (context, &padding);

  *x = margin.left + border.left + padding.left;
  *y = margin.top + border.top + padding.top;
}

static void
gtk_emacsfixed_native_check_resize (GtkNative *native)
{
  //  gtk_emacsfixed_check_resize (GTK_WINDOW (native));
}


static void gtk_emacsfixed_native_interface_init (GtkNativeInterface  *iface)
{
  iface->get_renderer = gtk_emacsfixed_native_get_renderer;
  iface->get_surface = gtk_emacsfixed_native_get_surface;
  iface->get_surface_transform = gtk_emacsfixed_native_get_surface_transform;
  iface->check_resize = gtk_emacsfixed_native_check_resize;
}
#endif
#if 1
#define FRAME_CR_SURFACE(f)	((f)->output_data.gtk4->cr_surface)

static void emacs_fixed_snapshot (GtkWidget   *widget,
				  GtkSnapshot *snapshot)
{
  GTK4_TRACE("snapshot____");
  EmacsFixed *fixed = EMACS_FIXED (widget);
  EmacsFixedPrivate *priv = G_TYPE_INSTANCE_GET_PRIVATE (fixed, emacs_fixed_get_type (),
				      EmacsFixedPrivate);
  cairo_t *cr = NULL;
  cairo_surface_t *src = NULL;
  struct frame *f = priv->f;

  cr = gtk_snapshot_append_cairo (snapshot,   &GRAPHENE_RECT_INIT (
				    0, 0,
				    gtk_widget_get_width (widget),
				    gtk_widget_get_height (widget)));
  src = FRAME_CR_SURFACE(f);

  cairo_set_source_surface (cr, src, 0, 0);
  cairo_paint(cr);

  GTK4_TRACE("snapshot____2");
  cairo_destroy(cr);
}

#endif
struct frame *
gtk4_any_window_to_frame (GtkWidget *window);


static gboolean
focus_in_event(GtkWidget *widget, GdkEvent *event, gpointer *user_data)
{
  GTK4_TRACE("focus_in_event");
  //  union buffered_input_event inev;
  struct frame *frame = gtk4_any_window_to_frame(widget);

  if (frame == NULL)
    return TRUE;

  /* EVENT_INIT (inev.ie); */
  /* inev.ie.kind = NO_EVENT; */
  /* inev.ie.arg = Qnil; */

  /* x_focus_changed (TRUE, FOCUS_IMPLICIT, */
  /*		   FRAME_DISPLAY_INFO(frame), frame, &inev); */
  /* if (inev.ie.kind != NO_EVENT) */
  /*   evq_enqueue (&inev); */
  return TRUE;
}



static void
emacs_fixed_class_init (EmacsFixedClass *klass)
{
  GtkWidgetClass *widget_class;

  widget_class = (GtkWidgetClass *) klass;
#ifndef HAVE_GTK4
  widget_class->get_preferred_width = emacs_fixed_get_preferred_width;
  widget_class->get_preferred_height = emacs_fixed_get_preferred_height;
#else
  widget_class->snapshot = emacs_fixed_snapshot;
#endif

#ifdef HAVE_XWIDGETS
  widget_class->size_allocate = emacs_fixed_gtk_widget_size_allocate;
#endif
  g_type_class_add_private (klass, sizeof (EmacsFixedPrivate));
}

static void
emacs_fixed_init (EmacsFixed *fixed)
{
  fixed->priv = G_TYPE_INSTANCE_GET_PRIVATE (fixed, emacs_fixed_get_type (),
					     EmacsFixedPrivate);
  fixed->priv->f = 0;

  GtkEventController *motion = gtk_event_controller_motion_new();
  g_signal_connect(motion, "focus", focus_in_event, fixed );

}

GtkWidget *
emacs_fixed_new (struct frame *f)
{
  EmacsFixed *fixed = g_object_new (emacs_fixed_get_type (), NULL);
  EmacsFixedPrivate *priv = fixed->priv;
  GtkWidget *gfixed = GTK_WIDGET (fixed);
  priv->f = f;

  gtk_widget_set_hexpand(gfixed, true);
  gtk_widget_set_vexpand(gfixed, true);

  return gfixed;
}

static void
emacs_fixed_get_preferred_width (GtkWidget *widget,
				 gint      *minimum,
				 gint      *natural)
{
  EmacsFixed *fixed = EMACS_FIXED (widget);
  EmacsFixedPrivate *priv = fixed->priv;
#ifdef HAVE_GTK4
  int w = priv->f->output_data.gtk4->size_hints.min_width;
  if (minimum) *minimum = w;
  if (natural) *natural = priv->f->output_data.gtk4->preferred_width;
#else
  int w = priv->f->output_data.x->size_hints.min_width;
  if (minimum) *minimum = w;
  if (natural) *natural = w;
#endif
}

static void
emacs_fixed_get_preferred_height (GtkWidget *widget,
				  gint      *minimum,
				  gint      *natural)
{
  EmacsFixed *fixed = EMACS_FIXED (widget);
  EmacsFixedPrivate *priv = fixed->priv;
#ifdef HAVE_GTK4
  int h = priv->f->output_data.gtk4->size_hints.min_height;
  if (minimum) *minimum = h;
  if (natural) *natural = priv->f->output_data.gtk4->preferred_height;
#else
  int h = priv->f->output_data.x->size_hints.min_height;
  if (minimum) *minimum = h;
  if (natural) *natural = h;
#endif
}




























#ifndef HAVE_GTK4

/* Override the X function so we can intercept Gtk+ 3 calls.
   Use our values for min_width/height so that KDE don't freak out
   (Bug#8919), and so users can resize our frames as they wish.  */

void
XSetWMSizeHints (Display *d,
		 Window w,
		 XSizeHints *hints,
		 Atom prop)
{
  struct x_display_info *dpyinfo = x_display_info_for_display (d);
  struct frame *f = x_top_window_to_frame (dpyinfo, w);
  long data[18];
  data[0] = hints->flags;
  data[1] = hints->x;
  data[2] = hints->y;
  data[3] = hints->width;
  data[4] = hints->height;
  data[5] = hints->min_width;
  data[6] = hints->min_height;
  data[7] = hints->max_width;
  data[8] = hints->max_height;
  data[9] = hints->width_inc;
  data[10] = hints->height_inc;
  data[11] = hints->min_aspect.x;
  data[12] = hints->min_aspect.y;
  data[13] = hints->max_aspect.x;
  data[14] = hints->max_aspect.y;
  data[15] = hints->base_width;
  data[16] = hints->base_height;
  data[17] = hints->win_gravity;

  if ((hints->flags & PMinSize) && f)
    {
#ifdef HAVE_GTK4
      int w = f->output_data.gtk4->size_hints.min_width;
      int h = f->output_data.gtk4->size_hints.min_height;
#else
      int w = f->output_data.x->size_hints.min_width;
      int h = f->output_data.x->size_hints.min_height;
#endif
      data[5] = w;
      data[6] = h;
    }

  XChangeProperty (d, w, prop, XA_WM_SIZE_HINTS, 32, PropModeReplace,
		   (unsigned char *) data, 18);
}

/* Override this X11 function.
   This function is in the same X11 file as the one above.  So we must
   provide it also.  */

void
XSetWMNormalHints (Display *d, Window w, XSizeHints *hints)
{
  XSetWMSizeHints (d, w, hints, XA_WM_NORMAL_HINTS);
}

#endif
