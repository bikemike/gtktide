// $Id: xxMouseWheelViewport.cc 2641 2007-09-02 21:31:02Z flaterco $

/*  xxMouseWheelViewport  Nontrivial callback to make a viewport
    responsive to the mouse wheel.  There might be an easier way.

    Copyright (C) 2007  David Flater.

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#include "xtide.hh"


xxMouseWheelViewport::xxMouseWheelViewport() {}


xxMouseWheelViewport::~xxMouseWheelViewport() {}


static void mouseButtonCallback (Widget w unusedParameter,
                                 XtPointer client_data,
                                 XEvent *event,
                                 Boolean *continue_dispatch unusedParameter) {
  assert (client_data);
  ((xxMouseWheelViewport*)client_data)->mouseButton ((XButtonEvent*)event);
}


void xxMouseWheelViewport::mouseButton (const XButtonEvent *xbe) {

  char *startScrollArgs[1];

  assert (xbe);
  switch (xbe->button) {
  case Button4:
    startScrollArgs[0] = "Backward";
    break;
  case Button5:
    startScrollArgs[0] = "Forward";
    break;
  default:
    return;
  }

  char *notifyScrollArgs[1] = {"FullLength"};

  // Passing NULL yields a segmentation fault, yet it seems not to
  // matter that this XEvent is uninitialized.  I have no idea what
  // the actions are doing with it.
  XEvent event;

  XtCallActionProc (scrollbarWidget, "StartScroll", &event, startScrollArgs,
		    1);
  XtCallActionProc (scrollbarWidget, "NotifyScroll", &event, notifyScrollArgs,
		    1);
  XtCallActionProc (scrollbarWidget, "EndScroll", &event, NULL, 0);
}


void xxMouseWheelViewport::obeyMouseWheel (Widget widget) {
  XtAddEventHandler (widget, ButtonPressMask, False,
                     mouseButtonCallback, (XtPointer)this);
}

// Cleanup2006 Done
