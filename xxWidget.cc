// $Id: xxWidget.cc 2641 2007-09-02 21:31:02Z flaterco $

/*  xxWidget  Wrapper for X11 Widgets that calls XtDestroyWidget when
    it is deleted.

    Copyright (C) 1998  David Flater.

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


xxWidget::xxWidget (Widget widget):
  _widget(widget) {}


xxWidget::~xxWidget() {
  XtDestroyWidget (_widget);
}


const Widget xxWidget::widget() const {
  return _widget;
}

// Cleanup2006 Done
