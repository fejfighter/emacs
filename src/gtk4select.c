/* Gtk selection processing for emacs.
   Copyright (C) 1993-1994, 2005-2006, 2008-2018 Free Software
   Foundation, Inc.

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

/*
Originally by Carl Edman
Updated by Christian Limpach (chris@nice.ch)
OpenStep/Rhapsody port by Scott Bender (sbender@harmony-ds.com)
macOS/Aqua port by Christophe de Dinechin (descubes@earthlink.net)
GNUstep port and post-20 update by Adrian Robert (arobert@cogsci.ucsd.edu)
*/

/* This should be the first include, as it may set up #defines affecting
   interpretation of even the system includes.  */
#include <config.h>

#include "lisp.h"
#include "gtk4term.h"
#include "termhooks.h"
#include "keyboard.h"
#include "gtk4select.h"
#include <gdk/gdk.h>

#if 0
static Lisp_Object Vselection_alist;
#endif

static GQuark quark_primary_data = 0;
static GQuark quark_primary_size = 0;
static GQuark quark_secondary_data = 0;
static GQuark quark_secondary_size = 0;
static GQuark quark_clipboard_data = 0;
static GQuark quark_clipboard_size = 0;

/* ==========================================================================

    Internal utility functions

   ========================================================================== */

/* From a Lisp_Object, return a suitable frame for selection
   operations.  OBJECT may be a frame, a terminal object, or nil
   (which stands for the selected frame--or, if that is not an gtk4
   frame, the first gtk4 display on the list).  If no suitable frame can
   be found, return NULL.  */

static struct frame *
frame_for_gtk4_selection (Lisp_Object object)
{
  Lisp_Object tail, frame;
  struct frame *f;

  if (NILP (object))
    {
      f = XFRAME (selected_frame);
      if (FRAME_GTK4_P (f) && FRAME_LIVE_P (f))
	return f;

      FOR_EACH_FRAME (tail, frame)
	{
	  f = XFRAME (frame);
	  if (FRAME_GTK4_P (f) && FRAME_LIVE_P (f))
	    return f;
	}
    }
  else if (TERMINALP (object))
    {
      struct terminal *t = decode_live_terminal (object);

      if (t->type == output_gtk4)
	FOR_EACH_FRAME (tail, frame)
	  {
	    f = XFRAME (frame);
	    if (FRAME_LIVE_P (f) && f->terminal == t)
	      return f;
	  }
    }
  else if (FRAMEP (object))
    {
      f = XFRAME (object);
      if (FRAME_GTK4_P (f) && FRAME_LIVE_P (f))
	return f;
    }

  return NULL;
}

static GtkClipboard *symbol_to_gtk_clipboard(GtkWidget *widget, Lisp_Object symbol)
{
  GdkAtom atom;

  CHECK_SYMBOL (symbol);
  /* if (NILP(symbol)) { */
  /*   atom = GDK_SELECTION_PRIMARY; */
  /* } else if (EQ(symbol, QCLIPBOARD)) { */
  /*   atom = GDK_SELECTION_CLIPBOARD; */
  /* } else if (EQ(symbol, QPRIMARY)) { */
  /*   atom = GDK_SELECTION_PRIMARY; */
  /* } else if (EQ(symbol, QSECONDARY)) { */
  /*   atom = GDK_SELECTION_SECONDARY; */
  /* } else if (EQ(symbol, Qt)) { */
  /*   atom = GDK_SELECTION_SECONDARY; */
  /* } else { */
  /*   atom = 0; */
  /*   error ("Bad selection"); */
  /* } */

  return gtk_widget_get_clipboard(widget);
}

static void selection_type_to_quarks(GdkAtom type, GQuark *quark_data, GQuark *quark_size)
{
  /* if (type == GDK_SELECTION_PRIMARY) { */
  /*   *quark_data = quark_primary_data; */
  /*   *quark_size = quark_primary_size; */
  /* } else if (type == GDK_SELECTION_SECONDARY) { */
  /*   *quark_data = quark_secondary_data; */
  /*   *quark_size = quark_secondary_size; */
  /* } else if (type == GDK_SELECTION_CLIPBOARD) { */
  /*   *quark_data = quark_clipboard_data; */
  /*   *quark_size = quark_clipboard_size; */
  /* } else { */
  /*   /\* fixme: Is it safe to use 'error' here? *\/ */
  /*   error("Unknown selection type."); */
  /* } */
}

static void
get_func(GtkClipboard *cb, GtkSelectionData *data, guint info, gpointer user_data_or_owner)
{
  GTK4_TRACE("get_func:");
  GObject *obj = G_OBJECT(user_data_or_owner);
  const char *str;
  int size;
  GQuark quark_data, quark_size;

  //  selection_type_to_quarks(gtk_clipboard_get_selection(cb), &quark_data, &quark_size);

  str = g_object_get_qdata(obj, quark_data);
  size = GPOINTER_TO_SIZE(g_object_get_qdata(obj, quark_size));
  GTK4_TRACE("get_func: str: %s", str);
  gtk_selection_data_set_text(data, str, size);
}

static void
clear_func(GtkClipboard *cb, gpointer user_data_or_owner)
{
  GTK4_TRACE("clear_func:");
  #if 0
  GObject *obj = G_OBJECT(user_data_or_owner);
  GQuark quark_data, quark_size;

  selection_type_to_quarks(gtk_clipboard_get_selection(cb), &quark_data, &quark_size);

  g_object_set_qdata(obj, quark_data, NULL);
  g_object_set_qdata(obj, quark_size, 0);

  #endif
}


/* ==========================================================================

    Functions used externally

   ========================================================================== */

void gtk4_selection_init(void)
{
  if (quark_primary_data == 0) {
    quark_primary_data = g_quark_from_static_string("gtk4-primary-data");
    quark_primary_size = g_quark_from_static_string("gtk4-primary-size");
    quark_secondary_data = g_quark_from_static_string("gtk4-secondary-data");
    quark_secondary_size = g_quark_from_static_string("gtk4-secondary-size");
    quark_clipboard_data = g_quark_from_static_string("gtk4-clipboard-data");
    quark_clipboard_size = g_quark_from_static_string("gtk4-clipboard-size");
  }
}

void gtk4_selection_lost(GtkWidget *widget, GdkEvent *event, gpointer user_data)
{
  GQuark quark_data, quark_size;
  GTK4_TRACE("gtk4_selection_lost:");

  //  selection_type_to_quarks(event->selection, &quark_data, &quark_size);

  g_object_set_qdata(G_OBJECT(widget), quark_data, NULL);
  g_object_set_qdata(G_OBJECT(widget), quark_size, 0);
}

static bool
gtk4_selection_usable (void)
{
  /*
   * https://github.com/GNOME/gtk/blob/gtk-3-24/gdk/wayland/gdkselection-wayland.c#L1033
   *
   * Gdk uses gdk_display_get_default() when handling selections, so
   * selections don't work properly on multi-display environment.
   *
   * ----------------
   * #include <gtk/gtk.h>
   *
   * static GtkWidget *top1, *top2;
   *
   * int main(int argc, char **argv)
   * {
   *     GtkWidget *w;
   *     GtkTextBuffer *buf;
   *
   *     gtk_init(&argc, &argv);
   *
   *     static char *text = "\
   * It is fine today.\n\
   * It will be fine tomorrow too.\n\
   * It is too hot.";
   *
   *     top1 = gtk_window_new(GTK_WINDOW_TOPLEVEL);
   *     gtk_window_set_title(GTK_WINDOW(top1), "default");
   *     gtk_widget_show(top1);
   *     w = gtk_text_view_new();
   *     gtk_container_add(GTK_CONTAINER(top1), w);
   *     gtk_widget_show(w);
   *     buf = gtk_text_view_get_buffer(GTK_TEXT_VIEW(w));
   *     gtk_text_buffer_insert_at_cursor(buf, text, strlen(text));
   *     gtk_text_buffer_add_selection_clipboard(buf, gtk_widget_get_clipboard(w, GDK_SELECTION_PRIMARY));
   *
   *     unsetenv("GDK_BACKEND");
   *     GdkDisplay *gdpy;
   *     const char *dpyname2;
   *     if (strcmp(G_OBJECT_TYPE_NAME(gtk_widget_get_window(top1)), "GdkWaylandWindow") == 0)
   *         dpyname2 = ":0";
   *     else
   *         dpyname2 = "wayland-0";
   *     gdpy = gdk_display_open (dpyname2);
   *     top2 = gtk_window_new (GTK_WINDOW_TOPLEVEL);
   *     gtk_window_set_title(GTK_WINDOW(top2), dpyname2);
   *     gtk_window_set_screen (GTK_WINDOW (top2), gdk_display_get_default_screen(gdpy));
   *     gtk_widget_show (top2);
   *     w = gtk_text_view_new();
   *     gtk_container_add(GTK_CONTAINER(top2), w);
   *     gtk_widget_show(w);
   *     buf = gtk_text_view_get_buffer(GTK_TEXT_VIEW(w));
   *     gtk_text_buffer_insert_at_cursor(buf, text, strlen(text));
   *     gtk_text_buffer_add_selection_clipboard(buf, gtk_widget_get_clipboard(w, GDK_SELECTION_PRIMARY));
   *
   *     gtk_main();
   *
   *     return 0;
   * }
   * ----------------
   *
   * This code fails if
   *   GDK_BACKEND=x11 ./test
   * and select on both of windows.
   *
   * ----------------
   * (test:15345): GLib-GObject-CRITICAL **: 01:56:38.041: g_object_ref: assertion 'G_IS_OBJECT (object)' failed
   *
   * (test:15345): GLib-GObject-CRITICAL **: 01:56:38.042: g_object_ref: assertion 'G_IS_OBJECT (object)' failed
   *
   * (test:15345): GLib-GObject-CRITICAL **: 01:56:39.113: g_object_ref: assertion 'G_IS_OBJECT (object)' failed
   *
   * (test:15345): GLib-GObject-CRITICAL **: 01:56:39.113: g_object_ref: assertion 'G_IS_OBJECT (object)' failed
   * ----------------
   * (gtk-3.24.10)
   *
   * This function checks whether selections work by the number of displays.
   * If you use more than 2 displays, then selection is disabled.
   */

  GdkDisplayManager *dpyman = gdk_display_manager_get ();
  GSList *list = gdk_display_manager_list_displays (dpyman);
  int len = g_slist_length (list);
  g_slist_free (list);
  return len < 2;
}

/* ==========================================================================

    Lisp Defuns

   ========================================================================== */


DEFUN ("gtk4-own-selection-internal", Fgtk4_own_selection_internal,
       Sgtk4_own_selection_internal, 2, 3, 0,
       doc: /* Assert an X selection of type SELECTION and value VALUE.
SELECTION is a symbol, typically `PRIMARY', `SECONDARY', or `CLIPBOARD'.
\(Those are literal upper-case symbol names, since that's what X expects.)
VALUE is typically a string, or a cons of two markers, but may be
anything that the functions on `selection-converter-alist' know about.

FRAME should be a frame that should own the selection.  If omitted or
nil, it defaults to the selected frame.*/)
     (Lisp_Object selection, Lisp_Object value, Lisp_Object frame)
{
  GTK4_TRACE("gtk4-own-selection-internal.");
  return Qnil;
#if 0
  Lisp_Object successful_p = Qnil;
  Lisp_Object target_symbol, rest;
  GtkClipboard *cb;
  struct frame *f;
  GQuark quark_data, quark_size;

  check_window_system (NULL);

  if (!gtk4_selection_usable ())
    return Qnil;

  if (NILP (frame)) frame = selected_frame;
  if (!FRAME_LIVE_P (XFRAME (frame)) || !FRAME_GTK4_P (XFRAME (frame)))
    error ("gtk4 selection unavailable for this frame");
  f = XFRAME(frame);

  cb = symbol_to_gtk_clipboard(FRAME_GTK_WIDGET(f), selection);
  selection_type_to_quarks(gtk_clipboard_get_selection(cb), &quark_data, &quark_size);

  /* We only support copy of text.  */
  target_symbol = QTEXT;
  if (STRINGP (value))
    {
      GtkTargetList *list;
      GtkTargetEntry *targets;
      gint n_targets;
      GtkWidget *widget;

      list = gtk_target_list_new (NULL, 0);
      gtk_target_list_add_text_targets (list, 0);

      targets = gtk_target_table_new_from_list (list, &n_targets);

      int size = SBYTES(value);
      gchar *str = xmalloc(size + 1);
      memcpy(str, SSDATA(value), size);
      str[size] = '\0';

      widget = FRAME_GTK_WIDGET(f);
      g_object_set_qdata_full(G_OBJECT(widget), quark_data, str, xfree);
      g_object_set_qdata_full(G_OBJECT(widget), quark_size, GSIZE_TO_POINTER(size), NULL);

      GTK4_TRACE("set_with_owner: owner=%p", FRAME_GTK_WIDGET(f));
      if (gtk_clipboard_set_with_owner (cb,
					targets, n_targets,
					get_func, clear_func,
					G_OBJECT(FRAME_GTK_WIDGET(f)))) {
	GTK4_TRACE("set_with_owner succeeded..");
	successful_p = Qt;
      } else {
	GTK4_TRACE("set_with_owner failed.");
      }
      gtk_clipboard_set_can_store (cb, NULL, 0);

      gtk_target_table_free (targets, n_targets);
      gtk_target_list_unref (list);
    }

  if (!EQ (Vgtk4_sent_selection_hooks, Qunbound))
    {
      /* FIXME: Use run-hook-with-args!  */
      for (rest = Vgtk4_sent_selection_hooks; CONSP (rest); rest = Fcdr (rest))
	call3 (Fcar (rest), selection, target_symbol, successful_p);
    }

  return value;

#endif
}


DEFUN ("gtk4-disown-selection-internal", Fgtk4_disown_selection_internal,
       Sgtk4_disown_selection_internal, 1, 3, 0,
       doc: /* If we own the selection SELECTION, disown it.
Disowning it means there is no such selection.

Sets the last-change time for the selection to TIME-OBJECT (by default
the time of the last event).

TERMINAL should be a terminal object or a frame specifying the X
server to query.  If omitted or nil, that stands for the selected
frame's display, or the first available X display.

On Nextstep, the TIME-OBJECT and TERMINAL arguments are unused.
On MS-DOS, all this does is return non-nil if we own the selection.
On GTK4, the TIME-OBJECT is unused.  */)
  (Lisp_Object selection, Lisp_Object time_object, Lisp_Object terminal)
{
  GTK4_TRACE("gtk4-disown-selection-internal.");

#if 0
  struct frame *f = frame_for_gtk4_selection (terminal);
  GtkClipboard *cb;

  if (!gtk4_selection_usable ())
    return Qnil;

  if (!f)
    return Qnil;

  cb = symbol_to_gtk_clipboard(FRAME_GTK_WIDGET(f), selection);

  gtk_clipboard_clear(cb);

  return Qt;
#endif
  return Qnil;

}


DEFUN ("gtk4-selection-exists-p", Fgtk4_selection_exists_p, Sgtk4_selection_exists_p,
       0, 2, 0, doc: /* Whether there is an owner for the given X selection.
SELECTION should be the name of the selection in question, typically
one of the symbols `PRIMARY', `SECONDARY', or `CLIPBOARD'.  (X expects
these literal upper-case names.)  The symbol nil is the same as
`PRIMARY', and t is the same as `SECONDARY'.

TERMINAL should be a terminal object or a frame specifying the X
server to query.  If omitted or nil, that stands for the selected
frame's display, or the first available X display.

On Nextstep, TERMINAL is unused.  */)
     (Lisp_Object selection, Lisp_Object terminal)
{
  GTK4_TRACE("gtk4-selection-exists-p.");
#if 0
  struct frame *f = frame_for_gtk4_selection (terminal);
  GtkClipboard *cb;

  if (!gtk4_selection_usable ())
    return Qnil;

  if (!f)
    return Qnil;

  cb = symbol_to_gtk_clipboard(FRAME_GTK_WIDGET(f), selection);

  return gtk_clipboard_wait_is_text_available(cb) ? Qt : Qnil;
#endif
  return Qnil;
}


DEFUN ("gtk4-selection-owner-p", Fgtk4_selection_owner_p, Sgtk4_selection_owner_p,
       0, 2, 0,
       doc: /* Whether the current Emacs process owns the given X Selection.
The arg should be the name of the selection in question, typically one of
the symbols `PRIMARY', `SECONDARY', or `CLIPBOARD'.
\(Those are literal upper-case symbol names, since that's what X expects.)
For convenience, the symbol nil is the same as `PRIMARY',
and t is the same as `SECONDARY'.

TERMINAL should be a terminal object or a frame specifying the X
server to query.  If omitted or nil, that stands for the selected
frame's display, or the first available X display.

On Nextstep, TERMINAL is unused.  */)
     (Lisp_Object selection, Lisp_Object terminal)
{
  GTK4_TRACE("gtk4-selection-owner-p.");
  //struct frame *f = frame_for_gtk4_selection (terminal);
  //GtkClipboard *cb;
  //GObject *obj;
  //GQuark quark_data, quark_size;

  //if (!gtk4_selection_usable ())
    return Qnil;

  //cb = symbol_to_gtk_clipboard(FRAME_GTK_WIDGET(f), selection);
  //  selection_type_to_quarks(gtk_clipboard_get_selection(cb), &quark_data, &quark_size);

  //obj = gtk_clipboard_get_owner(cb);

  //  return g_object_get_qdata(obj, quark_data) != NULL ? Qt : Qnil;
}


DEFUN ("gtk4-get-selection-internal", Fgtk4_get_selection_internal,
       Sgtk4_get_selection_internal, 2, 4, 0,
       doc: /* Return text selected from some X window.
SELECTION-SYMBOL is typically `PRIMARY', `SECONDARY', or `CLIPBOARD'.
\(Those are literal upper-case symbol names, since that's what X expects.)
TARGET-TYPE is the type of data desired, typically `STRING'.

TIME-STAMP is the time to use in the XConvertSelection call for foreign
selections.  If omitted, defaults to the time for the last event.

TERMINAL should be a terminal object or a frame specifying the X
server to query.  If omitted or nil, that stands for the selected
frame's display, or the first available X display.

On Nextstep, TIME-STAMP and TERMINAL are unused.
On GTK4, TIME-STAMP is unused.  */)
  (Lisp_Object selection_symbol, Lisp_Object target_type,
   Lisp_Object time_stamp, Lisp_Object terminal)
{
  struct frame *f = frame_for_gtk4_selection (terminal);
  //GtkClipboard *cb;

  CHECK_SYMBOL (selection_symbol);
  CHECK_SYMBOL (target_type);
  if (EQ (target_type, QMULTIPLE))
    error ("Retrieving MULTIPLE selections is currently unimplemented");
  if (!f)
    error ("GTK4 selection unavailable for this frame");

  if (!gtk4_selection_usable ())
    return Qnil;

  //cb = symbol_to_gtk_clipboard(FRAME_GTK_WIDGET(f), selection_symbol);

  //  gchar *s = gtk_clipboard_wait_for_text(cb);
  //if (s == NULL)
    return Qnil;
#if 0
    int size = strlen(s);
  Lisp_Object str = make_unibyte_string (s, size);
  Fput_text_property (make_fixnum (0), make_fixnum (size),
		      Qforeign_selection, QUTF8_STRING, str);
  return str;
  #endif
}


void
nxatoms_of_gtk4select (void)
{
  GTK4_TRACE("nxatoms_of_gtk4select");
}

void
syms_of_gtk4select (void)
{
  GTK4_TRACE("syms_of_gtk4select");

  DEFSYM (QCLIPBOARD, "CLIPBOARD");
  DEFSYM (QSECONDARY, "SECONDARY");
  DEFSYM (QTEXT, "TEXT");
  DEFSYM (QFILE_NAME, "FILE_NAME");
  DEFSYM (QMULTIPLE, "MULTIPLE");

  DEFSYM (Qforeign_selection, "foreign-selection");
  DEFSYM (QUTF8_STRING, "UTF8_STRING");

  defsubr (&Sgtk4_disown_selection_internal);
  defsubr (&Sgtk4_get_selection_internal);
  defsubr (&Sgtk4_own_selection_internal);
  defsubr (&Sgtk4_selection_exists_p);
  defsubr (&Sgtk4_selection_owner_p);

#if 0
  Vselection_alist = Qnil;
  staticpro (&Vselection_alist);
#endif

  DEFVAR_LISP ("gtk4-sent-selection-hooks", Vgtk4_sent_selection_hooks,
	       "A list of functions to be called when Emacs answers a selection request.\n\
The functions are called with four arguments:\n\
  - the selection name (typically `PRIMARY', `SECONDARY', or `CLIPBOARD');\n\
  - the selection-type which Emacs was asked to convert the\n\
    selection into before sending (for example, `STRING' or `LENGTH');\n\
  - a flag indicating success or failure for responding to the request.\n\
We might have failed (and declined the request) for any number of reasons,\n\
including being asked for a selection that we no longer own, or being asked\n\
to convert into a type that we don't know about or that is inappropriate.\n\
This hook doesn't let you change the behavior of Emacs's selection replies,\n\
it merely informs you that they have happened.");
  Vgtk4_sent_selection_hooks = Qnil;
}
