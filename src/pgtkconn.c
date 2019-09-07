/* Pure Gtk+-3 communication module.      -*- coding: utf-8 -*-

Copyright (C) 1989, 1993-1994, 2005-2006, 2008-2018 Free Software
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

/* This should be the first include, as it may set up #defines affecting
   interpretation of even the system includes. */
#include <config.h>

#include "lisp.h"
#include "dynlib.h"
#include "pgtkconn.h"


/***
 * Detect socket to display server, with display type specific functions.
 ***/

#ifdef GDK_WINDOWING_WAYLAND

#include <gdk/gdkwayland.h>

static int pgtk_detect_wayland_connection(dynlib_handle_ptr h, GdkDisplay *gdpy)
{
  /* Check gdpy is an instance of GdkWaylandDisplay. */
  if (!GDK_IS_WAYLAND_DISPLAY (gdpy))
    return -1;

  /* Obtain Wayland Display from GdkWaylandDisplay,
     and get file descriptor from it. */
  struct wl_display *dpy = gdk_wayland_display_get_wl_display (gdpy);

  int (*fn)(void *) = dynlib_sym(h, "wl_display_get_fd");
  if (fn == NULL)
    return -1;
  return fn (dpy);
}

#endif

#ifdef GDK_WINDOWING_X11

#include <gdk/gdkx.h>

static int pgtk_detect_x11_connection(dynlib_handle_ptr h, GdkDisplay *gdpy)
{
  /* Check gdpy is an instance of GdkX11Display. */
  if (!GDK_IS_X11_DISPLAY (gdpy))
    return -1;

  /* Obtain X Display from GdkX11Display, and get file descriptor from it. */
  Display *dpy = gdk_x11_display_get_xdisplay (gdpy);

  int (*fn)(void *) = dynlib_sym(h, "XConnectionNumber");
  if (fn == NULL)
    return -1;
  return fn (dpy);
}

#endif

int pgtk_detect_connection(GdkDisplay *gdpy)
{
  static dynlib_handle_ptr h = NULL;
  int fd;

  if (h == NULL) {
    if ((h = dynlib_open(NULL)) == NULL) {
      error("dynlib_open failed.");
      return -1;
    }
  }

#ifdef GDK_WINDOWING_X11
  if ((fd = pgtk_detect_x11_connection(h, gdpy)) != -1)
    return fd;
#endif

#ifdef GDK_WINDOWING_WAYLAND
  if ((fd = pgtk_detect_wayland_connection(h, gdpy)) != -1)
    return fd;
#endif

  /* I don't know how to detect fd if using other backend. */

  error("socket detection failed.");
  return -1;
}
